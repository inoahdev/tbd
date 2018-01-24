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
    enum create_tbd_options : uint64_t {
        create_tbd_option_print_paths                                = 1 << 0,
        create_tbd_option_ignore_missing_dynamic_library_information = 1 << 1,
    };

    bool create_tbd(tbd_with_options &all, tbd_with_options &tbd, macho::file &file, uint64_t options, uint64_t *user_input_information, const char *path);
}
