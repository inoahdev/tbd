//
//  src/mach-o/headers/build.h
//  tbd
//
//  Created by administrator on 8/3/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    namespace build_tool {
        enum class tool : uint32_t {
            clang = 1,
            swift,
            ld
        };

        struct build_tool_version {
            tool tool;
            uint32_t version;
        };
    }

    namespace build_version {
        enum class platform : uint32_t {
            macos = 1,
            ios,
            tvos,
            watchos,
            bridgeos
        };
    }
}
