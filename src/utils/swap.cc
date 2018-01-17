//
//  src/utils/swap.cc
//  tbd
//
//  Created by inoahdev on 11/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "swap.h"

namespace utils::swap {
    void uint16(uint16_t &uint16) noexcept {
        uint16 = ((uint16 >> 8) & 0x00ff) | ((uint16 << 8) & 0xff00);
    }
    
    void uint32(uint32_t &uint32) noexcept {
        uint32 = ((uint32 >> 8) & 0x00ff00ff)  | ((uint32 << 8) & 0xff00ff00);
        uint32 = ((uint32 >> 16) & 0x0000ffff) | ((uint32 << 16) & 0xffff0000);
    }
    
    void uint64(uint64_t &uint64) noexcept {
        uint64 = (uint64 & 0x00000000ffffffff) << 32 | (uint64 & 0xffffffff00000000) >> 32;
        uint64 = (uint64 & 0x0000ffff0000ffff) << 16 | (uint64 & 0xffff0000ffff0000) >> 16;
        uint64 = (uint64 & 0x00ff00ff00ff00ff) << 8  | (uint64 & 0xff00ff00ff00ff00) >> 8;
    }

}
