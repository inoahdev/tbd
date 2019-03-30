//
//  include/parse_macho_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef PARSE_MACHO_FOR_MAIN_H
#define PARSE_MACHO_FOR_MAIN_H

#include "tbd_for_main.h"

/*
 * magic_in should be atleast 4 bytes large.
 */

enum parse_macho_file_result {
    E_PARSE_MACHO_FILE_OK,
    E_PARSE_MACHO_FILE_NOT_A_MACHO,
    E_PARSE_MACHO_FILE_OTHER_ERROR
};

enum parse_macho_file_result
parse_macho_file(void *magic_in,
                 uint64_t *magic_in_size_in,
                 uint64_t *retained_info_in,
                 struct tbd_for_main *global,
                 struct tbd_for_main *tbd,
                 const char *path,
                 uint64_t path_length,
                 int fd,
                 bool ignore_non_macho_error,
                 bool print_paths);

#endif /* PARSE_MACHO_FOR_MAIN_H */
