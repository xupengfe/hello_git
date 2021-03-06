// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * xsave_list.h
 * Copyright (C) 2019 Intel Corporation
 * Author: Pengfei, Xu <pengfei.xu@intel.com>
 */

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/*
 * List of XSAVE features Linux knows about:
 */
enum xfeature {
	XFEATURE_FP,
	XFEATURE_SSE,
	/*
	 * Values above here are "legacy states".
	 * Those below are "extended states".
	 */
	XFEATURE_YMM,
	XFEATURE_BNDREGS,
	XFEATURE_BNDCSR,
	XFEATURE_OPMASK,
	XFEATURE_ZMM_Hi256,
	XFEATURE_Hi16_ZMM,
	XFEATURE_PT_UNIMPLEMENTED_SO_FAR,
	XFEATURE_PKRU,
	XFEATURE_PASID,
	XFEATURE_CET_USER,
	XFEATURE_CET_KERNEL,
	XFEATURE_RSRVD_COMP_13,
	XFEATURE_RSRVD_COMP_14,
	XFEATURE_RSRVD_COMP_15,
	XFEATURE_RSRVD_COMP_16,
	XFEATURE_XTILE_CFG,
	XFEATURE_XTILE_DATA,

	XFEATURE_MAX,
};

#define XSTATE_CPUID		0x0000000d

#define XFEATURE_MASK_FP		(1 << XFEATURE_FP)
#define XFEATURE_MASK_SSE		(1 << XFEATURE_SSE)
#define XFEATURE_MASK_YMM		(1 << XFEATURE_YMM)
#define XFEATURE_MASK_BNDREGS		(1 << XFEATURE_BNDREGS)
#define XFEATURE_MASK_BNDCSR		(1 << XFEATURE_BNDCSR)
#define XFEATURE_MASK_OPMASK		(1 << XFEATURE_OPMASK)
#define XFEATURE_MASK_ZMM_Hi256		(1 << XFEATURE_ZMM_Hi256)
#define XFEATURE_MASK_Hi16_ZMM		(1 << XFEATURE_Hi16_ZMM)
#define XFEATURE_MASK_PT		(1 << XFEATURE_PT_UNIMPLEMENTED_SO_FAR)
#define XFEATURE_MASK_PKRU		(1 << XFEATURE_PKRU)
#define XFEATURE_MASK_PASID		(1 << XFEATURE_PASID)
#define XFEATURE_MASK_CET_USER		(1 << XFEATURE_CET_USER)
#define XFEATURE_MASK_CET_KERNEL	(1 << XFEATURE_CET_KERNEL)
#define XFEATURE_MASK_XTILE_CFG		(1 << XFEATURE_XTILE_CFG)
#define XFEATURE_MASK_XTILE_DATA	(1 << XFEATURE_XTILE_DATA)

#define XFEATURE_MASK_XTILE		(XFEATURE_MASK_XTILE_DATA \
					 | XFEATURE_MASK_XTILE_CFG)

/* All currently supported user features */
#define SUPPORTED_XFEATURES_MASK_USER (XFEATURE_MASK_FP | \
				       XFEATURE_MASK_SSE | \
				       XFEATURE_MASK_YMM | \
				       XFEATURE_MASK_OPMASK | \
				       XFEATURE_MASK_ZMM_Hi256 | \
				       XFEATURE_MASK_Hi16_ZMM	 | \
				       XFEATURE_MASK_PKRU | \
				       XFEATURE_MASK_BNDREGS | \
				       XFEATURE_MASK_BNDCSR | \
				       XFEATURE_MASK_XTILE)

/* All currently supported supervisor features */
#define SUPPORTED_XFEATURES_MASK_SUPERVISOR (XFEATURE_MASK_PASID | \
					      XFEATURE_MASK_CET_USER)
