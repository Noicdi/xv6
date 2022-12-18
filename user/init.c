// init: The initial user-level program

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/spinlock.h"
#include "kernel/sleeplock.h"
#include "kernel/fs.h"
#include "kernel/file.h"
#include "user/user.h"
#include "kernel/fcntl.h"

char *argv[] = {"sh", 0};

int main(void) {
  int pid, wpid;

  // console -> stdin
  // stdin -> fd 0
  if (open("console", O_RDWR) < 0) {
    mknod("console", CONSOLE, 0);
    open("console", O_RDWR);
  }
  // dup(oldfd)，从最小的文件描述符开始，返回一个新的文件描述符
  // oldfd 和 newfd 指向同一个文件
  dup(0); // stdout -> fd 1
  dup(0); // stderr -> fd 2

  for (;;) {
    // 打开一个 sh
    printf("init: starting sh\n");
    pid = fork();
    if (pid < 0) {
      printf("init: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      exec("sh", argv);
      printf("init: exec sh failed\n");
      exit(1);
    }

    for (;;) {
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *)0);
      if (wpid == pid) {
        // the shell exited; restart it.
        // sh 正常退出，创建新 sh
        break;
      } else if (wpid < 0) {
        printf("init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
