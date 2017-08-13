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
    enum class platform {
        none,
        aix,
        amdhsa,
        ananas,
        cloudabi,
        cnk,
        contiki,
        cuda,
        darwin,
        dragonfly,
        elfiamcu,
        freebsd,
        fuchsia,
        haiku,
        ios,
        kfreebsd,
        linux,
        lv2,
        macosx,
        mesa3d,
        minix,
        nacl,
        netbsd,
        nvcl,
        openbsd,
        ps4,
        rtems,
        solaris,
        tvos,
        watchos,
        windows,
    };

    __attribute__((unused)) const char *platform_to_string(const platform &platform) noexcept;
    __attribute__((unused)) platform string_to_platform(const char *platform) noexcept;

    enum version {
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
        platform_not_found,
        platform_not_supported,
        multiple_platforms,
        not_a_library,
        has_no_uuid,
        contradictary_container_information,
    };

    __attribute__((unused)) creation_result create_from_macho_library(macho::file &library, FILE *output, unsigned int options, platform platform, version version, std::vector<const macho::architecture_info *> &architecture_overrides);
}
