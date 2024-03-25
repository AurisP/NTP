#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <linux/gpio.h>

#define GPIO_PIN 18


typedef struct{
    uint32_t status;
    uint32_t ctrl; 
}GPIOregs;
#define GPIO ((GPIOregs*)GPIOBase)
typedef struct
{
    uint32_t Out;
    uint32_t OE;
    uint32_t In;
    uint32_t InSync;
} rioregs;
#define rio ((rioregs *)RIOBase)
#define rioXOR ((rioregs *)(RIOBase + 0x1000 / 4))
#define rioSET ((rioregs *)(RIOBase + 0x2000 / 4))
#define rioCLR ((rioregs *)(RIOBase + 0x3000 / 4))


/*
 * POSIX Real Time Example
 * using a single pthread as RT thread
 */

static void wait_rest_of_period()
{
        //inc_period(pinfo);
        struct timespec sleep_time;
        clock_gettime(CLOCK_REALTIME, &sleep_time);
        //long remaining_nanos = 1000000000L - sleep_time.tv_nsec;
        sleep_time.tv_nsec = 200000000L;
        sleep_time.tv_sec += 1;
        /* for simplicity, ignoring possibilities of signal wakes */
        //printf("%ld ", sleep_time.tv_sec);
        int result = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &sleep_time, NULL);
        if (result != 0) {
        // Handle error (print error message or take appropriate action)
        //fprintf(stderr, "clock_nanosleep failed: %d\n", result);
    }
        //clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &sleep_time, NULL);
}

void *thread_func(void *data)
{

    int memfd = open("/dev/mem", O_RDWR | O_SYNC);
    uint32_t *map = (uint32_t *)mmap(
        NULL,
        64 * 1024 * 1024,
        (PROT_READ | PROT_WRITE),
        MAP_SHARED,
        memfd,
        0x1f00000000
    );
    if (map == MAP_FAILED)
    {
        printf("mmap failed: %s\n", strerror(errno));
        return NULL;
    };
    close(memfd);
    uint32_t *PERIBase = map;
    uint32_t *GPIOBase = PERIBase + 0xD0000 / 4;
    uint32_t *RIOBase = PERIBase + 0xe0000 / 4;
    uint32_t *PADBase = PERIBase + 0xf0000 / 4;
    uint32_t *pad = PADBase + 1;   
    
    uint32_t pin = GPIO_PIN;
    uint32_t fn = 5;
    GPIO[pin].ctrl=fn;
    pad[pin] = 0x10;
    rioSET->OE = 0x01<<pin;
    rioSET->Out = 0x00<<pin;

    while (1)
    {
        rioXOR->Out = 0x01<<pin;
        sleep(0.3);
        rioXOR->Out = 0x01<<pin;
        wait_rest_of_period();
    }
    return NULL;
}

int main(int argc, char* argv[])
{
        struct sched_param param;
        pthread_attr_t attr;
        pthread_t thread;
        int ret;

        /* Lock memory */
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                exit(-2);
        }

        /* Initialize pthread attributes (default values) */
        ret = pthread_attr_init(&attr);
        if (ret) {
                printf("init pthread attributes failed\n");
                goto out;
        }

        /* Set a specific stack size  */
        ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
        if (ret) {
            printf("pthread setstacksize failed\n");
            goto out;
        }

        /* Set scheduler policy and priority of pthread */
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret) {
                printf("pthread setschedpolicy failed\n");
                goto out;
        }
        param.sched_priority = 99;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) {
                printf("pthread setschedparam failed\n");
                goto out;
        }
        /* Use scheduling parameters of attr */
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) {
                printf("pthread setinheritsched failed\n");
                goto out;
        }

        /* Create a pthread with specified attributes */
        ret = pthread_create(&thread, &attr, thread_func, NULL);
        if (ret) {
                printf("create pthread failed\n");
                goto out;
        }

        /* Join the thread and wait until it is done */
        ret = pthread_join(thread, NULL);
        if (ret)
                printf("join pthread failed: %m\n");

out:
        return ret;
}
