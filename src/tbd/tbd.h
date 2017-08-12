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

namespace tbd {
    enum platform {
        ios = 1,
        macosx,
        watchos,
        tvos
    };

    __attribute__((unused)) const char *platform_to_string(const platform &platform) noexcept;
    __attribute__((unused)) platform string_to_platform(const char *platform) noexcept;

    enum class version {
        v1 = 1,
        v2
    };

    __attribute__((unused)) version string_to_version(const char *version) noexcept;

    enum class creation_result {
        ok,
        invalid_subtype,
        invalid_cputype,
        invalid_load_command,
        contradictary_load_command_information,
        uuid_is_not_unique,
        not_a_library,
        has_no_uuid,
        contradictary_container_information,
    };

    __attribute__((unused)) creation_result create_from_macho_library(macho::file &library, FILE *output, unsigned int options, const platform &platform, const version &version, std::vector<const macho::architecture_info *> &architecture_overrides);
}
