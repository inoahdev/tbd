//
//  include/parse_dsc_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef PARSE_DSC_FOR_MAIN_H
#define PARSE_DSC_FOR_MAIN_H

#include "magic_buffer.h"
#include "string_buffer.h"
#include "tbd_for_main.h"

/*
 * magic_in should be atleast 16 bytes large.
 */

struct parse_dsc_for_main_options {
    bool verify_write_path : 1;
};

struct parse_dsc_for_main_args {
    int fd;

    struct magic_buffer *magic_buffer;
    struct retained_user_info *retained;

    struct tbd_for_main *tbd;
    struct tbd_for_main *orig;

    const char *dsc_dir_path;
    uint64_t dsc_dir_path_length;

    const char *dsc_name;
    uint64_t dsc_name_length;

    FILE *combine_file;

    bool dont_handle_non_dsc_error;
    bool print_paths;

    struct string_buffer *export_trie_sb;
    struct parse_dsc_for_main_options options;
};

enum parse_dsc_for_main_result {
    E_PARSE_DSC_FOR_MAIN_OK,
    E_PARSE_DSC_FOR_MAIN_NOT_A_SHARED_CACHE,
    E_PARSE_DSC_FOR_MAIN_OTHER_ERROR,
    E_PARSE_DSC_FOR_MAIN_CLOSE_COMBINE_FILE_FAIL
};

enum parse_dsc_for_main_result
parse_dsc_for_main(struct parse_dsc_for_main_args args);

enum parse_dsc_for_main_result
parse_dsc_for_main_while_recursing(
    struct parse_dsc_for_main_args *__notnull args);

void print_list_of_dsc_images(int fd);
void print_list_of_dsc_images_ordered(int fd);

#endif /* PARSE_DSC_FOR_MAIN_H */
