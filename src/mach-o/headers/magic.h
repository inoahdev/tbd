//
//  src/mach-o/headers/magic.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    enum class magic : uint32_t {
        normal            = 0xfeedface,
        big_endian        = 0xcefaefde,
        bits64            = 0xfeedfacf,
        bits64_big_endian = 0xcffeedfe,
        fat               = 0xcafebabe,
        fat_big_endian    = 0xbebafeca,
        fat_64            = 0xcafebabf,
        fat_64_big_endian = 0xbfbafeca
    };
    
    inline bool magic_is_valid(magic magic) noexcept {
        return magic == magic::normal || magic == magic::big_endian || magic == magic::bits64 || magic == magic::bits64_big_endian || magic == magic::fat || magic == magic::fat_big_endian || magic == magic::fat_64 || magic == magic::fat_64_big_endian;
    }

    inline bool magic_is_fat(magic magic) noexcept {
        return magic == magic::fat || magic == magic::fat_big_endian || magic == magic::fat_64 || magic == magic::fat_64_big_endian;
    }

    inline bool magic_is_thin(magic magic) noexcept {
        return magic == magic::normal || magic == magic::big_endian || magic == magic::bits64 || magic == magic::bits64_big_endian;
    }

    inline bool magic_is_32_bit(magic magic) noexcept {
        return magic == magic::normal || magic == magic::big_endian;
    }

    inline bool magic_is_64_bit(magic magic) noexcept {
        return magic == magic::bits64 || magic == magic::bits64_big_endian;
    }

    inline bool magic_is_big_endian(magic magic) noexcept {
        return magic == magic::big_endian || magic == magic::bits64_big_endian || magic == magic::fat_big_endian || magic == magic::fat_64_big_endian;
    }

    inline bool magic_is_little_endian(magic magic) noexcept {
        return magic == magic::normal || magic == magic::bits64 || magic == magic::fat || magic == magic::fat_64;
    }

    inline bool magic_is_fat_64(magic magic) noexcept {
        return magic == magic::fat_64 || magic == magic::fat_64_big_endian;
    }

    inline bool magic_is_fat_32(magic magic) noexcept {
        return magic == magic::fat || magic == magic::fat_big_endian;
    }
}
