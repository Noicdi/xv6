#include "kernel/param.h"
#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "kernel/riscv.h"
#include "user/user.h"

void ugetpid_test();
void pgaccess_test();

int main(int argc, char *argv[]) {
  ugetpid_test();
  pgaccess_test();
  printf("pgtbltest: all tests succeeded\n");
  exit(0);
}

char *testname = "???";

void err(char *why) {
  printf("pgtbltest: %s failed: %s, pid=%d\n", testname, why, getpid());
  exit(1);
}

void ugetpid_test() {
  int i;

  printf("ugetpid_test starting\n");
  testname = "ugetpid_test";

  for (i = 0; i < 64; i++) {
    int ret = fork();
    if (ret != 0) {
      wait(&ret);
      if (ret != 0)
        exit(1);
      continue;
    }
    if (getpid() != ugetpid())
      err("missmatched PID");
    exit(0);
  }
  printf("ugetpid_test: OK\n");
}

void pgaccess_test() {
  char *buf;
  unsigned int abits; // accessed bits
  printf("pgaccess_test starting\n");
  testname = "pgaccess_test";

  // 分配 32 个 page，此时未使用，即 PTE_A 未设置
  buf = malloc(32 * PGSIZE);

  // 检查 buf 的 32 个 page
  // abits 应当全为 0
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed");

  // 使用了 1 2 30 这三个 page，即 accessed
  // TODO 暂时不了解为什么会被置位
  buf[PGSIZE * 1] += 1;
  buf[PGSIZE * 2] += 1;
  buf[PGSIZE * 30] += 1;

  // 检查后，abits 从 0 至 31 的 bit
  // 1 2 30 三个 bit 应该置为 1
  if (pgaccess(buf, 32, &abits) < 0)
    err("pgaccess failed");

  // 检查是否如上情况
  if (abits != ((1 << 1) | (1 << 2) | (1 << 30)))
    err("incorrect access bits set");

  free(buf);
  printf("pgaccess_test: OK\n");
}
