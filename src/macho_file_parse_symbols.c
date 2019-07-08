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
#include "copy.h"

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

static inline bool
is_objc_class_symbol(const char *const symbol,
                     const uint64_t first,
                     const uint32_t max_length,
                     const char **const symbol_out,
                     uint32_t *const length_out)
{
    if (max_length <= 13) {
        return false;
    }

    /*
     * Objc-class symbols can have different prefixes.
     */

    switch (first) {
        case 5495340712935444319: {
            /*
             * The check here is `if (first == "_OBJC_CL")`, checking of whether
             * the prefix is "_OBJC_CLASS_$".
             *
             * The check below is `if (second == "ASS_")`.
             */

            const uint32_t second = *(const uint32_t *)(symbol + 8);
            if (second != 1599296321) {
                return false;
            }

            if (symbol[12] != '$') {
                return false;
            }

            const char *const real_symbol = symbol + 13;

            *symbol_out = real_symbol;
            *length_out = (uint32_t)strnlen(real_symbol, max_length - 13);

            break;
        }

        case 4993752304437055327: {
            /*
             * The check here is `if (first == "_OBJC_ME")`, checking if the
             * prefix is "_OBJC_METACLASS_$".
             */

            if (max_length <= 17) {
                return false;
            }

            /*
             * The check below is `if (second == "TACLASS")`.
             */

            const uint64_t second = *(const uint64_t *)(symbol + 8);
            if (second != 6868925396587594068) {
                return false;
            }

            if (symbol[16] != '$') {
                return false;
            }

            const char *const real_symbol = symbol + 17;

            *symbol_out = real_symbol;
            *length_out = (uint32_t)strnlen(real_symbol, max_length - 17);

            break;
        }

        case 7810191059381808942: {
            /*
             * The check here is `if (first == ".objc_cl")`, checking if the
             * prefix is ".objc_class_name".
             */

            if (max_length <= 16) {
                return false;
            }

            /*
             * The check below is `if (second == ".ass_name")`.
             */

            const uint64_t second = *(const uint64_t *)(symbol + 8);
            if (second != 7308604896967881569) {
                return false;
            }

            const char *const real_symbol = symbol + 16;

            *symbol_out = real_symbol;
            *length_out = (uint32_t)strnlen(real_symbol, max_length - 16);

            break;
        }

        default:
            return false;
    }

    return true;
}

static inline bool
is_objc_ehtype_symbol(const char *const symbol,
                      const uint64_t first,
                      const uint64_t max_len,
                      const enum tbd_version version)
{
    /*
     * ObjC eh-type symbols are only officially classified in tbd-version v3.
     */

    if (version != TBD_VERSION_V3) {
        return false;
    }

    if (max_len <= 15) {
        return false;
    }

    if (first != 5207673286737153887) {
        return false;
    }

    const uint32_t second = *(const uint32_t *)(symbol + 8);
    if (second != 1162893652) {
        return false;
    }

    const uint16_t third = *(const uint16_t *)(symbol + 12);
    if (third != 9311) {
        return false;
    }

    const uint8_t fourth = *(const uint8_t *)(symbol + 14);
    if (fourth != 95) {
        return false;
    }

    return true;
}

static inline
bool is_objc_ivar_symbol(const char *const symbol, const uint64_t first) {
    /*
     * The check here is `if (first == "_OBJC_IV")`.
     */

    if (first != 6217605503174987615) {
        return false;
    }

    /*
     * The check here is `if (second == "AR_$")`.
     */

    const uint32_t second = *(const uint32_t *)(symbol + 8);
    if (second != 610226753) {
        return false;
    }

    return true;
}

static enum macho_file_parse_result
handle_symbol(struct tbd_create_info *const info_in,
              const uint64_t arch_bit,
              const uint32_t index,
              const uint32_t strsize,
              const char *const symbol_string,
              const uint16_t n_desc,
              const uint8_t n_type,
              const uint64_t options)
{
    /*
     * Short-circuit symbols if they're not external and no options have been
     * provided to allow any type of non-external symbols.
     */

    if (!(n_type & N_EXT)) {
        const uint64_t allow_internal_symbols_flags =
            O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS |
            O_TBD_PARSE_ALLOW_PRIVATE_OBJC_EHTYPE_SYMBOLS |
            O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS;

        if (!(options & allow_internal_symbols_flags)) {
            return E_MACHO_FILE_PARSE_OK;
        }
    }

    const char *string = symbol_string;
    uint32_t max_len = strsize - index;

    /*
     * Figure out the symbol-type from the symbol-string and desc.
     *
     * Also ensure only exported symbols are added, unless options have been
     * provided to allow otherwise.
     */

    enum tbd_export_type symbol_type = TBD_EXPORT_TYPE_NORMAL_SYMBOL;
    uint32_t length = 0;

    if (n_desc & N_WEAK_DEF) {
        if (!(n_type & N_EXT)) {
            return E_MACHO_FILE_PARSE_OK;
        }

        symbol_type = TBD_EXPORT_TYPE_WEAK_DEF_SYMBOL;
        length = (uint32_t)strnlen(symbol_string, max_len);
    } else {
        /*
         * Only check for symbols who max potential length is greater than 12,
         * as that is the minimum length for an objc-ivar symbol.
         */

        if (max_len > 12) {
            const enum tbd_version version = info_in->version;

            const uint64_t first = *(const uint64_t *)string;
            const char *const str = string;

            if (is_objc_class_symbol(str, first, max_len, &string, &length)) {
                if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS)) {
                    if (!(n_type & N_EXT)) {
                        return E_MACHO_FILE_PARSE_OK;
                    }
                }

                if (length == 0) {
                    return E_MACHO_FILE_PARSE_OK;
                }

                /*
                 * On tbd-version v3, the underscore at front of the class-name
                 * is removed.
                 */

                if (info_in->version == TBD_VERSION_V3) {
                    string += 1;
                    length -= 1;
                }

                symbol_type = TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL;
            } else if (is_objc_ehtype_symbol(str, first, max_len, version)) {
                if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_EHTYPE_SYMBOLS))
                {
                    if (!(n_type & N_EXT)) {
                        return E_MACHO_FILE_PARSE_OK;
                    }
                }

                string += 15;
                length = (uint32_t)strnlen(string, max_len - 15);

                if (length == 0) {
                    return E_MACHO_FILE_PARSE_OK;
                }

                symbol_type = TBD_EXPORT_TYPE_OBJC_EHTYPE_SYMBOL;
            } else if (is_objc_ivar_symbol(str, first)) {
                if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS)) {
                    if (!(n_type & N_EXT)) {
                        return E_MACHO_FILE_PARSE_OK;
                    }
                }

                /*
                 * On tbd-version v3, the underscore at front of the symbol is
                 * removed.
                 */

                if (info_in->version == TBD_VERSION_V3) {
                    string += 1;
                    max_len -= 1;
                }

                string += 12;
                length = (uint32_t)strnlen(string, max_len - 12);

                if (length == 0) {
                    return E_MACHO_FILE_PARSE_OK;
                }

                symbol_type = TBD_EXPORT_TYPE_OBJC_IVAR_SYMBOL;
            } else {
                if (!(n_type & N_EXT)) {
                    return E_MACHO_FILE_PARSE_OK;
                }

                length = (uint32_t)strnlen(string, max_len);
                if (length == 0) {
                    return E_MACHO_FILE_PARSE_OK;
                }
            }
        } else {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }

            length = (uint32_t)strnlen(string, max_len);
            if (length == 0) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }
    }

    struct tbd_export_info export_info = {
        .archs = arch_bit,
        .archs_count = 1,
        .length = length,
        .string = (char *)string,
        .type = symbol_type,
    };

    if (options & O_TBD_PARSE_EXPORTS_HAVE_FULL_ARCHS) {
        export_info.archs = info_in->archs;
        export_info.archs_count = info_in->archs_count;
    }

    struct array *const exports = &info_in->exports;
    struct array_cached_index_info cached_info = {};

    struct tbd_export_info *const existing_info =
        array_find_item_in_sorted(exports,
                                  sizeof(export_info),
                                  &export_info,
                                  tbd_export_info_no_archs_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        if (options & O_TBD_PARSE_EXPORTS_HAVE_FULL_ARCHS) {
            return E_MACHO_FILE_PARSE_OK;
        }

        /*
         * Ensure multiple symbols for the same arch are ignored (For the
         * sake of leniency).
         */

        const uint64_t archs = existing_info->archs;
        if (archs & arch_bit) {
            return E_MACHO_FILE_PARSE_OK;
        }

        existing_info->archs = archs | arch_bit;
        existing_info->archs_count += 1;

        return E_MACHO_FILE_PARSE_OK;
    }

    const bool needs_quotes = yaml_check_c_str(string, length);
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

    export_info.string = alloc_and_copy(export_info.string, export_info.length);
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
macho_file_parse_symbols_from_file(struct tbd_create_info *const info_in,
                                   const int fd,
                                   const struct range full_range,
                                   const struct range available_range,
                                   const uint64_t arch_bit,
                                   const bool is_big_endian,
                                   const uint32_t symoff,
                                   const uint32_t nsyms,
                                   const uint32_t stroff,
                                   const uint32_t strsize,
                                   const uint64_t tbd_options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Ensure the string-table and symbol-table are fully contained within
     * the mach-o file.
     */

    uint32_t symbol_table_size = sizeof(struct nlist);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint32_t absolute_symoff = (uint32_t)full_range.begin;
    if (guard_overflow_add(&absolute_symoff, symoff)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t symbol_table_end = absolute_symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = absolute_symoff,
        .end = symbol_table_end
    };

    /*
     * Validate that the symbol-table range is fully within the given
     * available-range.
     */

    if (!range_contains_range(available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint32_t absolute_stroff = (uint32_t)full_range.begin;
    if (guard_overflow_add(&absolute_stroff, stroff)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    const uint64_t string_table_end = absolute_stroff + strsize;
    const struct range string_table_range = {
        .begin = absolute_stroff,
        .end = string_table_end
    };

    /*
     * Validate that the string-table range is fully within the given
     * available-range.
     */

    if (!range_contains_range(available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    if (lseek(fd, absolute_symoff, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    struct nlist *const symbol_table = malloc(symbol_table_size);
    if (symbol_table == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, symbol_table, symbol_table_size) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    if (lseek(fd, absolute_stroff, SEEK_SET) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    char *const string_table = malloc(strsize);
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

    if (is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol either connects back to __TEXT, or is an
             * indirect symbol.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (index >= strsize) {
                continue;
            }

            const int16_t n_desc = swap_int16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(info_in,
                              arch_bit,
                              index,
                              strsize,
                              symbol_string,
                              (uint16_t)n_desc,
                              n_type,
                              tbd_options);

            if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
                free(symbol_table);
                free(string_table);

                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol either connects back to __TEXT, or is an
             * indirect symbol.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (index >= strsize) {
                continue;
            }

            const int16_t n_desc = swap_int16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(info_in,
                              arch_bit,
                              index,
                              strsize,
                              symbol_string,
                              (uint16_t)n_desc,
                              n_type,
                              tbd_options);

            if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
                free(symbol_table);
                free(string_table);

                return handle_symbol_result;
            }
        }
    }

    free(symbol_table);
    free(string_table);

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_64_from_file(struct tbd_create_info *const info_in,
                                      const int fd,
                                      const struct range full_range,
                                      const struct range available_range,
                                      const uint64_t arch_bit,
                                      const bool is_big_endian,
                                      const uint32_t symoff,
                                      const uint32_t nsyms,
                                      const uint32_t stroff,
                                      const uint32_t strsize,
                                      const uint64_t tbd_options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Get the size of the symbol table by multipying the symbol-count and the
     * size of symbol-table-entry.
     */

    uint64_t symbol_table_size = sizeof(struct nlist_64);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint32_t absolute_symoff = (uint32_t)full_range.begin;
    if (guard_overflow_add(&absolute_symoff, symoff)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t symbol_table_end = absolute_symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = absolute_symoff,
        .end = symbol_table_end
    };

    /*
     * Validate that the symbol-table range is fully within the given
     * available-range.
     */

    if (!range_contains_range(available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint32_t absolute_stroff = (uint32_t)full_range.begin;
    if (guard_overflow_add(&absolute_stroff, stroff)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const uint64_t string_table_end = absolute_stroff + strsize;
    const struct range string_table_range = {
        .begin = absolute_stroff,
        .end = string_table_end
    };

    /*
     * Validate that the string-table range is fully within the given
     * available-range.
     */

    if (!range_contains_range(available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    if (lseek(fd, absolute_symoff, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    struct nlist_64 *const symbol_table = malloc(symbol_table_size);
    if (symbol_table == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, symbol_table, symbol_table_size) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    if (lseek(fd, absolute_stroff, SEEK_SET) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    char *const string_table = malloc(strsize);
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
    const struct nlist_64 *const end = symbol_table + nsyms;

    if (is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol either connects back to __TEXT, or is an
             * indirect symbol.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (index >= strsize) {
                continue;
            }

            const uint16_t n_desc = swap_uint16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(info_in,
                              arch_bit,
                              index,
                              strsize,
                              symbol_string,
                              n_desc,
                              n_type,
                              tbd_options);

            if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
                free(symbol_table);
                free(string_table);

                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol either connects back to __TEXT, or is an
             * indirect symbol.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (index >= strsize) {
                continue;
            }

            const uint16_t n_desc = nlist->n_desc;

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(info_in,
                              arch_bit,
                              index,
                              strsize,
                              symbol_string,
                              n_desc,
                              n_type,
                              tbd_options);

            if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
                free(symbol_table);
                free(string_table);

                return handle_symbol_result;
            }
        }
    }

    free(symbol_table);
    free(string_table);

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_from_map(struct tbd_create_info *const info_in,
                                  const uint8_t *const map,
                                  const struct range available_range,
                                  const uint64_t arch_bit,
                                  const bool is_big_endian,
                                  const uint32_t symoff,
                                  const uint32_t nsyms,
                                  const uint32_t stroff,
                                  const uint32_t strsize,
                                  const uint64_t tbd_options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    const struct range string_table_range = {
        .begin = stroff,
        .end = stroff + strsize
    };

    if (!range_contains_range(available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    uint64_t symbol_table_size = sizeof(struct nlist);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t symbol_table_end = symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = symoff,
        .end = symbol_table_end
    };

    if (!range_contains_range(available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const char *const string_table = (const char *)(map + stroff);

    const struct nlist *nlist = (const struct nlist *)(map + symoff);
    const struct nlist *const end = nlist + nsyms;

    if (is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol either connects back to __TEXT, or is an
             * indirect symbol.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (index >= strsize) {
                continue;
            }

            const int16_t n_desc = swap_int16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(info_in,
                              arch_bit,
                              index,
                              strsize,
                              symbol_string,
                              (uint16_t)n_desc,
                              n_type,
                              tbd_options);

            if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol connects back to __TEXT, or is an
             * indirect symbol.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (index >= strsize) {
                continue;
            }

            const int16_t n_desc = nlist->n_desc;

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(info_in,
                              arch_bit,
                              index,
                              strsize,
                              symbol_string,
                              (uint16_t)n_desc,
                              n_type,
                              tbd_options);

            if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
                return handle_symbol_result;
            }
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_64_from_map(struct tbd_create_info *const info_in,
                                     const uint8_t *const map,
                                     const struct range available_range,
                                     const uint64_t arch_bit,
                                     const bool is_big_endian,
                                     const uint32_t symoff,
                                     const uint32_t nsyms,
                                     const uint32_t stroff,
                                     const uint32_t strsize,
                                     const uint64_t tbd_options)
{
    if (nsyms == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    const struct range string_table_range = {
        .begin = stroff,
        .end = stroff + strsize
    };

    if (!range_contains_range(available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Get the size of the symbol table by multipying the symbol-count and the
     * size of symbol-table-entry.
     */

    uint64_t symbol_table_size = sizeof(struct nlist_64);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t symbol_table_end = symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = symoff,
        .end = symbol_table_end
    };

    if (!range_contains_range(available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const char *const string_table = (const char *)(map + stroff);

    const struct nlist_64 *nlist = (const struct nlist_64 *)(map + symoff);
    const struct nlist_64 *const end = nlist + nsyms;

    if (is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol either connects back to __TEXT, or is an
             * indirect symbol.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (index >= strsize) {
                continue;
            }

            const uint16_t n_desc = swap_uint16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(info_in,
                              arch_bit,
                              index,
                              strsize,
                              symbol_string,
                              n_desc,
                              n_type,
                              tbd_options);

            if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol connects back to __TEXT, or is an
             * indirect symbol.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (index >= strsize) {
                continue;
            }

            const uint16_t n_desc = nlist->n_desc;

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(info_in,
                              arch_bit,
                              index,
                              strsize,
                              symbol_string,
                              n_desc,
                              n_type,
                              tbd_options);

            if (handle_symbol_result != E_MACHO_FILE_PARSE_OK) {
                return handle_symbol_result;
            }
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}
