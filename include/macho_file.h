//
//  include/macho_file.h
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef MACHO_FILE_H
#define MACHO_FILE_H

#include <stdio.h>

#include "mach-o/loader.h"
#include "magic_buffer.h"
#include "range.h"
#include "tbd.h"

struct macho_file_parse_options {
    /*
     * Ignore any errors where an invalid field was produced.
     */

    bool skip_invalid_archs : 1;
    bool dont_parse_exports : 1;

    /*
     * By default, strings are not copied for a mapped mach-o file.
     */

    bool copy_strings_in_map : 1;

    /*
     * Treat a section's offset as absolute.
     */

    bool sect_off_absolute : 1;

    /*
     * A non-dylib filetype is usually ignored.
     */

    bool ignore_wrong_filetype : 1;

    /*
     * Parse the symbol-table instead of the default export-trie.
     */

    bool use_symbol_table : 1;
};

struct macho_file {
    int fd;

    uint32_t magic;
    uint32_t nfat_arch;

    struct mach_header header;
    struct range range;
};

enum macho_file_open_result {
    E_MACHO_FILE_OPEN_OK,
    E_MACHO_FILE_OPEN_READ_FAIL,
    E_MACHO_FILE_OPEN_FSTAT_FAIL,

    E_MACHO_FILE_OPEN_NOT_A_MACHO
};

enum macho_file_open_result
macho_file_open(struct macho_file *__notnull macho,
                struct magic_buffer *__notnull buffer,
                int fd,
                struct range range);

enum macho_file_parse_result {
    E_MACHO_FILE_PARSE_OK,
    E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK,

    E_MACHO_FILE_PARSE_SEEK_FAIL,
    E_MACHO_FILE_PARSE_READ_FAIL,

    E_MACHO_FILE_PARSE_SIZE_TOO_SMALL,

    E_MACHO_FILE_PARSE_INVALID_RANGE,
    E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE,

    E_MACHO_FILE_PARSE_NO_ARCHITECTURES,
    E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES,

    E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE,
    E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES,

    E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE,
    E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_PLATFORM,

    E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES,

    E_MACHO_FILE_PARSE_ALLOC_FAIL,
    E_MACHO_FILE_PARSE_ARRAY_FAIL,

    E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS,
    E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS,

    E_MACHO_FILE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL,
    E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND,

    E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS,
    E_MACHO_FILE_PARSE_INVALID_SECTION,

    E_MACHO_FILE_PARSE_INVALID_CLIENT,
    E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE,
    E_MACHO_FILE_PARSE_INVALID_REEXPORT,
    E_MACHO_FILE_PARSE_INVALID_STRING_TABLE,
    E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE,

    E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO,

    E_MACHO_FILE_PARSE_NO_DATA,
    E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE,

    E_MACHO_FILE_PARSE_CREATE_SYMBOL_LIST_FAIL,
    E_MACHO_FILE_PARSE_CREATE_TARGET_LIST_FAIL
};

enum macho_file_parse_error {
    ERR_MACHO_FILE_PARSE_CURRENT_VERSION_CONFLICT,
    ERR_MACHO_FILE_PARSE_COMPAT_VERSION_CONFLICT,
    ERR_MACHO_FILE_PARSE_FILETYPE_CONFLICT,
    ERR_MACHO_FILE_PARSE_FLAGS_CONFLICT,
    ERR_MACHO_FILE_PARSE_INSTALL_NAME_CONFLICT,
    ERR_MACHO_FILE_PARSE_OBJC_CONSTRAINT_CONFLICT,
    ERR_MACHO_FILE_PARSE_PARENT_UMBRELLA_CONFLICT,
    ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT,
    ERR_MACHO_FILE_PARSE_TARGET_PLATFORM_CONFLICT,
    ERR_MACHO_FILE_PARSE_SWIFT_VERSION_CONFLICT,
    ERR_MACHO_FILE_PARSE_UUID_CONFLICT,

    ERR_MACHO_FILE_PARSE_INVALID_INSTALL_NAME,
    ERR_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA,
    ERR_MACHO_FILE_PARSE_INVALID_PLATFORM,
    ERR_MACHO_FILE_PARSE_INVALID_UUID,

    ERR_MACHO_FILE_PARSE_NOT_A_DYNAMIC_LIBRARY,
    ERR_MACHO_FILE_PARSE_NO_PLATFORM,
    ERR_MACHO_FILE_PARSE_NO_UUID,

    ERR_MACHO_FILE_PARSE_WRONG_FILETYPE,
};

typedef bool
(*macho_file_parse_error_callback)(struct tbd_create_info *__notnull info_in,
                                   enum macho_file_parse_error error,
                                   void *cb_info);

struct macho_file_parse_extra_args {
    macho_file_parse_error_callback callback;
    void *cb_info;

    struct string_buffer *export_trie_sb;
};

enum macho_file_parse_result
macho_file_parse_from_file(struct tbd_create_info *__notnull info_in,
                           struct macho_file *__notnull macho,
                           struct macho_file_parse_extra_args extra,
                           struct tbd_parse_options tbd_options,
                           struct macho_file_parse_options options);

void macho_file_print_archs(int fd);

#endif /* MACHO_FILE_H */
