/*                                                                  
 * POSIX Real Time Example
 * using a single pthread as RT thread
 */
 
#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <linux/gpio.h>
#include <time.h>
#include <lgpio.h>
#include <unistd.h>
#include <string.h>
#define GPIO_PIN 17

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
	//int count = 0;
   	while (1) {
        	// Check if there's data to flush
        	if (ringBuffer.head != ringBuffer.tail) {
            	// Flush data
            	printf("%ld: %ld\n", ringBuffer.buffer[ringBuffer.tail].time_value.tv_sec, ringBuffer.buffer[ringBuffer.tail].time_value.tv_nsec);
		fflush(stdout);
		ringBuffer.tail = (ringBuffer.tail + 1) % BUFFER_SIZE;  
		}
        // Add sleep or yield if necessary
	sleep(1.5);
    	}
    	return NULL;
}


void *thread_func(void *data)
{
//      struct period_info pinfo;
// 	periodic_task_init(&pinfo);
	struct timespec time_value;
	int h;
	h = lgGpiochipOpen(4); // Open the first GPIO chip (gpiochip0)
	if (h < 0) {
        	//fprintf(stderr, "Error opening GPIO chip: %d\n", h);
	}	
        lgGpioClaimInput(h,LG_SET_PULL_DOWN,GPIO_PIN);
	/* Do RT specific stuff here */
	while (1) 
	{
		if (lgGpioRead(h,GPIO_PIN) == 1) {
			clock_gettime(CLOCK_REALTIME, &time_value);
			time_value.tv_nsec -= 200000000L;
			memcpy(&ringBuffer.buffer[ringBuffer.head].time_value, &time_value, sizeof(struct timespec));	
			ringBuffer.head = (ringBuffer.head + 1) % BUFFER_SIZE;
			sleep(1.2);}
		//do_rt_task(h);
		//wait_rest_of_period();
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
