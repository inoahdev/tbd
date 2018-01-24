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
    struct entry {
        union {
            uint32_t n_strx;
        } n_un;

        uint8_t n_type;
        uint8_t n_sect;
        int16_t n_desc;
        uint32_t n_value;
    };

    struct entry_64 {
        union {
            uint32_t n_strx;
        } n_un;

        uint8_t n_type;
        uint8_t n_sect;
        uint16_t n_desc;
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
        absolute           = 0x2,
        section            = 0xe,
        prebound_undefined = 0xc,
        indirect           = 0xa
    };
    
    inline bool operator==(const flags &flags, const uint8_t &number) noexcept { return static_cast<uint8_t>(flags) == number; }
    inline bool operator==(const uint8_t &number, const flags &flags) noexcept { return flags == static_cast<uint8_t>(number); }
    
    inline bool operator!=(const flags &flags, const uint8_t &number) noexcept { return static_cast<uint8_t>(flags) != number; }
    inline bool operator!=(const uint8_t &number, const flags &flags) noexcept { return number != static_cast<uint8_t>(flags); }
    
    inline bool operator==(const type &type, const uint8_t &number) noexcept { return static_cast<uint8_t>(type) == number; }
    inline bool operator==(const uint8_t &number, type type) noexcept { return type == static_cast<uint8_t>(number); }
    
    inline bool operator!=(const type &type, const uint8_t &number) noexcept { return static_cast<uint8_t>(type) != number; }
    inline bool operator!=(const uint8_t &number, type type) noexcept { return number != static_cast<uint8_t>(type); }
    
    inline int operator&(const uint8_t &number, const flags &flags) noexcept { return number & static_cast<uint8_t>(flags); }
    inline int operator&(const flags &flags, const uint8_t &number) noexcept { return static_cast<uint8_t>(flags) & number; }
    
    enum class description : int16_t {
        no_dead_strip        = 0x0020,
        weak_reference       = 0x0040,
        weak_definition      = 0x0080,
        arm_thumb_definition = 0x0008,
        symbol_resolver      = 0x0100,
        alternate_entry      = 0x0200
    };
    
    inline int16_t operator|(const int16_t &lhs, const description &rhs) noexcept { return lhs | static_cast<int16_t>(rhs); }
    inline void operator|=(int16_t &lhs, const description &rhs) noexcept { lhs |= static_cast<int16_t>(rhs); }
    
    inline description operator|(const description &lhs, const int16_t &rhs) noexcept { return static_cast<description>(static_cast<int16_t>(lhs) | rhs); }
    inline void operator|=(description &lhs, const int16_t &rhs) noexcept { lhs = static_cast<description>(static_cast<int16_t>(lhs) | rhs); }
    
    inline description operator|(const description &lhs, const description &rhs) noexcept { return static_cast<description>(static_cast<int16_t>(lhs) | static_cast<int16_t>(rhs)); }
    inline void operator|=(description &lhs, const description &rhs) noexcept { lhs = static_cast<description>(static_cast<int16_t>(lhs) | static_cast<int16_t>(rhs)); }
    
    inline int16_t operator&(const int16_t &lhs, const description &rhs) noexcept { return lhs & static_cast<int16_t>(rhs); }
    inline void operator&=(int16_t &lhs, const description &rhs) noexcept { lhs &= static_cast<int16_t>(rhs); }
    
    inline description operator&(const description &lhs, const int16_t &rhs) noexcept { return static_cast<description>(static_cast<int16_t>(lhs) & rhs); }
    inline void operator&=(description &lhs, const int16_t &rhs) noexcept { lhs = static_cast<description>(static_cast<int16_t>(lhs) & rhs); }
    
    inline description operator&(const description &lhs, const description &rhs) noexcept { return static_cast<description>(static_cast<int16_t>(lhs) & static_cast<int16_t>(rhs)); }
    inline void operator&=(description &lhs, const description &rhs) noexcept { lhs = static_cast<description>(static_cast<int16_t>(lhs) & static_cast<int16_t>(rhs)); }
    
    inline uint16_t operator|(const uint16_t &lhs, const description &rhs) noexcept { return lhs | static_cast<uint16_t>(rhs); }
    inline void operator|=(uint16_t &lhs, const description &rhs) noexcept { lhs |= static_cast<uint16_t>(rhs); }
    
    inline description operator|(const description &lhs, const uint16_t &rhs) noexcept { return static_cast<description>(static_cast<uint16_t>(lhs) | rhs); }
    inline void operator|=(description &lhs, const uint16_t &rhs) noexcept { lhs = static_cast<description>(static_cast<uint16_t>(lhs) | rhs); }
    
    inline int16_t operator&(const uint16_t &lhs, const description &rhs) noexcept { return lhs & static_cast<uint16_t>(rhs); }
    inline void operator&=(uint16_t &lhs, const description &rhs) noexcept { lhs &= static_cast<uint16_t>(rhs); }
    
    inline description operator&(const description &lhs, const uint16_t &rhs) noexcept { return static_cast<description>(static_cast<uint16_t>(lhs) & rhs); }
    inline void operator&=(description &lhs, const uint16_t &rhs) noexcept { lhs = static_cast<description>(static_cast<uint16_t>(lhs) & rhs); }
    
    inline description operator~(const description &lhs) noexcept { return static_cast<description>(~static_cast<uint16_t>(lhs)); }
}
