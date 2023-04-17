#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

//schedulerLock  system call
void
schedulerLock(int password)
{
        if(password == 2019087192) {
                __asm__("int $129");
                exit(); 
        }
}

int
sys_schedulerLock(void)
{
        int password = 0;
        schedulerLock(password);
	return 0;
}
