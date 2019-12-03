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
#include "arch_info.h"

/*
 * Create extra cpusubtypes not in mach/machine.h, which we want to leave
 * untouched from Apple.
 */

static const cpu_subtype_t CPU_SUBTYPE_X86_64_ALL_LIB64 =
    (CPU_SUBTYPE_X86_64_ALL | CPU_SUBTYPE_LIB64);

static const struct arch_info arch_info_list[] = {
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

const struct arch_info *__notnull arch_info_get_list(void) {
    return arch_info_list;
}

uint64_t arch_info_list_get_size(void) {
    return sizeof(arch_info_list) / sizeof(struct arch_info);
}

const struct arch_info *
arch_info_for_cputype(const cpu_type_t cputype, const cpu_subtype_t cpusubtype)
{
    switch (cputype) {
        case CPU_TYPE_ANY:
            switch (cpusubtype) {
                case CPU_SUBTYPE_MULTIPLE:
                    return (arch_info_list + 0);

                case CPU_SUBTYPE_LITTLE_ENDIAN:
                    return (arch_info_list + 1);

                case CPU_SUBTYPE_BIG_ENDIAN:
                    return (arch_info_list + 2);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_MC680x0:
            switch (cpusubtype) {
                case CPU_SUBTYPE_MC680x0_ALL:
                    return (arch_info_list + 3);

                case CPU_SUBTYPE_MC68040:
                    return (arch_info_list + 4);

                case CPU_SUBTYPE_MC68030_ONLY:
                    return (arch_info_list + 5);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_X86:
            switch (cpusubtype) {
                case CPU_SUBTYPE_I386_ALL:
                    return (arch_info_list + 6);

                case CPU_SUBTYPE_486:
                    return (arch_info_list + 7);

                case CPU_SUBTYPE_486SX:
                    return (arch_info_list + 8);

                case CPU_SUBTYPE_PENT:
                    return (arch_info_list + 9);

                case CPU_SUBTYPE_PENTPRO:
                    return (arch_info_list + 10);

                case CPU_SUBTYPE_PENTII_M3:
                    return (arch_info_list + 11);

                case CPU_SUBTYPE_PENTII_M5:
                    return (arch_info_list + 12);

                case CPU_SUBTYPE_PENTIUM_4:
                    return (arch_info_list + 13);

                case CPU_SUBTYPE_X86_64_H:
                    return (arch_info_list + 14);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_HPPA:
            /*
             * CPU_SUBTYPE_HPPA_ALL and CPU_SUBTYPE_HPPA_7100 are the same
             * value.
             */

            if (cpusubtype == CPU_SUBTYPE_HPPA_ALL) {
                return (arch_info_list + 15);
            }

            return NULL;

        case CPU_TYPE_ARM:
            switch (cpusubtype) {
                /*
                 * Skip one index because of CPU_SUBTYPE_HPPA_7100 situation
                 * mentioned above.
                 */

                case CPU_SUBTYPE_ARM_ALL:
                    return (arch_info_list + 17);

                case CPU_SUBTYPE_ARM_V4T:
                    return (arch_info_list + 18);

                case CPU_SUBTYPE_ARM_V6:
                    return (arch_info_list + 19);

                case CPU_SUBTYPE_ARM_V5TEJ:
                    return (arch_info_list + 20);

                case CPU_SUBTYPE_ARM_XSCALE:
                    return (arch_info_list + 21);

                case CPU_SUBTYPE_ARM_V7:
                    return (arch_info_list + 22);

                case CPU_SUBTYPE_ARM_V7F:
                    return (arch_info_list + 23);

                case CPU_SUBTYPE_ARM_V7S:
                    return (arch_info_list + 24);

                case CPU_SUBTYPE_ARM_V7K:
                    return (arch_info_list + 25);

                case CPU_SUBTYPE_ARM_V6M:
                    return (arch_info_list + 26);

                case CPU_SUBTYPE_ARM_V7M:
                    return (arch_info_list + 27);

                case CPU_SUBTYPE_ARM_V7EM:
                    return (arch_info_list + 28);

                case CPU_SUBTYPE_ARM_V8:
                    return (arch_info_list + 29);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_MC88000:
            if (cpusubtype == CPU_SUBTYPE_MC88000_ALL) {
                return (arch_info_list + 30);
            }

            return NULL;

        case CPU_TYPE_SPARC:
            if (cpusubtype == CPU_SUBTYPE_SPARC_ALL) {
                return (arch_info_list + 31);
            }

            return NULL;

        case CPU_TYPE_I860:
            if (cpusubtype == CPU_SUBTYPE_I860_ALL) {
                return (arch_info_list + 32);
            }

            return NULL;

        case CPU_TYPE_POWERPC:
            switch (cpusubtype) {
                case CPU_SUBTYPE_POWERPC_ALL:
                    return (arch_info_list + 33);

                case CPU_SUBTYPE_POWERPC_601:
                    return (arch_info_list + 34);

                case CPU_SUBTYPE_POWERPC_602:
                    return (arch_info_list + 35);

                case CPU_SUBTYPE_POWERPC_603:
                    return (arch_info_list + 36);

                case CPU_SUBTYPE_POWERPC_603e:
                    return (arch_info_list + 37);

                case CPU_SUBTYPE_POWERPC_603ev:
                    return (arch_info_list + 38);

                case CPU_SUBTYPE_POWERPC_604:
                    return (arch_info_list + 39);

                case CPU_SUBTYPE_POWERPC_604e:
                    return (arch_info_list + 40);

                case CPU_SUBTYPE_POWERPC_750:
                    return (arch_info_list + 41);

                case CPU_SUBTYPE_POWERPC_7400:
                    return (arch_info_list + 42);

                case CPU_SUBTYPE_POWERPC_7450:
                    return (arch_info_list + 43);

                case CPU_SUBTYPE_POWERPC_970:
                    return (arch_info_list + 44);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_VEO:
            switch (cpusubtype) {
                /*
                 * CPU_SUBTYPE_VEO_ALL is defined as CPU_SUBTYPE_VEO_2
                 */

                case CPU_SUBTYPE_VEO_ALL:
                    return (arch_info_list + 45);

                case CPU_SUBTYPE_VEO_1:
                    return (arch_info_list + 46);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_X86_64:
            switch (cpusubtype) {
                /*
                 * Skip one index because of the CPU_SUBTYPE_VEO_2 situation
                 * described above.
                 */

                case CPU_SUBTYPE_X86_64_ALL_LIB64:
                    return (arch_info_list + 48);

                case CPU_SUBTYPE_X86_64_ALL:
                    return (arch_info_list + 49);

                case CPU_SUBTYPE_X86_64_H:
                    return (arch_info_list + 50);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_ARM64:
            switch (cpusubtype) {
                case CPU_SUBTYPE_ARM64_ALL:
                    return (arch_info_list + 51);

                case CPU_SUBTYPE_ARM64_V8:
                    return (arch_info_list + 52);

                case CPU_SUBTYPE_ARM64E:
                    return (arch_info_list + 53);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_POWERPC64:
            switch (cpusubtype) {
                case CPU_SUBTYPE_POWERPC_ALL:
                    return (arch_info_list + 54);

                case CPU_SUBTYPE_POWERPC_970:
                    return (arch_info_list + 55);

                default:
                    return NULL;
            }

            break;

        case CPU_TYPE_ARM64_32:
            if (cpusubtype == CPU_SUBTYPE_ARM64_ALL) {
                return (arch_info_list + 56);
            }

            return NULL;

        default:
            return NULL;
    }
}

const struct arch_info *arch_info_for_name(const char *__notnull const name) {
    const struct arch_info *arch = arch_info_list;
    const char *arch_name = arch->name;

    for (; arch_name != NULL; arch_name = (++arch)->name) {
        if (strcmp(arch_name, name) == 0) {
            return arch;
        }
    }

    return NULL;
}
