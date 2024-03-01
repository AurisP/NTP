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
/*
struct period_info {
        struct timespec next_period;
        long period_ns;
};

static void inc_period(struct period_info *pinfo) 
{
        pinfo->next_period.tv_nsec += pinfo->period_ns;
 
        while (pinfo->next_period.tv_nsec >= 1000000000) {
                * timespec nsec overflow *
                pinfo->next_period.tv_sec++;
                pinfo->next_period.tv_nsec -= 1000000000;
        }
}
 
static void periodic_task_init(struct period_info *pinfo)
{
        pinfo->period_ns = 1000000;
 
        clock_gettime(CLOCK_MONOTONIC, &(pinfo->next_period));
}
 
 
static void wait_rest_of_period()
{
        //inc_period(pinfo);
 	struct timespec sleep_time;
	clock_gettime(CLOCK_REALTIME, &sleep_time);
	sleep_time.tv_nsec =  1000000000L - sleep_time.tv_nsec;

	 for simplicity, ignoring possibilities of signal wakes:
        clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &sleep_time, NULL);
}
*/
#define BUFFER_SIZE 100

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
	
   	while (1) {
        	// Check if there's data to flush
        	if (ringBuffer.head != ringBuffer.tail) {
            	// Flush data
            	printf("Data: %ld\n", ringBuffer.buffer[ringBuffer.tail].time_value.tv_nsec);
		fflush(stdout);
		ringBuffer.tail = (ringBuffer.tail + 1) % BUFFER_SIZE;  
		}
        // Add sleep or yield if necessary
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
			memcpy(&ringBuffer.buffer[ringBuffer.head].time_value, &time_value, sizeof(struct timespec));	
			ringBuffer.head = (ringBuffer.head + 1) % BUFFER_SIZE;
			sleep(1.6);}
		//do_rt_task(h);
		//wait_rest_of_period();
	}
        return NULL;
}

int main(int argc, char* argv[])
{
        pthread_t thread, bgThread;
        int ret, ret2;
    	
        /* Create a pthread with specified attributes */
        ret = pthread_create(&thread, NULL, thread_func, NULL);
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
