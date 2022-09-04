/*
 *  Author: Noicdi noicdi99@gmail.com
 *  Date: 2022-08-24 16:07:34
 *  LastEditors: Noicdi noicdi99@gmail.com
 *  LastEditTime: 2022-09-04 01:46:29
 *  FilePath: /xv6-labs-2022/user/find.c
 *  Description: simple version of "find" of xv6 shell
 */

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char *fmtname(char *path) {
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  return p;
}

int find(char *dir, char *file) {
  int fd;
  struct stat st;

  // 验证目录是否存在
  if ((fd = open(dir, 0)) < 0) {
    fprintf(2, "find: '%s': No such directory\n", dir);
    return -1;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "find: '%s': No such directory\n", dir);
    return -1;
  }
  if (st.type != T_DIR) {
    fprintf(2, "find: '%s': No such directory\n", dir);
    return -1;
  }

  char buf[512], *p;
  if (strlen(dir) + 1 + DIRSIZ + 1 > sizeof buf) {
    printf("find: directory name too long\n");
    close(fd);
    return -1;
  }
  strcpy(buf, dir);
  p = buf + strlen(buf);
  *p++ = '/';

  struct dirent de;
  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    memmove(p, de.name, DIRSIZ);

    p[DIRSIZ] = 0;
    if (stat(buf, &st) < 0) {
      printf("find: cannot find %s\n", buf);
      continue;
    }

    switch (st.type) {
      case T_FILE:
        if (strcmp(fmtname(buf), file) == 0) {
          printf("%s\n", buf);
        }
        break;

      case T_DIR:
        if (strcmp(fmtname(buf), ".") != 0 && strcmp(fmtname(buf), "..") != 0) {
          find(buf, file);
        }
        break;
    }
  }

  return 0;
}

int main(int args, char *argv[]) {
  if (args == 2) {
    find(".", argv[1]);
  } else if (args == 3) {
    find(argv[1], argv[2]);
  } else {
    fprintf(2, "Usage: find [.|dir] [file]\n");
    exit(1);
  }

  exit(0);
}
