//
//  include/macho_file_parse_symbol_table.h
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef MACHO_FILE_PARSE_SYMBOLS_H
#define MACHO_FILE_PARSE_SYMBOLS_H

#include <stdio.h>
#include "macho_file.h"

enum macho_file_parse_result
macho_file_parse_symbols(struct tbd_create_info *info,
                         int fd,
                         uint64_t arch_bit,
                         uint64_t start,
                         uint64_t size,
                         bool is_big_endian,
                         uint32_t symoff,
                         uint32_t nsyms,
                         uint32_t stroff,
                         uint32_t strsize,
                         uint64_t parse_options,
                         uint64_t options);

enum macho_file_parse_result
macho_file_parse_symbols_64(struct tbd_create_info *info,
                            int fd,
                            uint64_t arch_bit,
                            uint64_t start,
                            uint64_t size,
                            bool is_big_endian,
                            uint32_t symoff,
                            uint32_t nsyms,
                            uint32_t stroff,
                            uint32_t strsize,
                            uint64_t parse_options,
                            uint64_t options);

#endif /* MACHO_FILE_PARSE_SYMBOLS_H */
