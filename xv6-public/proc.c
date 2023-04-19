#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"



//NPROC is the maximum number of processes per user.
static const uint MLFQ_TIME_QUANTUM[MLFQ_NUM] = {4, 6, 8};

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;


typedef struct proc_queue
{
  struct proc *data[NPROC];  //put in processes in data array.
  int front, rear; 
  int size;
} proc_queue_t;

struct
{
  //multi-level
  proc_queue_t queue[MLFQ_NUM];
  int global_executed_ticks; //MFLQ scheduler worked ticks
} mlfq_manager; // There is a global tick in mlfq (3-level feedback queue).

struct proc *lockedproc = 0;


void mlfq_init()
{
  int lev;
  proc_queue_t *queue;

  for (lev = 0; lev < MLFQ_NUM; ++lev)
  {
    queue = &mlfq_manager.queue[lev];
    memset(queue->data, 0, sizeof(struct proc *) * NPROC);
    queue->front = 1;
    queue->rear = 0;
    queue->size = 0;

    for(int i = 0; i < NPROC; i++)
    {
      if(queue->data[i] == 0 && (queue->data[i] == mlfq_manager.queue[lev].data[i]))
        cprintf("[L%d], Successfully allocated at index %d\n", lev, i);
    }
  }
  
  mlfq_manager.global_executed_ticks = 0;
  
}

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

int
queue_size(proc_queue_t *queue)
{
  return queue->rear >= queue->front ? queue->rear - queue->front : (NPROC + 1) + queue->rear - queue->front;
}


int mlfq_enqueue(int lev, struct proc *p)
{
  cprintf("ENQUEUE\n");
  proc_queue_t *const queue = &mlfq_manager.queue[lev];

  if (queue->size == NPROC)
    return -1; //The queue is full.
  queue->rear = (queue->rear + 1) % NPROC;
  queue->data[queue->rear] = p;
  (queue->size)++;
  
  p->level = lev;
  
  return 0;
}

//parameter : which level in mlfq, which process.
//if ret == 0, it's nothing, if ret = &p, it work.
int mlfq_dequeue(int lev, struct proc** ret)
{
  proc_queue_t *const queue = &mlfq_manager.queue[lev];
  cprintf("DEQUEUE [FRONT_IDX: %d]\n", queue->front);
  struct proc *p;
  // if queue is empty return  -1(error);
  if (queue->size == 0)
    return -1;
  p = queue->data[queue->front];
  //fill data = 0
  cprintf("DEQUEUE [PID: %d]\n", p->pid);
  queue->data[queue->front] = 0;

  queue->front = (queue->front + 1) % NPROC;
  //front + 1;
  queue->size = queue->size -1;
  //size -1

  //remove from the queue
  p->level = -1;
  
  if (ret != 0)
    *ret = p;

  return lev;
}


void mlfq_remove(struct proc *p)
{
  proc_queue_t *const queue = &mlfq_manager.queue[p->level];

  int i;

  for (i = 0; i < NPROC; ++i)
  {
    if (queue->data[i] == p)
    {
      break;
    }
  }

  while (i != queue->front)
  {
    queue->data[i] = queue->data[i ? i - 1 : NPROC - 1];

    i = i ? i - 1 : NPROC - 1;
  }
  queue->data[queue->front] = 0;
  queue->front = (queue->front + 1) % NPROC;
  queue->size = queue->size -1;
}


static void
mlfq_priority_boost(void)
{
  struct proc *p;
  int lev;
  for (lev = 1; lev < MLFQ_NUM; ++lev)
  {
    while (mlfq_manager.queue[lev].size) //if each queue is not empty
    {
      //dequeue and then enqueue to L0.
      mlfq_dequeue(lev, &p); 
      mlfq_enqueue(0, p);
      p->priority = 3;
      p->executed_ticks = 0; //time quantum reset.
    }
  }
  mlfq_manager.global_executed_ticks = 0;
}


void 
withdraw_lock(void) {
  cprintf("withdraw_lock is called...\n");
  int err = 0;
  if(lockedproc != 0) {
    //remove the lockedproc from the certain queue.
    proc_queue_t *const remove_queue = &mlfq_manager.queue[lockedproc->level];
    struct proc *p;
    // if queue is empty return  -1(error);
    if(remove_queue->size == 0) {
      cprintf("The queue is empty");
      err = 1;
    } else {
      p = remove_queue->data[remove_queue->front];
      //fill data = 0
      remove_queue->data[remove_queue->front] = 0;

      remove_queue->front = (remove_queue->front + 1) % NPROC;
      //front + 1;
      (remove_queue->size)--;
      //size -1
      p->level = 0;
    }
    if(err != 1) {
      /*pass the process to first of L0.*/
      proc_queue_t *const add_queue = &mlfq_manager.queue[0];
      if (add_queue->size == NPROC) {
        cprintf("The queue is full");
      } else {
        if(add_queue->front == 0) {
          add_queue->front = NPROC -1;
          //add_queue->data[63];
        } else {
          add_queue->front = (add_queue->front - 1) % NPROC;
        }
        //reset the certain process's time quantum.
        lockedproc->executed_ticks = 0;
        //reset the process's priority to 3
        lockedproc->priority = 3;
        lockedproc->lock = UNLOCKED;
        add_queue->data[add_queue->front] = lockedproc;
        (add_queue->size)++;
        p->level = 0;
        lockedproc = 0;
      }
    }
  }
  cprintf("withdraw_lock is finished...\n");
}

void set_global_tick_zero() {
  mlfq_manager.global_executed_ticks = 0;
}

void set_lockedproc() {
  lockedproc = myproc();
}

struct proc *
mlfq_select()
{
  struct proc *ret;
  int lev = 0, size, i;

  while (1)
  {
    //each queue size check 
    for (; lev < MLFQ_NUM; ++lev)
    {
      if (mlfq_manager.queue[lev].size > 0)
        break;
    }
    // no process in the mlfq (empty)
    if (lev == MLFQ_NUM)
      return 0;

    size = mlfq_manager.queue[lev].size;    
    proc_queue_t *const queue = &mlfq_manager.queue[lev];
    if(lev < 2) {
      for (i = 0; i < size; ++i)
      {
        ret = queue->data[queue->front];
        if(ret->state != RUNNABLE) {
          cprintf("************[2]************\n");
          mlfq_dequeue(lev, 0); //remove first process in queue (lev).
          mlfq_enqueue(lev, ret); //add again in the end of queue (lev).
        }
        else {
          if(ret->state == RUNNABLE) {
            // cprintf("[%d] name: %s\n", ret->pid, ret->name);
            goto found;
          }
        }
      }
    } else if (lev == 2) {
        //find the lowest priority in L2 queue
        int priority = 10000;
        int found_idx = 10000;
        //start front to end rear;
        for(int i = 0; i < size; i++){
          int idx = (queue->front + i) % NPROC;
          ret = queue->data[idx];
          //only ret->state == RUNNABLE -> found_idx can be changed.
          if(ret->priority < priority) {
            found_idx = idx;
            priority = ret->priority;
          }
        }
        if(priority != 10000 && found_idx != 10000) {
          ret = queue->data[found_idx];
          goto found;
        }
    }
    // queue has no runnable process
    // then find candidate at next lower queue
    ++lev;
  }

found: //if runnable process found. 
  // cprintf("\n\nFOUND PID: %d\n", ret->pid);
  (ret->executed_ticks)++;
  (mlfq_manager.global_executed_ticks)++;
  //pass the process to lev+1 queue.
  //case L0, L1 && if executed_ticks full.
  if (lev < MLFQ_NUM - 1 && ret->executed_ticks >= MLFQ_TIME_QUANTUM[lev])
  {
    //dequeue ret from current tree
    mlfq_dequeue(lev, 0);
    //enqueue ret to current lev + 1 queue
    mlfq_enqueue(lev + 1, ret);

    //executed_ticks reset to 0
    ret->executed_ticks = 0;

  } if(lev == MLFQ_NUM -1  && ret->executed_ticks >= MLFQ_TIME_QUANTUM[lev]) {
      ret->executed_ticks = 0;
      //if ret->priority == 0, just keep 
      if(ret->priority > 0) {
        ret->priority = ret->priority -1; //prority -
      } else {
        ret->priority = 0;
      }
      mlfq_dequeue(lev, 0);
      mlfq_enqueue(lev, ret);
  } else if (ret->executed_ticks % 1 == 0){
    cprintf("***********[1]****************\n");
    // mlfq_dequeue(lev, 0);
    // mlfq_enqueue(lev, ret);
  }
  return ret;
}


void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  mlfq_init();
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->priority = 3; //if Priority boosting work, prioriy reset to 3.
  p->executed_ticks=0;
  p->level=0;
  p->lock = UNLOCKED;

  cprintf("initialized %d\n", p->pid);
  //first push p in queue
  if (mlfq_enqueue(0, p) != 0)
  {
    cprintf("allocation error %d\n", p->pid );
    release(&ptable.lock);
    return 0;
  }

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);  
  p->state = RUNNABLE;
  cprintf("process runnable %d\n", p->pid);
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{

  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{

  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        mlfq_remove(p);
        
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        
        
        //reset of MLFQ
        p->priority = 3;
        p->lock = UNLOCKED;
        p->level = 0;
        p->executed_ticks = 0;
        release(&ptable.lock);
        
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

void print_mlfq()
{
  int lev, i;

  cprintf("\n\n######################[mlfq]#####################\n");
  for (lev = 0; lev < MLFQ_NUM; ++lev)
  {
    cprintf("[level %d]\n", lev);
    cprintf("SIZE: %d\n", mlfq_manager.queue[lev].size);
    cprintf("GLOBAL_TICKS: %d\n", mlfq_manager.global_executed_ticks);

    for (i = 0; i < mlfq_manager.queue[lev].size; ++i)
      cprintf("%d [priority : %d, executed_ticks: %d]   \n", mlfq_manager.queue[lev].data[(mlfq_manager.queue[lev].front + i) % NPROC] ? mlfq_manager.queue[lev].data[(mlfq_manager.queue[lev].front + i) % NPROC]->pid : -1, mlfq_manager.queue[lev].data[(mlfq_manager.queue[lev].front + i) % NPROC] ? mlfq_manager.queue[lev].data[(mlfq_manager.queue[lev].front + i) % NPROC]->priority : -1,  mlfq_manager.queue[lev].data[(mlfq_manager.queue[lev].front + i) % NPROC] ? mlfq_manager.queue[lev].data[(mlfq_manager.queue[lev].front + i) % NPROC]->executed_ticks : -1);

    cprintf("\n");
  }
}


//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    print_mlfq();
    // Enable interrupts on this processor.
    sti();
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    if(lockedproc != 0) {
      if(lockedproc->state != RUNNING) 
      {
        if(lockedproc->state == RUNNABLE) {
          lockedproc->state = RUNNING;
        } else {
           goto SCHEDULER;
        }
      }
      //Context Switching
      p = lockedproc;
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&(c->scheduler), p->context);
      switchkvm();
      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
      cprintf("&&&&&&&&&&&&&");
      (mlfq_manager.global_executed_ticks)++;
      if(mlfq_manager.global_executed_ticks >= MLFQ_GLOBAL_BOOSTING_TICK_INTERVAL){
          //if there is a lock just remove it!
          withdraw_lock();
      }
    } else {
      SCHEDULER:
        p = mlfq_select(); //select mlfq which to execute.

        if(p != 0)
        {
          // Switch to chosen process.  It is the process's job
          // to release ptable.lock and then reacquire it
          // before jumping back to us.
          c->proc = p;
          switchuvm(p);
          p->state = RUNNING;
          swtch(&(c->scheduler), p->context);
          switchkvm();

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
        }
        if(mlfq_manager.global_executed_ticks >= MLFQ_GLOBAL_BOOSTING_TICK_INTERVAL) {
          mlfq_priority_boost();
        }
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();
  if(!holding(&ptable.lock)) // make sure the ptable is locked.
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1) // make sure interrupt is disabled.
    panic("sched locks");
  if(p->state == RUNNING) //sleep, yield, exit
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
//same as before
void
yield(void)
{ 
  acquire(&ptable.lock); //DOC: yieldlock
  myproc()->executed_ticks +=1;
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
    // if(p->state == SLEEPING) {
    //   p->state = RUNNABLE;
    // }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
