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
    enum class flags : uint64_t {
        none,
        flat_namespace         = 1 << 0,
        not_app_extension_safe = 1 << 1
    };

    inline uint64_t operator|(const uint64_t &lhs, const flags &rhs) noexcept { return lhs | static_cast<uint64_t>(rhs); }
    inline void operator|=(uint64_t &lhs, const flags &rhs) noexcept { lhs |= static_cast<uint64_t>(rhs); }

    inline flags operator|(const flags &lhs, const uint64_t &rhs) noexcept { return (flags)(static_cast<uint64_t>(lhs) | rhs); }
    inline void operator|=(flags &lhs, const uint64_t &rhs) noexcept { lhs = (flags)(static_cast<uint64_t>(lhs) | rhs); }

    inline flags operator|(const flags &lhs, const flags &rhs) noexcept { return (flags)(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }
    inline void operator|=(flags &lhs, const flags &rhs) noexcept { lhs = (flags)(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }

    inline uint64_t operator&(const uint64_t &lhs, const flags &rhs) noexcept { return lhs & static_cast<uint64_t>(rhs); }
    inline void operator&=(uint64_t &lhs, const flags &rhs) noexcept { lhs &= static_cast<uint64_t>(rhs); }

    inline flags operator&(const flags &lhs, const uint64_t &rhs) noexcept { return (flags)(static_cast<uint64_t>(lhs) & rhs); }
    inline void operator&=(flags &lhs, const uint64_t &rhs) noexcept { lhs = (flags)(static_cast<uint64_t>(lhs) & rhs); }

    inline flags operator&(const flags &lhs, const flags &rhs) noexcept { return (flags)(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }
    inline void operator&=(flags &lhs, const flags &rhs) noexcept { lhs = (flags)(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }

    inline flags operator~(const flags &lhs) noexcept { return (flags)~static_cast<uint16_t>(lhs); }

    enum class objc_constraint : uint32_t {
        no_value,
        none,
        retain_release,
        retain_release_for_simulator,
        retain_release_or_gc,
        gc
    };

    __attribute__((unused)) const char *objc_constraint_to_string(const objc_constraint &constraint) noexcept;
    __attribute__((unused)) objc_constraint objc_constraint_from_string(const char *string) noexcept;

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
    __attribute__((unused)) platform platform_from_string(const char *platform) noexcept;

    enum version {
        none,
        v1,
        v2
    };

    __attribute__((unused)) version version_from_string(const char *version) noexcept;

    enum class options : uint64_t {
        none,
        allow_all_private_symbols    = 1 << 0,
        allow_private_normal_symbols = 1 << 1,
        allow_private_weak_symbols   = 1 << 2,
        allow_private_objc_symbols   = 1 << 3,
        allow_private_objc_classes   = 1 << 4,
        allow_private_objc_ivars     = 1 << 5,
        remove_objc_constraint       = 1 << 6,
        remove_flags                 = 1 << 7,
    };

    inline uint64_t operator|(const uint64_t &lhs, const options &rhs) noexcept { return lhs | static_cast<uint64_t>(rhs); }
    inline void operator|=(uint64_t &lhs, const options &rhs) noexcept { lhs |= static_cast<uint64_t>(rhs); }

    inline options operator|(const options &lhs, const uint64_t &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) | rhs); }
    inline void operator|=(options &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) | rhs); }

    inline options operator|(const options &lhs, const options &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }
    inline void operator|=(options &lhs, const options &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }

    inline uint64_t operator&(const uint64_t &lhs, const options &rhs) noexcept { return lhs & static_cast<uint64_t>(rhs); }
    inline void operator&=(uint64_t &lhs, const options &rhs) noexcept { lhs &= static_cast<uint64_t>(rhs); }

    inline options operator&(const options &lhs, const uint64_t &rhs) noexcept { return static_cast<options>((static_cast<uint64_t>(lhs) & rhs)); }
    inline void operator&=(options &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) & rhs); }

    inline options operator&(const options &lhs, const options &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }
    inline void operator&=(options &lhs, const options &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }

    inline options operator~(const options &lhs) noexcept { return static_cast<options>(~static_cast<uint64_t>(lhs)); }

    enum class creation_result {
        ok,
        stream_seek_error,
        stream_read_error,
        unsupported_filetype,
        inconsistent_flags,
        invalid_subtype,
        invalid_cputype,
        invalid_load_command,
        invalid_segment,
        invalid_sub_client,
        invalid_sub_umbrella,
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

    __attribute__((unused)) creation_result create_from_macho_library(file &library, int output, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides);
    __attribute__((unused)) creation_result create_from_macho_library(container &container, int output, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides);

    __attribute__((unused)) creation_result create_from_macho_library(file &library, FILE *output, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides);
    __attribute__((unused)) creation_result create_from_macho_library(container &container, FILE *output, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides);
}
