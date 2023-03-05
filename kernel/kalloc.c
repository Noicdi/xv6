// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte(4KB) pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct kmem {
  struct spinlock lock;
  struct run *freelist; // the listchain of free memory
};

struct kmem kmem[NCPU];

struct {
  struct spinlock lock;
  unsigned char count[COWCOUNTSZ];
} cowcounts;

char cowcount(uint64 pa, int flag) {
  char count = 0;

  acquire(&cowcounts.lock);
  if (flag > 0) {
    cowcounts.count[COWCOUNT(pa)]++;
  } else if (flag < 0) {
    cowcounts.count[COWCOUNT(pa)]--;
  }
  count = cowcounts.count[COWCOUNT(pa)];
  release(&cowcounts.lock);

  return count;
}

void kinit() {
  char lockbuf[8];
  for (int i = 0; i < NCPU; i++) {
    memset(lockbuf, 0, 8);
    snprintf(lockbuf, 8, "kmem%d", i);
    initlock(&kmem[i].lock, lockbuf);
  }
  initlock(&cowcounts.lock, "cowcount");
  // 这里初始化为 1 是为了 freerange() 是可以 kfree 每个页面
  memset(&cowcounts.count, 1, COWCOUNTSZ);
  freerange(end, (void *)PHYSTOP);
}

void freerange(void *pa_start, void *pa_end) {
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
    kfree(p);
  }
}

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
// 释放 pa 指向的 4KB 页块
// 将其全部置为 1
// 将 pa 转为 struct run
// 挂载到 freelist 中
void kfree(void *pa) {
  struct run *r;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP) {
    panic("kfree");
  }

  // 首先减少引用计数
  // 引用计数为 0 则表示最后一个进程释放了这个 page
  if (cowcount((uint64)pa, -1) != 0) {
    return;
  }

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

  push_off();
  int i = cpuid();
  pop_off();

  acquire(&kmem[i].lock);
  r->next = kmem[i].freelist;
  kmem[i].freelist = r;
  release(&kmem[i].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
// 从 freelist 中取下一个 struct run
// 将其看作 4KB 页块的指针
// 重置后返回给调用者
void *kalloc(void) {
  struct run *r;

  push_off();
  int i = cpuid();
  pop_off();

  for (int c = 0; c < NCPU; c++) {
    acquire(&kmem[i].lock);
    r = kmem[i].freelist;
    if (r) {
      kmem[i].freelist = r->next;
      release(&kmem[i].lock);
      break;
    }
    release(&kmem[i].lock);
    if (++i == NCPU) {
      i = 0;
    }
  }

  if (r) {
    memset((char *)r, 5, PGSIZE); // fill with junk
    cowcount((uint64)r, 1);
  }

  return (void *)r;
}

// collect the amount of free memory
uint64 collfree() {
  uint64 count = 0;
  struct run *freelist = 0;

  for (int i = 0; i < NCPU; i++) {
    freelist = kmem[i].freelist;
    while (freelist != 0) {
      count++;
      freelist = freelist->next;
    }
  }

  return count * PGSIZE;
}
