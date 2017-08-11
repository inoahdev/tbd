//
//  src/tbd/tbd.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include "../mach-o/architecture_info.h"
#include "../mach-o/file.h"

#include "symbol.h"

namespace tbd {
    enum platform {
        ios = 1,
        macosx,
        watchos,
        tvos
    };

    __attribute__((unused)) static const char *platform_to_string(const platform &platform) noexcept;
    __attribute__((unused)) static platform string_to_platform(const char *platform) noexcept;

    enum class version {
        v1 = 1,
        v2
    };

    __attribute__((unused)) static version string_to_version(const char *version) noexcept;
    __attribute__((unused)) void create_from_macho_library(macho::file &library, FILE *output, unsigned int options, std::vector<macho::architecture_info> &architecture_overrides);
}
