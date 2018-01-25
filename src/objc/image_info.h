//
//  src/objc/image_info.h
//  tbd
//
//  Created by inoahdev on 8/15/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace objc {
    typedef struct image_info {
        uint32_t version;
        uint32_t flags;

        struct flags {
            explicit flags() = default;
            explicit flags(uint32_t value) : value(value) {}

            union {
                uint32_t value = 0;

                struct {
                    bool is_replacement       : 1;
                    bool supports_gc          : 1;
                    bool requires_gc          : 1;
                    bool optimized_by_dyld    : 1;
                    bool corrected_synthesize : 1;
                    bool is_simulated         : 1;
                    bool has_category_class_properties : 1;
                    bool padding : 1;

                    uint32_t swift_version : 8;
                } __attribute__((packed));
            };

            inline bool has_none() const noexcept { return this->value == 0; }
            inline void clear() noexcept { this->value = 0; }

            inline bool operator==(const flags &flags) const noexcept { return this->value == flags.value; }
            inline bool operator!=(const flags &flags) const noexcept { return this->value == flags.value; }
        };
    } image_info;
}
