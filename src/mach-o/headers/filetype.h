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
}
