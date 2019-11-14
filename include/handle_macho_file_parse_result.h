//
//  include/handle_macho_file_parse_result.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef HANDLE_MACHO_PARSE_RESULT_H
#define HANDLE_MACHO_PARSE_RESULT_H

#include "macho_file.h"
#include "tbd_for_main.h"

struct handle_macho_file_parse_error_cb_info {
    uint64_t *retained_info_in;

    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    const char *dir_path;
    const char *name;

    bool print_paths;
    bool is_recursing;
};

bool
handle_macho_file_for_main_error_callback(
    struct tbd_create_info *__notnull info_in,
    enum macho_file_parse_error error,
    void *cb_info);

struct handle_macho_file_parse_result_args {
    uint64_t *retained_info_in;

    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    const char *dir_path;
    const char *name;

    enum macho_file_parse_result parse_result;
    bool print_paths;
};

/*
 * args.dir_path should be the full-path, while args.name is unused.
 */

bool
handle_macho_file_parse_result(struct handle_macho_file_parse_result_args args);

bool
handle_macho_file_parse_result_while_recursing(
	struct handle_macho_file_parse_result_args args);

#endif /* HANDLE_MACHO_PARSE_RESULT_H */
