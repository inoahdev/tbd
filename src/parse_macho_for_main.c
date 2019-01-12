//
//  src/parse_macho_for_main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdlib.h>
#include <unistd.h>

#include "handle_macho_file_parse_result.h"

#include "macho_file.h"
#include "parse_macho_for_main.h"

static void
clear_create_info(struct tbd_create_info *const info_in,
                  const struct tbd_create_info *const orig)
{
    tbd_create_info_destroy(info_in);
    *info_in = *orig;
}

bool
parse_macho_file(struct tbd_for_main *const global,
                 struct tbd_for_main *const tbd,
                 const char *const path,
                 const uint64_t path_length,
                 const int fd,
                 const uint64_t size,
                 const bool print_paths,
                 uint64_t *const retained_info)
{
    const uint64_t parse_options = tbd->parse_options;
    const uint64_t macho_options =
        O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS | tbd->macho_options;

    struct tbd_create_info *const create_info = &tbd->info;
    struct tbd_create_info original_info = *create_info;

    const enum macho_file_parse_result parse_result =
        macho_file_parse_from_file(create_info,
                                   fd,
                                   size,
                                   parse_options,
                                   macho_options);

    if (parse_result == E_MACHO_FILE_PARSE_NOT_A_MACHO) {
        lseek(fd, 0, SEEK_SET);
        return false;
    }

    const bool should_continue =
        handle_macho_file_parse_result(global,
                                       tbd,
                                       path,
                                       parse_result,
                                       print_paths,
                                       retained_info);

    if (!should_continue) {
        clear_create_info(create_info, &original_info);
        return true;
    }

    char *const tbd_write_path = tbd->write_path;
    if (tbd_write_path != NULL) {
        char *const write_path =
            tbd_for_main_create_write_path(tbd,
                                           tbd_write_path,
                                           tbd->write_path_length,
                                           path,
                                           path_length,
                                           "tbd",
                                           3,
                                           true,
                                           NULL);

        if (write_path == NULL) {
            fputs("Failed to allocate memory\n", stderr);
            exit(1);
        }
    
        tbd_for_main_write_to_path(tbd, path, write_path, true);
        free(write_path);
    } else {
        tbd_for_main_write_to_stdout(tbd, path, true);
    }

    clear_create_info(create_info, &original_info);
    return true;
}
