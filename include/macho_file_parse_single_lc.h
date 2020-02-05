//
//  include/macho_file_parse_single_lc.c
//  tbd
//
//  Created by inoahdev on 12/10/19.
//  Copyright © 2019 - 2020 inoahdev. All rights reserved.
//

#include "macho_file.h"

struct macho_file_parse_slc_flags {
    bool is_big_endian : 1;
    bool expecting_sim_platform : 1;

    bool found_build_version : 1;
    bool found_identification : 1;
    bool found_uuid : 1;

    bool found_iosmac_platform : 1;
};

struct macho_file_parse_slc_options {
    bool copy_strings : 1;
};

struct macho_file_parse_single_lc_info {
    struct tbd_create_info *info_in;
    enum tbd_platform *platform_in;

    struct macho_file_parse_slc_flags *flags_in;
    uint8_t *uuid_in;

    const uint8_t *lc_iter;
    struct load_command load_cmd;

    uint64_t arch_index;

    struct tbd_parse_options tbd_options;
    struct macho_file_parse_slc_options options;

    uint32_t *export_off_out;
    uint32_t *export_size_out;

    struct symtab_command *symtab_out;
};

enum macho_file_parse_result
macho_file_parse_single_lc(
    struct macho_file_parse_single_lc_info *__notnull info,
    const macho_file_parse_error_callback callback,
    void *const cb_info);
