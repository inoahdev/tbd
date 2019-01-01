//
//  src/dsc_image.c
//  tbd 
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <unistd.h>

#include "mach-o/fat.h"
#include "mach-o/nlist.h"

#include "dsc_image.h"
#include "guard_overflow.h"

#include "macho_file_parse_load_commands.h"
#include "macho_file_parse_symbols.h"

#include "range.h"

static enum dsc_image_parse_result
translate_macho_file_parse_result(const enum macho_file_parse_result result) {
    switch (result) {
        case E_MACHO_FILE_PARSE_OK:
            break;

        case E_MACHO_FILE_PARSE_ALLOC_FAIL:
            return E_DSC_IMAGE_PARSE_ALLOC_FAIL;

        case E_MACHO_FILE_PARSE_ARRAY_FAIL:
            return E_DSC_IMAGE_PARSE_ARRAY_FAIL;

        case E_MACHO_FILE_PARSE_SEEK_FAIL:
            return E_DSC_IMAGE_PARSE_SEEK_FAIL;

        case E_MACHO_FILE_PARSE_READ_FAIL:
            return E_DSC_IMAGE_PARSE_READ_FAIL;

        case E_MACHO_FILE_PARSE_NOT_A_MACHO:
            return E_DSC_IMAGE_PARSE_NOT_A_MACHO;

        case E_MACHO_FILE_PARSE_SIZE_TOO_SMALL:
            return E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL;

        /*
         * There is no appropriate error-code for this, but this will never get
         * returned anyways.
         */

        case E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE:
            return E_DSC_IMAGE_PARSE_NOT_A_MACHO;       

        case E_MACHO_FILE_PARSE_NO_ARCHITECTURES:
        case E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES:
        case E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE:
        case E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES:
        case E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE:
            return E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED;

        case E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS:
            return E_DSC_IMAGE_PARSE_NO_LOAD_COMMANDS;

        case E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS:
            return E_DSC_IMAGE_PARSE_TOO_MANY_LOAD_COMMANDS;
    
        case E_MACHO_FILE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL:
            return E_DSC_IMAGE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL;

        case E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND:
            return E_DSC_IMAGE_PARSE_INVALID_LOAD_COMMAND;

        case E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS:
            return E_DSC_IMAGE_PARSE_TOO_MANY_SECTIONS;
        
        case E_MACHO_FILE_PARSE_INVALID_SECTION:
            return E_DSC_IMAGE_PARSE_INVALID_SECTION;

        case E_MACHO_FILE_PARSE_INVALID_CLIENT:
            return E_DSC_IMAGE_PARSE_INVALID_CLIENT;

        case E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME:
            return E_DSC_IMAGE_PARSE_INVALID_INSTALL_NAME;

        case E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA:
            return E_DSC_IMAGE_PARSE_INVALID_PARENT_UMBRELLA;

        case E_MACHO_FILE_PARSE_INVALID_PLATFORM:
            return E_DSC_IMAGE_PARSE_INVALID_PLATFORM;

        case E_MACHO_FILE_PARSE_INVALID_REEXPORT:
            return E_DSC_IMAGE_PARSE_INVALID_REEXPORT;

        case E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE:
            return E_DSC_IMAGE_PARSE_INVALID_SYMBOL_TABLE;

        case E_MACHO_FILE_PARSE_INVALID_UUID:
            return E_DSC_IMAGE_PARSE_INVALID_UUID;

        case E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO:
        case E_MACHO_FILE_PARSE_CONFLICTING_FLAGS:
            return E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED;

        case E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION:
            return E_DSC_IMAGE_PARSE_CONFLICTING_IDENTIFICATION;

        case E_MACHO_FILE_PARSE_CONFLICTING_OBJC_CONSTRAINT:
            return E_DSC_IMAGE_PARSE_CONFLICTING_OBJC_CONSTRAINT;

        case E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA:
            return E_DSC_IMAGE_PARSE_CONFLICTING_PARENT_UMBRELLA;

        case E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM:
            return E_DSC_IMAGE_PARSE_CONFLICTING_PLATFORM;

        case E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION:
            return E_DSC_IMAGE_PARSE_CONFLICTING_SWIFT_VERSION;

        case E_MACHO_FILE_PARSE_CONFLICTING_UUID:
            return E_DSC_IMAGE_PARSE_CONFLICTING_UUID;

        case E_MACHO_FILE_PARSE_NO_IDENTIFICATION:
            return E_DSC_IMAGE_PARSE_NO_IDENTIFICATION;

        case E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE:
            return E_DSC_IMAGE_PARSE_NO_SYMBOL_TABLE;
        
        case E_MACHO_FILE_PARSE_NO_UUID:
            return E_DSC_IMAGE_PARSE_NO_UUID;
        
        case E_MACHO_FILE_PARSE_NO_EXPORTS:
            return E_DSC_IMAGE_PARSE_NO_EXPORTS;
    }

    return E_DSC_IMAGE_PARSE_OK;
}

static uint64_t
get_image_file_offset_from_address(struct dyld_shared_cache_info *const info,
                                   const uint64_t address,
                                   uint64_t *const size_out)
{
    const struct dyld_cache_mapping_info *const mappings = info->mappings;
    const uint64_t count = info->mappings_count;

    for (uint64_t i = 0; i < count; i++) {
        const struct dyld_cache_mapping_info *const mapping = mappings + i;

        const uint64_t mapping_begin = mapping->address;
        const uint64_t mapping_end = mapping_begin + mapping->size;

        const struct range mapping_range = {
            .begin = mapping_begin,
            .end = mapping_end
        };

        if (!range_contains_location(mapping_range, address)) {
            continue;
        }

        const uint64_t delta = address - mapping_begin;
        const uint64_t file_offset = mapping->fileOffset + delta;

        *size_out = mapping->size - delta;
        return file_offset;
    }

    return 0;
}

enum dsc_image_parse_result
dsc_image_parse(struct tbd_create_info *const info_in,
                const int fd,
                const uint64_t start,
                struct dyld_shared_cache_info *const dsc_info,
                struct dyld_cache_image_info *const image,
                const uint64_t macho_options,
                const uint64_t tbd_options,
                const uint64_t options)
{
    const uint64_t size = dsc_info->size;
    if (size != 0) {
        if (size < sizeof(struct mach_header)) {
            return E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL;
        }
    }

    /*
     * The mappings store the data-structures that make up a mach-o file for all
     * dyld_shared_cache images.
     *
     * To find out image's data, we have to recurse the mappings, to find the
     * one containing our file.
     */
    
    uint64_t max_image_size = 0;
    const uint64_t file_offset =
        get_image_file_offset_from_address(dsc_info,
                                           image->address,
                                           &max_image_size);

    if (file_offset == 0) {
        return E_DSC_IMAGE_PARSE_NO_CORRESPONDING_MAPPING;
    }

    if (lseek(fd, start + file_offset, SEEK_SET) < 0) {
        return E_DSC_IMAGE_PARSE_SEEK_FAIL;
    }

    struct mach_header header = {};
    if (read(fd, &header, sizeof(header)) < 0) {
        return E_DSC_IMAGE_PARSE_READ_FAIL;
    }

    const bool is_64 =
        header.magic == MH_MAGIC_64 || header.magic == MH_CIGAM_64;
    
    if (is_64) {
        if (max_image_size < sizeof(struct mach_header_64)) {
            return E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL;
        }

        /*
         * 64-bit mach-o files have a different header (struct mach_header_64),
         * which only differs by having an extra uint32_t field at the end.
         */

        if (lseek(fd, sizeof(uint32_t), SEEK_CUR) < 0) {
            return E_DSC_IMAGE_PARSE_SEEK_FAIL;
        }
    }

    const bool is_fat =
        header.magic == FAT_MAGIC || header.magic == FAT_MAGIC_64 ||
        header.magic == FAT_CIGAM || header.magic == FAT_CIGAM_64;

    if (is_fat) {
        return E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED;
    }

    const bool is_big_endian =
        header.magic == MH_CIGAM || header.magic == MH_CIGAM_64;

    if (!is_64 && !is_big_endian && header.magic != MH_MAGIC) {
        return E_DSC_IMAGE_PARSE_NOT_A_MACHO;
    }

    if (header.flags & MH_TWOLEVEL) {
        info_in->flags |= MH_TWOLEVEL;
    }

    if (!(header.flags & MH_APP_EXTENSION_SAFE)) {
        info_in->flags |= TBD_FLAG_NOT_APP_EXTENSION_SAFE;
    }

    struct symtab_command symtab = {};

    /*
     * The symbol-table and string-table offsets are absolute, not relative from
     * image's base, but we still need to account for shared-cache's start and
     * size. We do this by parsing the symbol-table separately.
     */ 

    const uint64_t arch_bit = dsc_info->arch_bit;
    const uint64_t lc_options =
        macho_options | O_MACHO_FILE_PARSE_DONT_PARSE_SYMBOL_TABLE;

    const enum macho_file_parse_result parse_load_commands_result =    
        macho_file_parse_load_commands(info_in,
                                       dsc_info->arch,
                                       arch_bit,
                                       fd,
                                       start,
                                       size,
                                       is_64,
                                       is_big_endian,
                                       header.ncmds,
                                       header.sizeofcmds,
                                       tbd_options,
                                       lc_options,
                                       &symtab);

    if (parse_load_commands_result != E_MACHO_FILE_PARSE_OK) {
        return translate_macho_file_parse_result(parse_load_commands_result);
    }

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    if (is_64) {
        ret =
            macho_file_parse_symbols_64(info_in,
                                        fd,
                                        arch_bit,
                                        start,
                                        size,
                                        is_big_endian,
                                        symtab.symoff,
                                        symtab.nsyms,
                                        symtab.stroff,
                                        symtab.strsize,
                                        tbd_options,
                                        options);
    } else {
        ret =
            macho_file_parse_symbols(info_in,
                                     fd,
                                     arch_bit,
                                     start,
                                     size,
                                     is_big_endian,
                                     symtab.symoff,
                                     symtab.nsyms,
                                     symtab.stroff,
                                     symtab.strsize,
                                     tbd_options,
                                     options);
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return translate_macho_file_parse_result(ret);
    }

    if (array_is_empty(&info_in->exports)) {
        return E_DSC_IMAGE_PARSE_NO_EXPORTS;
    }

    info_in->archs = arch_bit;
    return E_DSC_IMAGE_PARSE_OK;
}
