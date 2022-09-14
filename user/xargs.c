/*
 *  Author: Noicdi noicdi99@gmail.com
 *  Date: 2022-08-30 15:31:33
 *  LastEditors: Noicdi noicdi99@gmail.com
 *  LastEditTime: 2022-09-03 21:15:10
 *  FilePath: /xv6/user/xargs.c
 *  Description: simple version of xargs for xv6 shell
 */

#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define BUF_MAX_SIZE 512

int readline(int fd, char* buf) {
  while (read(fd, buf, 1) == 1) {
    if (*buf == '\n') {
      *buf = '\0';
      return 0;
    }
    buf++;
  }
  return -1;
}

void xargs(int args, char* argv[]) {
  // cmd: argv[0]
  // arg: argv[1] ~ argv[args-1]

  char buf[BUF_MAX_SIZE];

  // 每个整行作为一次输入
  // 则 xargs of xv6 的行为类似于
  // xargs -n 1 xxx
  while (readline(0, buf) == 0) {
    char* exec_argv[MAXARG];
    uint arg_count = 0;
    // 拷贝 argv[] 到 exec_argv[] 中
    while (arg_count < args) {
      exec_argv[arg_count] = argv[arg_count];
      arg_count++;
    }

    // 对 stdin 中获取的整行进行处理
    // 去除空格，作为参数加入 exec_argv[]
    uint start = 0;
    uint len = strlen(buf);
    while (start < len) {
      exec_argv[arg_count++] = &buf[start];

      while (buf[start] != ' ' && start < len) {
        start++;
      }
      buf[start++] = '\0';
    }

    // 调用相应的命令
    if (fork() == 0) {
      exec(exec_argv[0], exec_argv);
    }
    wait(0);

    memset(buf, 0, BUF_MAX_SIZE);
  }
}

int main(int args, char* argv[]) {
  if (args < 3) {
    printf("Usage: xargs amd argv[]\n");
  } else {
    xargs(args - 1, &argv[1]);
  }

  exit(0);
}
