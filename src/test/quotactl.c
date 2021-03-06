/* -*- Mode: C; tab-width: 8; c-basic-offset: 2; indent-tabs-mode: nil; -*- */

#include "rrutil.h"

static char home_device[1001];

static void find_home_device(void) {
  struct stat home_stat;
  const char* home = getenv("HOME");
  FILE* f;
  char mount_line[2000];

  if (!home || stat(home, &home_stat)) {
    atomic_printf("Can't stat %s; aborting test\n", home);
    atomic_puts("EXIT-SUCCESS");
    exit(0);
  }

  f = fopen("/proc/self/mountinfo", "rt");
  test_assert(f != NULL);
  while (fgets(mount_line, sizeof(mount_line), f)) {
    int maj, min;
    int ret;
    ret = sscanf(mount_line, "%*d %*d %d:%d %*s %*s %*s %*s - %*s %1000s %*s",
                 &maj, &min, home_device);
    // optional field (7) missing?
    if (ret != 3) {
      sscanf(mount_line, "%*d %*d %d:%d %*s %*s %*s - %*s %1000s %*s", &maj,
             &min, home_device);
    }
    if (maj == major(home_stat.st_dev) && min == minor(home_stat.st_dev)) {
      atomic_printf("%s (%d:%d) is on device special file %s\n", home, maj, min,
                    home_device);
      return;
    }
  }

  atomic_printf("Can't find filesystem containing %s (%d:%d); aborting test\n",
                home, major(home_stat.st_dev), minor(home_stat.st_dev));
  atomic_puts("EXIT-SUCCESS");
  exit(0);
}

int main(int argc, char* argv[]) {
  struct dqblk dq;
  int ret;

  find_home_device();
  ret = quotactl(QCMD(Q_GETQUOTA, USRQUOTA), home_device, getuid(),
                 (caddr_t) & dq);
  if (ret < 0 && errno == ENOSYS) {
    atomic_puts("Quotas not supported in this kernel; aborting test");
  } else if (ret < 0 && errno == ESRCH) {
    atomic_puts("Quotas not enabled on this file system; aborting test");
  } else {
    test_assert(0 == ret);
    atomic_printf("QIF bits=%x\n", dq.dqb_valid);
  }
  atomic_puts("EXIT-SUCCESS");
  return 0;
}
