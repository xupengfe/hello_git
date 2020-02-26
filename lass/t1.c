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

static void sigsegv(int sig, siginfo_t *info, void *ctx_void)
{
  unsigned long segv_err;
	ucontext_t *ctx = (ucontext_t *)ctx_void;

	segv_err =  ctx->uc_mcontext.gregs[REG_ERR];
	printf("Received sig:%d, num:%d, si_code:%d, errno:%d, segv_err:%ld\n",
    sig, info->si_signo, info->si_code, info->si_errno, segv_err);
}

static void sethandler(int sig, void (*handler)(int, siginfo_t *, void *),
		       int flags)
{
	struct sigaction sa;

	printf("Set sig:%d\n", sig);
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = handler;
	sa.sa_flags = SA_SIGINFO | flags;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig, &sa, 0))
		err(1, "sigaction");
}

int main()
{
  struct timeval tv, sys_tv;
  struct timeval *tp = &tv, *stp = &sys_tv;
  unsigned int hack_a;

  sethandler(SIGSEGV, sigsegv, 0);
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

  //hack_a = *(const int *)0xffffffffff600000;
  //printf("0xffffffffff600000:0x%x\n", hack_a);
  // hack_a = *(const int *)0xffffffffff601000; // will SIGSEGV
  return 0;
}
