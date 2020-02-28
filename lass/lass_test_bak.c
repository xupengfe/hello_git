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

#define MAPS_LINE_LEN 128
#define X86_EFLAGS_TF (1UL << 8)

#ifdef __x86_64__
# define VSYS(x) (x)
#else
# define VSYS(x) 0
#endif

static sig_atomic_t num_vsyscall_traps;
static int sig_num, result;
static jmp_buf jmpbuf;

/*
 * file /proc/self/maps contain 'r':vsyscall map:
 * 'r':vsyscall map: ffffffffff600000-ffffffffff601000 r-xp
 */
bool vsyscall_map_r = true, vsyscall_map_x = false;
static unsigned long segv_err;

typedef long (*gtod_t)(struct timeval *tv, struct timezone *tz);
const gtod_t vgtod = (gtod_t)VSYS(0xffffffffff600000);
gtod_t vdso_gtod;

typedef int (*vgettime_t)(clockid_t, struct timespec *);
vgettime_t vdso_gettime;

typedef long (*time_func_t)(time_t *t);
const time_func_t vtime = (time_func_t)VSYS(0xffffffffff600400);
time_func_t vdso_time;

typedef long (*getcpu_t)(unsigned *, unsigned *, void *);
const getcpu_t vgetcpu = (getcpu_t)VSYS(0xffffffffff600800);
getcpu_t vdso_getcpu;

/* syscalls */
static inline long sys_gtod(struct timeval *tv, struct timezone *tz)
{
	return syscall(SYS_gettimeofday, tv, tz);
}

static double tv_diff(const struct timeval *a, const struct timeval *b)
{
	return (double)(a->tv_sec - b->tv_sec) +
		(double)((int)a->tv_usec - (int)b->tv_usec) * 1e-6;
}

static int check_gtod(const struct timeval *tv_sys1,
		      const struct timeval *tv_sys2,
		      const struct timezone *tz_sys,
		      const char *which,
		      const struct timeval *tv_other,
		      const struct timezone *tz_other)
{
	int nerrs = 0;
	double d1, d2;

	if (tz_other && (tz_sys->tz_minuteswest != tz_other->tz_minuteswest || tz_sys->tz_dsttime != tz_other->tz_dsttime)) {
		printf("[FAIL] %s tz mismatch\n", which);
		nerrs++;
	}

	d1 = tv_diff(tv_other, tv_sys1);
	d2 = tv_diff(tv_sys2, tv_other); 
	printf("\t%s time offsets: %lf %lf\n", which, d1, d2);

	if (d1 < 0 || d2 < 0) {
		printf("[FAIL]\t%s time was inconsistent with the syscall\n", which);
		nerrs++;
	} else {
		printf("[OK]\t%s gettimeofday()'s timeval was okay\n", which);
	}

	return nerrs;
}

int usage(void)
{
	printf("Usage: [g|r|v|d|t|e]\n");
	printf("g  Test call 0xffffffffff600000\n");
	printf("r  Test read 0xffffffffff600000\n");
	printf("v  Test process_vm_readv read address\n");
	printf("d  Test dump 0xffffffffff600000-1000 key address\n");
	printf("t  Test syscall gettimeofday\n");
	printf("e  Test vsyscall emulation\n");
	printf("a  Test all\n");
	exit(2);
}

static void init_vdso(void)
{
	void *vdso = dlopen("linux-vdso.so.1", RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
	if (!vdso)
		vdso = dlopen("linux-gate.so.1", RTLD_LAZY | RTLD_LOCAL | RTLD_NOLOAD);
	if (!vdso) {
		printf("[WARN]\tfailed to find vDSO\n");
		return;
	}

	vdso_gtod = (gtod_t)dlsym(vdso, "__vdso_gettimeofday");
	if (!vdso_gtod)
		printf("[WARN]\tfailed to find gettimeofday in vDSO\n");

	vdso_gettime = (vgettime_t)dlsym(vdso, "__vdso_clock_gettime");
	if (!vdso_gettime)
		printf("[WARN]\tfailed to find clock_gettime in vDSO\n");

	vdso_time = (time_func_t)dlsym(vdso, "__vdso_time");
	if (!vdso_time)
		printf("[WARN]\tfailed to find time in vDSO\n");

	vdso_getcpu = (getcpu_t)dlsym(vdso, "__vdso_getcpu");
	if (!vdso_getcpu) {
		/* getcpu() was never wired up in the 32-bit vDSO. */
		printf("[%s]\tfailed to find getcpu in vDSO\n",
		       sizeof(long) == 8 ? "WARN" : "NOTE");
	}
}

int failed(const char *format)
{
	printf("[FAIL]\t %s\n", format);
	exit(1);
}

static int check_result(void)
{
	if (result >= 1) {
		printf("Check result pass:%d\n", result);
		return 0;
	} else {
		printf("Check result failed:%d\n", result);
		exit(1);
	}
}

static int init_vsys(void)
{
#ifdef __x86_64__
	result = 0;
	FILE *maps;
	char line[MAPS_LINE_LEN];
	bool found = false;

	maps = fopen("/proc/self/maps", "r");
	if (!maps) {
		printf("[WARN]\tCould not open /proc/self/maps -- assuming vsyscall is r-x\n");
		vsyscall_map_r = true;
		return 1;
	}

	while (fgets(line, MAPS_LINE_LEN, maps)) {
		char r, x;
		void *start, *end;
		char name[MAPS_LINE_LEN];

		/* sscanf() is safe here as strlen(name) >= strlen(line) */
		if (sscanf(line, "%p-%p %c-%cp %*x %*x:%*x %*u %s",
			   &start, &end, &r, &x, name) != 5)
			continue;

		if (strcmp(name, "[vsyscall]"))
			continue;

		printf("\tvsyscall map: %s", line);

		if (start != (void *)0xffffffffff600000 ||
		    end != (void *)0xffffffffff601000) {
			printf("[FAIL]\taddress range is nonsense\n");
			return 1;
		}

		printf("\tvsyscall permissions are %c-%c\n", r, x);
		vsyscall_map_r = (r == 'r');
		vsyscall_map_x = (x == 'x');
		printf("[WARN]\tr:%d, x:%d, should not readable in lass\n",
			vsyscall_map_r, vsyscall_map_x);

		found = true;
		break;
	}

	fclose(maps);

	if (!found) {
		printf("[PASS]\tno vsys map in /proc/self/maps in lass\n");
		vsyscall_map_r = false;
		vsyscall_map_x = false;
		result++;
	}
#endif
	//check_result();
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
		printf("sigsetjmp *(int *)0xffffffffff600000\n");
		a = *(int *)0xffffffffff600000;
		printf("0xffffffffff600000 content:%d\n", a);
		can_read = true;
	} else {
		can_read = false;
	}
	printf("can_read:%d, vsyscall_map_r:%d\n",
		can_read, vsyscall_map_r);
	if (can_read && !vsyscall_map_r) {
		printf("[FAIL]\tWe have read access, but we shouldn't\n");
		return 1;
	} else if (!can_read && vsyscall_map_r) {
		printf("[FAIL]\tWe don't have read access, but we should\n");
		return 1;
	} else if (can_read) {
		printf("[OK]\tWe have read access\n");
	} else {
		printf("[OK]\tWe do not have read access: #PF(0x%lx)\n",
			   segv_err);
	}
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
	printf("Received sig:%d, si_code:%d\n",
		sig, info->si_code);
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

	printf("-> buf before copy:\n");
	dump_buffer(buf, 4096);

	ret = process_vm_readv(getpid(), &local, 1, &remote, 1, 0);
	printf("-> After process_vm_readv copy to buf\n");
	dump_buffer(buf, 4096);

	if (ret != 4096) {
		printf("[OK]\tprocess_vm_readv failed (ret=%d, errno=%d)\n",
			ret, errno);
		return 0;
	}

	if (vsyscall_map_r) {
		if (!memcmp(buf, (const void *)0xffffffffff600000, 4096)) {
			printf("[OK]\tRead correct data\n");
		} else {
			printf("[FAIL]\tRead but incorrect data\n");
			return 1;
		}
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

	sig_num++;
	if (((ip ^ 0xffffffffff600000UL) & ~0xfffUL) == 0) {
		printf("Got sig:%d,si_code:%d,ip:%lx,REG_RIP:%d,sig_num:%d\n",
		sig, info->si_code, ip, REG_RIP, sig_num);
		num_vsyscall_traps++;
	}
}

static int test_emulation(void)
{
	time_t tmp = 0;
	bool is_native;

	printf("[RUN]\tchecking that vsyscalls are emulated\n");
	sethandler(SIGTRAP, sigtrap, 0);
	set_eflags(get_eflags() | X86_EFLAGS_TF);
	printf("&tmp:%p, tmp:%lx\n", &tmp, tmp);
	vtime(&tmp);
	printf("&tmp:%p, tmp:%lx\n", &tmp, tmp);
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

	return is_native;
}

int dump_vsyscall_key_address(void)
{
	int *a000, *a400, *a800;

	a000 = (int *)0xffffffffff600000;
	a400 = (int *)0xffffffffff600400;
	a800 = (int *)0xffffffffff600800;

	printf("a000:%x, 008:%x  012:%x\n",
		*a000, *(a000 + 1), *(a000 + 2));
	printf("a400:%x, 408:%x  412:%x\n",
		*a400, *(a400 + 1), *(a400 + 2));
	printf("a800:%x, 808:%x  812:%x\n",
		*a800, *(a800 + 1), *(a800 + 2));
	return 0;
}

int test_gtod(void)
{
	long ret_vsys = -1, ret_vdso = -1;
	struct timeval tv_sys1, tv_sys2, tv_vsys, tv_vdso;
	struct timezone tz_sys, tz_vsys, tz_vdso;

	printf("[RUN]\ttest gettimeofday()\n");
	if (sys_gtod(&tv_sys1, &tz_sys) != 0)
		failed("syscall gettimeofday");

	if (vsyscall_map_x) {
		printf("execute vgtod\n");
		ret_vsys = vgtod(&tv_vsys, &tz_vsys);
	}

	if (vdso_gtod)
		printf("access vdso_gtod!\n");
	ret_vdso = vdso_gtod(&tv_vdso, &tz_vdso);

	if (vdso_gtod) {
		if (ret_vdso == 0) {
			check_gtod(&tv_sys1, &tv_sys2, &tz_sys, "vDSO", &tv_vdso, &tz_vdso);
		} else {
			printf("[FAIL]\tvDSO gettimeofday() failed: %ld\n", ret_vdso);
			exit(1);
		}
	}

	return ret_vsys;
}

int main(int argc, char *argv[])
{
	char parm;
	struct timeval tv;

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
	init_vdso();
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
		dump_vsyscall_key_address();
		break;
	case 't':
		gettimeofday(&tv, NULL);
		break;
	case 'e':
		test_emulation();
		dump_vsyscall_key_address();
		break;
	case 'a':
		test_gtod();
		test_vsys_r();
		test_process_vm_readv();
		dump_vsyscall_key_address();
		gettimeofday(&tv, NULL);
		test_emulation();
		dump_vsyscall_key_address();
		break;
	default:
		usage();
	}
}
