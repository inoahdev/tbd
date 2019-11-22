//
//  src/tbd.c
//  tbd
//
//  Created by inoahdev on 11/22/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "copy.h"
#include "likely.h"
#include "tbd.h"
#include "tbd_write.h"
#include "yaml.h"

/*
 * We compare strings by using the largest possible byte size when reading from
 * memory to maximize our performance.
 */

static uint64_t
is_objc_class_symbol(const char *__notnull const symbol,
                     const uint64_t first,
                     const uint64_t max_length)
{
    /*
     * Ensure the max potential length of the symbol is at least 14 bytes.
     */

    if (max_length < 15) {
        return 0;
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
                return 0;
            }

            /*
             * The check below is `if (third != "$_")`.
             */

            const uint16_t third = *(const uint16_t *)(symbol + 12);
            if (third != 24356) {
                return 0;
            }

            /*
             * We return the underscore.
             */

            return 13;
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
                return 0;
            }

            /*
             * The check below is `if (second != "TACLASS_")`.
             */

            const uint64_t second = *(const uint64_t *)(symbol + 8);
            if (second != 6868925396587594068) {
                return 0;
            }

            /*
             * The check below is `if (third != "$_")`.
             */

            const uint16_t third = *(const uint16_t *)(symbol + 16);
            if (third != 24356) {
                return 0;
            }

            /*
             * We return the underscore.
             */

            return 17;
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
                return 0;
            }

            /*
             * The check below is `if (second != "ass_name")`.
             */

            const uint64_t second = *(const uint64_t *)(symbol + 8);
            if (second != 7308604896967881569) {
                return 0;
            }

            if (symbol[16] != '_') {
                return 0;
            }

            /*
             * We return the underscore.
             */

            return 16;
        }

        default:
            return 0;
    }
}

static uint64_t
is_objc_ehtype_sym(const char *__notnull const symbol,
                   const uint64_t first,
                   const uint64_t max_len,
                   const enum tbd_version version)
{
    /*
     * The ObjC eh-type group was introduced in tbd-version v3, with objc-eh
     * type symbols belonging to the normal-symbols group in previous versions.
     */

    if (version != TBD_VERSION_V3) {
        return 0;
    }

    if (max_len < 16) {
        return 0;
    }

    /*
     * The check below is `if (first != "_OBJC_EH")`.
     */

    if (first != 5207673286737153887) {
        return 0;
    }

    /*
     * The check below is `if (second != "TYPE")`.
     */

    const uint32_t second = *(const uint32_t *)(symbol + 8);
    if (second != 1162893652) {
        return 0;
    }

    /*
     * The check below is `if (third != "_$")`.
     */

    const uint16_t third = *(const uint16_t *)(symbol + 12);
    if (third != 9311) {
        return 0;
    }

    if (symbol[14] != '_') {
        return 0;
    }

    return 15;
}

static uint64_t
is_objc_ivar_symbol(const char *__notnull const symbol, const uint64_t first) {
    /*
     * The check here is `if (first == "_OBJC_IV")`.
     */

    if (first != 6217605503174987615) {
        return 0;
    }

    /*
     * The check here is `if (second == "AR_$")`.
     */

    const uint32_t second = *(const uint32_t *)(symbol + 8);
    if (second != 610226753) {
        return 0;
    }

    if (symbol[12] != '_') {
        return 0;
    }

    return 12;
}

enum tbd_ci_add_symbol_result
tbd_ci_add_symbol_with_type(struct tbd_create_info *__notnull const info_in,
                            const char *__notnull const string,
                            const uint64_t length,
                            const uint64_t arch_bit,
                            const enum tbd_symbol_type type,
                            const bool is_undef,
                            const uint64_t options)
{
    struct tbd_symbol_info symbol_info = {
        .length = length,
        .string = (char *)string,
        .type = type,
    };

    struct array *list = &info_in->fields.exports;
    if (is_undef) {
        list = &info_in->fields.undefineds;
    }

    struct array_cached_index_info cached_info = {};
    struct tbd_symbol_info *const existing_info =
        array_find_item_in_sorted(list,
                                  sizeof(symbol_info),
                                  &symbol_info,
                                  tbd_symbol_info_no_archs_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        if (options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS) {
            return E_TBD_CI_ADD_SYMBOL_OK;
        }

        /*
         * Avoid erroneously incrementing the archs-count in the rare case we
         * encounter the symbol multiple times in the same architecture.
         */

        const uint64_t archs = existing_info->archs;
        if (archs & arch_bit) {
            return E_TBD_CI_ADD_SYMBOL_OK;
        }

        existing_info->archs = archs | arch_bit;
        existing_info->archs_count += 1;

        return E_TBD_CI_ADD_SYMBOL_OK;
    }

    symbol_info.string = alloc_and_copy(symbol_info.string, symbol_info.length);
    if (unlikely(symbol_info.string == NULL)) {
        return E_TBD_CI_ADD_SYMBOL_ALLOC_FAIL;
    }

    const bool needs_quotes = yaml_check_c_str(string, length);
    if (needs_quotes) {
        symbol_info.flags |= F_TBD_SYMBOL_INFO_STRING_NEEDS_QUOTES;
    }

    if (options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS) {
        symbol_info.archs = info_in->fields.archs;
        symbol_info.archs_count = info_in->fields.archs_count;
    } else {
        symbol_info.archs = arch_bit;
        symbol_info.archs_count = 1;
    }

    const enum array_result add_export_info_result =
        array_add_item_with_cached_index_info(list,
                                              sizeof(symbol_info),
                                              &symbol_info,
                                              &cached_info,
                                              NULL);

    if (unlikely(add_export_info_result != E_ARRAY_OK)) {
        free(symbol_info.string);
        return E_TBD_CI_ADD_SYMBOL_ARRAY_FAIL;
    }

    return E_TBD_CI_ADD_SYMBOL_OK;
}

enum tbd_ci_add_symbol_result
tbd_ci_add_symbol_with_info(struct tbd_create_info *__notnull const info_in,
                            const char *__notnull string,
                            uint64_t lnmax,
                            const uint64_t arch_bit,
                            const enum tbd_symbol_type predefined_type,
                            const bool is_external,
                            const bool is_undef,
                            const uint64_t options)
{
    uint64_t length = 0;
    enum tbd_symbol_type type = TBD_SYMBOL_TYPE_NORMAL;

    /*
     * We can skip calls to is_objc_*_symbol if the symbol's max-length
     * disqualifies the symbol from being an objc symbol.
     */

    if (likely(predefined_type == TBD_SYMBOL_TYPE_NONE)) {
        if (lnmax > 13) {
            const uint64_t first = *(const uint64_t *)string;
            const char *const str = string;
            uint64_t offset = 0;
            const enum tbd_version vers = info_in->version;

            if ((offset = is_objc_class_symbol(str, first, lnmax)) != 0) {
                if (!is_external) {
                    if (!(options & O_TBD_PARSE_ALLOW_PRIV_OBJC_CLASS_SYMS)) {
                        return E_TBD_CI_ADD_SYMBOL_OK;
                    }
                }

                string += offset;

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the class-name is to be removed.
                 */

                if (vers == TBD_VERSION_V3) {
                    string += 1;
                    lnmax -= 1;
                }

                length = strnlen(string, lnmax - offset);
                if (likely(length != 0)) {
                    type = TBD_SYMBOL_TYPE_OBJC_CLASS;
                }
            } else if ((offset = is_objc_ivar_symbol(str, first)) != 0) {
                if (!is_external) {
                    if (!(options & O_TBD_PARSE_ALLOW_PRIV_OBJC_IVAR_SYMS)) {
                        return E_TBD_CI_ADD_SYMBOL_OK;
                    }
                }

                string += offset;

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the ivar-name is to be removed.
                 */

                if (vers == TBD_VERSION_V3) {
                    string += 1;
                    lnmax -= 1;
                }

                length = strnlen(string, lnmax - offset);
                if (likely(length != 0)) {
                    type = TBD_SYMBOL_TYPE_OBJC_IVAR;
                }
            } else if ((offset = is_objc_ehtype_sym(str, first, lnmax, vers))) {
                string += offset;
                length = strnlen(string, lnmax - offset);

                if (likely(length != 0)) {
                    type = TBD_SYMBOL_TYPE_OBJC_EHTYPE;
                }
            } else {
                if (!is_external) {
                    return E_TBD_CI_ADD_SYMBOL_OK;
                }

                length = (uint32_t)strnlen(string, lnmax);
                if (unlikely(length == 0)) {
                    return E_TBD_CI_ADD_SYMBOL_OK;
                }
            }
        } else {
            if (!is_external) {
                return E_TBD_CI_ADD_SYMBOL_OK;
            }

            length = (uint32_t)strnlen(string, lnmax);
            if (unlikely(length == 0)) {
                return E_TBD_CI_ADD_SYMBOL_OK;
            }
        }
    } else {
        if (!is_external) {
            return E_TBD_CI_ADD_SYMBOL_OK;
        }

        length = (uint32_t)strnlen(string, lnmax);
        if (unlikely(length == 0)) {
            return E_TBD_CI_ADD_SYMBOL_OK;
        }

        type = predefined_type;
    }

    const enum tbd_ci_add_symbol_result add_symbol_result =
        tbd_ci_add_symbol_with_type(info_in,
                                    string,
                                    length,
                                    arch_bit,
                                    type,
                                    is_undef,
                                    options);

    if (add_symbol_result != E_TBD_CI_ADD_SYMBOL_OK) {
        return add_symbol_result;
    }

    return E_TBD_CI_ADD_SYMBOL_OK;
}

enum tbd_ci_add_symbol_result
tbd_ci_add_symbol_with_info_and_len(
    struct tbd_create_info *__notnull const info_in,
    const char *__notnull string,
    uint64_t len,
    const uint64_t arch_bit,
    const enum tbd_symbol_type predefined_type,
    const bool is_external,
    const bool is_undef,
    const uint64_t options)
{
    enum tbd_symbol_type type = TBD_SYMBOL_TYPE_NORMAL;

    /*
     * We can skip calls to is_objc_*_symbol if the symbol is too short to be an
     * objc symbol.
     */

    if (predefined_type == TBD_SYMBOL_TYPE_NONE) {
        if (len > 13) {
            const uint64_t first = *(const uint64_t *)string;
            const char *const str = string;
            uint64_t offset = 0;
            const enum tbd_version vers = info_in->version;

            if ((offset = is_objc_class_symbol(str, first, len)) != 0) {
                if (!is_external) {
                    if (!(options & O_TBD_PARSE_ALLOW_PRIV_OBJC_CLASS_SYMS)) {
                        return E_TBD_CI_ADD_SYMBOL_OK;
                    }
                }

                string += offset;
                len -= offset;

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the class-name is to be removed.
                 */

                if (vers == TBD_VERSION_V3) {
                    string += 1;
                    len -= 1;
                }

                if (unlikely(len == 0)) {
                    return E_TBD_CI_ADD_SYMBOL_OK;
                }

                type = TBD_SYMBOL_TYPE_OBJC_CLASS;
            } else if ((offset = is_objc_ivar_symbol(str, first)) != 0) {
                if (!is_external) {
                    if (!(options & O_TBD_PARSE_ALLOW_PRIV_OBJC_IVAR_SYMS)) {
                        return E_TBD_CI_ADD_SYMBOL_OK;
                    }
                }

                string += offset;
                len -= offset;

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the ivar-name is to be removed.
                 */

                if (vers == TBD_VERSION_V3) {
                    string += 1;
                    len -= 1;
                }

                if (unlikely(len == 0)) {
                    return E_TBD_CI_ADD_SYMBOL_OK;
                }

                type = TBD_SYMBOL_TYPE_OBJC_IVAR;
            } else if ((offset = is_objc_ehtype_sym(str, first, len, vers))) {
                if (!is_external) {
                    if (!(options & O_TBD_PARSE_ALLOW_PRIV_OBJC_EHTYPE_SYMS)) {
                        return E_TBD_CI_ADD_SYMBOL_OK;
                    }
                }

                string += offset;
                len -= offset;

                if (unlikely(len == 0)) {
                    return E_TBD_CI_ADD_SYMBOL_OK;
                }

                type = TBD_SYMBOL_TYPE_OBJC_EHTYPE;
            }
        } else {
            if (!is_external) {
                return E_TBD_CI_ADD_SYMBOL_OK;
            }
        }
    } else {
        if (!is_external) {
            return E_TBD_CI_ADD_SYMBOL_OK;
        }

        type = predefined_type;
    }

    const enum tbd_ci_add_symbol_result add_symbol_result =
        tbd_ci_add_symbol_with_type(info_in,
                                    string,
                                    len,
                                    arch_bit,
                                    type,
                                    is_undef,
                                    options);

    if (add_symbol_result != E_TBD_CI_ADD_SYMBOL_OK) {
        return add_symbol_result;
    }

    return E_TBD_CI_ADD_SYMBOL_OK;
}

int
tbd_symbol_info_comparator(const void *__notnull const array_item,
                           const void *__notnull const item)
{
    const struct tbd_symbol_info *const array_info =
        (const struct tbd_symbol_info *)array_item;

    const struct tbd_symbol_info *const info =
        (const struct tbd_symbol_info *)item;

    const int array_archs_count = array_info->archs_count;
    const int archs_count = info->archs_count;

    if (array_archs_count != archs_count) {
        return (array_archs_count - archs_count);
    }

    const uint64_t array_archs = array_info->archs;
    const uint64_t archs = info->archs;

    if (array_archs > archs) {
        return 1;
    } else if (array_archs < archs) {
        return -1;
    }

    const enum tbd_symbol_type array_type = array_info->type;
    const enum tbd_symbol_type type = info->type;

    if (array_type != type) {
        return (int)(array_type - type);
    }

    const uint64_t array_length = array_info->length;
    const uint64_t length = info->length;

    const char *const array_string = array_info->string;
    const char *const string = info->string;

    /*
     * We try to avoid iterating and comparing over the whole string, so we
     * could check to ensure their lengths match up.
     *
     * However, we don't want to symbols to ever be organized by their length,
     * which would be the case if `(array_length - length)` was returned.
     *
     * So instead, we use separate memcmp() calls for when array_length and
     * length don't match, depending on which is bigger.
     *
     * This stops us from having to use strcmp(), which would be the case since
     * the lengths don't match.
     *
     * Add one to also compare the null-terminator.
     */

    if (array_length > length) {
        return memcmp(array_string, string, length + 1);
    } else {
        return memcmp(array_string, string, array_length + 1);
    }
}

/*
 * Have the symbols array be sorted first into groups of matching arch-lists.
 *
 * The arch-lists themselves are sorted to where arch-lists with more archs are
 * "greater", than those without. Arch-lists with the same number of archs are
 * just compared numberically.
 *
 * Within each arch-list group, the symbols are then organized by their types.
 * Within each type group, the symbols are then finally organized
 * alphabetically.
 *
 * This is done to make the creation of export-groups later on easier, as no the
 * symbols are already organized by their arch-lists, and then their types,
 * all alphabetically.
 */

int
tbd_symbol_info_no_archs_comparator(const void *__notnull const array_item,
                                    const void *__notnull const item)
{
    const struct tbd_symbol_info *const array_info =
        (const struct tbd_symbol_info *)array_item;

    const struct tbd_symbol_info *const info =
        (const struct tbd_symbol_info *)item;

    const enum tbd_symbol_type array_type = array_info->type;
    const enum tbd_symbol_type type = info->type;

    if (array_type != type) {
        return (int)(array_type - type);
    }

    const uint64_t array_length = array_info->length;
    const uint64_t length = info->length;

    const char *const array_string = array_info->string;
    const char *const string = info->string;

    /*
     * We try to avoid iterating and comparing over the whole string, so we
     * could check to ensure their lengths match up.
     *
     * However, we don't want to symbols to ever be organized by their length,
     * which would be the case if `(array_length - length)` was returned.
     *
     * So instead, we use separate memcmp() calls for when array_length and
     * length don't match, depending on which is bigger.
     *
     * This stops us from having to use strcmp(), which would be the case since
     * the lengths don't match.
     *
     * Add one to also compare the null-terminator.
     */

    if (array_length > length) {
        return memcmp(array_string, string, length + 1);
    } else {
        return memcmp(array_string, string, array_length + 1);
    }
}

int
tbd_uuid_info_is_unique_comparator(const void *__notnull const array_item,
                                   const void *__notnull const item)
{
    const struct tbd_uuid_info *const array_uuid_info =
        (const struct tbd_uuid_info *)array_item;

    const struct tbd_uuid_info *const uuid_info =
        (const struct tbd_uuid_info *)item;

    const uint8_t *const array_uuid = array_uuid_info->uuid;
    const uint8_t *const uuid = uuid_info->uuid;

    return memcmp(array_uuid, uuid, 16);
}

int
tbd_uuid_info_comparator(const void *__notnull const array_item,
                         const void *__notnull const item)
{
    const struct tbd_uuid_info *const array_uuid_info =
        (const struct tbd_uuid_info *)array_item;

    const struct tbd_uuid_info *const uuid_info =
        (const struct tbd_uuid_info *)item;

    const struct arch_info *const array_arch_info = array_uuid_info->arch;
    const struct arch_info *const arch_info = uuid_info->arch;

    if (array_arch_info > arch_info) {
        return 1;
    } else if (array_arch_info < arch_info) {
        return -1;
    }

    return 0;
}

enum tbd_create_result
tbd_create_with_info(const struct tbd_create_info *__notnull const info,
                     FILE *__notnull const file,
                     const uint64_t options)
{
    const enum tbd_version version = info->version;
    if (tbd_write_magic(file, version)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    const uint64_t archs = info->fields.archs;
    const int archs_count = info->fields.archs_count;

    if (tbd_write_archs_for_header(file, archs, archs_count)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    if (!(options & O_TBD_CREATE_IGNORE_UUIDS)) {
        if (version != TBD_VERSION_V1) {
            if (tbd_write_uuids(file, &info->fields.uuids)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    }

    if (tbd_write_platform(file, info->fields.platform)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    if (version != TBD_VERSION_V1) {
        if (!(options & O_TBD_CREATE_IGNORE_FLAGS)) {
            if (tbd_write_flags(file, info->fields.flags)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    }

    if (tbd_write_install_name(file, info)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    if (!(options & O_TBD_CREATE_IGNORE_CURRENT_VERSION)) {
        if (tbd_write_current_version(file, info->fields.current_version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (!(options & O_TBD_CREATE_IGNORE_COMPATIBILITY_VERSION)) {
        const uint32_t compatibility_version =
            info->fields.compatibility_version;

        if (tbd_write_compatibility_version(file, compatibility_version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (version != TBD_VERSION_V1) {
        if (!(options & O_TBD_CREATE_IGNORE_SWIFT_VERSION)) {
            const uint32_t swift_version = info->fields.swift_version;
            if (tbd_write_swift_version(file, version, swift_version)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }

        if (!(options & O_TBD_CREATE_IGNORE_OBJC_CONSTRAINT)) {
            const enum tbd_objc_constraint objc_constraint =
                info->fields.objc_constraint;

            if (tbd_write_objc_constraint(file, objc_constraint)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }

        if (!(options & O_TBD_CREATE_IGNORE_PARENT_UMBRELLA)) {
            if (tbd_write_parent_umbrella(file, info)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    }

    if (!(options & O_TBD_CREATE_IGNORE_EXPORTS)) {
        const struct array *const exports = &info->fields.exports;
        if (info->flags & F_TBD_CREATE_INFO_EXPORTS_HAVE_FULL_ARCHS) {
            if (tbd_write_exports_with_full_archs(info, exports, file)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        } else {
            if (tbd_write_exports(file, exports, version)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    }

    if (version != TBD_VERSION_V1) {
        if (!(options & O_TBD_CREATE_IGNORE_UNDEFINEDS)) {
            const struct array *const list = &info->fields.undefineds;
            if (info->flags & F_TBD_CREATE_INFO_UNDEFINEDS_HAVE_FULL_ARCHS) {
                if (tbd_write_undefineds_with_full_archs(info, list, file)) {
                    return E_TBD_CREATE_WRITE_FAIL;
                }
            } else {
                if (tbd_write_undefineds(file, list, version)) {
                    return E_TBD_CREATE_WRITE_FAIL;
                }
            }
        }
    }

    if (!(options & O_TBD_CREATE_IGNORE_FOOTER)) {
        if (tbd_write_footer(file)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    return E_TBD_CREATE_OK;
}

static void clear_symbols_array(struct array *__notnull const list) {
    struct tbd_symbol_info *info = list->data;
    const struct tbd_symbol_info *const end = list->data_end;

    for (; info != end; info++) {
        free(info->string);
    }

    array_clear(list);
}

void
tbd_create_info_clear_fields_and_create_from(
    struct tbd_create_info *__notnull const dst,
    const struct tbd_create_info *__notnull const src)
{
    const uint64_t flags = dst->flags;
    if (flags & F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED) {
        free((char *)dst->fields.install_name);
    }

    if (flags & F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED) {
        free((char *)dst->fields.parent_umbrella);
    }

    clear_symbols_array(&dst->fields.exports);
    clear_symbols_array(&dst->fields.undefineds);
    array_clear(&dst->fields.uuids);

    const struct array exports = dst->fields.exports;
    const struct array undefineds = dst->fields.undefineds;
    const struct array uuids = dst->fields.uuids;

    memcpy(&dst->fields, &src->fields, sizeof(dst->fields));
    dst->flags = src->flags;

    dst->fields.exports = exports;
    dst->fields.undefineds = undefineds;
    dst->fields.uuids = uuids;
}

static void destroy_symbols_array(struct array *__notnull const list) {
    struct tbd_symbol_info *info = list->data;
    const struct tbd_symbol_info *const end = list->data_end;

    for (; info != end; info++) {
        free(info->string);
    }

    array_destroy(list);
}

void tbd_create_info_destroy(struct tbd_create_info *__notnull const info) {
    const uint64_t flags = info->flags;
    if (flags & F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED) {
        free((char *)info->fields.install_name);
    }

    if (flags & F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED) {
        free((char *)info->fields.parent_umbrella);
    }

    destroy_symbols_array(&info->fields.exports);
    destroy_symbols_array(&info->fields.undefineds);
    array_destroy(&info->fields.uuids);

    const struct array exports = info->fields.exports;
    const struct array undefineds = info->fields.undefineds;
    const struct array uuids = info->fields.uuids;

    memset(&info->fields, 0, sizeof(info->fields));

    info->flags = 0;
    info->fields.exports = exports;
    info->fields.undefineds = undefineds;
    info->fields.uuids = uuids;
}
