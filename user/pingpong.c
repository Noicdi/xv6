/*
 *  Author: Noicdi noicdi99@gmail.com
 *  Date: 2022-09-04 01:38:52
 *  LastEditors: Noicdi noicdi99@gmail.com
 *  LastEditTime: 2022-09-04 01:41:56
 *  FilePath: /xv6-labs-2022/user/pingpong.c
 *  Description: send "ping" "pong" between two processes
 */

#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  /*
   *                 parent[1] -> parent[0]
   * parent process:                        :child process
   *                  child[0] <-  child[1]
   *
   * 详见《Linux/Unix 系统编程手册》 P731
   */
  int parent[2];
  int child[2];
  if (pipe(parent) == -1 || pipe(child) == -1) {
    fprintf(2, "Error: pingpong\n");
    exit(-1);
  }

  char buf[5];

  int pid = fork();
  if (pid > 0) {
    close(parent[0]);
    close(child[1]);

    write(parent[1], "ping", strlen("ping"));
    close(parent[1]);

    int status;
    wait(&status);
    if (status < 0) {
      fprintf(2, "Error: pingpong\n");
      exit(-1);
    }

    read(child[0], buf, 5);
    close(child[0]);
    fprintf(1, "%d: received %s\n", getpid(), buf);
  } else if (pid == 0) {
    close(parent[1]);
    close(child[0]);

    read(parent[0], buf, 5);
    close(parent[0]);
    fprintf(1, "%d: received %s\n", getpid(), buf);

    write(child[1], "pong", strlen("pong"));
    close(child[1]);
  } else {
    fprintf(2, "Usage: sleep [x] ticks, x >=0\n");
    exit(-1);
  }

  exit(0);
}
