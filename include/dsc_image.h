//
//  include/dsc_image.h
//  tbd 
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef DSC_IMAGE_H
#define DSC_IMAGE_H

#include "dyld_shared_cache.h"
#include "tbd.h"

enum dsc_image_parse_result {
    E_DSC_IMAGE_PARSE_OK,

    E_DSC_IMAGE_PARSE_ALLOC_FAIL,
    E_DSC_IMAGE_PARSE_ARRAY_FAIL,

    E_DSC_IMAGE_PARSE_SEEK_FAIL,
    E_DSC_IMAGE_PARSE_READ_FAIL,

    E_DSC_IMAGE_PARSE_NO_CORRESPONDING_MAPPING,
    E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL,

    E_DSC_IMAGE_PARSE_NOT_A_MACHO,
    E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED,
    
    E_DSC_IMAGE_PARSE_NO_LOAD_COMMANDS,
    E_DSC_IMAGE_PARSE_TOO_MANY_LOAD_COMMANDS,

    E_DSC_IMAGE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL,
    E_DSC_IMAGE_PARSE_INVALID_LOAD_COMMAND,

    E_DSC_IMAGE_PARSE_TOO_MANY_SECTIONS,
    E_DSC_IMAGE_PARSE_INVALID_SECTION,

    E_DSC_IMAGE_PARSE_INVALID_CLIENT,
    E_DSC_IMAGE_PARSE_INVALID_INSTALL_NAME,
    E_DSC_IMAGE_PARSE_INVALID_PARENT_UMBRELLA,
    E_DSC_IMAGE_PARSE_INVALID_PLATFORM,
    E_DSC_IMAGE_PARSE_INVALID_REEXPORT,
    E_DSC_IMAGE_PARSE_INVALID_SYMBOL_TABLE,
    E_DSC_IMAGE_PARSE_INVALID_UUID,

    E_DSC_IMAGE_PARSE_NO_IDENTIFICATION,
    E_DSC_IMAGE_PARSE_NO_SYMBOL_TABLE,
    E_DSC_IMAGE_PARSE_NO_UUID,
    
    E_DSC_IMAGE_PARSE_NO_EXPORTS
};

enum dsc_image_parse_result
dsc_image_parse(struct tbd_create_info *info_in,
                struct dyld_shared_cache_info *dsc_info,
                struct dyld_cache_image_info *image,
                uint64_t macho_options,
                uint64_t tbd_options,
                uint64_t options);

#endif /* DSC_IMAGE_H */
