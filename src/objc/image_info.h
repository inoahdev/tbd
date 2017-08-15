//
//  src/objc/image_info.h
//  tbd
//
//  Created by inoahdev on 8/15/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace objc {
    typedef struct image_info {
        uint32_t version;
        uint32_t flags;
        
        enum flags : uint32_t {
            is_replacement       = 1 << 0,
            supports_gc          = 1 << 1,
            requires_gc          = 1 << 2,
            optimized_by_dyld    = 1 << 3,
            corrected_synthesize = 1 << 4,
            is_simulated         = 1 << 5,
            has_category_class_properties  = 1 << 6,
            
            swift_version_shift = 8,
            swift_version_mask = 0xff
        };
    } image_info;
}
