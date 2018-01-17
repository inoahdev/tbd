//
//  src/mach-o/utils/validation/as_dynamic_library.h
//  tbd
//
//  Created by inoahdev on 12/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include "../../file.h"

namespace macho::utils::validation {
    bool as_dynamic_library(const container &container) noexcept;
    bool as_dynamic_library(const file &file) noexcept;

    bool as_dynamic_library(const char *path, file::open_result *error = nullptr) noexcept;
}
