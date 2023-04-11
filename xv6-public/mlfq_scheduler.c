#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "mlfq_scheduler.h"

//NPROC is the maximum number of processes per user.
#define GET_MIN(a, b) ((a) < (b) ? (a) : (b))
#define BEGIN(mlfq) (((mlfq)->front + 1) % (NPROC + 1))
#define NEXT(iter) (((iter) + 1) % (NPROC + 1))
#define PREV(iter) (((iter) + NPROC) % (NPROC + 1))
#define END(mlfq) (((mlfq)->rear + 1) % (NPROC + 1))

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

typedef struct proc_queue
{
  struct proc *data[NPROC];
  int front, rear;
  int size;
} proc_queue_t;

struct
{
  proc_queue_t queue[MLFQ_NUM];
  int executed_ticks; //MFLQ scheduler worked ticks
} mlfq_mgr;


void mlfq_init()
{
  int lev;
  proc_queue_t *queue;

  for (lev = 0; lev < MLFQ_NUM; ++lev)
  {
    queue = &mlfq_mgr.queue[lev];

    memset(queue->data, 0, sizeof(struct proc *) * NPROC);

    queue->front = 1;
    queue->rear = 0;
    queue->size = 0;
  }

  mlfq_mgr.executed_ticks = 0;
}


static void
mlfq_priority_boost(void)
{
  struct proc *p;
  int lev;
  for (lev = 1; lev < MLFQ_NUM; ++lev)
  {
    while (mlfq_mgr.queue[lev].size)
    {
      mlfq_dequeue(lev, &p);
      mlfq_enqueue(0, p);
      p->executed_ticks = 0;
    }
  }
  mlfq_mgr.executed_ticks = 0;
}

int mlfq_enqueue(int lev, struct proc *p)
{
  proc_queue_t *const queue = &mlfq_mgr.queue[lev];

  // if queue is full, return failure
  if (queue->size == NPROC)
    return -1;

  queue->rear = (queue->rear + 1) % NPROC;
  queue->data[queue->rear] = p;
  ++queue->size;

  p->level = lev;

  return 0;
}

int mlfq_dequeue(int lev, struct proc **ret)
{
  proc_queue_t *const queue = &mlfq_mgr.queue[lev];
  struct proc *p;

  // if queue is empty, return failure
  if (queue->size == 0)
    return -1;

  p = queue->data[queue->front];
  queue->data[queue->front] = 0;

  queue->front = (queue->front + 1) % NPROC;
  --queue->size;

  p->level = -1;

  if (ret != 0)
    *ret = p;

  return lev;
}


void
mlfq_print(void)
{
  int lev, idx;
  static const char *state2str[] = {
      [UNUSED] "unused",   [EMBRYO] "embryo",  [SLEEPING] "sleep ",
      [RUNNABLE] "runble", [RUNNING] "run   ", [ZOMBIE] "zombie"};
  cprintf("MLFQ Queue Info\n");
  for(lev = 0; lev < MLFQ_NUM; lev++) {
    cprintf("Level %d (%d)\n", lev, queue_size(&mlfq_mgr.queue[lev]));
    cprintf("f=%d, r=%d\n", mlfq_mgr.queue[lev].front, mlfq_mgr.queue[lev].rear);
    for(idx = BEGIN(&mlfq_mgr.queue[lev]); idx != END(&mlfq_mgr.queue[lev]); idx = NEXT(idx))
      cprintf("[%d] %s %s\n", idx, state2str[mlfq_mgr.queue[lev].data[idx]->state],
              mlfq_mgr.queue[lev].data[idx]->name);
    cprintf("\n");
  }
}

