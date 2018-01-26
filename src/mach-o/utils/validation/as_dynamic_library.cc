//
//  src/mach-o/utils/validation/as_dynamic_library.cc
//  tbd
//
//  Created by inoahdev on 12/10/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "../load_commands/retrieve.h"

#include "as_library.h"
#include "as_dynamic_library.h"

namespace macho::utils::validation {
    bool as_dynamic_library(const container &container) noexcept {
        const auto library_validation_result = as_library(container);
        if (!library_validation_result) {
            return false;
        }

        const auto identification_dylib = load_commands::retrieve_first_of_load_command_in_container(container, macho::load_commands::identification_dylib);
        if (!identification_dylib) {
            return false;
        }

        auto cmdsize = identification_dylib->cmdsize;
        if (container.is_big_endian()) {
            ::utils::swap::uint32(cmdsize);
        }

        if (cmdsize < sizeof(dylib_command)) {
            return false;
        }

        return true;
    }

    bool as_dynamic_library(const file &file) noexcept {
        auto result = true;
        for (const auto &container : file.containers) {
            result = as_dynamic_library(container);
            if (!result) {
                break;
            }
        }

        return result;
    }

    bool as_dynamic_library(const char *path, file::open_result *error) noexcept {
        auto file = macho::file();
        auto open_result = file.open(path);

        if (error != nullptr) {
            *error = open_result;
        }

        if (open_result != file::open_result::ok) {
            return false;
        }

        return as_dynamic_library(file);
    }
}
