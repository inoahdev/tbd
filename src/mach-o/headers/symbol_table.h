//
//  src/mach-o/headers/symbol_table.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    struct nlist {
        union {
#ifndef __LP64__
            char *n_name;
#endif
            uint32_t n_strx;
        } n_un;

        uint8_t n_type;
        uint8_t n_sect;
        int16_t n_desc;
        uint32_t n_value;
    };

    struct nlist_64 {
        union {
            uint32_t n_strx;
        } n_un;

        uint8_t n_type;
        uint8_t n_sect;
        uint16_t n_desc;
        uint64_t n_value;
    };

    namespace symbol_table {
        enum class flags : uint8_t {
            stab = 0xe0,
            private_external = 0x10,
            type = 0x0e,
            external = 0x01
        };

        enum class type : uint8_t {
            undefined,
            absolute = 0x2,
            section = 0xe,
            prebound_undefined = 0xc,
            indirect = 0xa
        };

        enum description : uint16_t {
            no_dead_strip = 0x0020,
            weak_reference = 0x0040,
            weak_definition = 0x0080,
            arm_thumb_definition = 0x0008,
            symbol_resolver = 0x0100,
            alternate_entry = 0x0200
        };

        inline bool operator==(flags flags, int number) noexcept { return (int)flags == number; }
        inline bool operator==(int number, flags flags) noexcept { return flags == (int)number; }

        inline bool operator!=(flags flags, int number) noexcept { return (int)flags != number; }
        inline bool operator!=(int number, flags flags) noexcept { return number != (int)flags; }

        inline bool operator==(type type, int number) noexcept { return (int)type == number; }
        inline bool operator==(int number, type type) noexcept { return type == (int)number; }

        inline bool operator!=(type type, int number) noexcept { return (int)type != number; }
        inline bool operator!=(int number, type type) noexcept { return number != (int)type; }

        inline int operator&(int number, flags flags) noexcept { return number & (int)flags; }
        inline int operator&(flags flags, int number) noexcept { return (int)flags & number; }
    }
}
