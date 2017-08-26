//
//  src/mach-o/headers/cputype.h
//  tbd
//
//  Created by inoahdev on 7/18/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    enum class cputype {
        none      = 0,
        any       = -1,
        arm       = 12,
        arm64     = 16777228,
        hppa      = 11,
        i386      = 7,
        i860      = 15,
        m680x0    = 6,
        m88000    = 13,
        powerpc   = 18,
        powerpc64 = 16777234,
        sparc     = 14,
        veo       = 255,
        x86_64    = 16777223,
    };

    enum class subtype {
        // cputype::none
        none = 0,

        // cputype::any
        any,
        little,
        big,

        // cputype::arm,
        arm,
        armv4t,
        armv5,
        xscale,
        armv6,
        armv6m,
        armv7,
        armv7f,
        armv7s,
        armv7k,
        armv7m,
        armv7em,
        armv8,

        // cputype::arm64
        arm64,

        // cputype::hppa
        hppa,
        hppa7100LC,

        // cputype::i386
        i386,
        i486,
        i486SX,
        pentium,
        pentpro,
        pentIIm3,
        pentIIm5,
        pentium4,
        x86_64h,

        // cputype::i860,
        i860,

        // cputype::m680x0
        m68k,
        m68030,
        m68040,

        // cputype::m88000
        m88k,

        // cputype::powerpc
        ppc,
        ppc601,
        ppc603,
        ppc603e,
        ppc603ev,
        ppc604,
        ppc604e,
        ppc750,
        ppc7400,
        ppc7450,
        ppc970,

        //cputype::powerpc64
        ppc64,

        // cputype::sparc
        sparc,

        // cputype::veo
        veo1,
        veo2,

        // cputype::x86_64
        x86_64,
        x86_64_all,
    };

    subtype subtype_from_cputype(cputype cputype, int32_t subtype);
    int32_t cpu_subtype_from_subtype(subtype subtype);
}
