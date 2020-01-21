//
//  include/parse_macho_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#ifndef PARSE_MACHO_FOR_MAIN_H
#define PARSE_MACHO_FOR_MAIN_H

#include "magic_buffer.h"
#include "string_buffer.h"
#include "tbd_for_main.h"

struct parse_macho_for_main_options {
    bool verify_write_path : 1;
};

struct parse_macho_for_main_args {
    /*
     * magic_in should be at least 4 bytes large.
     */

    int fd;

    struct magic_buffer *magic_buffer;
    struct retained_user_info *retained;

    struct tbd_for_main *tbd;
    struct tbd_for_main *orig;

    /*
     * When not recursing, name will be NULL and dir_path will store the entire
     * path.
     */

    const char *dir_path;
    uint64_t dir_path_length;

    const char *name;
    uint64_t name_length;

    FILE *combine_file;

    bool dont_handle_non_macho_error : 1;
    bool print_paths : 1;

    struct string_buffer *export_trie_sb;
    struct parse_macho_for_main_options options;
};

enum parse_macho_for_main_result {
    E_PARSE_MACHO_FOR_MAIN_OK,
    E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO,
    E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR
};

enum parse_macho_for_main_result
parse_macho_file_for_main(struct parse_macho_for_main_args args);

enum parse_macho_for_main_result
parse_macho_file_for_main_while_recursing(
    struct parse_macho_for_main_args *__notnull args_ptr);

#endif /* PARSE_MACHO_FOR_MAIN_H */
