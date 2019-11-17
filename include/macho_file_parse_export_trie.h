//
//  include/macho_file_parse_export_trie.h
//  tbd
//
//  Created by inoahdev on 11/3/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#ifndef MACHO_FILE_PARSE_EXPORT_TRIE_H
#define MACHO_FILE_PARSE_EXPORT_TRIE_H

#include "mach-o/loader.h"
#include "arch_info.h"
#include "macho_file.h"
#include "range.h"
#include "string_buffer.h"

struct macho_file_parse_export_trie_args {
    struct tbd_create_info *info_in;

    const struct arch_info *arch;
    uint64_t arch_bit;

    struct range available_range;

    bool is_64;
    bool is_big_endian;

    struct dyld_info_command dyld_info;
    struct string_buffer *sb_buffer;

    uint64_t tbd_options;
};

enum macho_file_parse_result
macho_file_parse_export_trie_from_file(
    struct macho_file_parse_export_trie_args args,
    int fd,
    uint64_t base_offset);

enum macho_file_parse_result
macho_file_parse_export_trie_from_map(
    struct macho_file_parse_export_trie_args args,
    const uint8_t *__notnull map);

#endif /* MACHO_FILE_PARSE_EXPORT_TRIE_H */
