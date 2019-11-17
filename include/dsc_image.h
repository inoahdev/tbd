//
//  include/dsc_image.h
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef DSC_IMAGE_H
#define DSC_IMAGE_H

#include "dyld_shared_cache.h"
#include "likely.h"
#include "macho_file.h"
#include "notnull.h"
#include "string_buffer.h"
#include "tbd.h"

enum dsc_image_parse_result {
    E_DSC_IMAGE_PARSE_OK,
    E_DSC_IMAGE_PARSE_ERROR_PASSED_TO_CALLBACK,

    E_DSC_IMAGE_PARSE_ALLOC_FAIL,
    E_DSC_IMAGE_PARSE_ARRAY_FAIL,

    E_DSC_IMAGE_PARSE_SEEK_FAIL,
    E_DSC_IMAGE_PARSE_READ_FAIL,

    E_DSC_IMAGE_PARSE_NO_MAPPING,
    E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL,

    E_DSC_IMAGE_PARSE_INVALID_RANGE,

    E_DSC_IMAGE_PARSE_NOT_A_MACHO,
    E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED,

    E_DSC_IMAGE_PARSE_NO_LOAD_COMMANDS,
    E_DSC_IMAGE_PARSE_TOO_MANY_LOAD_COMMANDS,

    E_DSC_IMAGE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL,
    E_DSC_IMAGE_PARSE_INVALID_LOAD_COMMAND,

    E_DSC_IMAGE_PARSE_TOO_MANY_SECTIONS,
    E_DSC_IMAGE_PARSE_INVALID_SECTION,

    E_DSC_IMAGE_PARSE_INVALID_CLIENT,
    E_DSC_IMAGE_PARSE_INVALID_REEXPORT,
    E_DSC_IMAGE_PARSE_INVALID_SYMBOL_TABLE,
    E_DSC_IMAGE_PARSE_INVALID_STRING_TABLE,

    E_DSC_IMAGE_PARSE_NO_EXPORTS,
    E_DSC_IMAGE_PARSE_NO_SYMBOL_TABLE,

    E_DSC_IMAGE_PARSE_INVALID_EXPORTS_TRIE,
    E_DSC_IMAGE_PARSE_CREATE_SYMBOLS_FAIL
};

enum dsc_image_parse_result
dsc_image_parse(struct tbd_create_info *__notnull info_in,
                struct dyld_shared_cache_info *__notnull dsc_info,
                struct dyld_cache_image_info *__notnull image,
                const macho_file_parse_error_callback callback,
                void *const callback_info,
                struct string_buffer *__notnull export_trie_sb,
                uint64_t macho_options,
                uint64_t tbd_options,
                uint64_t options);

#endif /* DSC_IMAGE_H */
