//
//  src/misc/current_directory.cc
//  tbd
//
//  Created by inoahdev on 1/14/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <cerrno>
#include <unistd.h>

#include "../utils/path.h"
#include "current_directory.h"

namespace misc {
    const char *retrieve_current_directory() {
        // Store current-directory as a static variable to avoid
        // calling getcwd more times than necessary.

        static auto current_directory = static_cast<const char *>(nullptr);
        if (!current_directory) {
            // getcwd(nullptr, 0) will return a dynamically
            // allocated buffer just large enough to store
            // the current-directory.

            const auto current_directory_string = getcwd(nullptr, 0);
            if (!current_directory_string) {
                fprintf(stderr, "Failed to retrieve current-directory, failing with error: %s\n", strerror(errno));
                exit(1);
            }

            current_directory = current_directory_string;
        }

        return current_directory;
    }

    std::string path_with_current_directory(const char *root) {
        if (*root == '/' || *root == '\\' ) {
            return std::string(root);
        }

        auto result = std::string();

        const auto current_directory = retrieve_current_directory();
        const auto current_directory_length = strlen(current_directory);

        const auto root_length = strlen(root);
        const auto total_length = current_directory_length + root_length;

        result.reserve(total_length);
        result.append(current_directory);

        utils::path::append_component(result, root, &root[root_length]);
        return result;
    }

}
