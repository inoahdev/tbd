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
#include "range.h"

enum macho_file_parse_result
macho_file_parse_symbols_from_file(struct tbd_create_info *info,
                                   int fd,
                                   struct range full_range,
                                   struct range available_range,
                                   uint64_t arch_bit,
                                   bool is_big_endian,
                                   uint32_t symoff,
                                   uint32_t nsyms,
                                   uint32_t stroff,
                                   uint32_t strsize,
                                   uint64_t tbd_options);

enum macho_file_parse_result
macho_file_parse_symbols_64_from_file(struct tbd_create_info *info,
                                      int fd,
                                      struct range full_range,
                                      struct range available_range,
                                      uint64_t arch_bit,
                                      bool is_big_endian,
                                      uint32_t symoff,
                                      uint32_t nsyms,
                                      uint32_t stroff,
                                      uint32_t strsize,
                                      uint64_t tbd_options);

enum macho_file_parse_result
macho_file_parse_symbols_from_map(struct tbd_create_info *info,
                                  const uint8_t *map,
                                  struct range available_range,
                                  uint64_t arch_bit,
                                  bool is_big_endian,
                                  uint32_t symoff,
                                  uint32_t nsyms,
                                  uint32_t stroff,
                                  uint32_t strsize,
                                  uint64_t tbd_options);

enum macho_file_parse_result
macho_file_parse_symbols_64_from_map(struct tbd_create_info *info,
                                     const uint8_t *map,
                                     struct range available_range,
                                     uint64_t arch_bit,
                                     bool is_big_endian,
                                     uint32_t symoff,
                                     uint32_t nsyms,
                                     uint32_t stroff,
                                     uint32_t strsize,
                                     uint64_t tbd_options);

#endif /* MACHO_FILE_PARSE_SYMBOLS_H */
