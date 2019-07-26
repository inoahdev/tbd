//
//  src/arch_info.c
//  tbd
//
//  Created by inoahdev on 11/19/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "mach/machine.h"

#include "array.h"
#include "arch_info.h"

/*
 * Create extra cpusubtypes not in mach/machine.h, which we want to leave
 * untouched from Apple.
 */

static const cpu_subtype_t CPU_SUBTYPE_X86_64_ALL_LIB64 =
    CPU_SUBTYPE_X86_64_ALL | CPU_SUBTYPE_LIB64;

/*
 * To support the use of fake-arrays, we don't const our arch-info table.
 */

static struct arch_info arch_info_list[] = {
    /*
     * Index starts at 0 and ends at 2.
     */

    { CPU_TYPE_ANY, CPU_SUBTYPE_MULTIPLE,      "any",    3 },
    { CPU_TYPE_ANY, CPU_SUBTYPE_LITTLE_ENDIAN, "little", 6 },
    { CPU_TYPE_ANY, CPU_SUBTYPE_BIG_ENDIAN,    "big",    3 },

    /*
     * Index starts at 3 and ends at 5.
     */

    { CPU_TYPE_MC680x0, CPU_SUBTYPE_MC680x0_ALL,  "m68k",   4 },
    { CPU_TYPE_MC680x0, CPU_SUBTYPE_MC68040,      "m68040", 6 },
    { CPU_TYPE_MC680x0, CPU_SUBTYPE_MC68030_ONLY, "m68030", 6 },

    /*
     * Index starts at 6 and ends at 14.
     */

    { CPU_TYPE_X86, CPU_SUBTYPE_I386_ALL,  "i386",     4 },
    { CPU_TYPE_X86, CPU_SUBTYPE_486,       "i486",     4 },
    { CPU_TYPE_X86, CPU_SUBTYPE_486SX,     "i486SX",   6 },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENT,      "pentium",  7 },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENTPRO,   "pentpro",  7 },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENTII_M3, "pentIIm3", 8 },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENTII_M5, "pentIIm5", 8 },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENTIUM_4, "pentium4", 8 },
    { CPU_TYPE_X86, CPU_SUBTYPE_X86_64_H,  "x86_64h",  7 },

    /*
     * Index starts at 15 and ends at 16.
     */

    { CPU_TYPE_HPPA, CPU_SUBTYPE_HPPA_ALL,  "hppa",       4  },
    { CPU_TYPE_HPPA, CPU_SUBTYPE_HPPA_7100, "hppa7100LC", 10 },

    /*
     * Index starts at 17 and ends at 29.
     */

    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_ALL,    "arm",     3 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V4T,    "armv4t",  6 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6,     "armv6",   5 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V5TEJ,  "armv5",   5 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_XSCALE, "xscale",  6 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7,     "armv7",   5 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7F,    "armv7f",  6 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S,    "armv7s",  6 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7K,    "armv7k",  6 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6M,    "armv6",   5 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7M,    "armv7m",  6 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7EM,   "armv7em", 7 },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V8,     "armv8",   5 },

    /*
     * Following's index is 30.
     */

    { CPU_TYPE_MC88000, CPU_SUBTYPE_MC88000_ALL, "m88k", 4 },

    /*
     * Following's index is 31.
     */

    { CPU_TYPE_SPARC, CPU_SUBTYPE_SPARC_ALL, "sparc", 5 },

    /*
     * Following's index is 32.
     */

    { CPU_TYPE_I860, CPU_SUBTYPE_I860_ALL, "i860", 4 },

    /*
     * Index starts at 33 and ends at 44.
     */

    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_ALL,   "ppc",      3 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_601,   "ppc601",   6 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_602,   "ppc602",   6 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603,   "ppc603",   6 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603e,  "ppc603e",  7 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603ev, "ppc603ev", 8 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_604,   "ppc604",   6 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_604e,  "ppc604e",  7 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_750,   "ppc750",   6 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_7400,  "ppc7400",  7 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_7450,  "ppc7450",  7 },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_970,   "ppc970",   6 },

    /*
     * Index starts at 45 and ends at 47.
     */

    { CPU_TYPE_VEO, CPU_SUBTYPE_VEO_ALL, "veo",  3 },
    { CPU_TYPE_VEO, CPU_SUBTYPE_VEO_1,   "veo1", 4 },
    { CPU_TYPE_VEO, CPU_SUBTYPE_VEO_2,   "veo2", 4 },

    /*
     * Index starts at 48 and ends at 50.
     */

    { CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_ALL_LIB64, "x86_64",  6 },
    { CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_ALL,       "x86_64",  6 },
    { CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_H,         "x86_64h", 7 },

    /*
     * Index starts from 51 and ends at 53.
     */

    { CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64_ALL, "arm64",  5 },
    { CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64_V8,  "arm64",  5 },
    { CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64E,    "arm64e", 6 },

    /*
     * Index starts at 54 and ends at 55.
     */

    { CPU_TYPE_POWERPC64, CPU_SUBTYPE_POWERPC_ALL, "ppc64",     5 },
    { CPU_TYPE_POWERPC64, CPU_SUBTYPE_POWERPC_970, "ppc970-64", 9 },

    /*
     * Following's index is 56.
     */

    { CPU_TYPE_ARM64_32, CPU_SUBTYPE_ARM64_ALL, "arm64_32", 8 },

    /*
     * Following's index is 57.
     */

    { 0, 0, NULL, 0 }
};

/*
 * Create a fake array to pass binary-search onto array's functions.
 */

static const struct array arch_info_array = {
    .data = (void *)arch_info_list,
    .data_end = (void *)(arch_info_list + 56),
    .alloc_end = (void *)(arch_info_list + 56)
};

struct arch_info_cputype_info {
    cpu_type_t cputype;

    uint64_t front;
    uint64_t back;
};

/*
 * To support the use of fake-arrays, we don't const our cputype-info table.
 */

static struct arch_info_cputype_info cputype_info_list[] = {
    { CPU_TYPE_ANY,        0, 2  },
    { CPU_TYPE_MC680x0,    3, 5  },
    { CPU_TYPE_X86,        6, 14 },
    { CPU_TYPE_HPPA,      15, 16 },
    { CPU_TYPE_ARM,       17, 29 },
    { CPU_TYPE_MC88000,   30, 30 },
    { CPU_TYPE_SPARC,     31, 31 },
    { CPU_TYPE_I860,      32, 32 },
    { CPU_TYPE_POWERPC,   33, 44 },
    { CPU_TYPE_VEO,       45, 47 },
    { CPU_TYPE_X86_64,    48, 50 },
    { CPU_TYPE_ARM64,     51, 53 },
    { CPU_TYPE_POWERPC64, 54, 55 },
    { CPU_TYPE_ARM64_32,  56, 56 }
};

/*
 * Create a fake array to pass binary-search onto array's functions.
 */

const struct array cputype_info_array = {
    .data = (void *)cputype_info_list,
    .data_end = (void *)(cputype_info_list + 14),
    .alloc_end = (void *)(cputype_info_list + 14)
};

const struct arch_info * __notnull arch_info_get_list(void) {
    return arch_info_list;
}

uint64_t arch_info_list_get_size(void) {
    return sizeof(arch_info_list) / sizeof(struct arch_info);
}

static uint64_t cputype_info_list_get_size(void) {
    return sizeof(cputype_info_list) / sizeof(struct arch_info_cputype_info);
}

static int
cputype_info_comparator(const void *__notnull const left,
                        const void *__notnull const right)
{
    const struct arch_info *const info = (const struct arch_info *)left;

    const cpu_type_t table_cputype = info->cputype;
    const cpu_type_t cputype = *(const cpu_type_t *)right;

    if (table_cputype > cputype) {
        return 1;
    } else if (table_cputype < cputype) {
        return -1;
    }

    return 0;
}

static int
arch_info_cpusubtype_comparator(const void *__notnull const left,
                                const void *__notnull const right)
{
    const struct arch_info *const info = (const struct arch_info *)left;

    const cpu_subtype_t table_cpusubtype = info->cpusubtype;
    const cpu_subtype_t cpusubtype = *(const cpu_subtype_t *)right;

    if (table_cpusubtype > cpusubtype) {
        return 1;
    } else if (table_cpusubtype < cpusubtype) {
        return -1;
    }

    return 0;
}

const struct arch_info *
arch_info_for_cputype(const cpu_type_t cputype, const cpu_subtype_t cpusubtype)
{
    /*
     * First find the cputype-info for the cputype provided to get the range
     * inside arch_info_list, where we will then search within for the right
     * arch.
     */

    const struct array_slice cputype_info_slice = {
        .front = 0,
        .back = cputype_info_list_get_size() - 1
    };

    const struct arch_info_cputype_info *const info =
        array_find_item_in_sorted_with_slice(
            &cputype_info_array,
            sizeof(struct arch_info_cputype_info),
            cputype_info_slice,
            &cputype,
            cputype_info_comparator,
            NULL);

    if (info == NULL) {
        return NULL;
    }

    const uint64_t info_front_index = info->front;
    if (info_front_index == info->back) {
        const struct arch_info *const arch = arch_info_list + info_front_index;
        if (arch->cpusubtype == cpusubtype) {
            return arch;
        }

        return NULL;
    }

    const struct array_slice slice = {
        .front = info->front,
        .back = info->back
    };

    const struct arch_info *const arch =
        array_find_item_in_sorted_with_slice(&arch_info_array,
                                             sizeof(struct arch_info),
                                             slice,
                                             &cpusubtype,
                                             arch_info_cpusubtype_comparator,
                                             NULL);

    if (arch == NULL) {
        return NULL;
    }

    return arch;
}

const struct arch_info *arch_info_for_name(const char *__notnull const name) {
    const struct arch_info *arch = arch_info_list;
    for (; arch->name != NULL; arch++) {
        if (strcmp(arch->name, name) != 0) {
            continue;
        }

        return arch;
    }

    return NULL;
}
