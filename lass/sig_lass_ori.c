// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  lass_test.c
 *  Copyright (C) 2020 Intel Corporation
 *  Author: Pengfei, Xu <pengfei.xu@intel.com>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <setjmp.h>

#define VSYS_ADDR 0xffffffffff601000
//#define VSYS_ADDR 0xffffffffff601000

static jmp_buf jmpbuf;

void segv_handler(int signum, siginfo_t *si, void *uc)
{
	unsigned long sig_err, ip, bp;
	ucontext_t *ucp = (ucontext_t *)uc;

	sig_err =  ucp->uc_mcontext.gregs[REG_ERR];
	ip = ucp->uc_mcontext.gregs[REG_RIP];
	bp = ucp->uc_mcontext.gregs[REG_RBP];
	printf("sig:%d,no:%d,err:%d,si_code:%d,rerr:%ld,ip:0x%lx,bp:0x%lx\n",
		signum, si->si_signo, si->si_errno, si->si_code,
		sig_err, ip, bp);
	if (si->si_code == 128) {
		printf("[PASS] Got #GP as expected in lass\n");
		exit(0);
	} else {
		printf("[FAIL] Got unexpected si_code:%d in lass\n",
			si->si_code);
		//exit(1);
	}
	//exit(1);
	siglongjmp(jmpbuf, 1);
}

int main(void)
{
	int r, hack_a;
	struct sigaction sa;

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

	printf("VSYS_ADDR:0x%lx\n", VSYS_ADDR);

	if (sigsetjmp(jmpbuf, 1) == 0) {
		hack_a = *(const int *)VSYS_ADDR;
		printf("%lx content:0x%x\n", VSYS_ADDR, hack_a);
		printf("[FAIL]: should not read kernel space in lass\n");
	} else 
		printf("Test done, could exit\n");

	return 1;
}
