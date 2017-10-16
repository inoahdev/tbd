//
//  src/recursive/mkdir.h
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

namespace recursive::mkdir {
    enum class result {
        ok,
        failed_to_create_intermediate_directories,

        failed_to_create_last_as_directory,
        failed_to_create_last_as_file
    };

    // Writing a null-terminator at iter provides
    // the path first created by mkdir

    result create(char *path, char **iter = nullptr);

    result create_ignorning_last(char *path, char **iter = nullptr);
    result create_with_last_as_file(char *path, char **iter = nullptr, int *last_descriptor = nullptr);
}
