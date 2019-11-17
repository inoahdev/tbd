//
//  include/macho_file_parse_symtab.h
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef MACHO_FILE_PARSE_SYMTAB_H
#define MACHO_FILE_PARSE_SYMTAB_H

#include <stdio.h>

#include "macho_file.h"
#include "notnull.h"
#include "range.h"

struct macho_file_parse_symtab_args {
    struct tbd_create_info *info_in;

    /*
     * Note: macho_file_parse_symbols_[64_]from_map() does not use macho_range.
     */

    struct range available_range;

    /*
     * `fully_parse` stores whether we parse the entire symbol-table, or only
     * private objc-symbols.
     */

    bool fully_parse;
    bool is_big_endian;

    uint32_t symoff;
    uint32_t nsyms;
    uint32_t stroff;
    uint32_t strsize;

    uint64_t arch_bit;
    uint64_t tbd_options;
};

enum macho_file_parse_result
macho_file_parse_symtab_from_file(
    const struct macho_file_parse_symtab_args *__notnull args,
    int fd,
    uint64_t base_offset);

enum macho_file_parse_result
macho_file_parse_symtab_64_from_file(
    const struct macho_file_parse_symtab_args *__notnull args,
    int fd,
    uint64_t base_offset);

enum macho_file_parse_result
macho_file_parse_symtab_from_map(
    const struct macho_file_parse_symtab_args *__notnull args,
    const uint8_t *__notnull map);

enum macho_file_parse_result
macho_file_parse_symtab_64_from_map(
    const struct macho_file_parse_symtab_args *__notnull args,
    const uint8_t *__notnull map);

#endif /* MACHO_FILE_PARSE_SYMTAB_H */
