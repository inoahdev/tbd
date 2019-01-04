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

/*
 * From: https://stackoverflow.com/a/2709523
 */

static inline int bits_count(uint64_t i) {
    i = i - ((i >> 1) & 0x5555555555555555);
    i = (i & 0x3333333333333333) + ((i >> 2) & 0x3333333333333333);

    return (((i + (i >> 4)) & 0xF0F0F0F0F0F0F0F) * 0x101010101010101) >> 56;
}

static int arch_list_compare(const uint64_t left, const uint64_t right) {
    if (left == right) {
        return 0;
    }
    
    const int left_count = bits_count(left);
    const int right_count = bits_count(right);

    if (left_count != right_count) {
        return left_count - right_count;
    }

    if (left > right) {
        return 1;
    } else if (left < right) {
        return -1;
    }

    return 0;
}

/*
 * Have the symbols array be sorted first into groups of matching arch-lists.
 * The arch-lists themselves are sorted to where arch-lists with more archs are
 * "greater", than those without.
 * Arch-lists with the same number of archs are just compared numberically.
 * 
 * Within each arch-list group, the symbols are then organized by their types.
 * Within each type group, the symbols are then finally organized
 * alphabetically.
 * 
 * This is done to make the creation of export-groups later on easier, as no
 * the symbols are already organized by their arch-lists, and then their types,
 * all alphabetically.
 */

int
tbd_export_info_no_archs_comparator(const void *const array_item,
                                    const void *const item)
{
    const struct tbd_export_info *const array_info =
        (const struct tbd_export_info *)array_item;

    const struct tbd_export_info *const info =
        (const struct tbd_export_info *)item;

    const enum tbd_export_type array_type = array_info->type;
    const enum tbd_export_type type = info->type;

    if (array_type != type) {        
        return array_type - type;
    }

    const char *const array_string = array_info->string;
    const char *const string = info->string;

    return strcmp(array_string, string);
}

int
tbd_export_info_comparator(const void *const array_item, const void *const item)
{
    const struct tbd_export_info *const array_info =
        (const struct tbd_export_info *)array_item;

    const struct tbd_export_info *const info =
        (const struct tbd_export_info *)item;

    const uint64_t array_archs = array_info->archs;
    const uint64_t archs = info->archs;

    const int archs_compare = arch_list_compare(array_archs, archs);
    if (archs_compare != 0) {
        return archs_compare;
    }

    const enum tbd_export_type array_type = array_info->type;
    const enum tbd_export_type type = info->type;

    if (array_type != type) {        
        return array_type - type;
    }

    const char *const array_string = array_info->string;
    const char *const string = info->string;

    return strcmp(array_string, string);
}

int
tbd_uuid_info_comparator(const void *const array_item, const void *const item) {
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
tbd_create_with_info(const struct tbd_create_info *const info,
                     FILE *const file,
                     const uint64_t options)
{
    const enum tbd_version version = info->version;
    if (tbd_write_magic(file, version)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    if (tbd_write_header_archs(file, info->archs)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    if (!(options & O_TBD_CREATE_IGNORE_UUIDS)) {
        if (version != TBD_VERSION_V1) {
            if (tbd_write_uuids(file, &info->uuids, options)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    }

    if (tbd_write_platform(file, info->platform)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }
    
    if (version != TBD_VERSION_V1) {
        if (!(options & O_TBD_CREATE_IGNORE_FLAGS)) {
            if (tbd_write_flags(file, info->flags_field)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }
    }

    if (tbd_write_install_name(file, info)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    if (!(options & O_TBD_CREATE_IGNORE_CURRENT_VERSION)) {
        if (tbd_write_current_version(file, info->current_version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (!(options & O_TBD_CREATE_IGNORE_COMPATIBILITY_VERSION)) {
        const uint32_t compatibility_version = info->compatibility_version;
        if (tbd_write_compatibility_version(file, compatibility_version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (version != TBD_VERSION_V1) {
        if (!(options & O_TBD_CREATE_IGNORE_SWIFT_VERSION)) {
            if (tbd_write_swift_version(file, version, info->swift_version)) {
                return E_TBD_CREATE_WRITE_FAIL;
            }
        }

        if (!(options & O_TBD_CREATE_IGNORE_OBJC_CONSTRAINT)) {
            if (tbd_write_objc_constraint(file, info->objc_constraint)) {
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
        if (tbd_write_exports(file, &info->exports, version)) {
            return E_TBD_CREATE_WRITE_FAIL;
        }
    }

    if (tbd_write_footer(file)) {
        return E_TBD_CREATE_WRITE_FAIL;
    }

    return E_TBD_CREATE_OK;
}

static void destroy_exports_array(struct array *const list) {
    struct tbd_export_info *info = list->data;
    const struct tbd_export_info *const end = list->data_end;

    for (; info != end; info++) {
        free(info->string);
    }
    
    array_destroy(list);
}

void tbd_create_info_destroy(struct tbd_create_info *const info) {
    if (info->flags & F_TBD_CREATE_INFO_STRINGS_WERE_COPIED) {
        free((char *)info->install_name);
        free((char *)info->parent_umbrella);
    }

    info->version = 0;

    info->archs = 0;
    info->flags = 0;

    info->platform = 0;
    info->objc_constraint = 0;

    info->install_name = NULL;
    info->parent_umbrella = NULL;

    info->current_version = 0;
    info->compatibility_version = 0;
    info->swift_version = 0;

    destroy_exports_array(&info->exports);
    array_destroy(&info->uuids);
}
