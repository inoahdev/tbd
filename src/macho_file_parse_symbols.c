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

#include "likely.h"
#include "range.h"
#include "swap.h"

#include "tbd.h"
#include "yaml.h"

/*
 * We compare strings by using the largest possible byte size when reading from
 * memory to maximize our performance.
 */

static inline bool
is_objc_class_symbol(const char *__notnull const symbol,
                     const uint64_t first,
                     const uint32_t max_length,
                     const char **__notnull const symbol_out,
                     uint32_t *__notnull const length_out)
{
    /*
     * Ensure the max potential length of the symbol is at least 14 bytes.
     */

    if (max_length < 15) {
        return false;
    }

    /*
     * Objc-class symbols may have different prefixes.
     */

    switch (first) {
        case 5495340712935444319: {
            /*
             * The check above is `if (first == "_OBJC_CL")`, checking if the
             * prefix is "_OBJC_CLASS_$_".
             *
             * The check below is `if (second != "ASS_")`.
             */

            const uint32_t second = *(const uint32_t *)(symbol + 8);
            if (second != 1599296321) {
                return false;
            }

            /*
             * The check below is `if (third != "$_")`.
             */

            const uint16_t third = *(const uint16_t *)(symbol + 12);
            if (third != 24356) {
                return false;
            }

            /*
             * We return the symbol with its preceding underscore.
             */

            const char *const real_symbol = symbol + 13;

            /*
             * Add one to real_symbol to avoid rechecking the underscore.
             */

            const uint32_t length =
                (uint32_t)strnlen(real_symbol + 1, max_length - 14) + 1;

            *symbol_out = real_symbol;
            *length_out = length;

            break;
        }

        case 4993752304437055327: {
            /*
             * The check above is `if (first == "_OBJC_ME")`, checking if the
             * prefix is "_OBJC_METACLASS_$_".
             *
             * Ensure the max potential length of the symbol is at least 18
             * bytes.
             */

            if (max_length < 19) {
                return false;
            }

            /*
             * The check below is `if (second != "TACLASS_")`.
             */

            const uint64_t second = *(const uint64_t *)(symbol + 8);
            if (second != 6868925396587594068) {
                return false;
            }

            /*
             * The check below is `if (third != "$_")`.
             */

            const uint16_t third = *(const uint16_t *)(symbol + 16);
            if (third != 24356) {
                return false;
            }

            /*
             * We return the symbol with its preceding underscore.
             */

            const char *const real_symbol = symbol + 17;

            /*
             * Add one to real_symbol to avoid rechecking the underscore.
             */

            const uint32_t length =
                (uint32_t)strnlen(real_symbol + 1, max_length - 18) + 1;

            *symbol_out = real_symbol;
            *length_out = length;

            break;
        }

        case 7810191059381808942: {
            /*
             * The check above is `if (first == ".objc_cl")`, checking if the
             * prefix is ".objc_class_name".
             *
             * Ensure that the max potential length of this symbol is at least
             * 17 btyes.
             */

            if (max_length < 18) {
                return false;
            }

            /*
             * The check below is `if (second != "ass_name")`.
             */

            const uint64_t second = *(const uint64_t *)(symbol + 8);
            if (second != 7308604896967881569) {
                return false;
            }

            if (symbol[16] != '_') {
                return false;
            }

            /*
             * We return the symbol with its preceding underscore.
             */

            const char *const real_symbol = symbol + 16;

            /*
             * Add one to real_symbol to avoid rechecking the underscore.
             */

            const uint32_t length =
                (uint32_t)strnlen(real_symbol + 1, max_length - 17) + 1;

            *symbol_out = real_symbol;
            *length_out = length;

            break;
        }

        default:
            return false;
    }

    return true;
}

static inline
bool is_objc_ehtype_symbol(const char *__notnull const symbol,
                           const uint64_t first,
                           const uint64_t max_len,
                           const enum tbd_version version)
{
    /*
     * The ObjC eh-type group was introduced in tbd-version v3, with objc-eh
     * type symbols belonging to the normal-symbols group in previous versions.
     */

    if (version != TBD_VERSION_V3) {
        return false;
    }

    if (max_len < 16) {
        return false;
    }

    /*
     * The check below is `if (first != "_OBJC_EH")`.
     */

    if (first != 5207673286737153887) {
        return false;
    }

    /*
     * The check below is `if (second != "TYPE")`.
     */

    const uint32_t second = *(const uint32_t *)(symbol + 8);
    if (second != 1162893652) {
        return false;
    }

    /*
     * The check below is `if (third != "_$")`.
     */

    const uint16_t third = *(const uint16_t *)(symbol + 12);
    if (third != 9311) {
        return false;
    }

    if (symbol[14] != '_') {
        return false;
    }

    return true;
}

static inline
bool is_objc_ivar_symbol(const char *__notnull const symbol,
                         const uint64_t first)
{
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

    if (symbol[12] != '_') {
        return false;
    }

    return true;
}

static enum macho_file_parse_result
handle_symbol(struct tbd_create_info *__notnull const info_in,
              const uint64_t arch_bit,
              const uint32_t index,
              const uint32_t strsize,
              const char *__notnull const symbol_string,
              const uint16_t n_desc,
              const uint8_t n_type,
              const uint64_t options)
{
    /*
     * We can exit this function quickly if the symbol isn't external and no
     * flags for private-symbols were provided.
     */

    if (!(n_type & N_EXT)) {
        const uint64_t allow_priv_symbols_flags =
            O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS |
            O_TBD_PARSE_ALLOW_PRIVATE_OBJC_EHTYPE_SYMBOLS |
            O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS;

        if ((options & allow_priv_symbols_flags) == allow_priv_symbols_flags) {
            return E_MACHO_FILE_PARSE_OK;
        }
    }

    /*
     * We save space on the heap by storing only the part of the string we will
     * write-out later for the tbd-file.
     */

    const char *string = symbol_string;
    uint32_t max_len = strsize - index;

    enum tbd_export_type symbol_type = TBD_EXPORT_TYPE_NORMAL_SYMBOL;
    uint32_t length = 0;

    if (likely(!(n_desc & N_WEAK_DEF))) {
        /*
         * We can skip calls to is_objc_*_symbol if the symbol's max-length
         * disqualifies the symbol from being an objc symbol.
         */

        if (max_len > 13) {
            const enum tbd_version version = info_in->version;

            const uint64_t first = *(const uint64_t *)string;
            const char *const str = string;

            if (is_objc_class_symbol(str, first, max_len, &string, &length)) {
                if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS)) {
                    if (!(n_type & N_EXT)) {
                        return E_MACHO_FILE_PARSE_OK;
                    }
                }

                if (unlikely(length == 0)) {
                    return E_MACHO_FILE_PARSE_OK;
                }

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the class-name is to be removed.
                 */

                if (info_in->version == TBD_VERSION_V3) {
                    string += 1;
                    length -= 1;
                }

                symbol_type = TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL;
            } else if (is_objc_ivar_symbol(str, first)) {
                if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS)) {
                    if (!(n_type & N_EXT)) {
                        return E_MACHO_FILE_PARSE_OK;
                    }
                }

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the ivar-name is to be removed.
                 */

                if (info_in->version == TBD_VERSION_V3) {
                    string += 1;
                    max_len -= 1;
                }

                string += 12;

                symbol_type = TBD_EXPORT_TYPE_OBJC_IVAR_SYMBOL;
                length = (uint32_t)strnlen(string, max_len - 12);

                if (unlikely(length == 0)) {
                    return E_MACHO_FILE_PARSE_OK;
                }
            } else if (is_objc_ehtype_symbol(str, first, max_len, version)) {
                if (!(options & O_TBD_PARSE_ALLOW_PRIVATE_OBJC_EHTYPE_SYMBOLS))
                {
                    if (!(n_type & N_EXT)) {
                        return E_MACHO_FILE_PARSE_OK;
                    }
                }

                string += 15;
                length = (uint32_t)strnlen(string, max_len - 15);

                if (unlikely(length == 0)) {
                    return E_MACHO_FILE_PARSE_OK;
                }

                symbol_type = TBD_EXPORT_TYPE_OBJC_EHTYPE_SYMBOL;
            } else {
                if (!(n_type & N_EXT)) {
                    return E_MACHO_FILE_PARSE_OK;
                }

                length = (uint32_t)strnlen(string, max_len);
                if (unlikely(length == 0)) {
                    return E_MACHO_FILE_PARSE_OK;
                }
            }
        } else {
            if (!(n_type & N_EXT)) {
                return E_MACHO_FILE_PARSE_OK;
            }

            length = (uint32_t)strnlen(string, max_len);
            if (unlikely(length == 0)) {
                return E_MACHO_FILE_PARSE_OK;
            }
        }
    } else {
        if (!(n_type & N_EXT)) {
            return E_MACHO_FILE_PARSE_OK;
        }

        symbol_type = TBD_EXPORT_TYPE_WEAK_DEF_SYMBOL;
        length = (uint32_t)strnlen(symbol_string, max_len);

        if (unlikely(length == 0)) {
            return E_MACHO_FILE_PARSE_OK;
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
        export_info.archs = info_in->fields.archs;
        export_info.archs_count = info_in->fields.archs_count;
    }

    struct array *const exports = &info_in->fields.exports;
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
         * Avoid erroneously incrementing the archs-count in the rare case we
         * encounter the symbol multiple times in the same architecture.
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

    export_info.string = alloc_and_copy(export_info.string, export_info.length);
    if (unlikely(export_info.string == NULL)) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    const enum array_result add_export_info_result =
        array_add_item_with_cached_index_info(exports,
                                              sizeof(export_info),
                                              &export_info,
                                              &cached_info,
                                              NULL);

    if (unlikely(add_export_info_result != E_ARRAY_OK)) {
        free(export_info.string);
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_from_file(
    const struct macho_file_parse_symbols_args args,
    const int fd)
{
    if (unlikely(args.nsyms == 0)) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Ensure the string-table and symbol-table can be fully contained within
     * the mach-o file.
     */

    uint32_t symbol_table_size = sizeof(struct nlist);
    if (guard_overflow_mul(&symbol_table_size, args.nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint32_t absolute_symoff = (uint32_t)args.full_range.begin;
    if (guard_overflow_add(&absolute_symoff, args.symoff)) {
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
     * Validate that the symbol-table range is fully within the provided
     * available-range.
     */

    if (!range_contains_other(args.available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint32_t absolute_stroff = (uint32_t)args.full_range.begin;
    if (guard_overflow_add(&absolute_stroff, args.stroff)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    const uint64_t string_table_end = absolute_stroff + args.strsize;
    const struct range string_table_range = {
        .begin = absolute_stroff,
        .end = string_table_end
    };

    /*
     * Validate that the string-table range is fully within the given
     * available-range.
     */

    if (!range_contains_other(args.available_range, string_table_range)) {
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

    char *const string_table = malloc(args.strsize);
    if (string_table == NULL) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    if (read(fd, string_table, args.strsize) < 0) {
        free(symbol_table);
        free(string_table);

        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    const struct nlist *nlist = symbol_table;
    const struct nlist *const end = symbol_table + args.nsyms;

    if (args.is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol is either an indirect symbol, or that the
             * symbol's n_value points back to the __TEXT segment.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (unlikely(index >= args.strsize)) {
                continue;
            }

            const int16_t n_desc = swap_int16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(args.info_in,
                              args.arch_bit,
                              index,
                              args.strsize,
                              symbol_string,
                              (uint16_t)n_desc,
                              n_type,
                              args.tbd_options);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                free(symbol_table);
                free(string_table);

                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol is either an indirect symbol, or that the
             * symbol's n_value points back to the __TEXT segment.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (unlikely(index >= args.strsize)) {
                continue;
            }

            const int16_t n_desc = swap_int16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(args.info_in,
                              args.arch_bit,
                              index,
                              args.strsize,
                              symbol_string,
                              (uint16_t)n_desc,
                              n_type,
                              args.tbd_options);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
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
macho_file_parse_symbols_64_from_file(
    const struct macho_file_parse_symbols_args args,
    const int fd)
{
    if (unlikely(args.nsyms == 0)) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Ensure the string-table and symbol-table can be fully contained within
     * the mach-o file.
     */

    uint64_t symbol_table_size = sizeof(struct nlist_64);
    if (guard_overflow_mul(&symbol_table_size, args.nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint32_t absolute_symoff = (uint32_t)args.full_range.begin;
    if (guard_overflow_add(&absolute_symoff, args.symoff)) {
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
     * Validate that the symbol-table range is fully within the provided
     * available-range.
     */

    if (!range_contains_other(args.available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint32_t absolute_stroff = (uint32_t)args.full_range.begin;
    if (guard_overflow_add(&absolute_stroff, args.stroff)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const uint64_t string_table_end = absolute_stroff + args.strsize;
    const struct range string_table_range = {
        .begin = absolute_stroff,
        .end = string_table_end
    };

    /*
     * Validate that the string-table range is fully within the given
     * available-range.
     */

    if (!range_contains_other(args.available_range, string_table_range)) {
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

    char *const string_table = malloc(args.strsize);
    if (string_table == NULL) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    if (read(fd, string_table, args.strsize) < 0) {
        free(symbol_table);
        free(string_table);

        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    const struct nlist_64 *nlist = symbol_table;
    const struct nlist_64 *const end = symbol_table + args.nsyms;

    if (args.is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol is either an indirect symbol, or that the
             * symbol's n_value points back to the __TEXT segment.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (unlikely(index >= args.strsize)) {
                continue;
            }

            const uint16_t n_desc = swap_uint16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(args.info_in,
                              args.arch_bit,
                              index,
                              args.strsize,
                              symbol_string,
                              n_desc,
                              n_type,
                              args.tbd_options);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                free(symbol_table);
                free(string_table);

                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol is either an indirect symbol, or that the
             * symbol's n_value points back to the __TEXT segment.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (unlikely(index >= args.strsize)) {
                continue;
            }

            const uint16_t n_desc = nlist->n_desc;

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(args.info_in,
                              args.arch_bit,
                              index,
                              args.strsize,
                              symbol_string,
                              n_desc,
                              n_type,
                              args.tbd_options);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
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
macho_file_parse_symbols_from_map(
    const struct macho_file_parse_symbols_args args,
    const uint8_t *__notnull const map)
{
    if (unlikely(args.nsyms == 0)) {
        return E_MACHO_FILE_PARSE_OK;
    }

    const struct range string_table_range = {
        .begin = args.stroff,
        .end = args.stroff + args.strsize
    };

    if (!range_contains_other(args.available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table can be fully contained within
     * the mach-o map.
     */

    uint64_t symbol_table_size = sizeof(struct nlist);
    if (guard_overflow_mul(&symbol_table_size, args.nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t symbol_table_end = args.symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = args.symoff,
        .end = symbol_table_end
    };

    if (!range_contains_other(args.available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const char *const string_table = (const char *)(map + args.stroff);

    const struct nlist *nlist = (const struct nlist *)(map + args.symoff);
    const struct nlist *const end = nlist + args.nsyms;

    if (args.is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol is either an indirect symbol, or that the
             * symbol's n_value points back to the __TEXT segment.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (unlikely(index >= args.strsize)) {
                continue;
            }

            const int16_t n_desc = swap_int16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(args.info_in,
                              args.arch_bit,
                              index,
                              args.strsize,
                              symbol_string,
                              (uint16_t)n_desc,
                              n_type,
                              args.tbd_options);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol is either an indirect symbol, or that the
             * symbol's n_value points back to the __TEXT segment.
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
            if (unlikely(index >= args.strsize)) {
                continue;
            }

            const int16_t n_desc = nlist->n_desc;

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(args.info_in,
                              args.arch_bit,
                              index,
                              args.strsize,
                              symbol_string,
                              (uint16_t)n_desc,
                              n_type,
                              args.tbd_options);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                return handle_symbol_result;
            }
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symbols_64_from_map(
    const struct macho_file_parse_symbols_args args,
    const uint8_t *__notnull const map)
{
    if (unlikely(args.nsyms == 0)) {
        return E_MACHO_FILE_PARSE_OK;
    }

    const struct range string_table_range = {
        .begin = args.stroff,
        .end = args.stroff + args.strsize
    };

    if (!range_contains_other(args.available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table can be fully contained within
     * the mach-o map.
     */

    uint64_t symbol_table_size = sizeof(struct nlist_64);
    if (guard_overflow_mul(&symbol_table_size, args.nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t symbol_table_end = args.symoff;
    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = args.symoff,
        .end = symbol_table_end
    };

    if (!range_contains_other(args.available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const char *const string_table = (const char *)(map + args.stroff);

    const struct nlist_64 *nlist = (const struct nlist_64 *)(map + args.symoff);
    const struct nlist_64 *const end = nlist + args.nsyms;

    if (args.is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol is either an indirect symbol, or that the
             * symbol's n_value points back to the __TEXT segment.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (unlikely(index >= args.strsize)) {
                continue;
            }

            const uint16_t n_desc = swap_uint16(nlist->n_desc);

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(args.info_in,
                              args.arch_bit,
                              index,
                              args.strsize,
                              symbol_string,
                              n_desc,
                              n_type,
                              args.tbd_options);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that each symbol is either an indirect symbol, or that the
             * symbol's n_value points back to the __TEXT segment.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = n_type & N_TYPE;

            if (type != N_SECT) {
                if (type != N_INDR) {
                    continue;
                }
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (unlikely(index >= args.strsize)) {
                continue;
            }

            const uint16_t n_desc = nlist->n_desc;

            const char *const symbol_string = string_table + index;
            const enum macho_file_parse_result handle_symbol_result =
                handle_symbol(args.info_in,
                              args.arch_bit,
                              index,
                              args.strsize,
                              symbol_string,
                              n_desc,
                              n_type,
                              args.tbd_options);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                return handle_symbol_result;
            }
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}
