//
//  misc/recurse.h
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <dirent.h>

#include <functional>
#include <string>

#include "../mach-o/file.h"

namespace recurse {
    void macho_libraries(const char *directory_path, bool recurse_subdirectories, const std::function<void (std::string &, macho::file &)> &callback);
    void macho_library_paths(const char *directory_path, bool recurse_subdirectories, const std::function<void (std::string &)> &callback);
}
