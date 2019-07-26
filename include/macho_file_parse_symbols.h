//
//  include/macho_file_parse_symbols.h
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef MACHO_FILE_PARSE_SYMBOLS_H
#define MACHO_FILE_PARSE_SYMBOLS_H

#include <stdio.h>

#include "macho_file.h"
#include "notnull.h"
#include "range.h"

struct macho_file_parse_symbols_args {
    struct tbd_create_info *info_in;

    /*
     * Note: macho_file_parse_symbols_[64_]from_map() does not use full_range.
     */

    struct range full_range;
    struct range available_range;

    uint64_t arch_bit;
    bool is_big_endian;

    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;

    uint64_t tbd_options;
};

enum macho_file_parse_result
macho_file_parse_symbols_from_file(struct macho_file_parse_symbols_args args,
                                   int fd);

enum macho_file_parse_result
macho_file_parse_symbols_64_from_file(struct macho_file_parse_symbols_args args,
                                      int fd);

enum macho_file_parse_result
macho_file_parse_symbols_from_map(struct macho_file_parse_symbols_args args,
                                  const uint8_t *__notnull map);

enum macho_file_parse_result
macho_file_parse_symbols_64_from_map(struct macho_file_parse_symbols_args args,
                                     const uint8_t *__notnull map);

#endif /* MACHO_FILE_PARSE_SYMBOLS_H */
