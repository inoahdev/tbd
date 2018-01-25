//
//  src/mach-o/headers/symbol_table.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho::symbol_table {
    struct n_type {
        union {
            uint8_t value = 0;

            struct {
                bool external : 1;
                uint8_t type  : 3;
            } __attribute__((packed));
        };

        inline bool has_none() noexcept { return this->value == 0; }
        inline void clear() noexcept { this->value = 0; }

        inline bool operator==(const n_type &type) const noexcept { return this->value == type.value; }
        inline bool operator!=(const n_type &type) const noexcept { return this->value != type.value; }
    };

    struct desc {
        union {
            uint16_t value = 0;

            struct {
                uint8_t type : 3;
                bool arm_thumb_definition : 1;

                uint8_t padding : 4;

                bool no_dead_strip   : 1;
                bool weak_reference  : 1;
                bool weak_definition : 1;
                bool alternate_entry : 1;
            } __attribute__((packed));
        };

        inline bool has_none() noexcept { return this->value == 0; }
        inline void clear() noexcept { this->value = 0; }

        inline bool operator==(const desc &desc) const noexcept { return this->value == desc.value; }
        inline bool operator!=(const desc &desc) const noexcept { return this->value != desc.value; }
    };

    struct entry {
        union {
            uint32_t n_strx;
        } n_un;

        n_type n_type;

        uint8_t n_sect;
        desc n_desc;
        uint32_t n_value;
    };

    struct entry_64 {
        union {
            uint32_t n_strx;
        } n_un;

        n_type n_type;

        uint8_t n_sect;
        desc n_desc;
        uint64_t n_value;
    };

    enum class flags : uint8_t {
        stab             = 0xe0,
        private_external = 0x10,
        type             = 0x0e,
        external         = 0x01
    };

    enum class type : uint8_t {
        undefined,
        absolute           = 0x1,
        section            = 0x7,
        prebound_undefined = 0x6,
        indirect           = 0x5
    };

    inline bool operator==(const flags &flags, const uint8_t &number) noexcept { return static_cast<uint8_t>(flags) == number; }
    inline bool operator==(const uint8_t &number, const flags &flags) noexcept { return flags == static_cast<uint8_t>(number); }

    inline bool operator!=(const flags &flags, const uint8_t &number) noexcept { return static_cast<uint8_t>(flags) != number; }
    inline bool operator!=(const uint8_t &number, const flags &flags) noexcept { return number != static_cast<uint8_t>(flags); }

    inline bool operator==(const type &type, const uint8_t &number) noexcept { return static_cast<uint8_t>(type) == number; }
    inline bool operator==(const uint8_t &number, type type) noexcept { return type == static_cast<uint8_t>(number); }

    inline bool operator!=(const type &type, const uint8_t &number) noexcept { return static_cast<uint8_t>(type) != number; }
    inline bool operator!=(const uint8_t &number, type type) noexcept { return number != static_cast<uint8_t>(type); }
}
