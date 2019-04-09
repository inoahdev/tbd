//
//  include/parse_dsc_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef PARSE_DSC_FOR_MAIN_H
#define PARSE_DSC_FOR_MAIN_H

#include "tbd_for_main.h"

/*
 * magic_in should be atleast 16 bytes large.
 */

enum parse_dsc_for_main_result {
    E_PARSE_DSC_FOR_MAIN_OK,
    E_PARSE_DSC_FOR_MAIN_NOT_A_SHARED_CACHE,
    E_PARSE_DSC_FOR_MAIN_OTHER_ERROR
};

struct parse_dsc_for_main_args {
    int fd;
    void *magic_in;

    uint64_t *magic_in_size_in;
    uint64_t *retained_info_in;

    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    const char *dsc_dir_path;
    uint64_t dsc_dir_path_length;

    const char *dsc_name;
    const uint64_t dsc_name_length;

    bool ignore_non_cache_error;
    bool print_paths;
};

enum parse_dsc_for_main_result
parse_dsc_for_main(struct parse_dsc_for_main_args args);

enum parse_dsc_for_main_result
parse_dsc_for_main_while_recursing(struct parse_dsc_for_main_args args);

void print_list_of_dsc_images(int fd);

#endif /* PARSE_DSC_FOR_MAIN_H */
