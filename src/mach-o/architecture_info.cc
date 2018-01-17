//
//  src/mach-o/architecture_info.cc
//  tbd
//
//  Created by inoahdev on 7/18/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <string.h>
#include "architecture_info.h"

namespace macho {
    architecture_info architecture_info_table[] = {
        { cputype::any, subtype::any,    "any"    },
        { cputype::any, subtype::little, "little" },
        { cputype::any, subtype::big,    "big"    },

        { cputype::arm, subtype::arm,     "arm"    },
        { cputype::arm, subtype::armv4t,  "armv4t" },
        { cputype::arm, subtype::xscale,  "xscale" },
        { cputype::arm, subtype::armv6,   "armv6"  },
        { cputype::arm, subtype::armv7,   "armv7"  },
        { cputype::arm, subtype::armv7k,  "armv7k" },
        { cputype::arm, subtype::armv7f,  "armv7f" },
        { cputype::arm, subtype::armv7s,  "armv7s" },
        { cputype::arm, subtype::armv7m,  "arm7m"  },
        { cputype::arm, subtype::armv7em, "arm7em" },
        { cputype::arm, subtype::armv8,   "armv8"  },

        { cputype::arm64, subtype::arm64, "arm64" },

        { cputype::hppa, subtype::hppa,       "hppa"       },
        { cputype::hppa, subtype::hppa7100LC, "hppa7100LC" },

        { cputype::i386, subtype::i386,     "i386"     },
        { cputype::i386, subtype::i486,     "i486"     },
        { cputype::i386, subtype::i486SX,   "i486SX"   },
        { cputype::i386, subtype::pentium,  "pentium"  },
        { cputype::i386, subtype::pentpro,  "pentpro"  },
        { cputype::i386, subtype::pentium,  "i586"     },
        { cputype::i386, subtype::pentpro,  "i686"     },
        { cputype::i386, subtype::pentIIm3, "pentIIm3" },
        { cputype::i386, subtype::pentIIm5, "pentIIm5" },
        { cputype::i386, subtype::pentium4, "pentium4" },
        { cputype::i386, subtype::x86_64h,  "x86_64h"  },

        { cputype::i860, subtype::i860, "i860" },

        { cputype::m680x0, subtype::m68k,   "m68k"   },
        { cputype::m680x0, subtype::m68030, "m68030" },
        { cputype::m680x0, subtype::m68040, "m68040" },

        { cputype::m88000, subtype::m88k,   "m88k" },

        { cputype::powerpc, subtype::ppc,      "ppc"      },
        { cputype::powerpc, subtype::ppc601,   "ppc601"   },
        { cputype::powerpc, subtype::ppc603,   "ppc603"   },
        { cputype::powerpc, subtype::ppc603e,  "ppc603e"  },
        { cputype::powerpc, subtype::ppc603ev, "ppc603ev" },
        { cputype::powerpc, subtype::ppc604,   "ppc604"   },
        { cputype::powerpc, subtype::ppc604e,  "ppc604e"  },
        { cputype::powerpc, subtype::ppc750,   "ppc750"   },
        { cputype::powerpc, subtype::ppc7400,  "ppc7400"  },
        { cputype::powerpc, subtype::ppc7450,  "ppc7450"  },
        { cputype::powerpc, subtype::ppc970,   "ppc970"   },

        { cputype::powerpc64, subtype::ppc64,  "ppc64"     },
        { cputype::powerpc64, subtype::ppc970, "ppc970-64" },

        { cputype::sparc, subtype::sparc, "sparc" },

        { cputype::veo, subtype::veo1, "veo1" },
        { cputype::veo, subtype::veo2, "veo2" },

        { cputype::x86_64, subtype::x86_64,      "x86_64"  },
        { cputype::x86_64, subtype::x86_64h,     "x86_64h" },
        { cputype::x86_64, subtype::x86_64_all,  "x86_64"  },

        { cputype::none, subtype::none, nullptr }
    };

    const architecture_info *get_architecture_info_table() {
        return architecture_info_table;
    }

    const size_t get_architecture_info_table_size() {
        return sizeof(architecture_info_table) / sizeof(architecture_info);
    }

    const architecture_info *architecture_info_from_index(size_t index) {
        if (index >= get_architecture_info_table_size()) {
            return nullptr;
        }

        return &architecture_info_table[index];
    }

    const architecture_info *architecture_info_from_name(const char *name) {
        auto architecture_info = architecture_info_table;
        while (architecture_info->name != nullptr) {
            if (architecture_info->name == name || strcmp(architecture_info->name, name) == 0) {
                return architecture_info;
            }

            architecture_info++;
        }

        return nullptr;
    }

    const architecture_info *architecture_info_from_cputype(cputype cputype, subtype subtype) {
        auto architecture_info = architecture_info_table;
        while (architecture_info->cputype != cputype::none) {
            if (architecture_info->cputype == cputype && architecture_info->subtype == subtype) {
                return architecture_info;
            }

            architecture_info++;
        }

        return nullptr;
    }

    size_t architecture_info_index_from_name(const char *name) {
        const auto architecture_info = architecture_info_from_name(name);
        if (!architecture_info) {
            return -1;
        }

        return (reinterpret_cast<uint64_t>(architecture_info) - reinterpret_cast<uint64_t>(architecture_info_table)) / sizeof(macho::architecture_info);
    }

    size_t architecture_info_index_from_cputype(cputype cputype, subtype subtype) {
        const auto architecture_info = architecture_info_from_cputype(cputype, subtype);
        if (!architecture_info) {
            return -1;
        }

        return (reinterpret_cast<uint64_t>(architecture_info) - reinterpret_cast<uint64_t>(architecture_info_table)) / sizeof(macho::architecture_info);
    }
}
