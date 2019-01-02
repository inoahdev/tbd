//
//  include/tbd.h
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef TBD_H
#define TBD_H

#include <stdio.h>

#include "arch_info.h"
#include "array.h"

/*
 * Options to handle when parsing out information for tbd_create_info.
 */

enum tbd_parse_options {
    O_TBD_PARSE_IGNORE_CLIENTS               = 1 << 0,
    O_TBD_PARSE_IGNORE_CURRENT_VERSION       = 1 << 1,
    O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION = 1 << 2,
    O_TBD_PARSE_IGNORE_FLAGS                 = 1 << 3,
    O_TBD_PARSE_IGNORE_INSTALL_NAME          = 1 << 4,
    O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT       = 1 << 5,
    O_TBD_PARSE_IGNORE_PARENT_UMBRELLA       = 1 << 6,
    O_TBD_PARSE_IGNORE_PLATFORM              = 1 << 7,
    O_TBD_PARSE_IGNORE_REEXPORTS             = 1 << 8,
    O_TBD_PARSE_IGNORE_SWIFT_VERSION         = 1 << 9,
    O_TBD_PARSE_IGNORE_SYMBOLS               = 1 << 10,
    O_TBD_PARSE_IGNORE_UUID                  = 1 << 11,

    /*
     * Options dictating what types of symbols should also be allowed in
     * addition to the default types.
     */

    O_TBD_PARSE_ALLOW_PRIVATE_NORMAL_SYMBOLS     = 1 << 12,
    O_TBD_PARSE_ALLOW_PRIVATE_WEAK_DEF_SYMBOLS   = 1 << 13,
    O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS = 1 << 14,
    O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS  = 1 << 15,

    O_TBD_PARSE_IGNORE_MISSING_IDENTIFICATIOn = 1 << 16,
    O_TBD_PARSE_IGNORE_MISSING_PLATFORM       = 1 << 17,
    O_TBD_PARSE_IGNORE_MISSING_SYMBOL_TABLE   = 1 << 18,
    O_TBD_PARSE_IGNORE_MISSING_UUIDS          = 1 << 19,
    O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS       = 1 << 20,
};

enum tbd_flags {
    TBD_FLAG_FLAT_NAMESPACE         = 1 << 0,
    TBD_FLAG_NOT_APP_EXTENSION_SAFE = 1 << 1
};

enum tbd_objc_constraint {
    TBD_OBJC_CONSTRAINT_NONE = 1,
    TBD_OBJC_CONSTRAINT_GC,
    TBD_OBJC_CONSTRAINT_RETAIN_RELEASE,
    TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_OR_GC,
    TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_FOR_SIMULATOR
};

enum tbd_platform {
    TBD_PLATFORM_MACOS = 1,
    TBD_PLATFORM_IOS,
    TBD_PLATFORM_WATCHOS,
    TBD_PLATFORM_TVOS,
};

enum tbd_export_type {
    TBD_EXPORT_TYPE_CLIENT = 1,
    TBD_EXPORT_TYPE_REEXPORT,
    TBD_EXPORT_TYPE_NORMAL_SYMBOL,
    TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL,
    TBD_EXPORT_TYPE_OBJC_IVAR_SYMBOL,
    TBD_EXPORT_TYPE_WEAK_DEF_SYMBOL
};

enum tbd_export_info_flags {
    F_TBD_EXPORT_INFO_STRING_NEEDS_QUOTES = 1 << 0
};

struct tbd_export_info {
    uint64_t archs;
    uint32_t length;

    enum tbd_export_type type;
    char *string;

    uint64_t flags;
};

int tbd_export_info_comparator(const void *array_item, const void *item);
int
tbd_export_info_no_archs_comparator(const void *array_item, const void *item);

struct tbd_uuid_info {
    const struct arch_info *arch;
    uint8_t uuid[16];  
};

int tbd_uuid_info_comparator(const void *array_item, const void *item);

enum tbd_version {
    TBD_VERSION_V1 = 1,
    TBD_VERSION_V2,
    TBD_VERSION_V3
};

enum tbd_create_info_flags {
    F_TBD_CREATE_INFO_INSTALL_NAME_NEEDS_QUOTES    = 1 << 0,
    F_TBD_CREATE_INFO_PARENT_UMBRELLA_NEEDS_QUOTES = 1 << 1,

    F_TBD_CREATE_INFO_STRINGS_WERE_COPIED = 1 << 2,
};

struct tbd_create_info {
    enum tbd_version version;

    uint64_t archs;
    uint32_t flags_field;

    enum tbd_platform platform;
    enum tbd_objc_constraint objc_constraint;
    
    const char *install_name;
    const char *parent_umbrella;

    uint32_t install_name_length;
    uint32_t parent_umbrella_length;

    uint32_t current_version;
    uint32_t compatibility_version;
    uint32_t swift_version;

    struct array exports;
    struct array uuids;

    uint64_t flags;
};

void tbd_create_info_destroy(struct tbd_create_info *info);

enum tbd_create_result {
    E_TBD_CREATE_OK,
    E_TBD_CREATE_WRITE_FAIL
};

enum tbd_create_options {
    O_TBD_CREATE_IGNORE_CURRENT_VERSION       = 1 << 0,
    O_TBD_CREATE_IGNORE_COMPATIBILITY_VERSION = 1 << 1,
    O_TBD_CREATE_IGNORE_EXPORTS               = 1 << 2,
    O_TBD_CREATE_IGNORE_FLAGS                 = 1 << 3,
    O_TBD_CREATE_IGNORE_OBJC_CONSTRAINT       = 1 << 4,
    O_TBD_CREATE_IGNORE_PARENT_UMBRELLA       = 1 << 5,
    O_TBD_CREATE_IGNORE_SWIFT_VERSION         = 1 << 6,
    O_TBD_CREATE_IGNORE_UUIDS                 = 1 << 7,
    O_TBD_CREATE_IGNORE_UNNECESSARY_FIELDS    = 1 << 8
};

enum tbd_create_result
tbd_create_with_info(const struct tbd_create_info *info,
                     FILE *file,
                     uint64_t options);

#endif /* TBD_H */
