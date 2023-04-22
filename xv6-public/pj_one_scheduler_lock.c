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
        cprintf("SchedulerLock is called...");
        if(password == 2019087192) {
                myproc()->lock = LOCKED;
                set_lockedproc();
                //global_tick_reset to 0.
                set_global_tick_zero();
                __asm__("int $129");
                exit();
        } else {
                cprintf("Password not matched!\n");
                cprintf("PID: %d TIME_QUANTUM: %d CURRENT_LEVEL: %d", myproc()->pid, myproc()->executed_ticks, myproc()->level);
                kill(myproc()->pid);
        }
}

int
sys_schedulerLock(void)
{
        int password;
        if(argint(0, &password) <0) 
		return -1;
        schedulerLock(password);
	return 0;
}
