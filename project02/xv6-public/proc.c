#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
int nexttid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
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
// static struct proc*
// allocproc(void)
// {
//   struct proc *p;
//   char *sp;
  
//   acquire(&ptable.lock);

//   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
//     if(p->state == UNUSED)
//       goto found;

//   release(&ptable.lock);
//   return 0;

// found:
//   p->state = EMBRYO;
//   p->pid = nextpid++;
  
//   p->threads[0].state = EMBRYO;
//   p->threads[0].tid = nexttid++;
//   release(&ptable.lock);

//   // Allocate kernel stack.
//   if((p->threads[0].kstack= kalloc()) == 0){
//     p->threads[0].state = UNUSED;
//     return 0;
//   }
//   sp = p->threads[0].kstack + KSTACKSIZE;

//   // Leave room for trap frame.
//   sp -= sizeof *p->threads[0].tf;
//   p->threads[0].tf = (struct trapframe*)sp;

//   // Set up new context to start executing at forkret,
//   // which returns to trapret.
//   sp -= 4;
//   *(uint*)sp = (uint)trapret;

//   sp -= sizeof *p->context;
//   p->threads[0].context = (struct context*)sp;
//   memset(p->threads[0].context, 0, sizeof *p->threads[0].context);
//   p->threads[0].context->eip = (uint)forkret;
//   p->curtid = 0;
//   return p;
// }

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
  
  p->threads[0].state = EMBRYO;
  p->threads[0].tid = nexttid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->threads[0].kstack = kalloc()) == 0){
    p->state = UNUSED;
    p->threads[0].state = UNUSED;
    return 0;
  }
  sp = p->threads[0].kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->threads[0].tf;
  p->threads[0].tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->threads[0].context;
  p->threads[0].context = (struct context*)sp;
  memset(p->threads[0].context, 0, sizeof *p->threads[0].context);
  p->threads[0].context->eip = (uint)forkret;
  return p;
}


//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  // cprintf("*******USERINIT*********\n");
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->threads[0].tf, 0, sizeof(*p->threads[0].tf));
  p->threads[0].tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->threads[0].tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->threads[0].tf->es = p->threads[0].tf->ds;
  p->threads[0].tf->ss = p->threads[0].tf->ds;
  p->threads[0].tf->eflags = FL_IF;
  p->threads[0].tf->esp = PGSIZE;
  p->threads[0].tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;
  p->threads[0].state = RUNNABLE;
  cprintf("*******USERINIT2*********\n");
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock); //ptable.lock
  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0) {
      release(&ptable.lock);
      return -1;
    }
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0) {
      release(&ptable.lock);
      return -1;
    }
  }
  release(&ptable.lock);
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
    kfree(np->threads[0].kstack);
    np->threads[0].kstack = 0;
    np->state = UNUSED;
    np->threads[0].state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->threads[0].tf = *CURTHREAD(curproc).tf;
  // Clear %eax so that fork returns 0 in the child.
  np->threads[0].tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  np->threads[0].state = RUNNABLE;

  for (i = 0; i < NTHREAD; ++i)
    np->ustack_pool[i] = curproc->ustack_pool[i];
  np->ustack_pool[0] = curproc->ustack_pool[curproc->curtid];
  np->ustack_pool[curproc->curtid] = curproc->ustack_pool[0];

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
  struct thread *t;
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

  for (t = curproc->threads; t < &curproc->threads[NTHREAD]; ++t)
  {
    if (t->state != UNUSED)
      t->state = ZOMBIE;
  }

  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  struct thread *t;
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
      if(p->state == ZOMBIE)
      {
        // Found one.
        // pid = p->pid;
        // freevm(p->pgdir);
        // kfree(p->kstack);
        // p->kstack = 0;
        cprintf("HERE\n");
        for (t = p->threads; t < &p->threads[NTHREAD]; ++t)
        {
          p->ustack_pool[t - p->threads] = 0;
          if (t->kstack != 0)
            kfree(t->kstack);

          t->kstack = 0;
          t->tid = 0;
          t->state = UNUSED;
        }
        pid = p->pid;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;

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
  // int start = 0;
  struct proc *p;
  struct thread *t;
  struct cpu *c = mycpu();
  c->proc = 0;

  for(;;){
    // Enable interrupts on this processor.
    sti();
    // Loop over process table looking for process to run.
  
    acquire(&ptable.lock);
  
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
          continue;
      //thread num이 = 0 이면 지나가고 0 이상이면 
      for(t = p->threads; t < &p->threads[NTHREAD]; t++){
        if(t->state != RUNNABLE)
          continue;
      
        t = &CURTHREAD(p);
          
        // if (start && t == &CURTHREAD(p))
        //   panic("invalid logic");
        // start = 1;
        p->curtid = t - p->threads;
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        c->proc = p;
      
        switchuvm(p);
        t->state = RUNNING;
          
        swtch(&(c->scheduler), t->context);
        
        switchkvm();
        
        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        // if(p->state != RUNNABLE)
        //   t = &p->threads[NTHREAD];
      }
    }
    release(&ptable.lock);
    // cprintf("************SCHEDULER*******\n");
  }
}
// void
// scheduler(void)
// {
//   struct proc *p;
//   struct cpu *c = mycpu();
//   c->proc = 0;
  
//   for(;;){
//     // Enable interrupts on this processor.
//     sti();

//     // Loop over process table looking for process to run.
//     acquire(&ptable.lock);
//     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//       if(p->state != RUNNABLE)
//         continue;

//       // Switch to chosen process.  It is the process's job
//       // to release ptable.lock and then reacquire it
//       // before jumping back to us.
//       c->proc = p;
//       switchuvm(p);
//       p->state = RUNNING;

//       swtch(&(c->scheduler), p->context);
//       switchkvm();

//       // Process is done running for now.
//       // It should have changed its p->state before coming back.
//       c->proc = 0;
//     }
//     release(&ptable.lock);

//   }
// }

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
// void
// sched(void)
// {
//   int intena;
//   struct proc *p = myproc();
//   struct thread *t = &CURTHREAD(p);
//   cprintf("########sched1########\n");
//   if(!holding(&ptable.lock))
//     panic("sched ptable.lock");
//   if(mycpu()->ncli != 1)
//     panic("sched locks");
//   if(t->state == RUNNING)
//     panic("sched running");
//   if(readeflags()&FL_IF)
//     panic("sched interruptible");
//   intena = mycpu()->intena;

//   swtch(&t->context, mycpu()->scheduler);
//   cprintf("########sched3########\n");
//   mycpu()->intena = intena;
// }

void
sched(void)
{
  int intena;
  struct proc *p = myproc();
  struct thread *t = &CURTHREAD(p);
  // cprintf("*******SCHED*********\n");
  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(t->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&t->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  struct proc *p = myproc();
  acquire(&ptable.lock);  //DOC: yieldlock 
  CURTHREAD(p).state = RUNNABLE;
  // cprintf("*******YEILD*********\n");
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
  struct thread *t = &CURTHREAD(p);

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
  t->chan = chan;
  t->state = SLEEPING;
  // cprintf("*******SLEEP*********\n");
  sched();
  // Tidy up.
  t->chan = 0;

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
  struct thread *t;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
     if(p->state == RUNNABLE) {
      for(t = p->threads; t < &p->threads[NTHREAD]; t++) {
        if(t->state == SLEEPING && t->chan == chan)
          t->state = RUNNABLE;
      }
      
    }
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
  cprintf("*******KILL START**************\n");
  struct proc *p;
  struct thread *t;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      for (t = p->threads; t < &p->threads[NTHREAD]; ++t) {
        if(t->state == SLEEPING)
          t->state = RUNNABLE;
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  cprintf("*******KILL END**************\n");
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


int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{
  cprintf("Thread_create 0 \n");
  struct proc *curproc = myproc();
  struct thread *t;
  int t_idx;
  uint sz;

  // struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  // for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
  //   if(p->state == UNUSED)
  for (t = curproc->threads; t < &curproc->threads[NTHREAD]; ++t)
    if (t->state == UNUSED)
      goto found;

  release(&ptable.lock);
  
  return -1;

found:
  cprintf("Thread_create 1 \n");
  t_idx = t - curproc->threads;
  t->tid = nexttid++;
  t->state = EMBRYO;

  // Allocate kernel stack.
  if((t->kstack = kalloc()) == 0){
    cprintf("Issue!\n thread_create\n");
    t->state = UNUSED;
    t->tid = 0;
    t->kstack = 0;
    return -1;
  }
  sp = t->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *t->tf;
  t->tf = (struct trapframe*)sp;
  *t->tf = *CURTHREAD(curproc).tf;
  // Set up new context to start executing at forkret,
  // which returns to trapret.

  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *t->context;
  t->context = (struct context*)sp;
  memset(t->context, 0, sizeof *t->context);
  t->context->eip = (uint)forkret;

  if (curproc->ustack_pool[t_idx] == 0)
  {
    sz = PGROUNDUP(curproc->sz);
    if ((sz = allocuvm(curproc->pgdir, sz, sz + PGSIZE)) == 0)
    {
      cprintf("cannot alloc user stack\n");
      goto bad;
    }

    curproc->ustack_pool[t_idx] = sz;
    curproc->sz = sz;
  }
  sp = (char *)curproc->ustack_pool[t_idx];

  sp -= 4;
  *(uint *)sp = (uint)arg;

  sp -= 4;
  *(uint *)sp = 0xffffffff;

  //go to start_routine
  cprintf("Thread_create 2 \n");
  t->tf->esp = (uint)sp;
  t->tf->eip = (uint)start_routine;

  *thread = t->tid;

  t->state = RUNNABLE;
  release(&ptable.lock);
  return 0;

bad:
  t->kstack = 0;
  t->tid = 0;
  t->state = UNUSED;

  release(&ptable.lock);

  return -1;
}

void thread_exit(void *retval)
{
  cprintf("*********thread_exit************\n");
  struct proc *curproc = myproc();
  struct thread *curthread = &CURTHREAD(curproc);

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1((void*)curthread->tid);

  // Jump into the scheduler, never to return.
  curthread->retval = retval;
  curthread->state = ZOMBIE;

  sched();
  panic("zombie exit");
}

int thread_join(thread_t thread, void **retval){
  //wait the exit of the thread and return the value returned by thread_exit().
  struct proc *p;
  struct thread *t;

  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; ++p)
    if (p->state == RUNNABLE)
      for (t = p->threads; t < &p->threads[NTHREAD]; ++t)
        if (t->tid == thread && t->state != UNUSED){
          cprintf("*********thread_join************\n");
          goto found;
        }
  cprintf("*********thread_join_-1************\n");
  release(&ptable.lock);
  return -1;

found: 
  if(t->state != ZOMBIE) {
    sleep((void*)thread, &ptable.lock);
  }

  if (retval != 0)
    *retval = t->retval;

  kfree(t->kstack);
  t->kstack = 0;
  t->retval = 0;
  t->tid = 0;
  t->state = UNUSED;
  release(&ptable.lock);

  return 0;
}

void list(void){
  // struct proc *curproc = myproc();
  acquire(&ptable.lock);
  struct proc *p;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    // if(p->state!=RUNNING)
    //     continue;
    if(p->pid != 0 && p->killed != 1){
      cprintf("name: %s\npid: %d \nstackpagenum: %d\nsz: %d\nlimit: %d\n\n", p->name,p->pid, p->stackpagenum,p->sz,p->limit);
    }
  }
  release(&ptable.lock);
  return;
}

int setmemorylimit(int pid, int limit) {
  struct proc *p;
  int check_point = 1;

  if(limit <0) {
    return -1;
  }

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid) {
      check_point = 0;
      if(p->sz >= limit) {

         //if the received memory is less than limit return -1
        release(&ptable.lock);
        return -1;
      } else {
        p->limit = limit;
        break;
      }
    }
  }
  // pid doesn't exist return -1
  if(check_point) {
    return -1;
  }
  release(&ptable.lock);

  cprintf("setmemorylimit is worked : pid: %d limit:  %d \n", p->pid, p->limit);

  return 0;
}

