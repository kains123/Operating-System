#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

#define MLFQ_NUM 3 //MLFQ의 큐는 3개로 이루어져 있다. 

static const uint MLFQ_BOOSTING_INTERVAL = 100;
static const uint MLFQ_MAX_TICKS[MLFQ_NUM] = {4, 6, 8}; //Ln : 2n + 4 ticks
int mlfq_enqueue(int lev, struct proc *p);
int mlfq_dequeue(int lev, struct proc **ret);