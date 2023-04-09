#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

//setPriority  system call

void
setPriority(int pid, int priority)
{

}

int
sys_set_priority(void)
{
	int pid = 0;
	int priority = 0;
        setPriority(pid, priority);
	return 0;
}
