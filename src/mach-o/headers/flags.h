//
//  src/mach-o/headers/flags.h
//  tbd
//
//  Created by inoahdev on 7/30/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    enum class flags : uint32_t {
        no_undefined_references         = 1 << 0,
        incremental_link                = 1 << 1,
        dyld_link                       = 1 << 2,
        bind_at_load                    = 1 << 4,
        prebound                        = 1 << 5,
        split_segments                  = 1 << 6,
        lazy_init                       = 1 << 7,
        two_level_namespaces            = 1 << 8,
        force_flat                      = 1 << 9,
        no_multiple_defines             = 1 << 10,
        no_fix_prebinding               = 1 << 11,
        all_mods_bound                  = 1 << 12,
        subsections_via_symbols         = 1 << 13,
        canonicalized                   = 1 << 14,
        binds_to_weak                   = 1 << 15,
        allow_stack_execution           = 1 << 17,
        root_safe                       = 1 << 18,
        setuid_safe                     = 1 << 19,
        no_reexported_dylibs            = 1 << 20,
        position_independent_executable = 1 << 21,
        dead_strippable_dylib           = 1 << 22,
        has_thread_local_variables      = 1 << 23,
        app_extension_safe              = 1 << 24
    };

    inline uint32_t operator|(const uint32_t &lhs, const flags &rhs) noexcept { return lhs | static_cast<uint32_t>(rhs); }
    inline void operator|=(uint32_t &lhs, const flags &rhs) noexcept { lhs |= static_cast<uint32_t>(rhs); }

    inline flags operator|(const flags &lhs, const uint32_t &rhs) noexcept { return static_cast<flags>(static_cast<uint32_t>(lhs) | rhs); }
    inline void operator|=(flags &lhs, const uint32_t &rhs) noexcept { lhs = static_cast<flags>(static_cast<uint32_t>(lhs) | rhs); }

    inline flags operator|(const flags &lhs, const flags &rhs) noexcept { return static_cast<flags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); }
    inline void operator|=(flags &lhs, const flags &rhs) noexcept { lhs = static_cast<flags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); }

    inline uint32_t operator&(const uint32_t &lhs, const flags &rhs) noexcept { return lhs & static_cast<uint32_t>(rhs); }
    inline void operator&=(uint32_t &lhs, const flags &rhs) noexcept { lhs &= static_cast<uint32_t>(rhs); }

    inline flags operator&(const flags &lhs, const uint32_t &rhs) noexcept { return static_cast<flags>(static_cast<uint32_t>(lhs) & rhs); }
    inline void operator&=(flags &lhs, const uint32_t &rhs) noexcept { lhs = static_cast<flags>(static_cast<uint32_t>(lhs) & rhs); }

    inline flags operator&(const flags &lhs, const flags &rhs) noexcept { return static_cast<flags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); }
    inline void operator&=(flags &lhs, const flags &rhs) noexcept { lhs = static_cast<flags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); }

    inline flags operator~(const flags &lhs) noexcept { return static_cast<flags>(~static_cast<uint32_t>(lhs)); }
}
