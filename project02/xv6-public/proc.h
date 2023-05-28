// Per-CPU state
struct cpu {
  uchar apicid;              // Local APIC ID
  struct context *scheduler; // swtch() here to enter scheduler
  struct taskstate ts;       // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS]; // x86 global descriptor table
  volatile uint started;     // Has the CPU started?
  int ncli;                  // Depth of pushcli nesting.
  int intena;                // Were interrupts enabled before pushcli?
  struct proc *proc;         // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

//Thread
struct thread
{
  thread_t tid; // Id of Thread
  enum procstate state;
  void *retval; //save the return value of thread
  struct context *context;  // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  char *kstack;                // Bottom of kernel stack for this thread
  struct trapframe *tf;        // Trap frame for current syscall
  int killed;                  // If non-zero, have been killed
};

//Process
struct proc 
{
  uint sz;                    // Size of process memory (bytes)
  pde_t *pgdir;               // Page table
  enum procstate state;       // Process state
  int pid;                    // Process ID
  struct proc *parent;        // Parent process
  int killed;                 // If non-zero, have been killed
  struct file *ofile[NOFILE]; // Open files
  struct inode *cwd;          // Current directory
  char name[16];              // Process name (debugging)
  uint level;  //Queue Level of the process (0, 1, 2)
  //threads
  struct thread threads[NTHREAD]; //threads in process
  thread_t curtid;              //currund thread's id
  uint  user_stack_pool[NTHREAD]; // user_stack_pool

  //pmanager
  int limit;                    //limit
  int stackpagenum;             //stack page number
};

// Process memory is laid out contiguously, low addresses first:
//   original data and bss
//   fixed-size stack
#define MLFQ_NUM 3
#define CURTHREAD(p) ((p)->threads[(p)->curtid])

// extern struct cpu *cpu asm("%gs:0");       // &cpus[cpunum()]
// extern struct proc *proc asm("%gs:4");     // cpus[cpunum()].proc
// extern struct thread *thread asm("%gs:8");     // cpus[cpunum()
