//
//  src/dir_recurse.c
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dir_recurse.h"
#include "our_io.h"
#include "path.h"

enum dir_recurse_result
dir_recurse(const char *__notnull const path,
            const uint64_t path_length,
            const int open_flags,
            void *const callback_info,
            __notnull const dir_recurse_callback callback,
            __notnull const dir_recurse_fail_callback fail_callback)
{
    const int dir_fd = our_open(path, O_RDONLY | O_DIRECTORY, 0);
    if (dir_fd < 0) {
        return E_DIR_RECURSE_FAILED_TO_OPEN;
    }

    DIR *const dir = our_fdopendir(dir_fd);
    if (dir == NULL) {
        close(dir_fd);
        return E_DIR_RECURSE_FAILED_TO_OPEN;
    }

    /*
     * Set errno to zero so we can distinguish later when readdir() has failed,
     * and when there are no more files and sub-directories left.
     */

    errno = 0;

    do {
        struct dirent *entry = our_readdir(dir);
        if (entry == NULL) {
            closedir(dir);

            if (errno != 0) {
                fail_callback(path,
                              path_length,
                              E_DIR_RECURSE_FAILED_TO_READ_ENTRY,
                              entry,
                              callback_info);
            }

            return E_DIR_RECURSE_OK;
        }

        const char *const name = entry->d_name;
        if (memcmp(name, ".", 2) == 0 || memcmp(name, "..", 3) == 0) {
            continue;
        }

        bool should_exit = false;
        switch (entry->d_type) {
            case DT_REG: {
                const int fd = our_openat(dir_fd, name, open_flags);
                if (fd < 0) {
                    const bool should_continue =
                        fail_callback(path,
                                      path_length,
                                      E_DIR_RECURSE_FAILED_TO_OPEN_FILE,
                                      entry,
                                      callback_info);

                    if (!should_continue) {
                        should_exit = true;
                    }

                    break;
                }

                if (!callback(path, path_length, fd, entry, callback_info)) {
                    should_exit = true;
                }

                /*
                 * We need to reset errno as we don't know what happened while
                 * in callback().
                 */

                errno = 0;
                break;
            }

            default:
                continue;
        }

        if (should_exit) {
            break;
        }
    } while (true);

    closedir(dir);
    return E_DIR_RECURSE_OK;
}

static enum dir_recurse_result
recurse_dir_fd(const int dir_fd,
               const char *__notnull const dir_path,
               const uint64_t dir_path_length,
               const int file_open_flags,
               void *const callback_info,
               __notnull const dir_recurse_callback callback,
               __notnull const dir_recurse_fail_callback fail_callback)
{
    DIR *const dir = our_fdopendir(dir_fd);
    if (dir == NULL) {
        close(dir_fd);
        return E_DIR_RECURSE_FAILED_TO_OPEN;
    }

    errno = 0;

    do {
        struct dirent *entry = our_readdir(dir);
        if (entry == NULL) {
            closedir(dir);

            if (errno != 0) {
                fail_callback(dir_path,
                              dir_path_length,
                              E_DIR_RECURSE_FAILED_TO_READ_ENTRY,
                              entry,
                              callback_info);
            }

            return E_DIR_RECURSE_OK;
        }

        const char *const name = entry->d_name;
        if (memcmp(name, ".", 2) == 0 || memcmp(name, "..", 3) == 0) {
            continue;
        }

        bool should_exit = false;
        switch (entry->d_type) {
            case DT_DIR: {
                const uint64_t name_length =
                    strnlen(name, sizeof(entry->d_name));

                uint64_t subdir_path_length = 0;
                char *const subdir_path =
                    path_append_component_with_len(dir_path,
                                                   dir_path_length,
                                                   name,
                                                   name_length,
                                                   &subdir_path_length);

                if (subdir_path == NULL) {
                    const bool should_continue =
                        fail_callback(dir_path,
                                      dir_path_length,
                                      E_DIR_RECURSE_FAILED_TO_ALLOC_PATH,
                                      entry,
                                      callback_info);

                    if (!should_continue) {
                        should_exit = true;
                    }

                    break;
                }

                const int subdir_fd =
                    our_openat(dir_fd, name, O_RDONLY | O_DIRECTORY);

                if (subdir_fd < 0) {
                    const bool should_continue =
                        fail_callback(subdir_path,
                                      subdir_path_length,
                                      E_DIR_RECURSE_FAILED_TO_OPEN_SUBDIR,
                                      entry,
                                      callback_info);

                    if (!should_continue) {
                        should_exit = true;
                    }

                    free(subdir_path);
                    break;
                }

                const enum dir_recurse_result recurse_subdir_result =
                    recurse_dir_fd(subdir_fd,
                                   subdir_path,
                                   subdir_path_length,
                                   file_open_flags,
                                   callback_info,
                                   callback,
                                   fail_callback);

                free(subdir_path);

                if (recurse_subdir_result != E_DIR_RECURSE_OK) {
                    return recurse_subdir_result;
                }

                break;
            }

            case DT_REG: {
                const int fd = our_openat(dir_fd, name, file_open_flags);
                if (fd < 0) {
                    const bool should_continue =
                        fail_callback(dir_path,
                                      dir_path_length,
                                      E_DIR_RECURSE_FAILED_TO_OPEN_FILE,
                                      entry,
                                      callback_info);

                    if (!should_continue) {
                        should_exit = true;
                    }

                    break;
                }

                const bool callback_result =
                    callback(dir_path,
                             dir_path_length,
                             fd,
                             entry,
                             callback_info);

                if (!callback_result) {
                    should_exit = true;
                }

                /*
                 * We need to reset errno as we don't know what happened while
                 * in callback().
                 */

                errno = 0;
                break;
            }
        }

        if (should_exit) {
            break;
        }
    } while (true);

    return E_DIR_RECURSE_OK;
}

enum dir_recurse_result
dir_recurse_with_subdirs(
    const char *__notnull const dir_path,
    const uint64_t dir_path_length,
    const int file_open_flags,
    void *const callback_info,
    __notnull const dir_recurse_callback callback,
    __notnull const dir_recurse_fail_callback fail_callback)
{
    const int dir_fd = our_open(dir_path, O_RDONLY | O_DIRECTORY, 0);
    if (dir_fd < 0) {
        return E_DIR_RECURSE_FAILED_TO_OPEN;
    }

    const enum dir_recurse_result recurse_dir_result =
        recurse_dir_fd(dir_fd,
                       dir_path,
                       dir_path_length,
                       file_open_flags,
                       callback_info,
                       callback,
                       fail_callback);

    if (recurse_dir_result != E_DIR_RECURSE_OK) {
        return recurse_dir_result;
    }

    return E_DIR_RECURSE_OK;
}
