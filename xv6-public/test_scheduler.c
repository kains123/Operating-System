#include "types.h"
#include "stat.h"
#include "user.h"

#define LIFETIME (1000) /* (ticks) */
#define COUNT_PERIOD (1000000) /* (iteration) */
#define MLFQ_LEVEL (3)  /* Number of level(priority) of MLFQ scheduler */
#define MAX_NUM_PROCESSES (10) 

struct workload
{
	void (*func)(int);
	int arg;
};

/**
 * This function request to make this process scheduled in MLFQ. 
 * MLFQ_NONE			: report only the cnt value
 * MLFQ_LEVCNT			: report the cnt values about each level
 * MLFQ_YIELD			: yield itself, report only the cnt value
 * MLFQ_YIELD_LEVCNT	: yield itself, report the cnt values about each level
 */

enum { MLFQ_NONE, MLFQ_LEVCNT, MLFQ_YIELD, MLFQ_LEVCNT_YIELD };

void
test_mlfq(int type)
{
  int cnt_level[MLFQ_LEVEL] = {0, 0, 0};
  int cnt = 0;
  int i = 0;
  int curr_mlfq_level;
  int start_tick;
  int curr_tick;

  /* Get start tick */
  start_tick = uptime();

  for(;;) {
    i++;
    if(i >= COUNT_PERIOD) {
      cnt++;
      i = 0;
      curr_mlfq_level = getLevel(); /* getLevel : system call */
      cnt_level[curr_mlfq_level]++;

      /* Get current tick */
      curr_tick = uptime();

      if(curr_tick - start_tick > LIFETIME) {
        /* Time to terminate */
        break;
      }
      /* Yield process itself, not by timer interrupt */
      yield();
    }
  }

  /* Report */
  printf(1, "MLfQ, cnt : %d, lev[0] : %d, lev[1] : %d, lev[2] : %d\n" ,cnt, cnt_level[0], cnt_level[1], cnt_level[2]);
}

int
main(int argc, char *argv[])
{
	int pid;
	int i;

  // printf(1, "****TEST_SCHEDULER****");

	#define WORKLOAD_NUM (2)
	/* Workload list */
	struct workload workloads[WORKLOAD_NUM] = {
		{test_mlfq, MLFQ_NONE},
		{test_mlfq, MLFQ_NONE},
	};


	for (i = 0; i < WORKLOAD_NUM; i++)
	{
		pid = fork();
		if (pid > 0)
		{
			/* Parent */
      printf(1, "Parent %d creating child %d\n", getpid(), pid);
			continue;
		}
		else if (pid == 0)
		{
			/* Child */
			void (*func)(int) = workloads[i].func;
			int arg = workloads[i].arg;
			/* Do this workload */
			func(arg);
			exit();
		}
		else
		{
			printf(1, "%d !failed in fork!\n", getpid());
			exit();
		}
	}

	for (i = 0; i < WORKLOAD_NUM; i++)
	{
		wait(); //wait until child finished
	}

	exit();
}