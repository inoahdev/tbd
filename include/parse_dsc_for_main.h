//
//  include/parse_dsc_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef PARSE_DSC_FOR_MAIN_H
#define PARSE_DSC_FOR_MAIN_H

#include "string_buffer.h"
#include "tbd_for_main.h"

/*
 * magic_in should be atleast 16 bytes large.
 */

enum parse_dsc_for_main_result {
    E_PARSE_DSC_FOR_MAIN_OK,
    E_PARSE_DSC_FOR_MAIN_NOT_A_SHARED_CACHE,
    E_PARSE_DSC_FOR_MAIN_OTHER_ERROR,
    E_PARSE_DSC_FOR_MAIN_CLOSE_COMBINE_FILE_FAIL
};

enum parse_dsc_for_main_options {
    O_PARSE_DSC_FOR_MAIN_VERIFY_WRITE_PATH = 1ull << 0
};

struct parse_dsc_for_main_args {
    int fd;
    void *magic_in;

    uint64_t *magic_in_size_in;
    uint64_t *retained_info_in;

    struct tbd_for_main *global;
    struct tbd_for_main *tbd;
    const struct tbd_create_info *orig;

    const char *dsc_dir_path;
    uint64_t dsc_dir_path_length;

    const char *dsc_name;
    uint64_t dsc_name_length;

    FILE *combine_file;

    bool dont_handle_non_dsc_error;
    bool print_paths;

    struct string_buffer *export_trie_sb;
    uint64_t options;
};

enum parse_dsc_for_main_result
parse_dsc_for_main(struct parse_dsc_for_main_args args);

enum parse_dsc_for_main_result
parse_dsc_for_main_while_recursing(
    struct parse_dsc_for_main_args *__notnull args);

void print_list_of_dsc_images(int fd);
void print_list_of_dsc_images_ordered(int fd);

#endif /* PARSE_DSC_FOR_MAIN_H */
