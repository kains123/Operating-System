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
schedulerUnlock(int password)
{

}

int
sys_scheduler_unlock(void)
{
        int password = 0;
        schedulerUnlock(password);
	return 0;
}
