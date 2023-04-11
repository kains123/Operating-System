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
//setPriority  system call

void
setPriority(int pid, int priority)
{
	struct proc *p;
	acquire(&ptable.lock);
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
		if(p->pid == pid) {
			p->priority = priority;
			break;
		}
	}
	release(&ptable.lock);
	return;
}

int
sys_set_priority(void)
{
	int pid;
	int priority;
	if(argint(0, &pid) <0) 
		return -1;
	if(argint(0, &priority) <0) 
		return -1;
	setPriority(pid, priority);
	return 0;
}
