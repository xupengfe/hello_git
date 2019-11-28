/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Find cpuid by cpu feature value and give result cpu could support
 * this feature or not
 * Author: Pengfei, Xu  <pengfei.xu@intel.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef unsigned int	u32;

void usage(void)
{
	printf("./cpu_feature feature_value\n");
	printf("  feature_value: check arch/x86/include/asm/cpufeatures.h\n");
}

static inline int boot_cpu_has(int flag)
{
	u32 eax, ebx, ecx, edx, eax_origin, result, ddt_result;
	char output;
	int bit_num;

	printf("flag:%d\n", flag);
	eax = (flag & 0x300) ? 7 :
		(flag & 0x20) ? 0x80000001 : 1;
	printf("eax:0x%02x\n", eax);
	eax_origin = eax;

	ecx = 0;

	asm volatile("cpuid"
		     : "+a" (eax), "=b" (ebx), "=d" (edx), "+c" (ecx));
	printf("eax:%x, ebx:%x, ecx:%x, edx:%x\n",
			eax, ebx, ecx, edx);
	output = (flag & 0x100 ? 'b' :
		(flag & 0x80) ? 'c' :
		(flag & 0x20) ? 'b' :
		(flag & 0x40) ? 'd' : 'c');
	bit_num = flag & 31;

	printf("CPUID.(EAX=0x%02x): output e%cx[bit %d]\n",
			eax_origin, output, bit_num);

	result = ((flag & 0x100 ? ebx :
		(flag & 0x80) ? ecx :
		(flag & 0x20) ? ebx :
		(flag & 0x40) ? edx : ecx) >> (flag & 31)) & 1;
	if (result == 0)
		printf("result:%d, check cpu_feature:%d failed!\n", result, flag);
	else
		printf("result:%d, check cpu_feature:%d pass!\n", result, flag);

	ddt_result = ~result & 1;
	printf("ddt_result:%d\n", ddt_result);
	exit (ddt_result);
}

int main(int argc, char *argv[])
{
	int cpu_feature;

	if (argc == 1) {
		usage();
		exit (2);
	} else {
		sscanf(argv[1], "%d", &cpu_feature);
		printf("1 used parameter: cpu_feature=%d\n", cpu_feature);
	}

	boot_cpu_has(cpu_feature);
}
