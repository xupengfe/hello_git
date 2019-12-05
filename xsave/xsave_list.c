#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <linux/types.h>
#include "xsave_list.h"

static int linux_xsave_size=0;

/*
 * We spell it out in here for xsave xfeature names
 */
static const char *xfeature_names[] =
{
	"x87 floating point registers"	,
	"SSE registers"			,
	"AVX registers"			,
	"MPX bounds registers"		,
	"MPX CSR"			,
	"AVX-512 opmask"		,
	"AVX-512 Hi256"			,
	"AVX-512 ZMM_Hi256"		,
	"Processor Trace (unused)"	,
	"Protection Keys User registers",
	"PASID state",
	"Control-flow User registers"	,
	"Control-flow Kernel registers"	,
	"Reserved Component (13)"	,
	"Reserved Component (14)"	,
	"Reserved Component (15)"	,
	"Reserved Component (16)"	,
	"AMX TILE config"		,
	"AMX TILE data"			,
	"unknown xstate feature"	,
};

static inline void cpuid_count(u32 id, u32 count,
		u32 *a, u32 *b, u32 *c, u32 *d)
{
	asm volatile(".ifnc %%ebx,%3 ; movl  %%ebx,%3 ; .endif	\n\t"
		     "cpuid					\n\t"
		     ".ifnc %%ebx,%3 ; xchgl %%ebx,%3 ; .endif	\n\t"
		    : "=a" (*a), "=c" (*c), "=d" (*d), "=b" (*b)
		    : "a" (id), "c" (count)
	);
}

static int xfeature_is_aligned(int xfeature_nr)
{
	u32 eax, ebx, ecx, edx;

	cpuid_count(XSTATE_CPUID, xfeature_nr, &eax, &ebx, &ecx, &edx);
	/*
	 * The value returned by ECX[1] indicates the alignment
	 * of state component 'i' when the compacted format
	 * of the extended region of an XSAVE area is used:
	 */
	return !!(ecx & 2);
}

static void xstate_dump_leaves(void)
{
	int i;
	u32 eax, ebx, ecx, edx;

	printf("Dump this CPU XSAVE related CPUID as below:\n");
	/*
	 * Dump out a few leaves past the ones that we support
	 * just in case there are some goodies up there
	 */
	for (i = 0; i < XFEATURE_MAX + 10; i++) {
		cpuid_count(XSTATE_CPUID, i, &eax, &ebx, &ecx, &edx);
		printf("CPUID[%02x, %02x]: eax=%08x ebx=%08x ecx=%08x edx=%08x\n",
			XSTATE_CPUID, i, eax, ebx, ecx, edx);
	}
}

u64 fpu_get_supported_xfeatures_mask(void)
{
	return SUPPORTED_XFEATURES_MASK_USER |
	       SUPPORTED_XFEATURES_MASK_SUPERVISOR;
}

void xfeature_name(u64 xfeature_idx, const char **feature_name)
{
	*feature_name = xfeature_names[xfeature_idx];
}

static unsigned int get_xfeature_size(int ecx)
{
	unsigned int eax, ebx, edx;
	cpuid_count(XSTATE_CPUID, ecx , &eax, &ebx, &ecx, &edx);
	return eax;
}

static void print_xstate_feature(u64 xstate_mask)
{
	const char *feature_name;
	int xfeature_size=0, is_aligned=0;


	xfeature_name(xstate_mask, &feature_name);
	is_aligned=xfeature_is_aligned(xstate_mask);
	if (xstate_mask == 0)
		xfeature_size=512;
	else if(xstate_mask == 1)
		xfeature_size=64;
	else
		xfeature_size=get_xfeature_size(xstate_mask);

	printf("CPU support XSAVE feature 0x%05lx| %04d |   %d   |'%s'\n",
		xstate_mask, xfeature_size, is_aligned, feature_name);
	linux_xsave_size=linux_xsave_size+xfeature_size;
}

int cpu_support_xstate_list(void)
{
	unsigned int eax, ebx, ecx, edx, bit_check=0x4000000;
	unsigned long i=0;
	u64 xfeatures_mask_all;

	// Verify this CPU could support XSAVE instructions
	cpuid_count(1, 0, &eax, &ebx, &ecx, &edx);
	if (!(ecx & bit_check)) {
		printf("This CPU could not support XSAVE instructions, exit.\n");
		return 1;
	}
	cpuid_count(XSTATE_CPUID, 0, &eax, &ebx, &ecx, &edx);
	xfeatures_mask_all = eax + ((u64)edx << 32);
	printf("cpu id 7 0 0 0, eax | edx, xfeatures_mask_all:%lx\n",
		xfeatures_mask_all);

	cpuid_count(XSTATE_CPUID, 1, &eax, &ebx, &ecx, &edx);
	xfeatures_mask_all |= ecx + ((u64)edx << 32);
	printf("With cpu id 7 0 1 0, ecx | edx, xfeatures_mask_all:%lx\n",
		xfeatures_mask_all);

	xfeatures_mask_all &= fpu_get_supported_xfeatures_mask();
	printf("After check FPU suppport, xfeatures_mask_all:%lx\n",
		xfeatures_mask_all);
	printf("Align should be CPUID.(EAX=0DH, ECX=xfeature_id):ECX[bit 2].\n");

	printf("CPU support XSAVE feature 0x000ID| Size | Align |'feature_name'\n");
	for(i=0; i<XFEATURE_MAX; i++) {
		if (!(xfeatures_mask_all & (1 << i)))
			continue;
		print_xstate_feature(i);
	}
}

void main(void)
{
	unsigned int eax, ebx, ecx, edx, cpu_xsave_size;

	cpuid_count(XSTATE_CPUID, 1, &eax, &ebx, &ecx, &edx);
	printf("This CPU could support XSAVE features total size:%d(0x%x)\n",
		ebx, ebx);
	cpu_xsave_size=ebx;

	cpu_support_xstate_list();
	if(cpu_xsave_size != linux_xsave_size)
		printf("WARN: cpu_xsave_size:%d is not equal to linux_xsave_size:%d\n",
			cpu_xsave_size, linux_xsave_size);
	xstate_dump_leaves();
}
