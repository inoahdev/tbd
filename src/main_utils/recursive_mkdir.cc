//
//  src/main_utils/recursive_mkdir.h
//  tbd
//
//  Created by inoahdev on 1/25/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "../recursive/mkdir.h"

namespace main_utils {
    void recursive_mkdir_last_as_file(char *path, bool must_exist, char **terminator, int *descriptor) {
        switch (recursive::mkdir::perform_with_last_as_file(path, must_exist, terminator, descriptor)) {
            case recursive::mkdir::result::ok:
            case recursive::mkdir::result::failed_to_create_last_as_directory:
                break;

            case recursive::mkdir::result::failed_to_create_last_as_file:
                if (!must_exist) {
                    fprintf(stderr,
                            "Failed to create file (at path %s), failing with error: %s\n",
                            path,
                            strerror(errno));
                }
                
                break;

            case recursive::mkdir::result::failed_to_create_intermediate_directories:
                fprintf(stderr,
                        "Failed to create intermediate directories for file (at path %s), failing with error: %s\n",
                        path,
                        strerror(errno));

                break;

            case recursive::mkdir::result::last_already_exists_not_as_file:
                fprintf(stderr,
                        "Object at path (%s) already exists, but is not a file\n",
                        path);
                
                break;

            default:
                break;
        }
    }
}
