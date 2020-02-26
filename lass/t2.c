/* SPDX-License-Identifier: GPL-2.0 */

#define _GNU_SOURCE

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/ucontext.h>
#include <errno.h>
#include <err.h>
#include <sched.h>
#include <string.h>

void segv_handler(int signum, siginfo_t *si, void *uc)
{
  printf("sig:%d, no:%d, err:%d, si_code:%d\n",
    signum, si->si_signo, si->si_errno, si->si_code);
  exit(0);
}

int main()
{
  struct timeval tv, sys_tv;
  struct timeval *tp = &tv, *stp = &sys_tv;
  struct sigaction sa;
  int r, hack_a;

  r = sigemptyset(&sa.sa_mask);
  if (r) {
    printf("Init empty signal failed\n");
    return -1;
  }
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = segv_handler;
  r = sigaction(SIGSEGV, &sa, NULL);
  if (r) {
    printf("Could not handle SIGSEGV(11)\n");
    return -1;
  }

  printf("syscall SYS_gettimeofday num:%d\n", SYS_gettimeofday);
  gettimeofday(&tv, NULL);
  printf("gettimeofday time: %ld sec %ld usec, &tv:%p\n",
    tv.tv_sec, tv.tv_usec, &tv);
  syscall(SYS_gettimeofday, &sys_tv, NULL);
  printf("gettimeofday time: %ld sec %ld usec, &tv:%p\n",
    tv.tv_sec, tv.tv_usec, &tv);
  printf("SYS_gettimeofday: %ld sec %ld usec, sys_tv:%p, %p, %p, %p\n",
    sys_tv.tv_sec, sys_tv.tv_usec, &sys_tv, stp, &stp, &(*stp));

  pid_t pid = getpid();
  printf("getpid pid = %d\n", pid);

  pid = syscall(SYS_getpid);
  printf("SYS_getpid pid = %d\n", pid);

  hack_a = *(const int *)0xffffffffff600000;
  printf("0xffffffffff600000:0x%x\n", hack_a);
  //hack_a = *(const int *)0xffffffffff601000; // will SIGSEGV

  return 0;
}
