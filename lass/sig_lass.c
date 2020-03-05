// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  sig_lass.c
 *  Copyright (C) 2020 Intel Corporation
 *  Author: Pengfei, Xu <pengfei.xu@intel.com>
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

#define VSYS_ADDR 0xffffffffff600000
#define ILLEGAL_ADDR 0xffffffffff601000

static int pass_num, fail_num;

int usage(void)
{
	printf("Usage: [u|v|r|]\n");
	printf("u  Read one illegal kernel space\n");
	printf("v  Read legal vsyscall address\n");
	printf("r  Read random kernel space\n");
	exit(2);
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
	printf("sig:%d, no:%d, err:%d, si_code:%d\n",
		signum, si->si_signo, si->si_errno, si->si_code);
	if (si->si_code == 128) {
		pass_case("Got #GP as expected for lass");
		exit(0);
	} else {
		printf("Got unexpected si_code:%d for lass\n",
			si->si_code);
		fail_case("Got unexpected si_code signal");
		exit(1);
	}
	exit(1);
}

int read_vsys_addr(void)
{
	int hack_a;

	printf("VSYS_ADDR:0x%lx\n", VSYS_ADDR);
	hack_a = *(const int *)VSYS_ADDR;
	printf("0x%lx content:0x%x\n", VSYS_ADDR, hack_a);
	fail_case("should not read vsyscall space for lass");
	return 1;
}

int read_kernel_addr(void)
{
	int kernel_addr;

	printf("ILLEGAL_ADDR:0x%lx\n", ILLEGAL_ADDR);
	kernel_addr = *(const int *)ILLEGAL_ADDR;
	printf("0x%lx content:0x%x\n", ILLEGAL_ADDR, kernel_addr);
	fail_case("should not read kernel space for lass");
	return 1;
}

int read_kernel_random(void)
{
	unsigned long a, b, addr;
	int addr_content;

	srand((unsigned) (time(NULL)));
	a = rand();
	b = rand();
	addr = ((a<<32) | 0xffff800000000000ul) | b;
	printf("kernel space random addr:0x%lx\n", addr);
	addr_content = *(const int *)addr;
	printf("0x%lx content:0x%x\n", addr, addr_content);
	fail_case("should not read random kernel space");
	return 1;
}

int main(int argc, char *argv[])
{
	char parm;
	int r;
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
	} else if (argc == 2) {
		if (sscanf(argv[1], "%c", &parm) != 1) {
			printf("Invalid parm:%c\n", parm);
			usage();
		}
		printf("parm:%c\n", parm);
	} else {
		usage();
	}

	switch (parm) {
	case 'v':
		read_vsys_addr();
		break;
	case 'u':
		read_kernel_addr();
		break;
	case 'r':
		read_kernel_random();
		break;
	default:
		usage();
	}

	return !(!fail_num);
}
