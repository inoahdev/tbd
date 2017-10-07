//
//  src/mach-o/headers/filetype.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    enum class filetype : uint32_t {
        object = 1,
        executable,
        fvmlib,
        core,
        preloaded_executable,
        dylib,
        dylinker,
        bundle,
        dylib_stub,
        dsym,
        kext
    };

    inline bool filetype_is_library(const filetype &filetype) noexcept { return filetype == filetype::dylib || filetype == filetype::dylib_stub || filetype == filetype::fvmlib; }
    inline bool filetype_is_dynamic_library(const filetype &filetype) noexcept { return filetype == filetype::dylib || filetype == filetype::dylib_stub; }
}
