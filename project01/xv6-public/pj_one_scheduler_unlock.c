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
        cprintf("SchedulerUnlock is called...\n");
        if(password == 2019087192) {
                myproc()->lock = UNLOCKED;
                withdraw_lock();
                __asm__("int $130");
                exit(); 
        } else {
                cprintf("Password not matched!\n");
                cprintf("PID: %d TIME_QUANTUM: %d CURRENT_LEVEL: %d", myproc()->pid, myproc()->executed_ticks, myproc()->level);
                kill(myproc()->pid);
        }
}

int
sys_schedulerUnlock(void)
{
        int password;
        if(argint(0, &password) <0) 
		return -1;
        schedulerUnlock(password);
	return 0;
}
