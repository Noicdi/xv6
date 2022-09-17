// Saved registers for kernel context switches.
struct context {
  uint64 ra; // return address
  uint64 sp; // stack pointer

  // callee-saved
  uint64 s0;
  uint64 s1;
  uint64 s2;
  uint64 s3;
  uint64 s4;
  uint64 s5;
  uint64 s6;
  uint64 s7;
  uint64 s8;
  uint64 s9;
  uint64 s10;
  uint64 s11;
};

// Per-CPU state.
struct cpu {
  struct proc *proc;      // The process running on this cpu, or null.
  struct context context; // swtch() here to enter scheduler().
  int noff;               // Depth of push_off() nesting.
  int intena;             // Were interrupts enabled before push_off()?
};

extern struct cpu cpus[NCPU];

// Figure 2.3
// per-process data for the trap handling code in trampoline.S.
// sits in a page by itself just under the trampoline page in the
// user page table. not specially mapped in the kernel page table.
// uservec in trampoline.S saves user registers in the trapframe,
// then initializes registers from the trapframe's
// kernel_sp, kernel_hartid, kernel_satp, and jumps to kernel_trap.
// usertrapret() and userret in trampoline.S set up
// the trapframe's kernel_*, restore user registers from the
// trapframe, switch to the user page table, and enter user space.
// the trapframe includes callee-saved user registers like s0-s11 because the
// return-to-user path via usertrapret() doesn't return through
// the entire kernel call stack.
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;            // return address
  /*  48 */ uint64 sp;            // stack pointer
  /*  56 */ uint64 gp;            // global pointer
  /*  64 */ uint64 tp;            // thread pointer
  /*  72 */ uint64 t0;            // temporary
  /*  80 */ uint64 t1;            // temporary
  /*  88 */ uint64 t2;            // temporary
  /*  96 */ uint64 s0;            // saved register / frame pointer
  /* 104 */ uint64 s1;            // saved register
  /* 112 */ uint64 a0;            // function argument / return value
  /* 120 */ uint64 a1;            // function argument / return value
  /* 128 */ uint64 a2;            // function
  /* 136 */ uint64 a3;            // function
  /* 144 */ uint64 a4;            // function
  /* 152 */ uint64 a5;            // function
  /* 160 */ uint64 a6;            // function
  /* 168 */ uint64 a7;            // function
  /* 176 */ uint64 s2;            // saved register
  /* 184 */ uint64 s3;            // saved register
  /* 192 */ uint64 s4;            // saved register
  /* 200 */ uint64 s5;            // saved register
  /* 208 */ uint64 s6;            // saved register
  /* 216 */ uint64 s7;            // saved register
  /* 224 */ uint64 s8;            // saved register
  /* 232 */ uint64 s9;            // saved register
  /* 240 */ uint64 s10;           // saved register
  /* 248 */ uint64 s11;           // saved register
  /* 256 */ uint64 t3;            // temporary
  /* 264 */ uint64 t4;            // temporary
  /* 272 */ uint64 t5;            // temporary
  /* 280 */ uint64 t6;            // temporary
};

// xv6-book P27:
// run, running, waiting for I/O, exiting
// ZOMBIE->僵尸
enum procstate {
  UNUSED,
  USED,
  SLEEPING,
  RUNNABLE,
  RUNNING,
  ZOMBIE
};

// Per-process state
struct proc {
  struct spinlock lock;

  // held -> hold -> 握住/持有
  // p->lock must be held when using these:
  enum procstate state; // Process state
  void *chan;           // If non-zero, sleeping on chan
  int killed;           // If non-zero, have been killed
  int xstate;           // Exit status to be returned to parent's wait
  int pid;              // Process ID

  // wait_lock must be held when using this:
  struct proc *parent; // Parent process

  // these are private to the process, so p->lock need not be held.
  uint64 kstack;               // Virtual address of kernel stack
  uint64 sz;                   // Size of process memory (bytes)
  pagetable_t pagetable;       // User page table
  struct trapframe *trapframe; // data page for trampoline.S
  struct context context;      // swtch() here to run process
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)

  int trace_mask; // trace mask
};
