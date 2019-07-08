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
    E_DIR_RECURSE_FAILED_TO_ALLOC_PATH,
    E_DIR_RECURSE_FAILED_TO_OPEN_FILE,
    E_DIR_RECURSE_FAILED_TO_OPEN_SUBDIR,
    E_DIR_RECURSE_FAILED_TO_READ_ENTRY
};

/*
 * caller is expected to close the file-descriptor.
 */

typedef bool
(*dir_recurse_callback)(const char *dir_path,
                        uint64_t dir_path_length,
                        int fd,
                        struct dirent *dirent,
                        void *info);

/*
 * dir_path and dir_path_length refer to the sub-directory when result is
 * E_DIR_RECURSE_FAILED_TO_OPEN_SUBDIR.
 *
 * Note:
 *     When result is E_DIR_RECURSE_FAILED_TO_READ_ENTRY, dirent is NULL.
 */

typedef bool
(*dir_recurse_fail_callback)(const char *dir_path,
                             uint64_t dir_path_length,
                             enum dir_recurse_fail_result result,
                             struct dirent *dirent,
                             void *info);

enum dir_recurse_result
dir_recurse(const char *path,
            uint64_t path_length,
            int file_open_flags,
            void *callback_info,
            dir_recurse_callback callback,
            dir_recurse_fail_callback fail_callback);

enum dir_recurse_result
dir_recurse_with_subdirs(const char *const path,
                         const uint64_t path_length,
                         int file_open_flags,
                         void *const callback_info,
                         const dir_recurse_callback callback,
                         const dir_recurse_fail_callback fail_callback);

#endif /* DIR_RECURSE_H */
