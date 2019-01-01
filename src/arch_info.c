//
//  src/macho_file.c
//  tbd
//
//  Created by inoahdev on 11/19/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "mach/machine.h"
#include "arch_info.h"

const struct arch_info arch_info_list[] = {
    { CPU_TYPE_ANY, CPU_SUBTYPE_MULTIPLE,      "any"    },
    { CPU_TYPE_ANY, CPU_SUBTYPE_LITTLE_ENDIAN, "little" },
    { CPU_TYPE_ANY, CPU_SUBTYPE_BIG_ENDIAN,    "big"    },

    /*
     * Index starts at 3 and ends at 15.
     */

    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_ALL,    "arm"     },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V4T,    "armv4t"  },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6,     "armv6"   },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V5TEJ,  "armv5"   },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_XSCALE, "xscale"  },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7,     "armv7"   },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7F,    "armv7f"  },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S,    "armv7s"  },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7K,    "armv7k"  },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6M,    "armv6"   },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7M,    "armv7m"  },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7EM,   "armv7em" },
    { CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V8,     "armv8"   },

    /*
     * Index starts at 16 and ends at 17.
     */

    { CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64_ALL, "arm64" },
    { CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64_V8,  "arm64" },

    /*
     * Index starts at 18 and ends at 19.
     */

    { CPU_TYPE_HPPA, CPU_SUBTYPE_HPPA_ALL,  "hppa"       },
    { CPU_TYPE_HPPA, CPU_SUBTYPE_HPPA_7100, "hppa7100LC" },

    /*
     * Index starts from 20 and ends at 28.
     */

    { CPU_TYPE_X86, CPU_SUBTYPE_I386_ALL,  "i386"     },
    { CPU_TYPE_X86, CPU_SUBTYPE_486,       "i486"     },
    { CPU_TYPE_X86, CPU_SUBTYPE_486SX,     "i486SX"   },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENT,      "pentium"  },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENTPRO,   "pentpro"  },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENTII_M3, "pentIIm3" },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENTII_M5, "pentIIm5" },
    { CPU_TYPE_X86, CPU_SUBTYPE_PENTIUM_4, "pentium4" },
    { CPU_TYPE_X86, CPU_SUBTYPE_X86_64_H,  "x86_64h"  },

    /*
     * Following's index is 29.
     */

    { CPU_TYPE_I860, CPU_SUBTYPE_I860_ALL, "i860" },

    /*
     * Index starts from 30 and ends at 32.
     */

    { CPU_TYPE_MC680x0, CPU_SUBTYPE_MC680x0_ALL,  "m68k"   },
    { CPU_TYPE_MC680x0, CPU_SUBTYPE_MC68040,      "m68040" },
    { CPU_TYPE_MC680x0, CPU_SUBTYPE_MC68030_ONLY, "m68030" },

    /*
     * Following's index is 33.
     */

    { CPU_TYPE_MC88000, CPU_SUBTYPE_MC88000_ALL, "m88k" },

    /*
     * Index starts from 34 and ends at 45.
     */

    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_ALL,   "ppc"      },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_601,   "ppc601"   },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_602,   "ppc602"   },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603,   "ppc603"   },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603e,  "ppc603e"  },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_603ev, "ppc603ev" },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_604,   "ppc604"   },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_604e,  "ppc604e"  },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_750,   "ppc750"   },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_7400,  "ppc7400"  },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_7450,  "ppc7450"  },
    { CPU_TYPE_POWERPC, CPU_SUBTYPE_POWERPC_970,   "ppc970"   },

    /*
     * Index starts from 46 and ends at 47.
     */

    { CPU_TYPE_POWERPC64, CPU_SUBTYPE_POWERPC_ALL, "ppc64"     },
    { CPU_TYPE_POWERPC64, CPU_SUBTYPE_POWERPC_970, "ppc970-64" },

    /*
     * Following's index is 48
     */

    { CPU_TYPE_SPARC, CPU_SUBTYPE_SPARC_ALL, "sparc" },

    /*
     * Index starts from 49 and ends at 51.
     */

    { CPU_TYPE_VEO, CPU_SUBTYPE_VEO_ALL, "veo"  },
    { CPU_TYPE_VEO, CPU_SUBTYPE_VEO_1,   "veo1" },
    { CPU_TYPE_VEO, CPU_SUBTYPE_VEO_2,   "veo2" },

    /*
     * Index starts from 52 and ends at 53.
     */

    { CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_ALL, "x86_64"  },
    { CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_H,   "x86_64h" },

    /*
     * Following's index is 53
     */

    { 0, 0, NULL }
};

const struct arch_info *arch_info_get_list(void) {
    return arch_info_list;
}

const uint64_t arch_info_list_get_size(void) {
    return sizeof(arch_info_list) / sizeof(struct arch_info);
}

const struct arch_info *
arch_info_for_cputype(const cpu_type_t cputype, const cpu_subtype_t cpusubtype)
{
    const struct arch_info *arch = arch_info_list;
    for (; arch->name != NULL; arch++) {
        if (arch->cputype != cputype || arch->cpusubtype != cpusubtype) {
            continue;
        }

        return arch;
    }

    return NULL;
}

const struct arch_info *arch_info_for_name(const char *const name) {
    const struct arch_info *arch = arch_info_list;
    for (; arch->name != NULL; arch++) {
        if (strcmp(arch->name, name) != 0) {
            continue;
        }

        return arch;
    }

    return NULL;
}
