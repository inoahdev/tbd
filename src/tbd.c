//
//  src/tbd.c
//  tbd
//
//  Created by inoahdev on 11/22/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#include <sys/types.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "copy.h"
#include "likely.h"
#include "target_list.h"
#include "tbd.h"
#include "tbd_write.h"
#include "yaml.h"

const char *tbd_version_to_string(const enum tbd_version version) {
    switch (version) {
        case TBD_VERSION_NONE:
            return NULL;

        case TBD_VERSION_V1:
            return "v1";

        case TBD_VERSION_V2:
            return "v2";

        case TBD_VERSION_V3:
            return "v3";

        case TBD_VERSION_V4:
            return "v4";
    }
}

const char *
tbd_platform_to_string(const enum tbd_platform platform,
                       const enum tbd_version version)
{
    switch (platform) {
        case TBD_PLATFORM_NONE:
            return NULL;

        case TBD_PLATFORM_MACOS:
            if (version == TBD_VERSION_V4) {
                return "macos";
            }

            return "macosx";

        case TBD_PLATFORM_IOS:
            return "ios";

        case TBD_PLATFORM_TVOS:
            return "tvos";

        case TBD_PLATFORM_WATCHOS:
            return "watchos";

        case TBD_PLATFORM_BRIDGEOS:
            return "bridgeos";

        case TBD_PLATFORM_IOSMAC:
            if (version == TBD_VERSION_V4) {
                return "maccatalyst";
            }

            return "iosmac";

        case TBD_PLATFORM_IOS_SIMULATOR:
            return "ios-simulator";

        case TBD_PLATFORM_TVOS_SIMULATOR:
            return "tvos-simulator";

        case TBD_PLATFORM_WATCHOS_SIMULATOR:
            return "watchos-simulator";

        case TBD_PLATFORM_DRIVERKIT:
            return "driverkit";
    }
}

bool
tbd_should_parse_objc_constraint(const struct tbd_parse_options options,
                                 const enum tbd_version version)
{
    if (options.ignore_objc_constraint) {
        return false;
    }

    if (version == TBD_VERSION_V1 || version == TBD_VERSION_V4) {
        return false;
    }

    return true;
}

bool
tbd_should_parse_swift_version(const struct tbd_parse_options options,
                               const enum tbd_version version)
{
    return (!options.ignore_swift_version && version != TBD_VERSION_V1);
}

bool
tbd_should_parse_flags(const struct tbd_parse_options options,
                       const enum tbd_version version)
{
    return (!options.ignore_flags && version != TBD_VERSION_V1);
}

bool tbd_uses_archs(const enum tbd_version version) {
    return (version != TBD_VERSION_V4);
}

enum tbd_ci_set_target_count_result
tbd_ci_set_target_count(struct tbd_create_info *__notnull const info_in,
                        const uint64_t count)
{
    const enum target_list_result reserve_count_result =
        target_list_reserve_count(&info_in->fields.targets, count);

    if (reserve_count_result != E_TARGET_LIST_OK) {
        return E_TBD_CI_SET_TARGET_COUNT_ALLOC_FAIL;
    }

    return E_TBD_CI_SET_TARGET_COUNT_OK;
}

static int
tbd_symbol_info_targets_comparator(const void *__notnull const array_item,
                                   const void *__notnull const item)
{
    const struct tbd_symbol_info *const array_info =
        (const struct tbd_symbol_info *)array_item;

    const struct tbd_symbol_info *const info =
        (const struct tbd_symbol_info *)item;

    const enum tbd_symbol_meta_type array_meta_type = array_info->meta_type;
    const enum tbd_symbol_meta_type meta_type = info->meta_type;

    if (array_meta_type != meta_type) {
        return (int)(array_meta_type - meta_type);
    }

    struct bit_list array_targets = array_info->targets;
    struct bit_list targets = info->targets;

    const uint64_t array_targets_count = array_targets.set_count;
    const uint64_t targets_count = targets.set_count;

    if (array_targets_count != targets_count) {
        if (array_targets_count > targets_count) {
            return 1;
        } else {
            return -1;
        }
    }

    const int compare = bit_list_equal_counts_compare(array_targets, targets);
    if (compare != 0) {
        return compare;
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

static int
tbd_symbol_info_no_targets_comparator(const void *__notnull const array_item,
                                      const void *__notnull const item)
{
    const struct tbd_symbol_info *const array_info =
        (const struct tbd_symbol_info *)array_item;

    const struct tbd_symbol_info *const info =
        (const struct tbd_symbol_info *)item;

    const enum tbd_symbol_meta_type array_meta_type = array_info->meta_type;
    const enum tbd_symbol_meta_type meta_type = info->meta_type;

    if (array_meta_type != meta_type) {
        return (int)(array_meta_type - meta_type);
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

static int
tbd_metadata_info_no_targets_comparator(const void *__notnull const array_item,
                                        const void *__notnull const item)
{
    const struct tbd_metadata_info *const array_info =
        (const struct tbd_metadata_info *)array_item;

    const struct tbd_metadata_info *const info =
        (const struct tbd_metadata_info *)item;

    const enum tbd_metadata_type array_type = array_info->type;
    const enum tbd_metadata_type type = info->type;

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

static int
tbd_metadata_info_comparator(const void *__notnull const array_item,
                             const void *__notnull const item)
{
    const struct tbd_metadata_info *const array_info =
        (const struct tbd_metadata_info *)array_item;

    const struct tbd_metadata_info *const info =
        (const struct tbd_metadata_info *)item;

    const enum tbd_metadata_type array_type = array_info->type;
    const enum tbd_metadata_type type = info->type;

    if (array_type != type) {
        return (int)(array_type - type);
    }

    const uint64_t array_targets_count = array_info->targets.set_count;
    const uint64_t targets_count = info->targets.set_count;

    if (array_targets_count != targets_count) {
        if (array_targets_count > targets_count) {
            return 1;
        } else {
            return -1;
        }
    }

    const int targets_compare =
        bit_list_equal_counts_compare(array_info->targets, info->targets);

    if (targets_compare != 0) {
        return targets_compare;
    }

    const uint64_t array_length = array_info->length;
    const uint64_t length = info->length;

    const char *const array_string = array_info->string;
    const char *const string = info->string;

    /*
     * We try to avoid iterating and comparing over the whole string, so we
     * could check to ensure their lengths match up.
     *
     * However, we don't want the symbols to ever be organized by their length,
     * which would be the case if `(array_length - length)` was returned.
     *
     * So instead, we use separate memcmp() calls for when array_length and
     * length don't match, depending on which is bigger.
     *
     * This stops us from having to use strcmp(), which would be the case since
     * the lengths don't match.
     *
     * We add one to also compare the null-terminator, so that the return-value
     * is accurate.
     */

    if (array_length > length) {
        return memcmp(array_string, string, length + 1);
    } else {
        return memcmp(array_string, string, array_length + 1);
    }
}

static enum tbd_ci_add_data_result
add_metadata_with_type(struct tbd_create_info *__notnull const info_in,
                       const char *__notnull const string,
                       const uint64_t length,
                       const uint64_t bit_index,
                       const enum tbd_metadata_type type,
                       const struct tbd_parse_options options)
{
    struct tbd_metadata_info info = {
        .string = (char *)string,
        .length = length,
        .type = type,
    };

    struct array_cached_index_info cached_info = {};
    struct tbd_metadata_info *const existing_info =
        array_find_item_in_sorted(&info_in->fields.metadata,
                                  sizeof(info),
                                  &info,
                                  tbd_metadata_info_no_targets_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        if (options.ignore_targets) {
            return E_TBD_CI_ADD_DATA_OK;
        }

        bit_list_set_bit(&existing_info->targets, bit_index);
        return E_TBD_CI_ADD_DATA_OK;
    }

    info.string = alloc_and_copy(info.string, info.length);
    if (unlikely(info.string == NULL)) {
        return E_TBD_CI_ADD_DATA_ALLOC_FAIL;
    }

    if (yaml_c_str_needs_quotes(string, length)) {
        info.flags.needs_quotes = true;
    }

    const uint64_t targets_count = info_in->fields.targets.set_count;
    const enum bit_list_result create_bits_result =
        bit_list_create_with_capacity(&info.targets, targets_count);

    if (create_bits_result != E_BIT_LIST_OK) {
        free(info.string);
        return E_TBD_CI_ADD_DATA_ALLOC_FAIL;
    }

    if (options.ignore_targets) {
        bit_list_set_first_n(&info.targets, targets_count);
    } else {
        bit_list_set_bit(&info.targets, bit_index);
    }

    struct array *const metadata = &info_in->fields.metadata;
    const enum array_result add_export_info_result =
        array_add_item_with_cached_index_info(metadata,
                                              sizeof(info),
                                              &info,
                                              &cached_info,
                                              NULL);

    if (unlikely(add_export_info_result != E_ARRAY_OK)) {
        free(info.string);
        return E_TBD_CI_ADD_DATA_ARRAY_FAIL;
    }

    return E_TBD_CI_ADD_DATA_OK;
}

static const struct tbd_metadata_info *
get_parent_umbrella(struct array *__notnull const list) {
    if (list->item_count == 0) {
        return NULL;
    }

    const struct tbd_metadata_info *const item =
        (const struct tbd_metadata_info *)list->data;

    if (item->type != TBD_METADATA_TYPE_PARENT_UMBRELLA) {
        return NULL;
    }

    return item;
}

enum tbd_ci_add_parent_umbrella_result
tbd_ci_add_parent_umbrella(struct tbd_create_info *__notnull const info_in,
                           const char *__notnull string,
                           const uint64_t length,
                           const uint64_t arch_index,
                           const struct tbd_parse_options options)
{
    if (options.ignore_parent_umbrellas) {
        return E_TBD_CI_ADD_PARENT_UMBRELLA_OK;
    }

    if (tbd_uses_archs(info_in->version)) {
        const struct tbd_metadata_info *const parent_umbrella =
            get_parent_umbrella(&info_in->fields.metadata);

        if (parent_umbrella != NULL) {
            if (parent_umbrella->length != length) {
                return E_TBD_CI_ADD_PARENT_UMBRELLA_INFO_CONFLICT;
            }

            if (memcmp(parent_umbrella->string, string, length) != 0) {
                return E_TBD_CI_ADD_PARENT_UMBRELLA_INFO_CONFLICT;
            }
        }
    }

    const enum tbd_ci_add_data_result add_metadata_result =
        add_metadata_with_type(info_in,
                               string,
                               length,
                               arch_index,
                               TBD_METADATA_TYPE_PARENT_UMBRELLA,
                               options);

    switch (add_metadata_result) {
        case E_TBD_CI_ADD_DATA_OK:
            break;

        case E_TBD_CI_ADD_DATA_ALLOC_FAIL:
            return E_TBD_CI_ADD_PARENT_UMBRELLA_ALLOC_FAIL;

        case E_TBD_CI_ADD_DATA_ARRAY_FAIL:
            return E_TBD_CI_ADD_PARENT_UMBRELLA_ARRAY_FAIL;
    }

    return E_TBD_CI_ADD_PARENT_UMBRELLA_OK;
}

struct tbd_metadata_info *
tbd_ci_get_single_parent_umbrella(
    const struct tbd_create_info *__notnull const info_in)
{
    const struct array *const metadata = &info_in->fields.metadata;
    if (metadata->item_count == 0) {
        return NULL;
    }

    struct tbd_metadata_info *const umbrella_info =
        (struct tbd_metadata_info *)metadata->data;

    if (umbrella_info->type != TBD_METADATA_TYPE_PARENT_UMBRELLA) {
        return NULL;
    }

    return umbrella_info;
}

void
tbd_ci_set_single_platform(struct tbd_create_info *__notnull const info,
                           const enum tbd_platform platform)
{
    target_list_replace_platform(&info->fields.targets, platform);

    struct tbd_uuid_info *uuid = info->fields.uuids.data;
    const struct tbd_uuid_info *const end = info->fields.uuids.data_end;

    for (; uuid != end; uuid++) {
        uuid->target = replace_platform_for_target(uuid->target, platform);
    }
}

enum tbd_platform
tbd_ci_get_single_platform(const struct tbd_create_info *__notnull const info) {
    const struct arch_info *arch = NULL;
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    target_list_get_target(&info->fields.targets, 0, &arch, &platform);
    return platform;
}

enum tbd_ci_add_data_result
tbd_ci_add_symbol_with_type(struct tbd_create_info *__notnull const info_in,
                            const char *__notnull const string,
                            const uint64_t length,
                            const uint64_t arch_index,
                            const enum tbd_symbol_type type,
                            enum tbd_symbol_meta_type meta_type,
                            const struct tbd_parse_options options)
{
    switch (type) {
        case TBD_SYMBOL_TYPE_NONE:
            break;

        case TBD_SYMBOL_TYPE_CLIENT:
            if (options.ignore_clients) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            break;

        case TBD_SYMBOL_TYPE_REEXPORT:
            if (options.ignore_reexports) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            break;

        case TBD_SYMBOL_TYPE_NORMAL:
            if (options.ignore_normal_syms) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            break;

        case TBD_SYMBOL_TYPE_OBJC_CLASS:
            if (options.ignore_objc_class_syms) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            break;

        case TBD_SYMBOL_TYPE_OBJC_EHTYPE:
            if (options.ignore_objc_ehtype_syms) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            break;

        case TBD_SYMBOL_TYPE_OBJC_IVAR:
            if (options.ignore_objc_ivar_syms) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            break;

        case TBD_SYMBOL_TYPE_WEAK_DEF:
            if (options.ignore_weak_defs_syms) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            break;

        case TBD_SYMBOL_TYPE_THREAD_LOCAL:
            if (options.ignore_thread_local_syms) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            break;
    }

    const enum tbd_version version = info_in->version;
    switch (version) {
        case TBD_VERSION_NONE:
            return E_TBD_CI_ADD_DATA_OK;

        case TBD_VERSION_V1:
            switch (meta_type) {
                case TBD_SYMBOL_META_TYPE_NONE:
                    return E_TBD_CI_ADD_DATA_OK;

                case TBD_SYMBOL_META_TYPE_EXPORT:
                    if (options.ignore_exports) {
                        return E_TBD_CI_ADD_DATA_OK;
                    }

                    break;

                case TBD_SYMBOL_META_TYPE_REEXPORT:
                    meta_type = TBD_SYMBOL_META_TYPE_EXPORT;
                    break;

                case TBD_SYMBOL_META_TYPE_UNDEFINED:
                    return E_TBD_CI_ADD_DATA_OK;
            }

            break;

        case TBD_VERSION_V2:
        case TBD_VERSION_V3:
            switch (meta_type) {
                case TBD_SYMBOL_META_TYPE_NONE:
                    return E_TBD_CI_ADD_DATA_OK;

                case TBD_SYMBOL_META_TYPE_EXPORT:
                    if (options.ignore_exports) {
                        return E_TBD_CI_ADD_DATA_OK;
                    }

                    break;

                case TBD_SYMBOL_META_TYPE_REEXPORT:
                    meta_type = TBD_SYMBOL_META_TYPE_EXPORT;
                    break;

                case TBD_SYMBOL_META_TYPE_UNDEFINED:
                    break;
            }

            break;

        /*
         * On tbd-version v4, clients and re-exports are their own metadata
         * sections.
         */

        case TBD_VERSION_V4:
            switch (type) {
                case TBD_SYMBOL_TYPE_NONE:
                case TBD_SYMBOL_TYPE_NORMAL:
                case TBD_SYMBOL_TYPE_OBJC_CLASS:
                case TBD_SYMBOL_TYPE_OBJC_IVAR:
                case TBD_SYMBOL_TYPE_OBJC_EHTYPE:
                case TBD_SYMBOL_TYPE_WEAK_DEF:
                case TBD_SYMBOL_TYPE_THREAD_LOCAL:
                    if (options.ignore_exports) {
                        if (meta_type == TBD_SYMBOL_META_TYPE_EXPORT) {
                            return E_TBD_CI_ADD_DATA_OK;
                        }
                    }

                    break;

                case TBD_SYMBOL_TYPE_CLIENT: {
                    const enum tbd_ci_add_data_result add_client_result =
                        add_metadata_with_type(info_in,
                                               string,
                                               length,
                                               arch_index,
                                               TBD_METADATA_TYPE_CLIENT,
                                               options);

                    return add_client_result;
                }

                case TBD_SYMBOL_TYPE_REEXPORT: {
                    const enum tbd_ci_add_data_result add_reexport_result =
                        add_metadata_with_type(
                            info_in,
                            string,
                            length,
                            arch_index,
                            TBD_METADATA_TYPE_REEXPORTED_LIBRARY,
                            options);

                    return add_reexport_result;
                }
            }

            break;
    }

    struct tbd_symbol_info symbol_info = {
        .length = length,
        .string = (char *)string,
        .type = type,
        .meta_type = meta_type
    };

    struct array_cached_index_info cached_info = {};
    struct tbd_symbol_info *const existing_info =
        array_find_item_in_sorted(&info_in->fields.symbols,
                                  sizeof(symbol_info),
                                  &symbol_info,
                                  tbd_symbol_info_no_targets_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        if (options.ignore_targets) {
            return E_TBD_CI_ADD_DATA_OK;
        }

        bit_list_set_bit(&existing_info->targets, arch_index);
        return E_TBD_CI_ADD_DATA_OK;
    }

    symbol_info.string = alloc_and_copy(symbol_info.string, symbol_info.length);
    if (unlikely(symbol_info.string == NULL)) {
        return E_TBD_CI_ADD_DATA_ALLOC_FAIL;
    }

    if (yaml_c_str_needs_quotes(string, length)) {
        symbol_info.flags.needs_quotes = true;
    }

    const uint64_t targets_count = info_in->fields.targets.set_count;
    const enum bit_list_result create_bits_result =
        bit_list_create_with_capacity(&symbol_info.targets, targets_count);

    if (create_bits_result != E_BIT_LIST_OK) {
        free(symbol_info.string);
        return E_TBD_CI_ADD_DATA_ALLOC_FAIL;
    }

    if (options.ignore_targets) {
        bit_list_set_first_n(&symbol_info.targets, targets_count);
    } else {
        bit_list_set_bit(&symbol_info.targets, arch_index);
    }

    const enum array_result add_export_info_result =
        array_add_item_with_cached_index_info(&info_in->fields.symbols,
                                              sizeof(symbol_info),
                                              &symbol_info,
                                              &cached_info,
                                              NULL);

    if (unlikely(add_export_info_result != E_ARRAY_OK)) {
        free(symbol_info.string);
        return E_TBD_CI_ADD_DATA_ARRAY_FAIL;
    }

    return E_TBD_CI_ADD_DATA_OK;
}


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

    if (version < TBD_VERSION_V3 || max_len < 16) {
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

enum tbd_ci_add_data_result
tbd_ci_add_symbol_with_info(struct tbd_create_info *__notnull const info_in,
                            const char *__notnull string,
                            uint64_t lnmax,
                            const uint64_t arch_index,
                            const enum tbd_symbol_type predefined_type,
                            const enum tbd_symbol_meta_type meta_type,
                            const bool is_exported,
                            const struct tbd_parse_options options)
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
                if (!is_exported && !options.allow_priv_objc_class_syms) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                string += offset;

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the class-name is to be removed.
                 */

                if (vers > TBD_VERSION_V2) {
                    string += 1;
                    lnmax -= 1;
                }

                length = strnlen(string, lnmax - offset);
                if (likely(length != 0)) {
                    type = TBD_SYMBOL_TYPE_OBJC_CLASS;
                }
            } else if ((offset = is_objc_ivar_symbol(str, first)) != 0) {
                if (!is_exported && !options.allow_priv_objc_ivar_syms) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                string += offset;

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the ivar-name is to be removed.
                 */

                if (vers > TBD_VERSION_V2) {
                    string += 1;
                    lnmax -= 1;
                }

                length = strnlen(string, lnmax - offset);
                if (likely(length != 0)) {
                    type = TBD_SYMBOL_TYPE_OBJC_IVAR;
                }
            } else if ((offset = is_objc_ehtype_sym(str, first, lnmax, vers))) {
                if (!is_exported && !options.allow_priv_objc_ehtype_syms) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                string += offset;
                length = strnlen(string, lnmax - offset);

                if (likely(length != 0)) {
                    type = TBD_SYMBOL_TYPE_OBJC_EHTYPE;
                }
            } else {
                if (!is_exported) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                length = (uint32_t)strnlen(string, lnmax);
                if (unlikely(length == 0)) {
                    return E_TBD_CI_ADD_DATA_OK;
                }
            }
        } else {
            if (!is_exported) {
                return E_TBD_CI_ADD_DATA_OK;
            }

            length = (uint32_t)strnlen(string, lnmax);
            if (unlikely(length == 0)) {
                return E_TBD_CI_ADD_DATA_OK;
            }
        }
    } else {
        if (!is_exported) {
            return E_TBD_CI_ADD_DATA_OK;
        }

        length = (uint32_t)strnlen(string, lnmax);
        if (unlikely(length == 0)) {
            return E_TBD_CI_ADD_DATA_OK;
        }

        type = predefined_type;
    }

    const enum tbd_ci_add_data_result add_symbol_result =
        tbd_ci_add_symbol_with_type(info_in,
                                    string,
                                    length,
                                    arch_index,
                                    type,
                                    meta_type,
                                    options);

    if (add_symbol_result != E_TBD_CI_ADD_DATA_OK) {
        return add_symbol_result;
    }

    return E_TBD_CI_ADD_DATA_OK;
}

enum tbd_ci_add_data_result
tbd_ci_add_symbol_with_info_and_len(
    struct tbd_create_info *__notnull const info_in,
    const char *__notnull string,
    uint64_t len,
    const uint64_t arch_index,
    const enum tbd_symbol_type predefined_type,
    const enum tbd_symbol_meta_type meta_type,
    const bool is_exported,
    const struct tbd_parse_options options)
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
                if (!is_exported && !options.allow_priv_objc_class_syms) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                string += offset;
                len -= offset;

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the class-name is to be removed.
                 */

                if (vers > TBD_VERSION_V2) {
                    string += 1;
                    len -= 1;
                }

                if (unlikely(len == 0)) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                type = TBD_SYMBOL_TYPE_OBJC_CLASS;
            } else if ((offset = is_objc_ivar_symbol(str, first)) != 0) {
                if (!is_exported && !options.allow_priv_objc_ivar_syms) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                string += offset;
                len -= offset;

                /*
                 * Starting from tbd-version v3, the underscore at the front of
                 * the ivar-name is to be removed.
                 */

                if (vers > TBD_VERSION_V2) {
                    string += 1;
                    len -= 1;
                }

                if (unlikely(len == 0)) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                type = TBD_SYMBOL_TYPE_OBJC_IVAR;
            } else if ((offset = is_objc_ehtype_sym(str, first, len, vers))) {
                if (!is_exported && !options.allow_priv_objc_ehtype_syms) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                string += offset;
                len -= offset;

                if (unlikely(len == 0)) {
                    return E_TBD_CI_ADD_DATA_OK;
                }

                type = TBD_SYMBOL_TYPE_OBJC_EHTYPE;
            }
        } else {
            if (!is_exported) {
                return E_TBD_CI_ADD_DATA_OK;
            }
        }
    } else {
        if (!is_exported) {
            return E_TBD_CI_ADD_DATA_OK;
        }

        type = predefined_type;
    }

    const enum tbd_ci_add_data_result add_symbol_result =
        tbd_ci_add_symbol_with_type(info_in,
                                    string,
                                    len,
                                    arch_index,
                                    type,
                                    meta_type,
                                    options);

    if (add_symbol_result != E_TBD_CI_ADD_DATA_OK) {
        return add_symbol_result;
    }

    return E_TBD_CI_ADD_DATA_OK;
}

int
tbd_uuid_info_comparator(const void *__notnull const array_item,
                         const void *__notnull const item)
{
    const struct tbd_uuid_info *const array_uuid_info =
        (const struct tbd_uuid_info *)array_item;

    const struct tbd_uuid_info *const uuid_info =
        (const struct tbd_uuid_info *)item;

    const uint64_t array_target = array_uuid_info->target;
    const struct arch_info *const array_arch_info =
        (const struct arch_info *)(array_target & TARGET_ARCH_INFO_MASK);

    const uint64_t target = uuid_info->target;
    const struct arch_info *const arch_info =
        (const struct arch_info *)(target & TARGET_ARCH_INFO_MASK);

    if (array_arch_info > arch_info) {
        return 1;
    } else if (array_arch_info < arch_info) {
        return -1;
    }

    return 0;
}

void tbd_ci_sort_info(struct tbd_create_info *__notnull const info_in) {
    array_sort_with_comparator(&info_in->fields.uuids,
                               sizeof(struct tbd_uuid_info),
                               tbd_uuid_info_comparator);

    array_sort_with_comparator(&info_in->fields.metadata,
                               sizeof(struct tbd_metadata_info),
                               tbd_metadata_info_comparator);

    array_sort_with_comparator(&info_in->fields.symbols,
                               sizeof(struct tbd_symbol_info),
                               tbd_symbol_info_targets_comparator);
}

static bool
tbd_uuid_info_is_unique_comparator(const void *__notnull const array_item,
                                   const void *__notnull const item)
{
    const struct tbd_uuid_info *const array_uuid_info =
        (const struct tbd_uuid_info *)array_item;

    const uint8_t *const array_uuid = array_uuid_info->uuid;
    const uint8_t *const uuid = (const uint8_t *)item;

    return (memcmp(array_uuid, uuid, sizeof(array_uuid_info->uuid)) == 0);
}

enum tbd_ci_add_uuid_result
tbd_ci_add_uuid(struct tbd_create_info *__notnull const info_in,
                const struct arch_info *__notnull const arch,
                const enum tbd_platform platform,
                const uint8_t uuid[const 16])
{
    const struct tbd_uuid_info *const array_uuid =
        array_find_item(&info_in->fields.uuids,
                        sizeof(struct tbd_uuid_info),
                        uuid,
                        tbd_uuid_info_is_unique_comparator,
                        NULL);

    if (array_uuid != NULL) {
        return E_TBD_CI_ADD_UUID_NON_UNIQUE_UUID;
    }

    struct tbd_uuid_info uuid_info = {
        .target = target_list_create_target(arch, platform)
    };

    memcpy(uuid_info.uuid, uuid, 16);
    const enum array_result add_uuid_info_result =
        array_add_item(&info_in->fields.uuids,
                       sizeof(uuid_info),
                       &uuid_info,
                       NULL);

    if (add_uuid_info_result != E_ARRAY_OK) {
        return E_TBD_CI_ADD_UUID_ARRAY_FAIL;
    }

    return E_TBD_CI_ADD_UUID_OK;
}

enum tbd_create_result
tbd_create_with_info(const struct tbd_create_info *__notnull const info,
                     FILE *__notnull const file,
                     const struct tbd_create_options options)
{
    const enum tbd_version version = info->version;
    if (tbd_write_magic(file, version)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    const struct target_list targets = info->fields.targets;
    const bool uses_archs = tbd_uses_archs(version);

    if (!uses_archs) {
        if (tbd_write_targets_for_header(file, targets, version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    } else {
        if (tbd_write_archs_for_header(file, targets)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (!options.ignore_uuids) {
        const struct array *const uuids = &info->fields.uuids;
        if (!uses_archs) {
            if (tbd_write_uuids_for_targets(file, uuids, version)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        } else if (version != TBD_VERSION_V1) {
            if (tbd_write_uuids_for_archs(file, uuids)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    }

    if (uses_archs) {
        if (tbd_write_platform(file, info, version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (version != TBD_VERSION_V1 && !options.ignore_flags) {
        if (tbd_write_flags(file, info->fields.flags)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (tbd_write_install_name(file, info)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    if (!options.ignore_current_version) {
        if (tbd_write_current_version(file, info->fields.current_version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (!options.ignore_compat_version) {
        const uint32_t compatibility_version =
            info->fields.compatibility_version;

        if (tbd_write_compatibility_version(file, compatibility_version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (version != TBD_VERSION_V1) {
        if (!options.ignore_swift_version) {
            const uint32_t swift_version = info->fields.swift_version;
            if (tbd_write_swift_version(file, version, swift_version)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }

        if (uses_archs) {
            if (!options.ignore_objc_constraint) {
                const enum tbd_objc_constraint objc_constraint =
                    info->fields.archs.objc_constraint;

                if (tbd_write_objc_constraint(file, objc_constraint)) {
                    return E_TBD_CREATE_WRITE_FAIL;
                }
            }

            if (!options.ignore_parent_umbrellas) {
                if (tbd_write_parent_umbrella_for_archs(file, info)) {
                    return E_TBD_CREATE_WRITE_FAIL;
                }
            }
        }
    }

    if (!uses_archs) {
        if (info->flags.uses_full_targets) {
            if (tbd_write_metadata_with_full_targets(file, info, options)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }

            if (tbd_write_symbols_with_full_targets(file, info, options)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        } else {
            if (tbd_write_metadata(file, info, options)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }

            if (tbd_write_symbols_for_targets(file, info, options)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    } else {
        if (info->flags.uses_full_targets) {
            if (tbd_write_symbols_with_full_archs(file, info, options)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        } else {
            if (tbd_write_symbols_for_archs(file, info, options)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    }

    if (!options.ignore_footer) {
        if (tbd_write_footer(file)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    return E_TBD_CREATE_OK;
}

static void clear_metadata_array(struct array *__notnull const list) {
    struct tbd_metadata_info *info = list->data;
    const struct tbd_metadata_info *const end = list->data_end;

    for (; info != end; info++) {
        bit_list_destroy(&info->targets);
        free(info->string);
    }

    array_clear(list);
}

static void clear_symbols_array(struct array *__notnull const list) {
    struct tbd_symbol_info *info = list->data;
    const struct tbd_symbol_info *const end = list->data_end;

    for (; info != end; info++) {
        bit_list_destroy(&info->targets);
        free(info->string);
    }

    array_clear(list);
}

void
tbd_create_info_clear_fields_and_create_from(
    struct tbd_create_info *__notnull const dst,
    const struct tbd_create_info *__notnull const src)
{
    if (dst->flags.install_name_was_allocated) {
        free((char *)dst->fields.install_name);
    }

    clear_metadata_array(&dst->fields.metadata);
    clear_symbols_array(&dst->fields.symbols);
    array_clear(&dst->fields.uuids);

    const struct array metadata = dst->fields.metadata;
    const struct array symbols = dst->fields.symbols;
    const struct array uuids = dst->fields.uuids;

    memcpy(&dst->fields, &src->fields, sizeof(dst->fields));
    dst->flags = src->flags;

    dst->fields.metadata = metadata;
    dst->fields.symbols = symbols;
    dst->fields.uuids = uuids;
}

static void destroy_metadata_array(struct array *__notnull const list) {
    struct tbd_metadata_info *info = list->data;
    const struct tbd_metadata_info *const end = list->data_end;

    for (; info != end; info++) {
        bit_list_destroy(&info->targets);
        free(info->string);
    }

    array_destroy(list);
}

static void destroy_symbols_array(struct array *__notnull const list) {
    struct tbd_symbol_info *info = list->data;
    const struct tbd_symbol_info *const end = list->data_end;

    for (; info != end; info++) {
        bit_list_destroy(&info->targets);
        free(info->string);
    }

    array_destroy(list);
}

void tbd_create_info_destroy(struct tbd_create_info *__notnull const info) {
    if (info->flags.install_name_was_allocated) {
        free((char *)info->fields.install_name);
    }

    destroy_metadata_array(&info->fields.metadata);
    destroy_symbols_array(&info->fields.symbols);

    target_list_destroy(&info->fields.targets);
    array_destroy(&info->fields.uuids);

    memset(&info->fields, 0, sizeof(info->fields));

    info->flags.install_name_was_allocated = false;
    info->flags.install_name_needs_quotes = false;
}

