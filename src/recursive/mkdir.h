//
//  src/recursive/mkdir.h
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

namespace recursive::mkdir {
    enum class result {
        ok,
        failed_to_create_intermediate_directories,

        failed_to_create_last_as_directory,
        failed_to_create_last_as_file,

        last_already_exists_not_as_file,
        last_already_exists_not_as_directory
    };

    // Writing a null-terminator at terminator provides
    // the path first created by mkdir

    result perform(char *path, char **terminator = nullptr) noexcept;

    result perform_ignorning_last(char *path, char **terminator = nullptr) noexcept;
    result perform_with_last_as_file(char *path, bool must_create = false, char **terminator = nullptr, int *last_descriptor = nullptr) noexcept;
}
