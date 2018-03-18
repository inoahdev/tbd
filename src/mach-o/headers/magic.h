//
//  src/mach-o/headers/magic.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    struct magic {
        enum class values : uint32_t {
            normal            = 0xfeedface,
            big_endian        = 0xcefaefde,
            bits64            = 0xfeedfacf,
            bits64_big_endian = 0xcffeedfe,
            fat               = 0xcafebabe,
            fat_big_endian    = 0xbebafeca,
            fat_64            = 0xcafebabf,
            fat_64_big_endian = 0xbfbafeca
        };
        
        uint32_t value = 0;
        
        inline bool is_valid() const noexcept;
        
        inline bool is_thin() const noexcept;
        inline bool is_fat() const noexcept;
        
        inline bool is_32_bit() const noexcept;
        inline bool is_64_bit() const noexcept;
    
        inline bool is_little_endian() const noexcept;
        inline bool is_big_endian() const noexcept;

        inline bool is_fat_32() const noexcept;
        inline bool is_fat_64() const noexcept;
    };
    
    inline static bool operator==(uint32_t magic, magic::values value) noexcept {
        return magic == static_cast<uint32_t>(value);
    }
    
    inline static bool operator!=(uint32_t magic, magic::values value) noexcept {
        return magic == static_cast<uint32_t>(value);
    }
    
    inline static bool operator==(magic::values value, uint32_t magic) noexcept {
        return static_cast<uint32_t>(value) == magic;
    }
    
    inline static bool operator!=(magic::values value, uint32_t magic) noexcept {
        return static_cast<uint32_t>(value) == magic;
    }
    
    inline bool magic::is_valid() const noexcept {
        return this->value == values::normal || this->value == values::big_endian        ||
               this->value == values::bits64 || this->value == values::bits64_big_endian ||
               this->value == values::fat    || this->value == values::fat_big_endian    ||
               this->value == values::fat_64 || this->value == values::fat_64_big_endian;
    }
    
    inline bool magic::is_thin() const noexcept {
        return this->value == values::normal || this->value == values::big_endian ||
               this->value == values::bits64 || this->value == values::bits64_big_endian;
    }
    
    inline bool magic::is_fat() const noexcept {
        return this->value == values::fat || this->value == values::fat_big_endian ||
               this->value == values::fat_64 || this->value == values::fat_64_big_endian;
    }
    
    inline bool magic::is_32_bit() const noexcept {
        return this->value == values::normal || this->value == values::big_endian;
    }
    
    inline bool magic::is_64_bit() const noexcept {
        return this->value == values::bits64 || this->value == values::bits64_big_endian;
    }
    
    inline bool magic::is_little_endian() const noexcept {
        return this->value == values::normal || this->value == values::bits64 ||
               this->value == values::fat || this->value == values::fat_64;
    }
    
    inline bool magic::is_big_endian() const noexcept {
        return this->value == values::big_endian || this->value == values::bits64_big_endian ||
               this->value == values::fat_big_endian || this->value == values::fat_64_big_endian;
    }
    
    inline bool magic::is_fat_32() const noexcept {
        return value == values::fat || value == values::fat_big_endian;
    }
    
    inline bool magic::is_fat_64() const noexcept {
        return value == values::fat_64 || value == values::fat_64_big_endian;
    }
}
