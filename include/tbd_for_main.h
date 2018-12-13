//
//  src/tbd_for_main.h
//  tbd
//
//  Created by inoahdev on 11/30/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef TBD_FOR_MAIN_H
#define TBD_FOR_MAIN_H

#include <stdint.h>
#include "tbd.h"

enum tbd_for_main_options {
    O_TBD_FOR_MAIN_RECURSE_DIRECTORIES    = 1 << 0,
    O_TBD_FOR_MAIN_RECURSE_SUBDIRECTORIES = 1 << 1,

    O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS    = 1 << 2,
    O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS    = 1 << 3,

    O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_HIERARCHY = 1 << 5,

    O_TBD_FOR_MAIN_NO_OVERWRITE           = 1 << 6,
    O_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION = 1 << 7,

    O_TBD_FOR_MAIN_IGNORE_WARNINGS = 1 << 9,
    O_TBD_FOR_MAIN_NO_REQUESTS     = 1 << 10,
};

struct tbd_for_main {
    struct tbd_create_info info;

    char *parse_path;
    char *write_path;

    /*
     * Store the paths' length here to avoid multiple strlen() calls on
     * the paths in main's dir_recurse callback.
     */

    uint64_t parse_path_length;
    uint64_t write_path_length;

    uint64_t parse_options;
    uint64_t write_options;

    uint64_t options;

    /*
     * Archs and flags to either replace/remove ones found in a mach-o file.
     * Default is to replace.
     */

    uint64_t archs_re;
    uint64_t flags_re;
};

bool
tbd_for_main_parse_option(struct tbd_for_main *tbd,
                          int argc,
                          const char *const *argv,
                          const char *option,
                          int *index);

void
tbd_for_main_apply_from(struct tbd_for_main *dst,
                        const struct tbd_for_main *src);

void tbd_for_main_destroy(struct tbd_for_main *tbd);

#endif /* TBD_FOR_MAIN_H */