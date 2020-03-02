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

# define VSYS(x) (x)

typedef long (*gtod_t)(struct timeval *tv, struct timezone *tz);
const gtod_t vgtod = (gtod_t)VSYS(0xffffffffff600000);

int main(void)
{
	struct timeval tv;
	struct timezone tz;

	vgtod(&tv, &tz);
	printf("tv:%p, tv.sec:0x%lx usec:%lx\n", &tv, tv.tv_sec, tv.tv_usec);
	return 0;
}
