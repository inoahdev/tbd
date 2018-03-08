//
//  src/mach-o/utils/validation/as_library.cc
//  tbd
//
//  Created by inoahdev on 12/10/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "../swap.h"
#include "as_library.h"

namespace macho::utils::validation {
    bool as_library(const container &container) noexcept {
        auto filetype = container.header.filetype;
        if (container.is_big_endian()) {
            swap::filetype(filetype);
        }

        return filetype_is_library(filetype);
    }

    bool as_library(const file &file) noexcept {
        auto is_library = true;
        for (const auto &container : file.containers) {
            is_library = as_library(container);
            if (!is_library) {
                break;
            }
        }

        return is_library;
    }

    bool as_library(const char *path, file::open_result *error) noexcept {
        auto file = macho::file();
        auto open_result = file.open(path);

        if (error != nullptr) {
            *error = open_result;
        }

        if (open_result != file::open_result::ok) {
            return false;
        }

        return as_library(file);
    }
}
