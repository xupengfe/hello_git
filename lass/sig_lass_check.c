// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  sig_lass_check.c
 *  Copyright (C) 2020 Intel Corporation
 *  Author: Pengfei, Xu <pengfei.xu@intel.com>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <setjmp.h>

#define VSYS_ADDR 0xffffffffff600000
#define ILLEGAL_ADDR 0xffffffffff601000
#define KERNEL_BORDER_ADDR 0xffff800000000000
#define TEST_NUM 100

static int pass_num, fail_num;
static jmp_buf jmpbuf;
static unsigned long kernel_random_addr;

int usage(void)
{
	printf("Usage: [i|v|r|a]\n");
	printf("i  Read one illegal kernel space\n");
	printf("v  Read legal vsyscall address\n");
	printf("r  Read random kernel space\n");
	printf("a  Test all\n");
	return 2;
}

static int fail_case(const char *format)
{
	printf("[FAIL]\t%s\n", format);
	fail_num++;
	return 1;
}

static int pass_case(const char *format)
{
	printf("[PASS]\t%s\n", format);
	pass_num++;
	return 0;
}

void segv_handler(int signum, siginfo_t *si, void *uc)
{
	unsigned long sig_err, ip, bp, ax, bx, cx, dx, trap, di, rsi;
	ucontext_t *ucp = (ucontext_t *)uc;

	sig_err =  ucp->uc_mcontext.gregs[REG_ERR];
	ip = ucp->uc_mcontext.gregs[REG_RIP];
	bp = ucp->uc_mcontext.gregs[REG_RBP];
	ax = ucp->uc_mcontext.gregs[REG_RAX];
	bx = ucp->uc_mcontext.gregs[REG_RBX];
	cx = ucp->uc_mcontext.gregs[REG_RCX];
	dx = ucp->uc_mcontext.gregs[REG_RDX];
	trap = ucp->uc_mcontext.gregs[REG_TRAPNO];
	di = ucp->uc_mcontext.gregs[REG_RDI];
	rsi = ucp->uc_mcontext.gregs[REG_RSI];
	ucp->uc_mcontext.gregs[REG_RDI];
	printf("sig:%d,no:%d,err:%d,si_code:%d,rerr:%ld,ip:%lx,bp:%lx\n",
		signum, si->si_signo, si->si_errno, si->si_code,
		sig_err, ip, bp);
	printf("ax:%lx,bx:%lx,cx:%lx,dx:%lx,trap:%lx,di:%lx,rsi:%lx\n",
		ax, bx, cx, dx, trap, di, rsi);
	if (si->si_code == 128) {
		pass_case("Got #GP as expected for lass");
	} else {
		printf("Got unexpected si_code:%d for lass\n",
			si->si_code);
		fail_case("Got unexpected si_code signal");
	}
	siglongjmp(jmpbuf, 1);
}

int read_kernel_linear(unsigned long addr)
{
	int addr_content;

	if (addr < KERNEL_BORDER_ADDR) {
		printf("addr:0x%lx is smaller than 0x%lx\n",
			addr, KERNEL_BORDER_ADDR);
		fail_case("Set addr error");
		return 1;
	}
	printf("Kernel linear addr:0x%lx\n", addr);
	if (sigsetjmp(jmpbuf, 1) == 0) {
		addr_content = *(const int *)addr;
		printf("0x%lx content:0x%x\n", addr, addr_content);
		fail_case("should not read kernel linear addr for lass");
	}

	return 0;
}

int get_kernel_random(void)
{
	unsigned long a, b;

	/* this is for some special time test
	 * srand((unsigned) (time(NULL)));
	 */
	a = rand();
	b = rand();
	kernel_random_addr = ((a<<32) | 0xffff800000000000ul) | b;

	return 0;
}

int read_kernel_random(void)
{
	int i;

	printf("Test kernel random addr for lass:\n");
	for (i = 1; i <= TEST_NUM; i++) {
		printf("\t%d times test:\n", i);
		get_kernel_random();
		read_kernel_linear(kernel_random_addr);
	}

	return 0;
}

int check_kernel_random(void)
{
	int record_fail, record_pass;

	record_fail = fail_num;
	record_pass = pass_num;
	read_kernel_random();
	if (fail_num > record_fail) {
		fail_num = record_fail + 1;
		pass_num = record_pass;
	} else
		pass_num = record_pass + 1;

	return 0;
}

int main(int argc, char *argv[])
{
	char parm;
	int r, record;
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

	if (argc == 1) {
		usage();
		return 2;
	} else if (argc == 2) {
		if (sscanf(argv[1], "%c", &parm) != 1) {
			printf("Invalid parm:%c\n", parm);
			usage();
		}
		printf("parm:%c\n", parm);
	} else {
		usage();
		return 2;
	}

	switch (parm) {
	case 'v':
		read_kernel_linear(VSYS_ADDR);
		break;
	case 'i':
		read_kernel_linear(ILLEGAL_ADDR);
		break;
	case 'r':
		check_kernel_random();
		break;
	case 'a':
		read_kernel_linear(VSYS_ADDR);
		read_kernel_linear(ILLEGAL_ADDR);
		check_kernel_random();
		break;
	default:
		usage();
		return 2;
	}

	printf("[Results] pass_num:%d, fail_num:%d\n",
		pass_num, fail_num);
	return !(!fail_num);
}
