//
//  src/utils/swap.h
//  tbd
//
//  Created by inoahdev on 11/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace utils::swap {
    void uint16(uint16_t &uint16) noexcept;
    void uint32(uint32_t &uint32) noexcept;
    void uint64(uint64_t &uint64) noexcept;
    
    inline void int16(int16_t &int16) noexcept { return uint16(reinterpret_cast<uint16_t &>(int16)); }
    inline void int32(int32_t &int32) noexcept { return uint32(reinterpret_cast<uint32_t &>(int32)); }
    inline void int64(int64_t &int64) noexcept { return uint64(reinterpret_cast<uint64_t &>(int64)); }
}
