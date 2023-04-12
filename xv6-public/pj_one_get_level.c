#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

extern struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;
//getLevel system call

int
getLevel(void)
{
	struct proc *p = myproc();
  acquire(&ptable.lock);
  uint level = p->level; //Fetch level of process
  release(&ptable.lock);
  if(0 <= level && level < MLFQ_NUM)
    return level;
  return -1;
}

int
sys_get_level(void)
{
	return getLevel();
}
