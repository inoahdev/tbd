//
//  src/mach-o/headers/architecture.h
//  tbd
//
//  Created by inoahdev on 7/18/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    typedef struct architecture {
        int32_t cputype;
        int32_t cpusubtype;

        uint32_t offset;
        uint32_t size;
        uint32_t align;
    } architecture;

    typedef struct architecture_64 {
        int32_t cputype;
        int32_t cpusubtype;

        uint64_t offset;
        uint64_t size;
        uint32_t align;
    } architecture_64;
}
