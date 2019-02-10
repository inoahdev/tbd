//
//  include/dir_recurse.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef DIR_RECURSE_H
#define DIR_RECURSE_H

#include <dirent.h>
#include <stdbool.h>
#include <stdint.h>

enum dir_recurse_result {
    E_DIR_RECURSE_OK,
    E_DIR_RECURSE_FAILED_TO_OPEN
};

enum dir_recurse_fail_result {
    E_DIR_RECURSE_FAILED_TO_ALLOCATE_PATH,
    E_DIR_RECURSE_FAILED_TO_OPEN_SUBDIR,
    E_DIR_RECURSE_FAILED_TO_READ_ENTRY
};

typedef bool
(*dir_recurse_callback)(const char *path,
                        uint64_t length,
                        struct dirent *dirent,
                        void *info);

typedef bool
(*dir_recurse_fail_callback)(const char *path,
                             uint64_t length,
                             enum dir_recurse_fail_result result,
                             struct dirent *dirent,
                             void *info);

enum dir_recurse_result
dir_recurse(const char *path,
            uint64_t path_length,
            bool sub_dirs,
            void *callback_info,
            dir_recurse_callback callback,
            dir_recurse_fail_callback fail_callback);

#endif /* DIR_RECURSE_H */
