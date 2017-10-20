//
//  src/utils/directory.cc
//  tbd
//
//  Created by inoahdev on 10/17/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "directory.h"

namespace utils {
    directory::open_result directory::open(const char *path) {
        this->dir = opendir(path);
        if (!dir) {
            return open_result::failed_to_open_directory;
        }

        this->path = path;
        return open_result::ok;
    }

    directory::open_result directory::open(const std::string &path) {
        this->dir = opendir(path.data());
        if (!dir) {
            return open_result::failed_to_open_directory;
        }

        this->path = path;
        return open_result::ok;
    }

    directory::~directory() noexcept {
        if (dir != nullptr) {
            closedir(dir);
            dir = nullptr;
        }
    }
}
