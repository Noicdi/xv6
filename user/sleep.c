/*
 *  Author: Noicdi noicdi99@gmail.com
 *  Date: 2022-09-04 01:37:17
 *  LastEditors: Noicdi noicdi99@gmail.com
 *  LastEditTime: 2022-09-04 01:37:31
 *  FilePath: /xv6-labs-2022/user/sleep.c
 *  Description: sleep of simple version for xv6 shell
 */

#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(2, "Usage: sleep [x] ticks, x >=0\n");
    exit(1);
  }

  const char *str = argv[1];

  for (uint i = 0; str[i] != '\0'; i++) {
    if (!('0' <= str[i] && str[i] <= '9')) {
      fprintf(2, "Usage: sleep [x] ticks, x >=0\n");
      exit(1);
    }
  }

  uint ticks = atoi(str);
  sleep(ticks);

  exit(0);
}
