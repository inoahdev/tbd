//
//  src/mach-o/utils/validation/as_normal.h
//  tbd
//
//  Created by inoahdev on 12/12/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include "../../file.h"

namespace macho::utils::validation {
    bool as_normal(const char *path, file::open_result *error = nullptr) noexcept;
}
