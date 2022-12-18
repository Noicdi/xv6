#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define N 5
char buf[N];

void pong(int *parent_to_child, int *child_to_parent) {
  if (read(parent_to_child[0], buf, N) < 0) {
    printf("read failed\n");
  }
  printf("%d: received %s\n", getpid(), buf);
  if (write(child_to_parent[1], "pong", 4) != 4) {
    printf("write failed\n");
  }
}

void ping(int *parent_to_child, int *child_to_parent) {

  if (write(parent_to_child[1], "ping", 4) != 4) {
    printf("write failed\n");
  }
  if (read(child_to_parent[0], buf, N) < 0) {
    printf("read failed\n");
  }
  printf("%d: received %s\n", getpid(), buf);
}

int main(int argc, char *argv[]) {
  int parent_to_child[2];
  int child_to_parent[2];

  int pid;

  if (pipe(parent_to_child) < 0 || pipe(child_to_parent) < 0) {
    printf("pipe failed\n");
  }
  if ((pid = fork()) < 0) {
    printf("fork failed\n");
  }
  if (pid == 0) {
    pong(parent_to_child, child_to_parent);
  } else {
    ping(parent_to_child, child_to_parent);
  }

  exit(0);
}

// int main(int argc, char *argv[]) {
/*
 *                 parent[1] -> parent[0]
 * parent process:                        :child process
 *                  child[0] <-  child[1]
 *
 * 详见《Linux/Unix 系统编程手册》 P731
 */
/*
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
*/
