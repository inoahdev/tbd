//
//  include/mach/machine.h
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef MACH_MACHINE_H
#define MACH_MACHINE_H

#include <stdint.h>

typedef int32_t cpu_type_t;
typedef int32_t cpu_subtype_t;

/*
 * Capability bits used in the definition of cpu_type.
 */

#define CPU_ARCH_MASK       0xff000000      /* mask for architecture bits */
#define CPU_ARCH_ABI64      0x01000000      /* 64 bit ABI */
#define CPU_ARCH_ABI64_32   0x02000000      /* ABI for 64-bit hardware with 32-bit types; LP32 */

/*
 *	Machine types known by all.
 */

#define CPU_TYPE_ANY		((cpu_type_t) -1)
#define CPU_TYPE_VAX		((cpu_type_t) 1)

/* skip				((cpu_type_t) 2)	*/
/* skip				((cpu_type_t) 3)	*/
/* skip				((cpu_type_t) 4)	*/
/* skip				((cpu_type_t) 5)	*/

#define	CPU_TYPE_MC680x0	((cpu_type_t) 6)
#define CPU_TYPE_X86		((cpu_type_t) 7)
#define CPU_TYPE_I386		CPU_TYPE_X86		/* compatibility */
#define	CPU_TYPE_X86_64		(CPU_TYPE_X86 | CPU_ARCH_ABI64)

/* skip CPU_TYPE_MIPS		((cpu_type_t) 8)	*/
/* skip 			((cpu_type_t) 9)	*/

#define CPU_TYPE_MC98000	((cpu_type_t) 10)
#define CPU_TYPE_HPPA       ((cpu_type_t) 11)
#define CPU_TYPE_ARM		((cpu_type_t) 12)
#define CPU_TYPE_ARM64      (CPU_TYPE_ARM | CPU_ARCH_ABI64)
#define CPU_TYPE_ARM64_32   (CPU_TYPE_ARM | CPU_ARCH_ABI64_32)
#define CPU_TYPE_MC88000	((cpu_type_t) 13)
#define CPU_TYPE_SPARC		((cpu_type_t) 14)
#define CPU_TYPE_I860		((cpu_type_t) 15)

/* skip	CPU_TYPE_ALPHA		((cpu_type_t) 16)	*/
/* skip				((cpu_type_t) 17)	*/

#define CPU_TYPE_POWERPC		((cpu_type_t) 18)
#define CPU_TYPE_POWERPC64		(CPU_TYPE_POWERPC | CPU_ARCH_ABI64)
#define CPU_TYPE_VEO		    ((cpu_type_t) 255)

/*
 * Capability bits used in the definition of cpu_subtype.
 */

#define CPU_SUBTYPE_MASK	0xff000000	/* mask for feature flags */
#define CPU_SUBTYPE_LIB64	0x80000000	/* 64 bit libraries */


/*
 *	Object files that are hand-crafted to run on any
 *	implementation of an architecture are tagged with
 *	CPU_SUBTYPE_MULTIPLE.  This functions essentially the same as
 *	the "ALL" subtype of an architecture except that it allows us
 *	to easily find object files that may need to be modified
 *	whenever a new implementation of an architecture comes out.
 *
 *	It is the responsibility of the implementor to make sure the
 *	software handles unsupported implementations elegantly.
 */

#define	CPU_SUBTYPE_MULTIPLE		((cpu_subtype_t) -1)
#define CPU_SUBTYPE_LITTLE_ENDIAN	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_BIG_ENDIAN		((cpu_subtype_t) 1)

/*
 *     Machine threadtypes.
 *     This is none - not defined - for most machine types/subtypes.
 */

#define CPU_THREADTYPE_NONE		((cpu_threadtype_t) 0)

/*
 *	VAX subtypes (these do *not* necessary conform to the actual cpu
 *	ID assigned by DEC available via the SID register).
 */

#define	CPU_SUBTYPE_VAX_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_VAX780	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_VAX785	((cpu_subtype_t) 2)
#define CPU_SUBTYPE_VAX750	((cpu_subtype_t) 3)
#define CPU_SUBTYPE_VAX730	((cpu_subtype_t) 4)
#define CPU_SUBTYPE_UVAXI	((cpu_subtype_t) 5)
#define CPU_SUBTYPE_UVAXII	((cpu_subtype_t) 6)
#define CPU_SUBTYPE_VAX8200	((cpu_subtype_t) 7)
#define CPU_SUBTYPE_VAX8500	((cpu_subtype_t) 8)
#define CPU_SUBTYPE_VAX8600	((cpu_subtype_t) 9)
#define CPU_SUBTYPE_VAX8650	((cpu_subtype_t) 10)
#define CPU_SUBTYPE_VAX8800	((cpu_subtype_t) 11)
#define CPU_SUBTYPE_UVAXIII	((cpu_subtype_t) 12)

/*
 * 	680x0 subtypes
 *
 * The subtype definitions here are unusual for historical reasons.
 * NeXT used to consider 68030 code as generic 68000 code.  For
 * backwards compatability:
 *
 *	CPU_SUBTYPE_MC68030 symbol has been preserved for source code
 *	compatability.
 *
 *	CPU_SUBTYPE_MC680x0_ALL has been defined to be the same
 *	subtype as CPU_SUBTYPE_MC68030 for binary comatability.
 *
 *	CPU_SUBTYPE_MC68030_ONLY has been added to allow new object
 *	files to be tagged as containing 68030-specific instructions.
 */

#define	CPU_SUBTYPE_MC680x0_ALL		((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MC68030		((cpu_subtype_t) 1) /* compat */
#define CPU_SUBTYPE_MC68040		((cpu_subtype_t) 2)
#define	CPU_SUBTYPE_MC68030_ONLY	((cpu_subtype_t) 3)

/*
 *	I386 subtypes
 */

#define CPU_SUBTYPE_INTEL(f, m)	((cpu_subtype_t) (f) + ((m) << 4))

#define	CPU_SUBTYPE_I386_ALL			CPU_SUBTYPE_INTEL(3, 0)
#define CPU_SUBTYPE_386					CPU_SUBTYPE_INTEL(3, 0)
#define CPU_SUBTYPE_486					CPU_SUBTYPE_INTEL(4, 0)
#define CPU_SUBTYPE_486SX				CPU_SUBTYPE_INTEL(4, 8)
#define CPU_SUBTYPE_586					CPU_SUBTYPE_INTEL(5, 0)
#define CPU_SUBTYPE_PENT	            CPU_SUBTYPE_INTEL(5, 0)
#define CPU_SUBTYPE_PENTPRO	            CPU_SUBTYPE_INTEL(6, 1)
#define CPU_SUBTYPE_PENTII_M3	        CPU_SUBTYPE_INTEL(6, 3)
#define CPU_SUBTYPE_PENTII_M5           CPU_SUBTYPE_INTEL(6, 5)
#define CPU_SUBTYPE_CELERON				CPU_SUBTYPE_INTEL(7, 6)
#define CPU_SUBTYPE_CELERON_MOBILE		CPU_SUBTYPE_INTEL(7, 7)
#define CPU_SUBTYPE_PENTIUM_3			CPU_SUBTYPE_INTEL(8, 0)
#define CPU_SUBTYPE_PENTIUM_3_M			CPU_SUBTYPE_INTEL(8, 1)
#define CPU_SUBTYPE_PENTIUM_3_XEON		CPU_SUBTYPE_INTEL(8, 2)
#define CPU_SUBTYPE_PENTIUM_M			CPU_SUBTYPE_INTEL(9, 0)
#define CPU_SUBTYPE_PENTIUM_4			CPU_SUBTYPE_INTEL(10, 0)
#define CPU_SUBTYPE_PENTIUM_4_M			CPU_SUBTYPE_INTEL(10, 1)
#define CPU_SUBTYPE_ITANIUM				CPU_SUBTYPE_INTEL(11, 0)
#define CPU_SUBTYPE_ITANIUM_2			CPU_SUBTYPE_INTEL(11, 1)
#define CPU_SUBTYPE_XEON				CPU_SUBTYPE_INTEL(12, 0)
#define CPU_SUBTYPE_XEON_MP				CPU_SUBTYPE_INTEL(12, 1)

#define CPU_SUBTYPE_INTEL_FAMILY(x)	 ((x) & 15)
#define CPU_SUBTYPE_INTEL_FAMILY_MAX 15

#define CPU_SUBTYPE_INTEL_MODEL(x)	((x) >> 4)
#define CPU_SUBTYPE_INTEL_MODEL_ALL	0

/*
 *	X86 subtypes.
 */

#define CPU_SUBTYPE_X86_ALL		    ((cpu_subtype_t)3)
#define CPU_SUBTYPE_X86_64_ALL		((cpu_subtype_t)3)
#define CPU_SUBTYPE_X86_ARCH1		((cpu_subtype_t)4)
#define CPU_SUBTYPE_X86_64_H		((cpu_subtype_t)8)	/* Haswell subset */

#define CPU_THREADTYPE_INTEL_HTT	((cpu_threadtype_t) 1)

/*
 *	Mips subtypes.
 */

#define	CPU_SUBTYPE_MIPS_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MIPS_R2300	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MIPS_R2600	((cpu_subtype_t) 2)
#define CPU_SUBTYPE_MIPS_R2800	((cpu_subtype_t) 3)
#define CPU_SUBTYPE_MIPS_R2000a	((cpu_subtype_t) 4)	/* pmax */
#define CPU_SUBTYPE_MIPS_R2000	((cpu_subtype_t) 5)
#define CPU_SUBTYPE_MIPS_R3000a	((cpu_subtype_t) 6)	/* 3max */
#define CPU_SUBTYPE_MIPS_R3000	((cpu_subtype_t) 7)

/*
 *	MC98000 (PowerPC) subtypes
 */

#define	CPU_SUBTYPE_MC98000_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MC98601	    ((cpu_subtype_t) 1)

/*
 *	HPPA subtypes for Hewlett-Packard HP-PA family of
 *	risc processors. Port by NeXT to 700 series.
 */

#define	CPU_SUBTYPE_HPPA_ALL		((cpu_subtype_t) 0)
#define CPU_SUBTYPE_HPPA_7100		((cpu_subtype_t) 0) /* compat */
#define CPU_SUBTYPE_HPPA_7100LC		((cpu_subtype_t) 1)

/*
 *	MC88000 subtypes.
 */

#define	CPU_SUBTYPE_MC88000_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_MC88100	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_MC88110	((cpu_subtype_t) 2)

/*
 *	SPARC subtypes
 */

#define	CPU_SUBTYPE_SPARC_ALL		((cpu_subtype_t) 0)

/*
 *	I860 subtypes
 */

#define CPU_SUBTYPE_I860_ALL	((cpu_subtype_t) 0)
#define CPU_SUBTYPE_I860_860	((cpu_subtype_t) 1)

/*
 *	PowerPC subtypes
 */

#define CPU_SUBTYPE_POWERPC_ALL		((cpu_subtype_t) 0)
#define CPU_SUBTYPE_POWERPC_601		((cpu_subtype_t) 1)
#define CPU_SUBTYPE_POWERPC_602		((cpu_subtype_t) 2)
#define CPU_SUBTYPE_POWERPC_603		((cpu_subtype_t) 3)
#define CPU_SUBTYPE_POWERPC_603e	((cpu_subtype_t) 4)
#define CPU_SUBTYPE_POWERPC_603ev	((cpu_subtype_t) 5)
#define CPU_SUBTYPE_POWERPC_604		((cpu_subtype_t) 6)
#define CPU_SUBTYPE_POWERPC_604e	((cpu_subtype_t) 7)
#define CPU_SUBTYPE_POWERPC_620		((cpu_subtype_t) 8)
#define CPU_SUBTYPE_POWERPC_750		((cpu_subtype_t) 9)
#define CPU_SUBTYPE_POWERPC_7400	((cpu_subtype_t) 10)
#define CPU_SUBTYPE_POWERPC_7450	((cpu_subtype_t) 11)
#define CPU_SUBTYPE_POWERPC_970		((cpu_subtype_t) 100)

/*
 *	ARM subtypes
 */

#define CPU_SUBTYPE_ARM_ALL     ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_ARM_V4T     ((cpu_subtype_t) 5)
#define CPU_SUBTYPE_ARM_V6      ((cpu_subtype_t) 6)
#define CPU_SUBTYPE_ARM_V5TEJ   ((cpu_subtype_t) 7)
#define CPU_SUBTYPE_ARM_XSCALE	((cpu_subtype_t) 8)
#define CPU_SUBTYPE_ARM_V7		((cpu_subtype_t) 9)
#define CPU_SUBTYPE_ARM_V7F		((cpu_subtype_t) 10)
#define CPU_SUBTYPE_ARM_V7S		((cpu_subtype_t) 11)
#define CPU_SUBTYPE_ARM_V7K		((cpu_subtype_t) 12)
#define CPU_SUBTYPE_ARM_V6M		((cpu_subtype_t) 14)
#define CPU_SUBTYPE_ARM_V7M		((cpu_subtype_t) 15)
#define CPU_SUBTYPE_ARM_V7EM	((cpu_subtype_t) 16)
#define CPU_SUBTYPE_ARM_V8		((cpu_subtype_t) 13)

/*
 *  ARM64 subtypes
 */

#define CPU_SUBTYPE_ARM64_ALL           ((cpu_subtype_t) 0)
#define CPU_SUBTYPE_ARM64_V8            ((cpu_subtype_t) 1)
#define CPU_SUBTYPE_ARM64E              ((cpu_subtype_t) 2)

/*
 * VEO subtypes
 * Note: the CPU_SUBTYPE_VEO_ALL will likely change over time to be defined as
 * one of the specific subtypes.
 */

#define CPU_SUBTYPE_VEO_1	((cpu_subtype_t) 1)
#define CPU_SUBTYPE_VEO_2	((cpu_subtype_t) 2)
#define CPU_SUBTYPE_VEO_3	((cpu_subtype_t) 3)
#define CPU_SUBTYPE_VEO_4	((cpu_subtype_t) 4)
#define CPU_SUBTYPE_VEO_ALL	CPU_SUBTYPE_VEO_2

#endif /* MACH_MACHINE_H */
