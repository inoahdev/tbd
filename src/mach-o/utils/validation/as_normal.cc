//
//  src/mach-o/utils/validation/as_normal.cc
//  tbd
//
//  Created by inoahdev on 12/12/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "../../../stream/file/unique.h"
#include "../../headers/header.h"

#include "as_normal.h"

namespace macho::utils::validation {
    bool as_normal(const char *path, file::open_result *error) noexcept {
        auto file = macho::file();
        auto open_result = file.open(path);
        
        if (error != nullptr) {
            *error = open_result;
        }
        
        return open_result == file::open_result::ok;
    }
}
