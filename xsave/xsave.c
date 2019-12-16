#include <stdio.h>
#include <string.h>
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


struct i387_fxsave_struct {
	uint16_t		      cwd; /* Control Word		  */
	uint16_t		      swd; /* Status Word		   */
	uint16_t		      twd; /* Tag Word		      */
	uint16_t		      fop; /* Last Instruction Opcode	*/
	union {
	       struct {
		      uint64_t	rip; /* Instruction Pointer	    */
		      uint64_t	rdp; /* Data Pointer		  */
	       };
	       struct {
		      uint32_t	fip; /* FPU IP Offset		 */
		      uint32_t	fcs; /* FPU IP Selector		*/
		      uint32_t	foo; /* FPU Operand Offset	     */
		      uint32_t	fos; /* FPU Operand Selector	   */
	       };
	};
	uint32_t		      mxcsr;	 /* MXCSR Register State */
	uint32_t		      mxcsr_mask;     /* MXCSR Mask	  */

	/* 8*16 bytes for each FP-reg = 128 bytes:		    */
	uint32_t		      st_space[32];

	/* 16*16 bytes for each XMM-reg = 256 bytes:		  */
	uint32_t		      xmm_space[64];

	uint32_t		      padding[12];

	union {
	       uint32_t	       padding1[12];
	       uint32_t	       sw_reserved[12];
	};

} __attribute__((aligned(16)));

struct ymmh_struct {
	/* 16 * 16 bytes for each YMMH-reg = 256 bytes */
	uint32_t ymmh_space[64];
};

/* We don't support LWP yet: */
struct lwp_struct {
	uint8_t reserved[128];
};

struct bndregs_struct {
	uint64_t bndregs[8];
} __attribute__((packed));

struct bndcsr_struct {
	uint64_t cfg_reg_u;
	uint64_t status_reg;
} __attribute__((packed));

struct xsave_hdr_struct {
	uint64_t xstate_bv;
	uint64_t xcomp_bv;
	uint64_t reserved[6];
} __attribute__((packed));


struct xsave_struct {
	struct i387_fxsave_struct i387;
	struct xsave_hdr_struct xsave_hdr;
	struct ymmh_struct ymmh;
	struct lwp_struct lwp;
	struct bndregs_struct bndregs;
	struct bndcsr_struct bndcsr;
	/* new processor state extensions will go here */
} __attribute__ ((packed, aligned (64)));




static inline void copy_fxregs_to_kernel(struct fxregs_state *fxsave)
{
	//printf("Before xsave, fxsave:%p, &fxsave:%p\n",
	//	fxsave, &fxsave);
	//x86_64 bit os and only for intel CPU xsave
	asm volatile( "fxsaveq %[fx]" : [fx] "=m" (*fxsave));
}

static inline void xgetbv(unsigned int *eax, unsigned int *ebx,
	unsigned int *ecx, unsigned int *edx)
{
	printf("xgetbv try\n");
	asm volatile( "xgetbv"
		: "=a"(*eax),
		"=b" (*ebx),
		"=c" (*ecx),
		"=d" (*edx)
		:"2" (*ecx)
		: "memory");
	printf("xgetbv done\n");
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
	struct xsave_struct xsv;
	unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;

	// init fxsave and xsv
	fxsave = (struct fxregs_state*)malloc(sizeof(struct fxregs_state));
	memset(&xsv, 0, sizeof(struct xsave_struct));

	printf("before fxsave, fxsave:%p, &fxsave:%p, size fxregs_state:%ld, size *fxsave:%ld\n",
		fxsave, &fxsave, sizeof(struct fxregs_state), sizeof(*fxsave));
	printf("before copy_xregs_to_kernel fxsave->cwd:%d\n",fxsave->cwd);
	copy_fxregs_to_kernel(fxsave);
	printf("fxsave->cwd:%x\n",fxsave->cwd);
	print_mem(fxsave, sizeof(struct fxregs_state));
	printf("fxsave->rip:%lx\n",fxsave->rip);

	printf("Before xsave64, xsv:\n");
	print_mem(&xsv, sizeof(struct xsave_struct));
	asm volatile ("xsave64 %0": :"m" (xsv));
	printf("xsv size:%ld\n", sizeof(struct xsave_struct));
	printf("xsv.i387.cwd:%d, xstate_bv:%ld, xcomp_bv:%ld\n",
		 xsv.i387.cwd, xsv.xsave_hdr.xstate_bv, xsv.xsave_hdr.xcomp_bv);
	print_mem(&xsv, sizeof(struct xsave_struct));
	printf("i387 size:%ld\n", sizeof(struct i387_fxsave_struct));
	printf("xsave header size:%ld\n", sizeof(struct xsave_hdr_struct));
	print_mem(&(xsv.xsave_hdr), sizeof(struct xsave_hdr_struct));
	printf("ymmh_struct size:%ld\n", sizeof(struct ymmh_struct));
	printf("lwp_struct size:%ld\n", sizeof(struct lwp_struct));
	printf("bndregs_struct size:%ld\n", sizeof(struct bndregs_struct));
	printf("bndcsr_struct size:%ld\n", sizeof(struct bndcsr_struct));
	printf("Before xgetbv, eax:%x, ebx:%x, ecx:%x, edx:%x\n",
			eax, ebx, ecx, edx);
	xgetbv(&eax, &ebx, &ecx, &edx);
	printf("After xgetbv, eax:%x, ebx:%x, ecx:%x, edx:%x\n",
		eax, ebx, ecx, edx);
}
