//
//  misc/recurse.cc
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>
#include <dirent.h>

#include "../mach-o/file.h"
#include "recurse.h"

namespace recurse {
    macho::file::open_result _macho_open_file_as_filetype(macho::file &macho_file, const char *path, macho_file_type filetype) {
        auto open_result = macho::file::open_result::ok;
        switch (filetype) {
            case macho_file_type::none:
                open_result = macho_file.open(path);
                break;

            case macho_file_type::library:
                open_result = macho_file.open_from_library(path);
                break;

            case macho_file_type::dynamic_library:
                open_result = macho_file.open_from_dynamic_library(path);
                break;
        }

        return open_result;
    }

    bool _macho_check_file_as_filetype(const char *path, macho_file_type filetype, macho::file::check_error &error) {
        auto result = false;
        switch (filetype) {
            case macho_file_type::none:
                result = macho::file::is_valid_file(path, &error);
                break;

            case macho_file_type::library:
                result = macho::file::is_valid_library(path, &error);
                break;

            case macho_file_type::dynamic_library:
                result = macho::file::is_valid_dynamic_library(path, &error);
                break;
        }

        return result;
    }
}
