// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  lass_test.c
 *  Copyright (C) 2020 Intel Corporation
 *  Author: Pengfei, Xu <pengfei.xu@intel.com>
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <dlfcn.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <errno.h>
#include <err.h>
#include <sched.h>
#include <stdbool.h>
#include <setjmp.h>
#include <sys/uio.h>

#define X86_EFLAGS_TF (1UL << 8)
#define MAPS_LINE_LEN 128

#ifdef __x86_64__
# define VSYS(x) (x)
#else
# define VSYS(x) 0
#endif

static sig_atomic_t num_vsyscall_traps;
static int sig_num, pass_num, fail_num;
static jmp_buf jmpbuf;

/*
 * /proc/self/maps, r means readable, x means excutable
 * vsyscall map: ffffffffff600000-ffffffffff601000
 */
bool vsyscall_map_r = false, vsyscall_map_x = false;
static unsigned long segv_err;

typedef long (*gtod_t)(struct timeval *tv, struct timezone *tz);
const gtod_t vgtod = (gtod_t)VSYS(0xffffffffff600000);

typedef long (*time_func_t)(time_t *t);
const time_func_t vtime = (time_func_t)VSYS(0xffffffffff600400);

/* syscalls */
static inline long sys_gtod(struct timeval *tv, struct timezone *tz)
{
	return syscall(SYS_gettimeofday, tv, tz);
}

int usage(void)
{
	printf("Usage: [g|r|v|d|t|e]\n");
	printf("g  Test call 0xffffffffff600000\n");
	printf("r  Test read 0xffffffffff600000\n");
	printf("v  Test process_vm_readv read address\n");
	printf("d  Test vsyscall trigger page fault or not\n");
	printf("t  Test syscall gettimeofday\n");
	printf("e  Test vsyscall emulation\n");
	printf("a  Test all\n");
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

static int init_vsys(void)
{
#ifdef __x86_64__
	int nerrs = 0;
	FILE *maps;
	char line[MAPS_LINE_LEN];
	bool found = false;

	maps = fopen("/proc/self/maps", "r");
	if (!maps) {
		printf("[WARN]\tCould not open /proc/self/maps\n");
		vsyscall_map_r = true;
		return 0;
	}

	while (fgets(line, MAPS_LINE_LEN, maps)) {
		char r, x;
		void *start, *end;
		char name[MAPS_LINE_LEN];

		/* sscanf() is safe as strlen(name) >= strlen(line) */
		if (sscanf(line, "%p-%p %c-%cp %*x %*x:%*x %*u %s",
			   &start, &end, &r, &x, name) != 5)
			continue;

		if (strcmp(name, "[vsyscall]"))
			continue;

		printf("\tvsyscall map: %s", line);

		if (start != (void *)0xffffffffff600000 ||
		    end != (void *)0xffffffffff601000) {
			fail_case("address range is nonsense\n");
		}

		printf("\tvsyscall permissions are %c-%c\n", r, x);
		vsyscall_map_r = (r == 'r');
		vsyscall_map_x = (x == 'x');
		printf("vsyscall_map_r:%d, vsyscall_map_x:%d\n",
			vsyscall_map_r, vsyscall_map_x);

		found = true;
		break;
	}

	fclose(maps);

	if (!found) {
		printf("\tno vsyscall map in /proc/self/maps\n");
		vsyscall_map_r = false;
		vsyscall_map_x = false;
	}

	return nerrs;
#else
	return 0;
#endif
}

void dump_buffer(unsigned char *buf, int size)
{
	int i, j;

	printf("-----------------------------------------------------\n");
	printf("buf addr:%p size = %d (%03xh)\n", buf, size, size);

	for (i = 0; i < size; i += 16) {
		printf("%04x: ", i);

		for (j = i; ((j < i + 16) && (j < size)); j++)
			printf("%02x ", buf[j]);

		printf("\n");
	}
}

static int test_vsys_r(void)
{
	bool can_read;
	int a = 0;

#ifdef __x86_64__
	printf("[RUN]\tChecking read access to the vsyscall page\n");
	if (sigsetjmp(jmpbuf, 1) == 0) {
		printf("Access 0xffffffffff600000\n");
		a = *(int *)0xffffffffff600000;
		printf("0xffffffffff600000 content:0x%x\n", a);
		can_read = true;
	} else {
		can_read = false;
	}
	printf("can_read:%d, vsyscall_map_r:%d\n",
		can_read, vsyscall_map_r);
	if (vsyscall_map_r)
		printf("[WARN]\tvsyscall should not set read for lass\n");
	if (can_read)
		fail_case("Readable vsyscall address is not as expected");
	else
		pass_case("Could not read vsyscall addr as expected");
#endif

	return 0;
}

static void sethandler(int sig, void (*handler)(int, siginfo_t *, void *),
			   int flags)
{
	struct sigaction sa;

	printf("Set receive sig:%d\n", sig);
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = handler;
	sa.sa_flags = SA_SIGINFO | flags;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig, &sa, 0))
		err(1, "sigaction");
}

static void sigsegv(int sig, siginfo_t *info, void *ctx_void)
{
	ucontext_t *ctx = (ucontext_t *)ctx_void;

	segv_err =  ctx->uc_mcontext.gregs[REG_ERR];
	unsigned long ip = ctx->uc_mcontext.gregs[REG_RIP];
	unsigned long bp = ctx->uc_mcontext.gregs[REG_RBP];

	printf("Received sig:%d,si_code:%d,ip:0x%lx,bp:0x%lx\n",
		sig, info->si_code, ip, bp);
	siglongjmp(jmpbuf, 1);
}

static int test_process_vm_readv(void)
{
#ifdef __x86_64__
	unsigned char buf[4096] = "";
	struct iovec local, remote;
	int ret;

	printf("[RUN]\tprocess_vm_readv() from vsyscall page\n");

	local.iov_base = buf;
	local.iov_len = 4096;
	remote.iov_base = (void *)0xffffffffff600000;
	remote.iov_len = 4096;

	ret = process_vm_readv(getpid(), &local, 1, &remote, 1, 0);
	printf("After process_vm_readv copy to buf\n");
	dump_buffer(buf, 4096);

	if (ret != 4096) {
		printf("[OK]\tprocess_vm_readv failed (ret=%d, errno=%d)\n",
			ret, errno);
		pass_case("Could not process_vm_readv as expected");
		return 0;
	}

	if (vsyscall_map_r) {
		if (memcmp(buf, (const void *)0xffffffffff600000, 4096))
			printf("[WARN]\t Read incorrect data\n");
		fail_case("read vsyscall data is not acceptable in lass");
		return 1;
	}
#endif

	return 0;
}

static int test_call_gtod(void)
{
	int ret_sys = -1;
	struct timeval tv;

	ret_sys = gettimeofday(&tv, NULL);
	if (ret_sys)
		fail_case("gettimeofday func failed, check lass");
	else
		pass_case("gettimeofday func pass");

	return ret_sys;
}

static int test_vsys_x(void)
{
	bool can_exec;
	struct timeval tv;
	struct timezone tz;

#ifdef __x86_64__
	if (vsyscall_map_x)
		printf("It's executable as expected for lass\n");
	else
		printf("[WARN]\tIt's not executable vsyscall\n");

	printf("[RUN]\tMake sure that vsyscalls is executable\n");
	if (sigsetjmp(jmpbuf, 1) == 0) {
		vgtod(&tv, &tz);
		can_exec = true;
	} else {
		can_exec = false;
	}
	printf("can_exec:%d\n", can_exec);
	if (can_exec) {
		pass_case("Execute the vsyscall without page fault");
		return 1;
	} else if (segv_err & (1 << 4)) { /* INSTR */
		printf("Right page fault(& 0x%x): #PF(0x%lx)\n",
		    (1 << 4), segv_err);
		fail_case("Should no page fault with correct err");
	} else {
		printf("Page fault with wrong error: #PF(0x%lx)\n",
		    segv_err);
		fail_case("Trigger wrong page fault");
		return 1;
	}
#endif

	return 0;
}

static unsigned long get_eflags(void)
{
	unsigned long eflags;

	asm volatile ("pushfq\n\tpopq %0" : "=rm" (eflags));
	return eflags;
}

static void set_eflags(unsigned long eflags)
{
	asm volatile ("pushq %0\n\tpopfq" : : "rm" (eflags) : "flags");
}

static void sigtrap(int sig, siginfo_t *info, void *ctx_void)
{
	ucontext_t *ctx = (ucontext_t *)ctx_void;
	unsigned long ip = ctx->uc_mcontext.gregs[REG_RIP];
	unsigned long bp = ctx->uc_mcontext.gregs[REG_RBP];
	/* Check sig number and rip rbp progress, block test
	 *printf("sig_num:%d, ip:%lx, rbp:%lx\n",
	 *	sig_num, ip, bp);
	 */
	sig_num++;
	if (((ip ^ 0xffffffffff600000UL) & ~0xfffUL) == 0) {
		printf("Got sig:%d,si_code:%d,ip:%lx,rbp:%lx,sig_num:%d\n",
		sig, info->si_code, ip, bp, sig_num);
		num_vsyscall_traps++;
	}
}

static int test_emulation(void)
{
	time_t tmp = 0;
	bool is_native;

	num_vsyscall_traps = 0;
	if (!vsyscall_map_x) {
		printf("Could not execute vsyscall\n");
		fail_case("Should execute vsyscall as expected\n");
		return 1;
	}

	printf("[RUN]\tchecking vsyscall is emulated\n");
	sethandler(SIGTRAP, sigtrap, 0);
	set_eflags(get_eflags() | X86_EFLAGS_TF);
	printf("&tmp:%p, tmp:%lx\n", &tmp, tmp);
	vtime(&tmp);
	printf("&tmp:%p, tmp:%lx,sig_num:%d\n",
		&tmp, tmp, sig_num);
	set_eflags(get_eflags() & ~X86_EFLAGS_TF);

	/*
	 * If vsyscalls are emulated, we expect a single trap in the
	 * vsyscall page -- the call instruction will trap with RIP
	 * pointing to the entry point before emulation takes over.
	 * In native mode, we expect two traps, since whatever code
	 * the vsyscall page contains will be more than just a ret
	 * instruction.
	 */
	is_native = (num_vsyscall_traps > 1);
	printf("is_native:%d, num_vsyscall_traps:%d\n",
		is_native, num_vsyscall_traps);
	printf("[%s]\tvsyscalls are %s (%d instructions in vsyscall page)\n",
		   (is_native ? "FAIL" : "OK"),
		   (is_native ? "native" : "emulated"),
		   (int)num_vsyscall_traps);
	if (is_native)
		fail_case("It's native mode, traps num more than 1");
	else
		pass_case("Not native mode, traps num is 1");

	return is_native;
}

int test_gtod(void)
{
	long ret_vsys = -1;
	struct timeval tv_vsys, tv_sys;
	struct timezone tz_vsys, tz_sys;

	printf("[RUN]\ttest gettimeofday\n");

	ret_vsys = sys_gtod(&tv_sys, &tz_sys);
	if (ret_vsys)
		fail_case("test sys gettimeofday failed");

	if (vsyscall_map_x) {
		printf("execute vgtod\n");
		ret_vsys = vgtod(&tv_vsys, &tz_vsys);
		if (ret_vsys)
			fail_case("vgtod failed");
	}
	printf("tv_sys.sec:%ld usec:%ld ret:%ld, &tv:%p, &tz:%p\n",
		tv_sys.tv_sec, tv_sys.tv_usec, ret_vsys, &tv_sys, &tz_sys);

	if (!ret_vsys)
		pass_case("test gettimeofday pass");
	return ret_vsys;
}

int main(int argc, char *argv[])
{
	char parm;

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

	sethandler(SIGSEGV, sigsegv, 0);
	init_vsys();

	switch (parm) {
	case 'g':
		test_gtod();
		break;
	case 'r':
		test_vsys_r();
		break;
	case 'v':
		test_process_vm_readv();
		break;
	case 'd':
		test_vsys_x();
		break;
	case 't':
		test_call_gtod();
		break;
	case 'e':
		test_emulation();
		break;
	case 'a':
		test_gtod();
		test_vsys_r();
		test_process_vm_readv();
		test_vsys_x();
		test_call_gtod();
		test_emulation();
		break;
	default:
		usage();
	}
	printf("[Results] pass_num:%d, fail_num:%d\n",
		pass_num, fail_num);
	return !(!fail_num);
}
