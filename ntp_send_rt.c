/*                                                                  
 * POSIX Real Time Example
 * using a single pthread as RT thread
 */
#include <errno.h> 
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
#define GPIO_PIN 18
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

static void do_rt_task(int h)
{
        /* Do RT stuff here. */
	//printf("toggle pin");
    	lgGpioWrite(h,GPIO_PIN,1);
	sleep(0.3);
	lgGpioWrite(h,GPIO_PIN,0);
	sleep(1);

	
}

void *thread_func(void *data)
{
//      struct period_info pinfo;
// 	periodic_task_init(&pinfo);
	//printf("started thread/n");
	int h;
	h = lgGpiochipOpen(4); // Open the first GPIO chip (gpiochip0)
	if (h < 0) {
        //	fprintf(stderr, "Error opening GPIO chip: %d\n", h);
	}	
        lgGpioClaimOutput(h,0,GPIO_PIN,0);
	/* Do RT specific stuff here */
	while (1) 
	{
		do_rt_task(h);
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
 
        /* Join the thread and wait until it is done */
        ret = pthread_join(thread, NULL);
        if (ret)
                printf("join pthread failed: %m\n");
 
out:
        return ret;
}
