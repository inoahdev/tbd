//
//  src/dir_recurse.c
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <errno.h>
#include <dirent.h>

#include <stdlib.h>
#include <string.h>

#include "dir_recurse.h"
#include "path.h"

enum dir_recurse_result
dir_recurse(const char *const path,
            const bool sub_dirs,
            void *const callback_info,
            const dir_recurse_callback callback)
{
    DIR *const dir = opendir(path);
    if (dir == NULL) {
        return E_DIR_RECURSE_FAILED_TO_OPEN;
    }

    const uint64_t path_length = strlen(path);

    do {
        const int prev_errno = errno;
        struct dirent *entry = readdir(dir);

        if (entry == NULL) {
            closedir(dir);

            if (errno != prev_errno) {
                return E_DIR_RECURSE_FAILED_TO_READ_ENTRY;
            }

            return E_DIR_RECURSE_OK;
        }

        const char *const name = entry->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
            continue;
        }

        bool should_exit = false;
        switch (entry->d_type) {
            case DT_DIR: {
                if (!sub_dirs) {
                    continue;
                }

                char *const entry_path =
                    path_append_component_with_len(path,
                                                   path_length,
                                                   name,
                                                   strlen(name));

                if (entry_path == NULL) {
                    closedir(dir);
                    free(entry_path);

                    return E_DIR_RECURSE_ALLOC_FAIL;
                }

                const enum dir_recurse_result recurse_subdir_result =
                    dir_recurse(entry_path, true, callback_info, callback);

                free(entry_path);

                switch (recurse_subdir_result) {
                    case E_DIR_RECURSE_OK:
                        break;

                    case E_DIR_RECURSE_FAILED_TO_OPEN:
                        closedir(dir);
                        return E_DIR_RECURSE_FAILED_TO_OPEN_SUBDIR;

                    default:
                        closedir(dir);
                        return recurse_subdir_result;
                }

                break;
            }

            case DT_REG: {
                char *const entry_path =
                    path_append_component_with_len(path,
                                                   path_length,
                                                   name,
                                                   strlen(name));

                if (entry_path == NULL) {
                    closedir(dir);
                    return E_DIR_RECURSE_ALLOC_FAIL;
                }

                if (!callback(entry_path, entry, callback_info)) {
                    should_exit = true;
                }

                free(entry_path);
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