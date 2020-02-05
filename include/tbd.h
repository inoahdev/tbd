//
//  include/tbd.h
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#ifndef TBD_H
#define TBD_H

#include <stdint.h>
#include <stdio.h>

#include "arch_info.h"
#include "array.h"

#include "bit_list.h"
#include "notnull.h"
#include "target_list.h"

/*
 * Options to handle when parsing out information for tbd_create_info.
 */

struct tbd_parse_options {
    bool ignore_clients : 1;
    bool ignore_current_version : 1;
    bool ignore_compat_version : 1;
    bool ignore_exports : 1;
    bool ignore_flags : 1;
    bool ignore_install_name : 1;
    bool ignore_objc_constraint : 1;
    bool ignore_parent_umbrellas : 1;
    bool ignore_platform : 1;
    bool ignore_reexports : 1;
    bool ignore_swift_version : 1;
    bool ignore_targets : 1;
    bool ignore_undefineds : 1;
    bool ignore_uuids : 1;

    /*
     * Options dictating what types of symbols should also be allowed in
     * addition to the default types.
     */

    bool allow_priv_objc_class_syms : 1;
    bool allow_priv_objc_ehtype_syms : 1;
    bool allow_priv_objc_ivar_syms : 1;

    bool ignore_missing_exports : 1;
    bool ignore_missing_uuids : 1;
    bool ignore_non_unique_uuids : 1;
};

struct tbd_flags {
    union {
        uint32_t value;

        struct {
            bool flat_namespace : 1;
            bool not_app_extension_safe : 1;
        };
    };
};

enum tbd_objc_constraint {
    TBD_OBJC_CONSTRAINT_NO_VALUE,

    TBD_OBJC_CONSTRAINT_NONE,
    TBD_OBJC_CONSTRAINT_GC,
    TBD_OBJC_CONSTRAINT_RETAIN_RELEASE,
    TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_OR_GC,
    TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_FOR_SIMULATOR
};

enum tbd_platform {
    TBD_PLATFORM_NONE,

    TBD_PLATFORM_MACOS,
    TBD_PLATFORM_IOS,
    TBD_PLATFORM_TVOS,
    TBD_PLATFORM_WATCHOS,

    /*
     * Apple's mach-o/loader.h doesn't yet contain this platform, even though
     * this platform exists in mach-o files, and so must be manually supported.
     */

    TBD_PLATFORM_BRIDGEOS,
    TBD_PLATFORM_IOSMAC,

    /*
     * Apple's mach-o/loader.h doesn't yet contain the simulator platforms, but
     * they are supported in mach-o files, and so are provided here.
     */

    TBD_PLATFORM_IOS_SIMULATOR,
    TBD_PLATFORM_TVOS_SIMULATOR,
    TBD_PLATFORM_WATCHOS_SIMULATOR,
    TBD_PLATFORM_DRIVERKIT
};

enum tbd_version {
    TBD_VERSION_NONE,

    TBD_VERSION_V1,
    TBD_VERSION_V2,
    TBD_VERSION_V3,
    TBD_VERSION_V4
};

const char *tbd_version_to_string(enum tbd_version version);

const char *
tbd_platform_to_string(enum tbd_platform platform, enum tbd_version version);

enum tbd_metadata_type {
    TBD_METADATA_TYPE_NONE,

    TBD_METADATA_TYPE_PARENT_UMBRELLA,
    TBD_METADATA_TYPE_CLIENT,
    TBD_METADATA_TYPE_REEXPORTED_LIBRARY,
};

enum tbd_symbol_meta_type {
    TBD_SYMBOL_META_TYPE_NONE,

    TBD_SYMBOL_META_TYPE_EXPORT,
    TBD_SYMBOL_META_TYPE_REEXPORT,
    TBD_SYMBOL_META_TYPE_UNDEFINED
};

enum tbd_symbol_type {
    TBD_SYMBOL_TYPE_NONE,

    TBD_SYMBOL_TYPE_CLIENT,
    TBD_SYMBOL_TYPE_REEXPORT,
    TBD_SYMBOL_TYPE_NORMAL,
    TBD_SYMBOL_TYPE_OBJC_CLASS,
    TBD_SYMBOL_TYPE_OBJC_EHTYPE,
    TBD_SYMBOL_TYPE_OBJC_IVAR,
    TBD_SYMBOL_TYPE_WEAK_DEF,
    TBD_SYMBOL_TYPE_THREAD_LOCAL
};

struct tbd_data_info_flags {
    bool needs_quotes : 1;
};

struct tbd_metadata_info {
    struct bit_list targets;

    char *string;
    uint64_t length;

    enum tbd_metadata_type type;
    struct tbd_data_info_flags flags;
};

struct tbd_symbol_info {
    struct bit_list targets;

    char *string;
    uint64_t length;

    enum tbd_symbol_meta_type meta_type;
    enum tbd_symbol_type type;

    struct tbd_data_info_flags flags;
};

struct tbd_uuid_info {
    uint64_t target;
    uint8_t uuid[16];
};

bool
tbd_should_parse_objc_constraint(struct tbd_parse_options options,
                                 enum tbd_version version);

bool
tbd_should_parse_swift_version(struct tbd_parse_options options,
                               enum tbd_version version);

bool
tbd_should_parse_flags(struct tbd_parse_options options,
                       enum tbd_version version);

bool tbd_uses_archs(enum tbd_version version);

struct tbd_create_info_flags {
    bool install_name_needs_quotes : 1;
    bool install_name_was_allocated : 1;

    /*
     * Indicte that all exports have the same targets as the tbd.
     *
     * This can lead to a performance boost as a different function is used that
     * does not check for targets.
     */

    bool uses_full_targets : 1;
};

struct tbd_create_info_fields {
    struct target_list targets;

    struct {
        enum tbd_objc_constraint objc_constraint;
    } archs;

    struct tbd_flags flags;

    const char *install_name;
    uint64_t install_name_length;

    uint32_t current_version;
    uint32_t compatibility_version;
    uint32_t swift_version;

    struct array metadata;
    struct array symbols;
    struct array uuids;
};

struct tbd_create_info {
    enum tbd_version version;

    struct tbd_create_info_fields fields;
    struct tbd_create_info_flags flags;
};

enum tbd_ci_set_target_count_result {
    E_TBD_CI_SET_TARGET_COUNT_OK,
    E_TBD_CI_SET_TARGET_COUNT_ALLOC_FAIL
};

enum tbd_ci_set_target_count_result
tbd_ci_set_target_count(struct tbd_create_info *__notnull info_in,
                        uint64_t count);

enum tbd_ci_add_parent_umbrella_result {
    E_TBD_CI_ADD_PARENT_UMBRELLA_OK,

    E_TBD_CI_ADD_PARENT_UMBRELLA_ALLOC_FAIL,
    E_TBD_CI_ADD_PARENT_UMBRELLA_ARRAY_FAIL,

    E_TBD_CI_ADD_PARENT_UMBRELLA_INFO_CONFLICT
};

enum tbd_ci_add_parent_umbrella_result
tbd_ci_add_parent_umbrella(struct tbd_create_info *__notnull info_in,
                           const char *__notnull string,
                           uint64_t length,
                           uint64_t arch_index,
                           struct tbd_parse_options options);

struct tbd_metadata_info *
tbd_ci_get_single_parent_umbrella(const struct tbd_create_info *__notnull info);

enum tbd_ci_add_data_result {
    E_TBD_CI_ADD_DATA_OK,

    E_TBD_CI_ADD_DATA_ALLOC_FAIL,
    E_TBD_CI_ADD_DATA_ARRAY_FAIL,
};

enum tbd_ci_add_data_result
tbd_ci_add_metadata(struct tbd_create_info *__notnull info_in,
                    const char *__notnull string,
                    uint64_t length,
                    uint64_t arch_index,
                    enum tbd_metadata_type type,
                    bool copy_string,
                    struct tbd_parse_options options);

enum tbd_ci_add_data_result
tbd_ci_add_symbol_with_type(struct tbd_create_info *__notnull info_in,
                            const char *__notnull string,
                            uint64_t length,
                            uint64_t arch_index,
                            enum tbd_symbol_type type,
                            enum tbd_symbol_meta_type meta_type,
                            struct tbd_parse_options options);

enum tbd_ci_add_data_result
tbd_ci_add_symbol_with_info(struct tbd_create_info *__notnull info_in,
                            const char *__notnull string,
                            uint64_t max_len,
                            uint64_t arch_index,
                            enum tbd_symbol_type predefined_type,
                            enum tbd_symbol_meta_type meta_type,
                            bool is_exported,
                            struct tbd_parse_options options);

enum tbd_ci_add_data_result
tbd_ci_add_symbol_with_info_and_len(struct tbd_create_info *__notnull info_in,
                                    const char *__notnull string,
                                    uint64_t len,
                                    uint64_t arch_index,
                                    enum tbd_symbol_type predefined_type,
                                    enum tbd_symbol_meta_type meta_type,
                                    bool is_exported,
                                    struct tbd_parse_options options);


enum tbd_platform
tbd_ci_get_single_platform(const struct tbd_create_info *__notnull info);

void
tbd_ci_set_single_platform(struct tbd_create_info *__notnull info,
                           enum tbd_platform platform);

void tbd_ci_sort_info(struct tbd_create_info *__notnull info_in);

enum tbd_ci_add_uuid_result {
    E_TBD_CI_ADD_UUID_OK,
    E_TBD_CI_ADD_UUID_ARRAY_FAIL,
    E_TBD_CI_ADD_UUID_NON_UNIQUE_UUID
};

enum tbd_ci_add_uuid_result
tbd_ci_add_uuid(struct tbd_create_info *__notnull info_in,
                const struct arch_info *__notnull arch,
                enum tbd_platform platform,
                const uint8_t uuid[16]);

enum tbd_create_result {
    E_TBD_CREATE_OK,
    E_TBD_CREATE_WRITE_FAIL
};

struct tbd_create_options {
    union {
        uint32_t value;

        struct {
            bool ignore_clients : 1;
            bool ignore_current_version : 1;
            bool ignore_compat_version : 1;
            bool ignore_exports : 1;
            bool ignore_flags : 1;
            bool ignore_footer : 1;
            bool ignore_objc_constraint : 1;
            bool ignore_parent_umbrellas : 1;
            bool ignore_reexports : 1;
            bool ignore_swift_version : 1;
            bool ignore_undefineds : 1;
            bool ignore_uuids : 1;
            bool ignore_unnecessary_fields : 1;
            bool use_full_targets : 1;
        };
    };
};

enum tbd_create_result
tbd_create_with_info(const struct tbd_create_info *__notnull info,
                     FILE *__notnull file,
                     struct tbd_create_options options);

void
tbd_create_info_clear_fields_and_create_from(
    struct tbd_create_info *__notnull dst,
    const struct tbd_create_info *__notnull src);

void tbd_create_info_destroy(struct tbd_create_info *__notnull info);

#endif /* TBD_H */
