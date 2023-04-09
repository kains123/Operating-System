#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"

//getLevel system call

int 
getLevel(void)
{
	return 0;
}

int
sys_get_level(void)
{
	return getLevel();
}
