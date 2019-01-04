//
//  src/macho_file_parse_symbols.c
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "mach-o/nlist.h"
#include "arch_info.h"

#include "guard_overflow.h"
#include "macho_file_parse_symbols.h"

#include "range.h"
#include "swap.h"

#include "tbd.h"
#include "yaml.h"

/*
 * For performance, compare strings using the largest byte-size integers
 * possible, instead of iterating with strncmp.
 */

static bool
is_objc_class_symbol(const char *const symbol,
                     const uint64_t length,
                     const char **const symbol_out,
                     uint64_t *const length_out)
{
    if (length < 13) {
        return false;
    }

    const uint64_t first = *(uint64_t *)symbol;

    /*
     * Objc-class symbols can have different prefixes.
     */

    if (first == 5495340712935444319) {
        /*
         * The check here is `if (first == "_OBJC_CL")`, checking of whether
         * the prefix is "_OBJC_CLASS_$".
         *
         * The check below is `if (second == "ASS_")`.
         */

        const uint32_t second = *(uint32_t *)(symbol + 8);
        if (second != 1599296321) {
            return false;
        }

        if (symbol[12] != '$') {
            return false;
        }

        *symbol_out = symbol + 13;
        *length_out = length - 13;
    } else if (first == 4993752304437055327) {
        /*
         * The check here is `if (first == "_OBJC_ME")`, checking if the prefix
         * is "_OBJC_METACLASS_$".
         */

        if (length < 17) {
            return false;
        }

        /* 
         * The check below is `if (second == "TACLASS")`.
         */

        const uint64_t second = *(uint64_t *)(symbol + 8);
        if (second != 6868925396587594068) {
            return false;
        }

        if (symbol[16] != '$') {
            return false;
        }

        *symbol_out = symbol + 17;
        *length_out = length - 17;
    } else if (first == 7810191059381808942) {
        /*
         * The check here is `if (first == ".objc_cl")`, checking if the prefix
         * is ".objc_class_name".
         */

        if (length < 16) {
            return false;
        }

        /* 
         * The check below is `if (second == ".ass_name")`.
         */

        const uint64_t second = *(uint64_t *)(symbol + 8);
        if (second != 7308604896967881569) {
            return false;
        }

        *symbol_out = symbol + 16;
        *length_out = length - 16;
    } else {
        return false;
    }

    return true;
}

static bool 
is_objc_ivar_symbol(const char *const symbol, const uint64_t length) {
    if (length < 12) {
        return false;
    }
    
    /*
     * The check here is `if (first == "_OBJC_IV")`.
     */

    const uint64_t first = *(uint64_t *)symbol;
    if (first != 6217605503174987615) {
        return false;
    }

    /*
     * The check here is `if (second == "AR_$")`.
     */

    const uint32_t second = *(uint32_t *)(symbol + 8);
    if (second != 610226753) {
        return false;
    }

    return true;
}

static enum macho_file_parse_result
handle_symbol(struct tbd_create_info *const info,
              const uint64_t arch_bit,
              const char *const symbol_string,
              const uint64_t symbol_length,
              const uint16_t n_desc,
              const uint8_t n_type,
              const uint64_t options)
{
    /*
     * Figure out the symbol-type from the symbol-string and desc.
     * Also ensure the symbol is exported externally, unless otherwise stated.
     */
    
    enum tbd_export_type symbol_type = TBD_EXPORT_TYPE_NORMAL_SYMBOL;
    
    const char *string = symbol_string;
    uint64_t length = symbol_length;

    if (n_desc & N_WEAK_DEF) {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_NORMAL_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }

        symbol_type = TBD_EXPORT_TYPE_WEAK_DEF_SYMBOL;
    } else if (is_objc_class_symbol(string, length, &string, &length)) {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }

        symbol_type = TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL;
    } else if (is_objc_ivar_symbol(symbol_string, length)) {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }

        string += 12;
        length -= 12;
        
        symbol_type = TBD_EXPORT_TYPE_OBJC_IVAR_SYMBOL;
    } else {
        if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_NORMAL_SYMBOLS)) {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }
    }
    
    struct tbd_export_info export_info = {
        .archs = arch_bit,
        .length = length,
        .string = (char *)string,
        .type = symbol_type,
    };

    struct array *const exports = &info->exports;
    struct array_cached_index_info cached_info = {};

    struct tbd_export_info *const existing_info =
        array_find_item_in_sorted(exports,
                                  sizeof(export_info),
                                  &export_info,
                                  tbd_export_info_no_archs_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        existing_info->archs |= arch_bit;
        return E_MACHO_FILE_PARSE_OK;
    }

    bool needs_quotes = false;
    yaml_check_c_str(string, length, &needs_quotes);

    if (needs_quotes) {
        export_info.flags |= F_TBD_EXPORT_INFO_STRING_NEEDS_QUOTES;
    }

    /*
     * Add our symbol-info to the list, as a matching symbol-info was not found.
     *
     * Note: As the symbol is from a large allocation in the call hierarchy that
     * will eventually be freed, we need to allocate a copy of the symbol before
     * placing it in the list.
     */

    export_info.string = strndup(export_info.string, export_info.length);
    if (export_info.string == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }
    
    const enum array_result add_export_info_result =
        array_add_item_with_cached_index_info(exports,
                                              sizeof(export_info),
                                              &export_info,
                                              &cached_info,
                                              NULL);

    if (add_export_info_result != E_ARRAY_OK) {
        free(export_info.string);
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}
            
enum macho_file_parse_result
macho_file_parse_symbols_from_file(struct tbd_create_info *const info,
                                   const int fd,
                                   const uint64_t start,
                                   const uint64_t size,
                                   const uint64_t arch_bit,
                                   const bool is_big_endian,
                                   const uint32_t symoff,
                                   const uint32_t nsyms,
                                   const uint32_t stroff,
                                   const uint32_t strsize,
                                   const uint64_t tbd_options,
                                   const uint64_t options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    uint32_t symbol_table_size = sizeof(struct nlist);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint64_t string_table_end = stroff + strsize;
    if (size != 0) {
        if (string_table_end > size) {
            return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
        }
    }

    uint32_t absolute_symoff = start;
    if (guard_overflow_add(&absolute_symoff, symoff)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    uint64_t symbol_table_end = symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = symoff,
        .end = symbol_table_end
    };

    const struct range string_table_range = {
        .begin = stroff,
        .end = string_table_end
    };

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Read the symbol-table.
     */

    if (lseek(fd, absolute_symoff, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    struct nlist *const symbol_table = calloc(1, symbol_table_size);
    if (symbol_table == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, symbol_table, symbol_table_size) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    /*
     * Read the string-table.
     */

    uint32_t absolute_stroff = start;
    if (guard_overflow_add(&absolute_stroff, stroff)) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    if (lseek(fd, absolute_stroff, SEEK_SET) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    char *const string_table = calloc(1, strsize);
    if (string_table == NULL) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    if (read(fd, string_table, strsize) < 0) {
        free(symbol_table);
        free(string_table);

        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    const struct nlist *nlist = symbol_table;
    const struct nlist *const end = symbol_table + nsyms;

    for (; nlist != end; nlist++) {
        uint16_t n_desc = nlist->n_desc;
        uint32_t index = nlist->n_un.n_strx;

        if (is_big_endian) {
            n_desc = swap_uint16(n_desc);
            index = swap_uint32(index);
        }

        /*
         * Ensure that each symbol connects back to __TEXT, or is an indirect
         * symbol.
         */

        const uint8_t n_type = nlist->n_type;
        const uint8_t type = n_type & N_TYPE;

        if (type != N_SECT && type != N_INDR) {
            continue;
        }

        /*
         * For leniency reasons, ignore invalid symbol-references instead of
         * erroring out.
         */

        if (index >= strsize) {
            continue;
        }

        const char *const symbol_string = string_table + index;
        const uint32_t string_length = strnlen(symbol_string, strsize - index);

        /*
         * Ignore empty strings.
         */

        if (string_length == 0) {
            continue;
        }

        const enum macho_file_parse_result handle_symbol_result =
            handle_symbol(info,
                          arch_bit,
                          symbol_string,
                          string_length,
                          n_desc,
                          n_type,
                          tbd_options);

        if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
            free(symbol_table);
            free(string_table);

            return handle_symbol_result;
        }
    }

    free(symbol_table);
    free(string_table);

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_64_from_file(struct tbd_create_info *const info,
                                      const int fd,
                                      const uint64_t start,
                                      const uint64_t size,
                                      const uint64_t arch_bit,
                                      const bool is_big_endian,
                                      const uint32_t symoff,
                                      const uint32_t nsyms,
                                      const uint32_t stroff,
                                      const uint32_t strsize,
                                      const uint64_t tbd_options,
                                      const uint64_t options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    uint32_t symbol_table_size = sizeof(struct nlist_64);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint64_t string_table_end = stroff + strsize;
    if (size != 0) {
        if (string_table_end > size) {
            return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
        }
    }

    uint32_t absolute_symoff = start;
    if (guard_overflow_add(&absolute_symoff, symoff)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    uint64_t symbol_table_end = symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = symoff,
        .end = symbol_table_end
    };

    const struct range string_table_range = {
        .begin = stroff,
        .end = string_table_end
    };

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    if (lseek(fd, absolute_symoff, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    struct nlist_64 *const symbol_table = calloc(1, symbol_table_size);
    if (symbol_table == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, symbol_table, symbol_table_size) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    uint32_t absolute_stroff = start;
    if (guard_overflow_add(&absolute_stroff, stroff)) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }
    
    if (lseek(fd, absolute_stroff, SEEK_SET) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    char *const string_table = calloc(1, strsize);
    if (string_table == NULL) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    if (read(fd, string_table, strsize) < 0) {
        free(symbol_table);
        free(string_table);

        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    const struct nlist_64 *nlist = symbol_table;
    const struct nlist_64 *const nlist_end = symbol_table + nsyms;

    for (; nlist != nlist_end; nlist++) {
        uint16_t n_desc = nlist->n_desc;
        uint32_t index = nlist->n_un.n_strx;

        if (is_big_endian) {
            n_desc = swap_uint16(n_desc);
            index = swap_uint32(index);
        }

        /*
         * Ensure that each symbol connects back to __TEXT, or is an indirect
         * symbol.
         */

        const uint8_t n_type = nlist->n_type;
        const uint8_t type = n_type & N_TYPE;

        if (type != N_SECT && type != N_INDR) {
            continue;
        }

        /*
         * For leniency reasons, ignore invalid symbol-references instead of
         * erroring out.
         */

        if (index >= strsize) {
            continue;
        }

        const char *const symbol_string = string_table + index;
        const uint32_t string_length = strnlen(symbol_string, strsize - index);

        /*
         * Ignore empty strings.
         */

        if (string_length == 0) {
            continue;
        }

        const enum macho_file_parse_result handle_symbol_result =
            handle_symbol(info,
                          arch_bit,
                          symbol_string,
                          string_length,
                          n_desc,
                          n_type,
                          tbd_options);

        if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
            free(symbol_table);
            free(string_table);

            return handle_symbol_result;
        }
    }

    free(symbol_table);
    free(string_table);
    
    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_from_map(struct tbd_create_info *const info,
                                  const uint8_t *const map,
                                  const uint64_t size,
                                  const uint64_t arch_bit,
                                  const bool is_big_endian,
                                  const uint32_t symoff,
                                  const uint32_t nsyms,
                                  const uint32_t stroff,
                                  const uint32_t strsize,
                                  const uint64_t tbd_options,
                                  const uint64_t options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    uint64_t symbol_table_size = sizeof(struct nlist);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint64_t string_table_end = stroff + strsize;
    if (string_table_end > size) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t symbol_table_end = symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    const struct range symbol_table_range = {
        .begin = symoff,
        .end = symbol_table_end
    };

    const struct range string_table_range = {
        .begin = stroff,
        .end = string_table_end
    };

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const char *const string_table = (const char *)(map + stroff);

    const struct nlist *nlist = (const struct nlist *)(map + symoff);
    const struct nlist *const end = nlist + nsyms;

    for (; nlist != end; nlist++) {
        uint16_t n_desc = nlist->n_desc;
        uint32_t index = nlist->n_un.n_strx;

        if (is_big_endian) {
            n_desc = swap_uint16(n_desc);
            index = swap_uint32(index);
        }

        /*
         * Ensure that each symbol connects back to __TEXT, or is an indirect
         * symbol.
         */

        const uint8_t n_type = nlist->n_type;
        const uint8_t type = n_type & N_TYPE;

        if (type != N_SECT && type != N_INDR) {
            continue;
        }

        /*
         * For leniency reasons, ignore invalid symbol-references instead of
         * erroring out.
         */

        if (index >= strsize) {
            continue;
        }

        const char *const symbol_string = string_table + index;
        const uint32_t string_length = strnlen(symbol_string, strsize - index);

        /*
         * Ignore empty strings.
         */

        if (string_length == 0) {
            continue;
        }

        const enum macho_file_parse_result handle_symbol_result =
            handle_symbol(info,
                          arch_bit,
                          symbol_string,
                          string_length,
                          n_desc,
                          n_type,
                          tbd_options);

        if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
            return handle_symbol_result;
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_64_from_map(struct tbd_create_info *const info,
                                     const uint8_t *const map,
                                     const uint64_t size,
                                     const uint64_t arch_bit,
                                     const bool is_big_endian,
                                     const uint32_t symoff,
                                     const uint32_t nsyms,
                                     const uint32_t stroff,
                                     const uint32_t strsize,
                                     const uint64_t tbd_options,
                                     const uint64_t options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    uint64_t symbol_table_size = sizeof(struct nlist_64);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint64_t string_table_end = stroff + strsize;
    if (string_table_end > size) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t symbol_table_end = symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    const struct range symbol_table_range = {
        .begin = symoff,
        .end = symbol_table_end
    };

    const struct range string_table_range = {
        .begin = stroff,
        .end = string_table_end
    };

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const char *const string_table = (const char *)(map + stroff);

    const struct nlist_64 *nlist = (const struct nlist_64 *)(map + symoff);
    const struct nlist_64 *const end = nlist + nsyms;

    for (; nlist != end; nlist++) {
        uint16_t n_desc = nlist->n_desc;
        uint32_t index = nlist->n_un.n_strx;

        if (is_big_endian) {
            n_desc = swap_uint16(n_desc);
            index = swap_uint32(index);
        }

        /*
         * Ensure that each symbol connects back to __TEXT, or is an indirect
         * symbol.
         */

        const uint8_t n_type = nlist->n_type;
        const uint8_t type = n_type & N_TYPE;

        if (type != N_SECT && type != N_INDR) {
            continue;
        }

        /*
         * For leniency reasons, ignore invalid symbol-references instead of
         * erroring out.
         */

        if (index >= strsize) {
            continue;
        }

        const char *const symbol_string = string_table + index;
        const uint32_t string_length = strnlen(symbol_string, strsize - index);

        /*
         * Ignore empty strings.
         */

        if (string_length == 0) {
            continue;
        }

        const enum macho_file_parse_result handle_symbol_result =
            handle_symbol(info,
                          arch_bit,
                          symbol_string,
                          string_length,
                          n_desc,
                          n_type,
                          tbd_options);

        if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
            return handle_symbol_result;
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}
