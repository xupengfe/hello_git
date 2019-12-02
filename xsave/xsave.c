#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <linux/types.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/*
 * The legacy fx SSE/MMX FPU state format, as saved by FXSAVE and
 * restored by the FXRSTOR instructions. It's similar to the FSAVE
 * format, but differs in some areas, plus has extensions at
 * the end for the XMM registers.
 */
struct fxregs_state {
	u16			cwd; /* Control Word			*/
	u16			swd; /* Status Word			*/
	u16			twd; /* Tag Word			*/
	u16			fop; /* Last Instruction Opcode		*/
	union {
		struct {
			u64	rip; /* Instruction Pointer		*/
			u64	rdp; /* Data Pointer			*/
		};
		struct {
			u32	fip; /* FPU IP Offset			*/
			u32	fcs; /* FPU IP Selector			*/
			u32	foo; /* FPU Operand Offset		*/
			u32	fos; /* FPU Operand Selector		*/
		};
	};
	u32			mxcsr;		/* MXCSR Register State */
	u32			mxcsr_mask;	/* MXCSR Mask		*/

	/* 8*16 bytes for each FP-reg = 128 bytes:			*/
	u32			st_space[32];

	/* 16*16 bytes for each XMM-reg = 256 bytes:			*/
	u32			xmm_space[64];

	u32			padding[12];

	union {
		u32		padding1[12];
		u32		sw_reserved[12];
	};

} __attribute__((aligned(16)));


static inline void copy_fxregs_to_kernel(struct fxregs_state *fxsave)
{
	//printf("Before xsave, fxsave:%p, &fxsave:%p\n",
	//	fxsave, &fxsave);
	//x86_64 bit os and only for intel CPU xsave
	asm volatile( "fxsaveq %[fx]" : [fx] "=m" (*fxsave));
}

void print_mem(void const *vp, size_t n)
{
	unsigned char const *p = vp;
	for (size_t i=0; i < n; i++) {
		printf("%02x ", p[i]);
		if(!( (i+1)%16 ))
			printf("\n");
	}
    putchar('\n');
}

void main(void)
{
	struct fxregs_state *fxsave;

	fxsave = (struct fxregs_state*)malloc(sizeof(struct fxregs_state));
	printf("before fxsave, fxsave:%p, &fxsave:%p, size fxregs_state:%ld, size *fxsave:%ld\n",
		fxsave, &fxsave, sizeof(struct fxregs_state), sizeof(*fxsave));
	printf("before copy_xregs_to_kernel fxsave->cwd:%d\n",fxsave->cwd);
	copy_fxregs_to_kernel(fxsave);
	printf("fxsave->cwd:%d\n",fxsave->cwd);
	print_mem(fxsave, sizeof(*fxsave));
}
