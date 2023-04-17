#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"


//schedulerUnlock  system call
void
schedulerUnlock(int password)
{
        if(password == 2019087192) {
                __asm__("int $130");
                exit(); 
        }
}

int
sys_schedulerUnlock(void)
{
        int password = 0;
        schedulerUnlock(password);
	return 0;
}
