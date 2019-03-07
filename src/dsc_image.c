//
//  src/dsc_image.c
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <unistd.h>

#include "mach-o/fat.h"
#include "mach-o/nlist.h"

#include "dsc_image.h"
#include "guard_overflow.h"

#include "macho_file_parse_load_commands.h"
#include "macho_file_parse_symbols.h"

#include "range.h"

/*
 * To avoid duplicating code, we pass on the mach-o verification to macho_file's
 * operations, which have a different error-code.
 *
 * We have many of the same error-codes, save for a few (no fat support), so all
 * that is needed is a simple translation.
 */

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

        /*
         * This error should never be returned from
         * macho_file_parse_load_commands.
         */

        case E_MACHO_FILE_PARSE_FSTAT_FAIL:
            return E_DSC_IMAGE_PARSE_READ_FAIL;

        case E_MACHO_FILE_PARSE_NOT_A_MACHO:
            return E_DSC_IMAGE_PARSE_NOT_A_MACHO;

        case E_MACHO_FILE_PARSE_SIZE_TOO_SMALL:
            return E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL;

        case E_MACHO_FILE_PARSE_INVALID_RANGE:
            return E_DSC_IMAGE_PARSE_INVALID_RANGE;

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
        case E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES:
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

        case E_MACHO_FILE_PARSE_INVALID_STRING_TABLE:
            return E_DSC_IMAGE_PARSE_INVALID_STRING_TABLE;

        case E_MACHO_FILE_PARSE_INVALID_UUID:
            return E_DSC_IMAGE_PARSE_INVALID_UUID;

        /*
         * Conflicting error-codes are only returned for fat-files.
         */

        case E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO:
        case E_MACHO_FILE_PARSE_CONFLICTING_FLAGS:
        case E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION:
        case E_MACHO_FILE_PARSE_CONFLICTING_OBJC_CONSTRAINT:
        case E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA:
        case E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM:
        case E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION:
        case E_MACHO_FILE_PARSE_CONFLICTING_UUID:
            return E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED;

        case E_MACHO_FILE_PARSE_NO_IDENTIFICATION:
            return E_DSC_IMAGE_PARSE_NO_IDENTIFICATION;

        case E_MACHO_FILE_PARSE_NO_PLATFORM:
            return E_DSC_IMAGE_PARSE_NO_PLATFORM;

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
                                   uint64_t *const max_size_out)
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

        *max_size_out = mapping->size - delta;
        return file_offset;
    }

    return 0;
}

enum dsc_image_parse_result
dsc_image_parse(struct tbd_create_info *const info_in,
                struct dyld_shared_cache_info *const dsc_info,
                struct dyld_cache_image_info *const image,
                const uint64_t macho_options,
                const uint64_t tbd_options,
                const uint64_t options)
{
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

    if (max_image_size < sizeof(struct mach_header)) {
        return E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL;
    }

    const uint8_t *const map = dsc_info->map;
    const struct mach_header *const header =
        (const struct mach_header *)(map + file_offset);

    const uint32_t magic = header->magic;

    const bool is_64 = magic == MH_MAGIC_64 || magic == MH_CIGAM_64;
    const bool is_big_endian = magic == MH_CIGAM || magic == MH_CIGAM_64;

    if (is_64) {
        if (max_image_size < sizeof(struct mach_header_64)) {
            return E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL;
        }
    } else {
        const bool is_fat =
            magic == FAT_MAGIC || magic == FAT_MAGIC_64 ||
            magic == FAT_CIGAM || magic == FAT_CIGAM_64;

        if (is_fat) {
            return E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED;
        }

        if (!is_big_endian && magic != MH_MAGIC) {
            return E_DSC_IMAGE_PARSE_NOT_A_MACHO;
        }
    }

    const uint32_t flags = header->flags;
    if (flags & MH_TWOLEVEL) {
        info_in->flags_field |= TBD_FLAG_FLAT_NAMESPACE;
    }

    if (!(flags & MH_APP_EXTENSION_SAFE)) {
        info_in->flags_field |= TBD_FLAG_NOT_APP_EXTENSION_SAFE;
    }

    struct symtab_command symtab = {};

    /*
     * The symbol-table and string-table offsets are absolute, not relative from
     * image's base, but we still need to account for shared-cache's start and
     * size.
     *
     * To accomplish this, we parse the symbol-table separately.
     *
     * The section's offset are also absolute (relative to the map, not to the
     * header).
     */

    const uint64_t arch_bit = dsc_info->arch_bit;
    const uint64_t lc_options =
        O_MACHO_FILE_PARSE_DONT_PARSE_SYMBOL_TABLE |
        O_MACHO_FILE_PARSE_SECT_OFF_ABSOLUTE |
        macho_options;

    struct mf_parse_load_commands_from_map_info info = {
        .map = map,
        .map_size = dsc_info->size,

        .macho = (const uint8_t *)header,
        .macho_size = max_image_size,

        .arch = dsc_info->arch,
        .arch_bit = arch_bit,

        .available_map_range = dsc_info->available_range,

        .is_64 = is_64,
        .is_big_endian = is_big_endian,

        .ncmds = header->ncmds,
        .sizeofcmds = header->sizeofcmds,

        .tbd_options = tbd_options,
        .options = lc_options
    };

    const enum macho_file_parse_result parse_load_commands_result =
        macho_file_parse_load_commands_from_map(info_in, &info, &symtab);

    if (parse_load_commands_result != E_MACHO_FILE_PARSE_OK) {
        return translate_macho_file_parse_result(parse_load_commands_result);
    }

    /*
     * If symtab is invalid, we can simply assume that no symbol-table was
     * found, but that this was ok from the options as
     * macho_file_parse_load_commands_from_map didn't return an error-code.
     */

    if (symtab.cmd != LC_SYMTAB) {
        return E_DSC_IMAGE_PARSE_OK;
    }

    /*
     * For parsing the symbol-tables, we provide the full dyld_shared_cache map
     * as the symbol-table and string-table offsets are relative to the full
     * map, not relative to the mach-o header.
     */

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    if (is_64) {
        ret =
            macho_file_parse_symbols_64_from_map(info_in,
                                                 map,
                                                 dsc_info->available_range,
                                                 arch_bit,
                                                 is_big_endian,
                                                 symtab.symoff,
                                                 symtab.nsyms,
                                                 symtab.stroff,
                                                 symtab.strsize,
                                                 tbd_options);
    } else {
        ret =
            macho_file_parse_symbols_from_map(info_in,
                                              map,
                                              dsc_info->available_range,
                                              arch_bit,
                                              is_big_endian,
                                              symtab.symoff,
                                              symtab.nsyms,
                                              symtab.stroff,
                                              symtab.strsize,
                                              tbd_options);
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return translate_macho_file_parse_result(ret);
    }

    if (!(options & O_TBD_PARSE_IGNORE_MISSING_EXPORTS)) {
        if (array_is_empty(&info_in->exports)) {
            return E_DSC_IMAGE_PARSE_NO_EXPORTS;
        }
    }

    info_in->archs = arch_bit;
    return E_DSC_IMAGE_PARSE_OK;
}
