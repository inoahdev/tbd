//
//  include/dir_recurse.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef DIR_RECURSE_H
#define DIR_RECURSE_H

#include <dirent.h>
#include <stdbool.h>

enum dir_recurse_result {
    E_DIR_RECURSE_OK,
  
    E_DIR_RECURSE_FAILED_TO_OPEN,
    E_DIR_RECURSE_FAILED_TO_OPEN_SUBDIR,

    E_DIR_RECURSE_FAILED_TO_READ_ENTRY,
    E_DIR_RECURSE_FAILED_TO_OPEN_FILE,

    E_DIR_RECURSE_ALLOC_FAIL
};

typedef bool
(*dir_recurse_callback)(const char *path, struct dirent *dirent, void *info);

enum dir_recurse_result
dir_recurse(const char *path,
            bool sub_dirs,
            void *callback_info,
            dir_recurse_callback callback);

#endif /* DIR_RECURSE_H */
