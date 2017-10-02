//
//  src/mach-o/headers/segment.h
//  tbd
//
//  Created by inoahdev on 8/13/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    namespace segments {
        enum class flags : uint32_t {
            high_vm = 1,
            fixed_vm_library,
            no_relocations = 4,
            protected_version_1 = 8
        };

        inline uint32_t operator|(const uint32_t &lhs, const flags &rhs) noexcept { return lhs | static_cast<uint32_t>(rhs); }
        inline void operator|=(uint32_t &lhs, const flags &rhs) noexcept { lhs |= static_cast<uint32_t>(rhs); }

        inline flags operator|(const flags &lhs, const uint32_t &rhs) noexcept { return static_cast<flags>(static_cast<uint32_t>(lhs) | rhs); }
        inline void operator|=(flags &lhs, const uint32_t &rhs) noexcept { lhs = (flags)(static_cast<uint32_t>(lhs) | rhs); }

        inline flags operator|(const flags &lhs, const flags &rhs) noexcept { return static_cast<flags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); }
        inline void operator|=(flags &lhs, const flags &rhs) noexcept { lhs = (flags)(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); }

        inline uint32_t operator&(const uint32_t &lhs, const flags &rhs) noexcept { return lhs & static_cast<uint32_t>(rhs); }
        inline void operator&=(uint32_t &lhs, const flags &rhs) noexcept { lhs &= static_cast<uint32_t>(rhs); }

        inline flags operator&(const flags &lhs, const uint32_t &rhs) noexcept { return static_cast<flags>((static_cast<uint32_t>(lhs) & rhs)); }
        inline void operator&=(flags &lhs, const uint32_t &rhs) noexcept { lhs = static_cast<flags>(static_cast<uint32_t>(lhs) & rhs); }

        inline flags operator&(const flags &lhs, const flags &rhs) noexcept { return static_cast<flags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); }
        inline void operator&=(flags &lhs, const flags &rhs) noexcept { lhs = static_cast<flags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); }

        inline flags operator~(const flags &lhs) noexcept { return static_cast<flags>(~(uint32_t)lhs); }

        typedef struct section {
            char sectname[16];
            char segname[16];
            uint32_t addr;
            uint32_t size;
            uint32_t offset;
            uint32_t align;
            uint32_t reloff;
            uint32_t nreloc;
            uint32_t flags;
            uint32_t reserved1;
            uint32_t reserved2;
        } section;

        typedef struct section_64 {
            char sectname[16];
            char segname[16];
            uint64_t addr;
            uint64_t size;
            uint32_t offset;
            uint32_t align;
            uint32_t reloff;
            uint32_t nreloc;
            uint32_t flags;
            uint32_t reserved1;
            uint32_t reserved2;
            uint32_t reserved3;
        } section_64;
    }
}
