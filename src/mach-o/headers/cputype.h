//
//  src/mach-o/headers/cputype.h
//  tbd
//
//  Created by inoahdev on 7/18/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    enum class cputype : int32_t {
        none,
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
}
