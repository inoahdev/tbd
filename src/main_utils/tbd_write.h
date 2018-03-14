//
//  src/main_utils/tbd_write.h
//  tbd
//
//  Created by inoahdev on 3/12/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#pragma once
#include "tbd_with_options.h"

namespace main_utils {
    bool tbd_write(int descriptor, const tbd_with_options &tbd, const std::string &path, const std::string &write_path, bool should_print_paths);
}
