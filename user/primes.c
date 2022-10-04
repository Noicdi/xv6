/*
 *  Author: Noicdi noicdi99@gmail.com
 *  Date: 2022-09-04 01:43:23
 *  LastEditors: Noicdi noicdi99@gmail.com
 *  LastEditTime: 2022-09-04 01:45:05
 *  FilePath: /xv6-labs-2022/user/primes.c
 *  Description: get prime numbers via pipeline
 */

#include "kernel/types.h"
#include "user/user.h"

int primes(int) __attribute__((noreturn));

int primes(int parent) {
  uint min = 0;
  if (read(parent, &min, sizeof(uint)) == 0) {
    exit(0);
  }
  fprintf(1, "prime %d\n", min);

  int child[2];
  if (pipe(child) == -1) {
    fprintf(2, "Pid(%d) Error: pipe()\n", getpid());
    exit(-1);
  }

  int pid = fork();
  if (pid > 0) {
    close(child[0]);
    uint temp = 0;

    while (read(parent, &temp, sizeof(uint)) != 0) {
      if (temp % min != 0) {
        write(child[1], &temp, sizeof(uint));
      }
    }

    close(parent);
    close(child[1]);
  } else if (pid == 0) {
    close(child[1]);
    primes(child[0]);
  } else {
    fprintf(2, "Pid(%d) Error: fork()\n", getpid());
    exit(-1);
  }

  int status = 0;
  wait(&status);
  if (status < 0) {
    fprintf(2, "Pid(%d) Error: wait()\n", getpid());
    exit(-1);
  }
  exit(0);
}

int main(int args, char *argv[]) {
  int child[2];
  if (pipe(child) == -1) {
    fprintf(2, "Pid(%d) Error: pipe()\n", getpid());
    exit(-1);
  }

  int pid = fork();
  if (pid > 0) {
    close(child[0]);

    for (uint i = 2; i <= 35; i++) {
      write(child[1], &i, sizeof(uint));
    }

    close(child[1]);
  } else if (pid == 0) {
    close(child[1]);
    primes(child[0]);
  } else {
    fprintf(2, "Error: fork()\n");
    exit(-1);
  }

  int status = 0;
  wait(&status);
  if (status < 0) {
    fprintf(2, "Pid(%d) Error: wait()\n", getpid());
    exit(-1);
  }
  exit(0);
}
