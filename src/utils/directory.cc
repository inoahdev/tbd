//
//  src/utils/directory.cc
//  tbd
//
//  Created by inoahdev on 10/17/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "directory.h"

namespace utils {
    directory::open_result directory::open(const char *path) noexcept {
        this->entries.begin.dir = opendir(path);
        if (!this->entries.begin.dir) {
            return open_result::failed_to_open_directory;
        }

        this->entries.begin = readdir(this->entries.begin.dir);
        return open_result::ok;
    }

    std::string directory::iterator::entry_path_from_directory_path(const char *root) const noexcept {
        const auto root_length = strlen(root);
        const auto name_length = strlen(ptr->d_name);

        auto path_length = root_length + name_length;
        auto path = std::string();

        if (path[root_length - 1] != '/') {
            path_length++;
        }

        if (ptr->d_type == DT_DIR) {
            path_length++;
        }

        path.reserve(path_length);
        path.append(root);

        utils::path::append_component(path, ptr->d_name, &ptr->d_name[name_length]);

        if (ptr->d_type == DT_DIR) {
            path.append(1, '/');
        }

        return path;
    }

    void directory::close() noexcept {
        if (this->entries.begin.dir != nullptr) {
            closedir(this->entries.begin.dir);
            this->entries.begin.dir = nullptr;
        }
    }
}
