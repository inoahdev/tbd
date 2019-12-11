//
//  include/macho_file_parse_single_lc.c
//  tbd
//
//  Created by inoahdev on 12/10/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include "macho_file.h"

enum macho_file_parse_single_lc_flags {
    F_MF_PARSE_SLC_FOUND_BUILD_VERSION  = 1ull << 0,
    F_MF_PARSE_SLC_FOUND_IDENTIFICATION = 1ull << 1,
    F_MF_PARSE_SLC_FOUND_UUID           = 1ull << 2,

    F_MF_PARSE_SLC_FOUND_CATALYST_PLATFORM = 1ull << 3
};

enum macho_file_parse_single_lc_options {
    O_MF_PARSE_SLC_COPY_STRINGS  = 1ull << 0,
    O_MF_PARSE_SLC_IS_BIG_ENDIAN = 1ull << 1
};

struct macho_file_parse_single_lc_info {
    struct tbd_create_info *info_in;
    enum tbd_platform *platform_in;

    uint64_t *flags_in;
    uint8_t *uuid_in;

    const uint8_t *load_cmd_iter;
    struct load_command load_cmd;

    uint64_t arch_bit;
    int arch_index;

    uint64_t tbd_options;
    uint64_t options;

    struct dyld_info_command *dyld_info_out;
    struct symtab_command *symtab_out;
};

enum macho_file_parse_result
macho_file_parse_single_lc(
    struct macho_file_parse_single_lc_info *__notnull info,
    const macho_file_parse_error_callback callback,
    void *const cb_info);