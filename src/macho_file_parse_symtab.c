//
//  src/macho_file_parse_symtab.c
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <fcntl.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mach-o/nlist.h"
#include "arch_info.h"
#include "copy.h"

#include "guard_overflow.h"
#include "likely.h"

#include "macho_file.h"
#include "macho_file_parse_symtab.h"
#include "our_io.h"

#include "range.h"
#include "swap.h"

#include "tbd.h"
#include "yaml.h"

static inline bool is_weak_symbol(const uint16_t n_desc) {
    return ((n_desc & (N_WEAK_DEF | N_WEAK_REF)) != 0);
}

static inline bool is_external_symbol(const uint8_t n_type) {
    const uint64_t check_flags = (N_EXT | N_STAB);
    return ((n_type & check_flags) == N_EXT);
}

static enum macho_file_parse_result
handle_symbol(struct tbd_create_info *__notnull const info_in,
              const uint64_t arch_bit,
              const uint32_t index,
              const uint32_t strsize,
              const char *__notnull string,
              const uint16_t n_desc,
              const uint8_t n_type,
              const bool is_undef,
              const uint64_t options)
{
    /*
     * We can exit this function quickly if the symbol isn't external and no
     * flags for private-symbols were provided.
     */

    const bool is_external = is_external_symbol(n_type);
    if (!is_external) {
        if (is_undef) {
            return E_MACHO_FILE_PARSE_OK;
        }

        const uint64_t allow_priv_symbols_flags =
            O_TBD_PARSE_ALLOW_PRIV_OBJC_CLASS_SYMS |
            O_TBD_PARSE_ALLOW_PRIV_OBJC_EHTYPE_SYMS |
            O_TBD_PARSE_ALLOW_PRIV_OBJC_IVAR_SYMS;

        if ((options & allow_priv_symbols_flags) == 0) {
            return E_MACHO_FILE_PARSE_OK;
        }
    }

    const uint32_t max_len = strsize - index;
    enum tbd_symbol_type predefined_type = TBD_SYMBOL_TYPE_NONE;

    if (is_weak_symbol(n_desc)) {
        predefined_type = TBD_SYMBOL_TYPE_WEAK_DEF;
    }

    const enum tbd_ci_add_symbol_result add_symbol_result =
        tbd_ci_add_symbol_with_info(info_in,
                                    string,
                                    max_len,
                                    arch_bit,
                                    predefined_type,
                                    is_external,
                                    is_undef,
                                    options);

    if (add_symbol_result != E_TBD_CI_ADD_SYMBOL_OK) {
        return E_MACHO_FILE_PARSE_CREATE_SYMBOLS_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static bool
should_parse_undef(const enum tbd_version version,
                   const uint64_t n_value,
                   const uint64_t tbd_options)
{
    if (tbd_options & O_TBD_PARSE_IGNORE_UNDEFINEDS) {
        return false;
    }

    if (version != TBD_VERSION_V1) {
        return false;
    }

    if (n_value != 0) {
        return false;
    }

    return true;
}

enum macho_file_parse_result
macho_file_parse_symtab_from_file(
    const struct macho_file_parse_symtab_args *__notnull const args,
    const int fd,
    const uint64_t base_offset)
{
    const uint64_t nsyms = args->nsyms;
    if (unlikely(nsyms == 0)) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Ensure the string-table and symbol-table can be fully contained within
     * the mach-o file.
     */

    uint32_t symbol_table_size = sizeof(struct nlist);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t absolute_symoff = base_offset;
    if (guard_overflow_add(&absolute_symoff, args->symoff)) {
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

    const struct range available_range = args->available_range;
    if (!range_contains_other(available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t absolute_stroff = base_offset;
    if (guard_overflow_add(&absolute_stroff, args->stroff)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    const uint32_t strsize = args->strsize;

    uint64_t string_table_end = absolute_stroff;
    if (guard_overflow_add(&string_table_end, strsize)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    const struct range string_table_range = {
        .begin = absolute_stroff,
        .end = string_table_end
    };

    /*
     * Validate that the string-table range is fully within the given
     * available-range.
     */

    if (!range_contains_other(available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    if (our_lseek(fd, absolute_symoff, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    struct nlist *const symbol_table = malloc(symbol_table_size);
    if (symbol_table == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (our_read(fd, symbol_table, symbol_table_size) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    if (our_lseek(fd, absolute_stroff, SEEK_SET) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    char *const string_table = malloc(strsize);
    if (string_table == NULL) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    if (our_read(fd, string_table, strsize) < 0) {
        free(symbol_table);
        free(string_table);

        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    struct tbd_create_info *const info_in = args->info_in;
    const enum tbd_version version = info_in->version;

    const uint64_t arch_bit = args->arch_bit;
    const uint64_t tbd_opt = args->tbd_options;

    const struct nlist *nlist = symbol_table;
    const struct nlist *const end = symbol_table + nsyms;

    if (args->is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that either each symbol is an indirect symbol, each
             * symbol's n_value points back to the __TEXT segment, or that each
             * symbol is an undefined symbol with a n_value of 0.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = (n_type & N_TYPE);

            bool is_undef = false;
            switch (type) {
                case N_SECT:
                case N_INDR:
                    if (tbd_opt & O_TBD_PARSE_IGNORE_EXPORTS) {
                        continue;
                    }

                    break;

                case N_UNDF: {
                    const uint64_t n_value = nlist->n_value;
                    if (!should_parse_undef(version, n_value, tbd_opt)) {
                        continue;
                    }

                    is_undef = true;
                    break;
                }

                default:
                    continue;
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (unlikely(index >= strsize)) {
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
                              is_undef,
                              tbd_opt);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                free(symbol_table);
                free(string_table);

                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that either each symbol is an indirect symbol, each
             * symbol's n_value points back to the __TEXT segment, or that each
             * symbol is an undefined symbol with a n_value of 0.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = (n_type & N_TYPE);

            bool is_undef = false;
            switch (type) {
                case N_SECT:
                case N_INDR:
                    if (tbd_opt & O_TBD_PARSE_IGNORE_EXPORTS) {
                        continue;
                    }

                    break;

                case N_UNDF: {
                    const uint64_t n_value = nlist->n_value;
                    if (!should_parse_undef(version, n_value, tbd_opt)) {
                        continue;
                    }

                    is_undef = true;
                    break;
                }

                default:
                    continue;
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (unlikely(index >= strsize)) {
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
                              is_undef,
                              tbd_opt);

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
macho_file_parse_symtab_64_from_file(
    const struct macho_file_parse_symtab_args *__notnull const args,
    const int fd,
    const uint64_t base_offset)
{
    const uint64_t nsyms = args->nsyms;
    if (unlikely(nsyms == 0)) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Ensure the string-table and symbol-table can be fully contained within
     * the mach-o file.
     */

    uint64_t symbol_table_size = sizeof(struct nlist_64);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t absolute_symoff = base_offset;
    if (guard_overflow_add(&absolute_symoff, args->symoff)) {
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

    const struct range available_range = args->available_range;
    if (!range_contains_other(available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    uint64_t absolute_stroff = base_offset;
    if (guard_overflow_add(&absolute_stroff, args->stroff)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    const uint32_t strsize = args->strsize;

    uint64_t string_table_end = absolute_stroff;
    if (guard_overflow_add(&string_table_end, strsize)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    const struct range string_table_range = {
        .begin = absolute_stroff,
        .end = string_table_end
    };

    /*
     * Validate that the string-table range is fully within the given
     * available-range.
     */

    if (!range_contains_other(available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    if (our_lseek(fd, absolute_symoff, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    struct nlist_64 *const symbol_table = malloc(symbol_table_size);
    if (symbol_table == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (our_read(fd, symbol_table, symbol_table_size) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    if (our_lseek(fd, absolute_stroff, SEEK_SET) < 0) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    char *const string_table = malloc(strsize);
    if (string_table == NULL) {
        free(symbol_table);
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    if (our_read(fd, string_table, strsize) < 0) {
        free(symbol_table);
        free(string_table);

        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    struct tbd_create_info *const info_in = args->info_in;
    const enum tbd_version version = info_in->version;

    const uint64_t arch_bit = args->arch_bit;
    const uint64_t tbd_opt = args->tbd_options;

    const struct nlist_64 *nlist = symbol_table;
    const struct nlist_64 *const end = symbol_table + nsyms;

    if (args->is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that either each symbol is an indirect symbol, each
             * symbol's n_value points back to the __TEXT segment, or that each
             * symbol is an undefined symbol with a n_value of 0.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = (n_type & N_TYPE);

            bool is_undef = false;
            switch (type) {
                case N_SECT:
                case N_INDR:
                    if (tbd_opt & O_TBD_PARSE_IGNORE_EXPORTS) {
                        continue;
                    }

                    break;

                case N_UNDF: {
                    const uint64_t n_value = nlist->n_value;
                    if (!should_parse_undef(version, n_value, tbd_opt)) {
                        continue;
                    }

                    is_undef = true;
                    break;
                }

                default:
                    continue;
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (unlikely(index >= strsize)) {
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
                              is_undef,
                              tbd_opt);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                free(symbol_table);
                free(string_table);

                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that either each symbol is an indirect symbol, each
             * symbol's n_value points back to the __TEXT segment, or that each
             * symbol is an undefined symbol with a n_value of 0.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = (n_type & N_TYPE);

            bool is_undef = false;
            switch (type) {
                case N_SECT:
                case N_INDR:
                    if (tbd_opt & O_TBD_PARSE_IGNORE_EXPORTS) {
                        continue;
                    }

                    break;

                case N_UNDF: {
                    const uint64_t n_value = nlist->n_value;
                    if (!should_parse_undef(version, n_value, tbd_opt)) {
                        continue;
                    }

                    is_undef = true;
                    break;
                }

                default:
                    continue;
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (unlikely(index >= strsize)) {
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
                              is_undef,
                              tbd_opt);

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
macho_file_parse_symtab_from_map(
    const struct macho_file_parse_symtab_args *__notnull const args,
    const uint8_t *__notnull const map)
{
    const uint64_t nsyms = args->nsyms;
    if (unlikely(nsyms == 0)) {
        return E_MACHO_FILE_PARSE_OK;
    }

    const uint32_t strsize = args->strsize;
    const struct range string_table_range = {
        .begin = args->stroff,
        .end = string_table_range.begin + strsize
    };

    const struct range available_range = args->available_range;
    if (!range_contains_other(available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table can be fully contained within
     * the mach-o map.
     */

    uint64_t symbol_table_size = sizeof(struct nlist);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const uint64_t symoff = args->symoff;
    uint64_t symbol_table_end = symoff;

    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = symoff,
        .end = symbol_table_end
    };

    if (!range_contains_other(available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    struct tbd_create_info *const info_in = args->info_in;
    const enum tbd_version version = info_in->version;

    const uint64_t arch_bit = args->arch_bit;
    const uint64_t tbd_opt = args->tbd_options;

    const char *const string_table =
        (const char *)(map + string_table_range.begin);

    const struct nlist *nlist = (const struct nlist *)(map + symoff);
    const struct nlist *const end = nlist + nsyms;

    if (args->is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that either each symbol is an indirect symbol, each
             * symbol's n_value points back to the __TEXT segment, or that each
             * symbol is an undefined symbol with a n_value of 0.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = (n_type & N_TYPE);

            bool is_undef = false;
            switch (type) {
                case N_SECT:
                case N_INDR:
                    if (tbd_opt & O_TBD_PARSE_IGNORE_EXPORTS) {
                        continue;
                    }

                    break;

                case N_UNDF: {
                    const uint64_t n_value = nlist->n_value;
                    if (!should_parse_undef(version, n_value, tbd_opt)) {
                        continue;
                    }

                    is_undef = true;
                    break;
                }

                default:
                    continue;
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (unlikely(index >= strsize)) {
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
                              is_undef,
                              tbd_opt);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that either each symbol is an indirect symbol, each
             * symbol's n_value points back to the __TEXT segment, or that each
             * symbol is an undefined symbol with a n_value of 0.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = (n_type & N_TYPE);

            bool is_undef = false;
            switch (type) {
                case N_SECT:
                case N_INDR:
                    if (tbd_opt & O_TBD_PARSE_IGNORE_EXPORTS) {
                        continue;
                    }

                    break;

                case N_UNDF: {
                    const uint64_t n_value = nlist->n_value;
                    if (!should_parse_undef(version, n_value, tbd_opt)) {
                        continue;
                    }

                    is_undef = true;
                    break;
                }

                default:
                    continue;
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (unlikely(index >= strsize)) {
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
                              is_undef,
                              tbd_opt);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                return handle_symbol_result;
            }
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_symtab_64_from_map(
    const struct macho_file_parse_symtab_args *__notnull const args,
    const uint8_t *__notnull const map)
{
    const uint64_t nsyms = args->nsyms;
    if (unlikely(nsyms == 0)) {
        return E_MACHO_FILE_PARSE_OK;
    }

    const uint32_t strsize = args->strsize;
    const struct range string_table_range = {
        .begin = args->stroff,
        .end = string_table_range.begin + strsize
    };

    const struct range available_range = args->available_range;
    if (!range_contains_other(available_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_STRING_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table can be fully contained within
     * the mach-o map.
     */

    uint64_t symbol_table_size = sizeof(struct nlist_64);
    if (guard_overflow_mul(&symbol_table_size, nsyms)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const uint64_t symoff = args->symoff;
    uint64_t symbol_table_end = symoff;

    if (guard_overflow_add(&symbol_table_end, symbol_table_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    const struct range symbol_table_range = {
        .begin = symoff,
        .end = symbol_table_end
    };

    if (!range_contains_other(available_range, symbol_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    /*
     * Ensure the string-table and symbol-table don't overlap.
     */

    if (ranges_overlap(symbol_table_range, string_table_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
    }

    struct tbd_create_info *const info_in = args->info_in;
    const enum tbd_version version = info_in->version;

    const uint64_t arch_bit = args->arch_bit;
    const uint64_t tbd_opt = args->tbd_options;

    const char *const string_table =
        (const char *)(map + string_table_range.begin);

    const struct nlist_64 *nlist = (const struct nlist_64 *)(map + symoff);
    const struct nlist_64 *const end = nlist + nsyms;

    if (args->is_big_endian) {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that either each symbol is an indirect symbol, each
             * symbol's n_value points back to the __TEXT segment, or that each
             * symbol is an undefined symbol with a n_value of 0.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = (n_type & N_TYPE);

            bool is_undef = false;
            switch (type) {
                case N_SECT:
                case N_INDR:
                    if (tbd_opt & O_TBD_PARSE_IGNORE_EXPORTS) {
                        continue;
                    }

                    break;

                case N_UNDF: {
                    const uint64_t n_value = nlist->n_value;
                    if (!should_parse_undef(version, n_value, tbd_opt)) {
                        continue;
                    }

                    is_undef = true;
                    break;
                }

                default:
                    continue;
            }

            /*
             * For the sake of leniency, we avoid erroring out for symbols with
             * invalid string-table references.
             */

            const uint32_t index = swap_uint32(nlist->n_un.n_strx);
            if (unlikely(index >= strsize)) {
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
                              is_undef,
                              tbd_opt);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                return handle_symbol_result;
            }
        }
    } else {
        for (; nlist != end; nlist++) {
            /*
             * Ensure that either each symbol is an indirect symbol, each
             * symbol's n_value points back to the __TEXT segment, or that each
             * symbol is an undefined symbol with a n_value of 0.
             */

            const uint8_t n_type = nlist->n_type;
            const uint8_t type = (n_type & N_TYPE);

            bool is_undef = false;
            switch (type) {
                case N_SECT:
                case N_INDR:
                    if (tbd_opt & O_TBD_PARSE_IGNORE_EXPORTS) {
                        continue;
                    }

                    break;

                case N_UNDF: {
                    const uint64_t n_value = nlist->n_value;
                    if (!should_parse_undef(version, n_value, tbd_opt)) {
                        continue;
                    }

                    is_undef = true;
                    break;
                }

                default:
                    continue;
            }

            /*
             * For leniency reasons, ignore invalid symbol-references instead of
             * erroring out.
             */

            const uint32_t index = nlist->n_un.n_strx;
            if (unlikely(index >= strsize)) {
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
                              is_undef,
                              tbd_opt);

            if (unlikely(handle_symbol_result != E_MACHO_FILE_PARSE_OK)) {
                return handle_symbol_result;
            }
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}
