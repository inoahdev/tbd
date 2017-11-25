//
//  src/utils/directory.cc
//  tbd
//
//  Created by inoahdev on 10/17/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "directory.h"

namespace utils {
    directory::open_result directory::open(const char *path) noexcept {
        this->dir = opendir(path);
        if (!dir) {
            return open_result::failed_to_open_directory;
        }

        this->path = path;
        return open_result::ok;
    }

    directory::open_result directory::open(const std::string &path) noexcept {
        this->dir = opendir(path.data());
        if (!dir) {
            return open_result::failed_to_open_directory;
        }

        this->path = path;
        return open_result::ok;
    }

    directory::open_result directory::open(std::string &&path) noexcept {
        this->dir = opendir(path.data());
        if (!dir) {
            return open_result::failed_to_open_directory;
        }

        this->path = path;
        return open_result::ok;
    }

    std::string directory::_construct_path(const char *path, size_t path_length, const char *name, bool is_directory) noexcept {
        const auto name_length = strlen(name);

        auto new_path_length = path_length + name_length;
        auto new_path = std::string();

        if (is_directory) {
            new_path_length++;
        }

        new_path.reserve(new_path_length);
        new_path.append(path);

        utils::path::add_component(new_path, name, &name[name_length]);

        if (is_directory) {
            new_path.append(1, '/');
        }

        return new_path;
    }

    void directory::close() noexcept {
        if (dir != nullptr) {
            closedir(dir);
            dir = nullptr;
        }
    }

    directory::~directory() noexcept {
        close();
    }
}
