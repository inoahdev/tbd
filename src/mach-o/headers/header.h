//
//  src/mach-o/headers/header.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include "filetype.h"
#include "magic.h"

namespace macho {
    typedef struct header {
        magic magic;
        int32_t cputype;
        int32_t cpusubtype;
        filetype filetype;
        uint32_t ncmds;
        uint32_t sizeofcmds;
        uint32_t flags;
    } header;

    typedef struct fat_header {
        magic magic;
        uint32_t nfat_arch;
    } fat;
}
