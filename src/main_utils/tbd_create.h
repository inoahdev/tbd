//
//  src/main_utils/tbd_create.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#pragma once

#include "tbd_with_options.h"
#include "tbd_user_input.h"

namespace main_utils {
    struct create_tbd_options {
        explicit inline create_tbd_options() = default;
        explicit inline create_tbd_options(uint64_t value) : value(value) {};

        union {
            uint64_t value = 0;

            struct {
                bool print_paths                                : 1;
                bool ignore_missing_dynamic_library_information : 1;
            } __attribute__((packed));
        };

        inline bool has_none() const noexcept { return this->value == 0; }
        inline void clear() noexcept { this->value = 0; }

        inline bool operator==(const create_tbd_options &options) const noexcept { return this->value == options.value; }
        inline bool operator!=(const create_tbd_options &options) const noexcept { return this->value != options.value; }
    };

    bool create_tbd(tbd_with_options &all, tbd_with_options &tbd, macho::file &file, const create_tbd_options &options, create_tbd_retained *user_input_information, const char *path);
}
