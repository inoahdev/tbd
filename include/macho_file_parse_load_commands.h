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
#include "range.h"

struct mf_parse_load_commands_from_file_info {
    int fd;

    const struct arch_info *arch;
    uint64_t arch_bit;

    struct range full_range;
    struct range available_range;

    bool is_64;
    bool is_big_endian;

    uint32_t ncmds;
    uint32_t sizeofcmds;

    uint64_t tbd_options;
    uint64_t options;
};

enum macho_file_parse_result
macho_file_parse_load_commands_from_file(
    struct tbd_create_info *info_in,
    const struct mf_parse_load_commands_from_file_info *parse_info,
    struct symtab_command *symtab_out);

struct mf_parse_load_commands_from_map_info {
    const uint8_t *map;
    uint64_t map_size;

    const uint8_t *macho;
    uint64_t macho_size;

    const struct arch_info *arch;
    uint64_t arch_bit;

    struct range available_map_range;

    bool is_64;
    bool is_big_endian;

    uint32_t ncmds;
    uint32_t sizeofcmds;

    uint64_t tbd_options;
    uint64_t options;
};

enum macho_file_parse_result
macho_file_parse_load_commands_from_map(
    struct tbd_create_info *info_in,
    const struct mf_parse_load_commands_from_map_info *parse_info,
    struct symtab_command *symtab_out);

#endif /* MACHO_FILE_PARSE_LOAD_COMMANDS_H */
