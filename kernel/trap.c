#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void) {
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void trapinithart(void) {
  w_stvec((uint64)kernelvec);
}

// write a cow page
// return -1 if
// 1. page address form va is not cow page
// 2. xv6 not have enough pages
// 3. fail to mappages
int writecowpage(pagetable_t pagetable, uint64 va) {
  va = PGROUNDDOWN(va);
  pte_t *pte = walk(pagetable, va, 0);
  if (pte == 0) {
    panic("COW: fail to get pte");
  }

  uint flags = PTE_FLAGS(*pte);
  if (!(flags & PTE_COW)) {
    // this page is not a cow page
    printf("COW: This page from %p is not a cow page\n", va);
    return -1;
  }

  uint64 pa = PTE2PA(*pte);
  if (cowcount(pa, 0) > 1) {
    char *mem = 0;
    flags |= PTE_W;
    flags &= ~PTE_COW;

    if ((mem = kalloc()) == 0) {
      printf("COW: fail to kalloc, kill current process\n");
      // 如果没有足够的页面则杀死进程
      return -1;
    }

    memmove(mem, (char *)pa, PGSIZE);
    uvmunmap(pagetable, va, 1, 1);
    if (mappages(pagetable, va, PGSIZE, (uint64)mem, flags) != 0) {
      printf("COW: fail to mappages\n");
      kfree(mem);
      return -1;
    }
  } else {
    *pte |= PTE_W;
    *pte &= ~PTE_COW;
  }

  return 0;
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void) {
  int which_dev = 0;

  // r_sstatus() 从 sstatus 寄存器中获取状态码
  // 状态码为 0 则是用户态
  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();

  // save user program counter.
  // 可以会调度到其他进程
  // 需要保存当前进程的 user pc
  p->trapframe->epc = r_sepc();

  // RISC-V manual Chinese P100 图10.3
  // 关于异常和中断的原因
  if (r_scause() == 8) {
    // system call

    if (killed(p))
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    // recover to next instruction of 'ecall'
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    // 之前需要读写寄存器，于是关中断
    // 现在所有寄存器已经设置好了
    // 就需要开中断
    intr_on();

    syscall();
  } else if (r_scause() == 15) {
    // Store page fault
    uint64 va = r_stval();

    if (va > p->sz) {
      printf("Error: The virtual address greater then this process's size\n");
      p->killed = 1;
    } else if (writecowpage(p->pagetable, va) != 0) {
      printf("Error: This page is not a cow-page or xv6 don't have enouth page, so not allow to write\n");
      p->killed = 1;
    }
  } else if ((which_dev = devintr()) != 0) {
    // ok
  } else {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if (killed(p))
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2) {
    // p->sigticks != 0 -> sigalarm() 被设置
    // p->sigframe == 0 -> handler() 未被调用
    if (p->sigticks != 0) {
      p->sigcount++;
      if (p->sigcount == p->sigticks) {
        p->sigcount = 0;
        if (p->sigframe == 0) {
          if ((p->sigframe = (struct trapframe *)kalloc()) != 0) {
            memmove(p->sigframe, p->trapframe, PGSIZE);
            p->trapframe->epc = p->sighandler;
          }
        }
      }
    }

    // 定时器中断，线程调度
    yield();
  }

  usertrapret();
}

//
// return to user space
//
void usertrapret(void) {
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.

  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void kerneltrap() {
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if (intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if ((which_dev = devintr()) == 0) {
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void clockintr() {
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int devintr() {
  uint64 scause = r_scause();

  if ((scause & 0x8000000000000000L) && (scause & 0xff) == 9) {
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if (irq == UART0_IRQ) {
      uartintr();
    } else if (irq == VIRTIO0_IRQ) {
      virtio_disk_intr();
    } else if (irq) {
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if (irq)
      plic_complete(irq);

    return 1;
  } else if (scause == 0x8000000000000001L) {
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if (cpuid() == 0) {
      clockintr();
    }

    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}
