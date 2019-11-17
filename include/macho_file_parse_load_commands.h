//
//  include/macho_file_parse_load_commands.h
//  tbd
//
//  Created by inoahdev on 11/20/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef MACHO_FILE_PARSE_LOAD_COMMANDS_H
#define MACHO_FILE_PARSE_LOAD_COMMANDS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "arch_info.h"
#include "macho_file.h"
#include "notnull.h"
#include "range.h"

enum mf_parse_load_commands_flags {
    F_MF_PARSE_LOAD_COMMANDS_IS_64 = (1ull << 0),
    F_MF_PARSE_LOAD_COMMANDS_IS_BIG_ENDIAN = (1ull << 1)
};

struct mf_parse_lc_from_file_info {
    int fd;

    const struct arch_info *arch;
    uint64_t arch_bit;

    struct range macho_range;
    struct range available_range;

    uint32_t ncmds;
    uint32_t sizeofcmds;
    uint32_t header_size;

    uint64_t tbd_options;
    uint64_t options;

    uint64_t flags;
};

struct macho_file_symbol_lc_info_out {
    struct dyld_info_command dyld_info;
    struct symtab_command symtab;
};

enum macho_file_parse_result
macho_file_parse_load_commands_from_file(
    struct tbd_create_info *__notnull info_in,
    const struct mf_parse_lc_from_file_info *__notnull parse_info,
    struct macho_file_parse_extra_args extra,
    struct macho_file_symbol_lc_info_out *sym_info_out);

struct mf_parse_lc_from_map_info {
    const uint8_t *map;
    uint64_t map_size;

    const uint8_t *macho;
    uint64_t macho_size;

    const struct arch_info *arch;
    uint64_t arch_bit;

    struct range available_map_range;

    uint32_t ncmds;
    uint32_t sizeofcmds;

    uint64_t tbd_options;
    uint64_t options;

    uint64_t flags;
};

enum macho_file_parse_result
macho_file_parse_load_commands_from_map(
    struct tbd_create_info *__notnull info_in,
    const struct mf_parse_lc_from_map_info *__notnull parse_info,
    struct macho_file_parse_extra_args extra,
    struct macho_file_symbol_lc_info_out *sym_info_out);

#endif /* MACHO_FILE_PARSE_LOAD_COMMANDS_H */
