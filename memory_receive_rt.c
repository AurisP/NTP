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
#include <lgpio.h>
#define GPIO_PIN 17


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

#define BUFFER_SIZE 1000

typedef struct {
        struct timespec time_value;
        // Add any other fields as needed
} BufferElement;

typedef struct {
        BufferElement buffer[BUFFER_SIZE];
        int head;
        int tail;
        // Add any other fields as needed
} RingBuffer;

RingBuffer ringBuffer;

void *bgThread_func(void *arg) {
        // Background thread logic here
        // Flush data from ring buffer
        // Example:
        int count = 0;
        while (1) {
                // Check if there's data to flush
                if (ringBuffer.head != ringBuffer.tail) {
                // Flush data
                printf("%ld: %ld\n", ringBuffer.buffer[ringBuffer.tail].time_value.tv_sec, ringBuffer.buffer[ringBuffer.tail].time_value.tv_nsec);
                fflush(stdout);
                ringBuffer.tail = (ringBuffer.tail + 1) % BUFFER_SIZE;
                }
        // Add sleep or yield if necessary
        sleep(2);
        }
        return NULL;
}


void *thread_func(void *data)
{
    struct timespec time_value; 
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
    //GPIO[pin].ctrl=fn;
    //pad[pin] = 0x44;
    //rioSET->OE = 0x00<<pin;
    //rioSET-> = 0x01<<pin;
    volatile uint32_t *addrGP0Status = GPIOBase + (pin * 8 / 4);
    while (1)
    {
       int32_t value = *addrGP0Status;
       if (value & (1 << 23))
       {
  	    clock_gettime(CLOCK_REALTIME, &time_value);
            time_value.tv_nsec -= 200000000L;
            memcpy(&ringBuffer.buffer[ringBuffer.head].time_value, &time_value, sizeof(struct timespec));
            ringBuffer.head = (ringBuffer.head + 1) % BUFFER_SIZE;
            sleep(1.2);
       }
   } 
    return NULL;
}

int main(int argc, char* argv[])
{
        struct sched_param param;
        pthread_attr_t attr;
        pthread_t thread, bgThread;
        int ret, ret2;

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
        param.sched_priority = 95;
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

        /* Create a pthread with specified attributes */
        ret2 = pthread_create(&bgThread, NULL, bgThread_func, NULL);
        if (ret2) {
                printf("create bgpthread failed\n");
                goto out;
        }


        /* Join the thread and wait until it is done */
        ret = pthread_join(thread, NULL);
        if (ret)
                printf("join pthread failed: %m\n");

        /* Join the thread and wait until it is done */
        ret2 = pthread_join(bgThread, NULL);
        if (ret2)
                printf("join bgpthread failed: %m\n");
out:
        return ret;
}
