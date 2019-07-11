//
//  include/tbd.h
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
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
    O_TBD_PARSE_IGNORE_ARCHS                 = 1 << 0,
    O_TBD_PARSE_IGNORE_CLIENTS               = 1 << 1,
    O_TBD_PARSE_IGNORE_CURRENT_VERSION       = 1 << 2,
    O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION = 1 << 3,
    O_TBD_PARSE_IGNORE_FLAGS                 = 1 << 4,
    O_TBD_PARSE_IGNORE_INSTALL_NAME          = 1 << 5,
    O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT       = 1 << 6,
    O_TBD_PARSE_IGNORE_PARENT_UMBRELLA       = 1 << 7,
    O_TBD_PARSE_IGNORE_PLATFORM              = 1 << 8,
    O_TBD_PARSE_IGNORE_REEXPORTS             = 1 << 9,
    O_TBD_PARSE_IGNORE_SWIFT_VERSION         = 1 << 10,
    O_TBD_PARSE_IGNORE_SYMBOLS               = 1 << 11,
    O_TBD_PARSE_IGNORE_UUIDS                 = 1 << 12,

    /*
     * Options dictating what types of symbols should also be allowed in
     * addition to the default types.
     */

    O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS  = 1 << 13,
    O_TBD_PARSE_ALLOW_PRIVATE_OBJC_EHTYPE_SYMBOLS = 1 << 14,
    O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS   = 1 << 15,

    O_TBD_PARSE_IGNORE_MISSING_EXPORTS        = 1 << 16,
    O_TBD_PARSE_IGNORE_MISSING_IDENTIFICATION = 1 << 17,
    O_TBD_PARSE_IGNORE_MISSING_PLATFORM       = 1 << 18,
    O_TBD_PARSE_IGNORE_MISSING_UUIDS          = 1 << 19,
    O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS       = 1 << 20,

    O_TBD_PARSE_EXPORTS_HAVE_FULL_ARCHS = 1 << 21
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
    TBD_PLATFORM_TVOS,
    TBD_PLATFORM_WATCHOS,

    /*
     * Apple's mach-o/loader.h doesn't yet contain this platform, even though
     * this platform exists in mach-o files, and so must be manually supported.
     */

    TBD_PLATFORM_BRIDGEOS,

    /*
     * Apple's mach-o/loader.h doesn't yet contain the simulator platforms, but
     * they are supported in mach-o files, and so are provided here.
     */

    TBD_PLATFORM_IOS_SIMULATOR = 7,
    TBD_PLATFORM_TVOS_SIMULATOR,
    TBD_PLATFORM_WATCHOS_SIMULATOR,

    /*
     * The platforms below aren't yet supported in a mach-o file, however,
     * Apple's official libtapi supports these platforms, so they are included
     * here.
     */

    TBD_PLATFORM_IOSMAC = 6,
    TBD_PLATFORM_ZIPPERED = 10
};

enum tbd_export_type {
    TBD_EXPORT_TYPE_CLIENT = 1,
    TBD_EXPORT_TYPE_REEXPORT,
    TBD_EXPORT_TYPE_NORMAL_SYMBOL,
    TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL,
    TBD_EXPORT_TYPE_OBJC_EHTYPE_SYMBOL,
    TBD_EXPORT_TYPE_OBJC_IVAR_SYMBOL,
    TBD_EXPORT_TYPE_WEAK_DEF_SYMBOL
};

enum tbd_export_info_flags {
    F_TBD_EXPORT_INFO_STRING_NEEDS_QUOTES = 1 << 0
};

struct tbd_export_info {
    uint64_t archs;
    uint64_t archs_count;

    enum tbd_export_type type;
    char *string;

    uint32_t length;
    uint64_t flags;
};

int
tbd_export_info_compare(const struct tbd_export_info *const array_item,
                        const struct tbd_export_info *const item);

int tbd_export_info_comparator(const void *array_item, const void *item);
int tbd_export_info_no_archs_comparator(const void *array_item,
                                        const void *item);

struct tbd_uuid_info {
    const struct arch_info *arch;
    uint8_t uuid[16];
};

int tbd_uuid_info_comparator(const void *array_item, const void *item);
int tbd_uuid_info_is_unique_comparator(const void *array_item,
                                       const void *item);

enum tbd_version {
    TBD_VERSION_V1 = 1,
    TBD_VERSION_V2,
    TBD_VERSION_V3
};

enum tbd_create_info_flags {
    F_TBD_CREATE_INFO_INSTALL_NAME_NEEDS_QUOTES    = 1 << 0,
    F_TBD_CREATE_INFO_PARENT_UMBRELLA_NEEDS_QUOTES = 1 << 1,

    F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED    = 1 << 2,
    F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED = 1 << 3,

    /*
     * Indicte that all exports have the same arch-set as the tbd.
     *
     * This can lead to a performance boost as a different function is used that
     * does not check for archs.
     */

    F_TBD_CREATE_INFO_EXPORTS_HAVE_FULL_ARCHS = 1 << 4
};

struct tbd_create_info {
    enum tbd_version version;

    uint64_t archs;
    uint64_t archs_count;

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

void tbd_create_info_destroy(struct tbd_create_info *info);

#endif /* TBD_H */
