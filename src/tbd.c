//
//  src/tbd.c
//  tbd
//
//  Created by inoahdev on 11/22/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tbd.h"
#include "tbd_write.h"

int
tbd_export_info_comparator(const void *__notnull const array_item,
                           const void *__notnull const item)
{
    const struct tbd_export_info *const array_info =
        (const struct tbd_export_info *)array_item;

    const struct tbd_export_info *const info =
        (const struct tbd_export_info *)item;

    const uint64_t array_archs_count = array_info->archs_count;
    const uint64_t archs_count = info->archs_count;

    if (array_archs_count > archs_count) {
        return 1;
    } else if (array_archs_count < archs_count) {
        return -1;
    }

    const uint64_t array_archs = array_info->archs;
    const uint64_t archs = info->archs;

    if (array_archs > archs) {
        return 1;
    } else if (array_archs < archs) {
        return -1;
    }

    const enum tbd_export_type array_type = array_info->type;
    const enum tbd_export_type type = info->type;

    if (array_type > type) {
        return 1;
    } else if (array_type < type) {
        return -1;
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
tbd_export_info_no_archs_comparator(const void *__notnull const array_item,
                                    const void *__notnull const item)
{
    const struct tbd_export_info *const array_info =
        (const struct tbd_export_info *)array_item;

    const struct tbd_export_info *const info =
        (const struct tbd_export_info *)item;

    const enum tbd_export_type array_type = array_info->type;
    const enum tbd_export_type type = info->type;

    if (array_type > type) {
        return 1;
    } else if (array_type < type) {
        return -1;
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

    if (tbd_write_archs_for_header(file, info->fields.archs)) {
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
        if (info->flags & F_TBD_CREATE_INFO_EXPORTS_HAVE_FULL_ARCHS) {
            if (tbd_write_exports_with_full_archs(info, file)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        } else {
            if (tbd_write_exports(file, &info->fields.exports, version)) {
                return E_TBD_CREATE_WRITE_FAIL;
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

static void clear_exports_array(struct array *__notnull const list) {
    struct tbd_export_info *info = list->data;
    const struct tbd_export_info *const end = list->data_end;

    for (; info != end; info++) {
        free(info->string);
    }

    array_clear(list);
}

void tbd_create_info_clear(struct tbd_create_info *__notnull const info) {
    if (info->flags & F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED) {
        free((char *)info->fields.install_name);
    }

    if (info->flags & F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED) {
        free((char *)info->fields.parent_umbrella);
    }

    info->version = 0;

    info->fields.archs = 0;
    info->fields.flags = 0;

    info->fields.platform = TBD_PLATFORM_NONE;
    info->fields.objc_constraint = TBD_OBJC_CONSTRAINT_NO_VALUE;

    info->fields.install_name = NULL;
    info->fields.parent_umbrella = NULL;

    info->fields.current_version = 0;
    info->fields.compatibility_version = 0;
    info->fields.swift_version = 0;

    clear_exports_array(&info->fields.exports);
    array_clear(&info->fields.uuids);
}

static void destroy_exports_array(struct array *__notnull const list) {
    struct tbd_export_info *info = list->data;
    const struct tbd_export_info *const end = list->data_end;

    for (; info != end; info++) {
        free(info->string);
    }

    array_destroy(list);
}

void tbd_create_info_destroy(struct tbd_create_info *__notnull const info) {
    if (info->flags & F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED) {
        free((char *)info->fields.install_name);
    }

    if (info->flags & F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED) {
        free((char *)info->fields.parent_umbrella);
    }

    info->version = TBD_VERSION_NONE;

    info->fields.archs = 0;
    info->fields.flags = 0;

    info->fields.platform = TBD_PLATFORM_NONE;
    info->fields.objc_constraint = TBD_OBJC_CONSTRAINT_NO_VALUE;

    info->fields.install_name = NULL;
    info->fields.parent_umbrella = NULL;

    info->fields.current_version = 0;
    info->fields.compatibility_version = 0;
    info->fields.swift_version = 0;

    destroy_exports_array(&info->fields.exports);
    array_destroy(&info->fields.uuids);
}
