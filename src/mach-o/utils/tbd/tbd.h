//
//  src/tbd/tbd.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include "../../architecture_info.h"
#include "../../file.h"

namespace macho::utils::tbd {
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

    enum class symbol_options : uint64_t {
        allow_all_private_symbols    = 1 << 0,
        allow_private_normal_symbols = 1 << 1,
        allow_private_weak_symbols   = 1 << 2,
        allow_private_objc_symbols   = 1 << 3,
        allow_private_objc_classes   = 1 << 4,
        allow_private_objc_ivars     = 1 << 5,
    };

    inline uint64_t operator|(const uint64_t &lhs, const symbol_options &rhs) noexcept { return lhs | (uint64_t)rhs; }
    inline void operator|=(uint64_t &lhs, const symbol_options &rhs) noexcept { lhs |= (uint64_t)rhs; }

    inline symbol_options operator|(const symbol_options &lhs, const uint64_t &rhs) noexcept { return (symbol_options)((uint64_t)lhs | rhs); }
    inline void operator|=(symbol_options &lhs, const uint64_t &rhs) noexcept { lhs = (symbol_options)((uint64_t)lhs | rhs); }

    inline symbol_options operator|(const symbol_options &lhs, const symbol_options &rhs) noexcept { return (symbol_options)((uint64_t)lhs | (uint64_t)rhs); }
    inline void operator|=(symbol_options &lhs, const symbol_options &rhs) noexcept { lhs = (symbol_options)((uint64_t)lhs | (uint64_t)rhs); }

    inline uint64_t operator&(const uint64_t &lhs, const symbol_options &rhs) noexcept { return lhs & (uint64_t)rhs; }
    inline void operator&=(uint64_t &lhs, const symbol_options &rhs) noexcept { lhs &= (uint64_t)rhs; }

    inline symbol_options operator&(const symbol_options &lhs, const uint64_t &rhs) noexcept { return (symbol_options)((uint64_t)lhs & rhs); }
    inline void operator&=(symbol_options &lhs, const uint64_t &rhs) noexcept { lhs = (symbol_options)((uint64_t)lhs & rhs); }

    inline symbol_options operator&(const symbol_options &lhs, const symbol_options &rhs) noexcept { return (symbol_options)((uint64_t)lhs & (uint64_t)rhs); }
    inline void operator&=(symbol_options &lhs, const symbol_options &rhs) noexcept { lhs = (symbol_options)((uint64_t)lhs & (uint64_t)rhs); }

    enum class creation_result {
        ok,
        invalid_subtype,
        invalid_cputype,
        invalid_load_command,
        invalid_segment,
        failed_to_iterate_load_commands,
        failed_to_iterate_symbols,
        contradictary_load_command_information,
        empty_installation_name,
        uuid_is_not_unique,
        platform_not_found,
        platform_not_supported,
        unrecognized_platform,
        multiple_platforms,
        not_a_library,
        has_no_uuid,
        contradictary_container_information,
        no_provided_architectures,
        failed_to_allocate_memory,
        no_symbols_or_reexports
    };

    __attribute__((unused)) creation_result create_from_macho_library(macho::file &library, FILE *output, uint64_t options, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides);
    __attribute__((unused)) creation_result create_from_macho_library(macho::container &container, FILE *output, uint64_t options, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides);
}
