//
//  src/macho_file_parse_single_lc.c
//  tbd
//
//  Created by inoahdev on 12/10/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include <string.h>
#include "copy.h"

#include "macho_file_parse_single_lc.h"
#include "macho_file.h"

#include "swap.h"
#include "tbd.h"
#include "yaml.h"

static bool
call_callback(const macho_file_parse_error_callback callback,
              struct tbd_create_info *__notnull const info_in,
              const enum macho_file_parse_error error,
              void *const cb_info)
{
    if (callback != NULL) {
        if (callback(info_in, error, cb_info)) {
            return true;
        }
    }

    return false;
}

static enum macho_file_parse_result
add_export_to_info(struct tbd_create_info *__notnull const info_in,
                   const uint64_t arch_index,
                   const enum tbd_symbol_type type,
                   const char *__notnull const string,
                   const uint32_t length,
                   const uint64_t tbd_options)
{
    const enum tbd_ci_add_data_result add_export_result =
        tbd_ci_add_symbol_with_type(info_in,
                                    string,
                                    length,
                                    arch_index,
                                    type,
                                    TBD_SYMBOL_META_TYPE_EXPORT,
                                    tbd_options);

    if (add_export_result != E_TBD_CI_ADD_DATA_OK) {
        return E_MACHO_FILE_PARSE_CREATE_SYMBOL_LIST_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static inline bool
set_platform_if_allowed(const enum tbd_platform parsed_platform,
                        const enum tbd_platform platform,
                        const uint64_t flags,
                        enum tbd_platform *__notnull const platform_out)
{
    if (!(flags & F_MF_PARSE_SLC_FOUND_BUILD_VERSION)) {
        *platform_out = platform;
        return true;
    }

    if (parsed_platform == TBD_PLATFORM_NONE) {
        *platform_out = platform;
        return true;
    }

    if (parsed_platform == platform) {
        return true;
    }

    return false;
}

static inline bool is_mac_or_catalyst_platform(const enum tbd_platform platform)
{
    switch (platform) {
        case TBD_PLATFORM_NONE:
        case TBD_PLATFORM_IOS:
        case TBD_PLATFORM_TVOS:
        case TBD_PLATFORM_WATCHOS:
        case TBD_PLATFORM_BRIDGEOS:
        case TBD_PLATFORM_ZIPPERED:
        case TBD_PLATFORM_IOS_SIMULATOR:
        case TBD_PLATFORM_TVOS_SIMULATOR:
        case TBD_PLATFORM_WATCHOS_SIMULATOR:
            return false;

        case TBD_PLATFORM_MACOS:
        case TBD_PLATFORM_MACCATALYST:
            return true;
    }
}

static enum macho_file_parse_result
parse_bv_platform(
    struct macho_file_parse_single_lc_info *__notnull const parse_info,
    struct tbd_create_info *__notnull const info_in,
    const struct load_command load_cmd,
    const uint8_t *const load_cmd_iter,
    const macho_file_parse_error_callback callback,
    void *const cb_info,
    const uint64_t tbd_options)
{
    if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Every build-version load-command should be large enough to store
     * the build-version load-command's basic information.
     */

    if (load_cmd.cmdsize < sizeof(struct build_version_command)) {
        return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
    }

    const struct build_version_command *const build_version =
        (const struct build_version_command *)load_cmd_iter;

    uint32_t platform = build_version->platform;
    if (parse_info->options & O_MF_PARSE_SLC_IS_BIG_ENDIAN) {
        platform = swap_uint32(platform);
    }

    switch (platform) {
        case TBD_PLATFORM_NONE:
        case TBD_PLATFORM_MACOS:
        case TBD_PLATFORM_IOS:
        case TBD_PLATFORM_TVOS:
        case TBD_PLATFORM_WATCHOS:
        case TBD_PLATFORM_BRIDGEOS:
        case TBD_PLATFORM_MACCATALYST:
        case TBD_PLATFORM_IOS_SIMULATOR:
        case TBD_PLATFORM_TVOS_SIMULATOR:
        case TBD_PLATFORM_WATCHOS_SIMULATOR:
        case TBD_PLATFORM_ZIPPERED:
            break;

        default: {
            const bool should_continue =
                call_callback(callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_INVALID_PLATFORM,
                              cb_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }

            return E_MACHO_FILE_PARSE_OK;
        }
    }

    const enum tbd_platform parsed_platform = *parse_info->platform_in;
    const uint64_t flags = *parse_info->flags_in;

    if (set_platform_if_allowed(parsed_platform,
                                platform,
                                flags,
                                parse_info->platform_in))
    {
        *parse_info->flags_in = (flags | F_MF_PARSE_SLC_FOUND_BUILD_VERSION);
        return E_MACHO_FILE_PARSE_OK;
    }

    if (tbd_uses_archs(info_in->version)) {
        const bool should_continue =
            call_callback(callback,
                          info_in,
                          ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT,
                          cb_info);

        if (!should_continue) {
            return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
        }

        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * We only allow duplicate, differing build-version platforms if they're
     * both either a macOS or macCatalyst platform.
     */

    if (is_mac_or_catalyst_platform(parsed_platform) ||
        is_mac_or_catalyst_platform(platform))
    {
        const uint64_t add_flags =
            (F_MF_PARSE_SLC_FOUND_BUILD_VERSION |
             F_MF_PARSE_SLC_FOUND_CATALYST_PLATFORM);

        *parse_info->flags_in = (flags | add_flags);
        *parse_info->platform_in = TBD_PLATFORM_MACCATALYST;

        return E_MACHO_FILE_PARSE_OK;
    }

    const bool should_continue =
        call_callback(callback,
                      info_in,
                      ERR_MACHO_FILE_PARSE_TARGET_PLATFORM_CONFLICT,
                      cb_info);

    if (!should_continue) {
        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static inline enum macho_file_parse_result
parse_version_min_lc(
    const struct macho_file_parse_single_lc_info *__notnull const parse_info,
    struct tbd_create_info *__notnull const info_in,
    const struct load_command load_cmd,
    const enum tbd_platform expected_platform,
    const macho_file_parse_error_callback callback,
    void *const cb_info,
    const uint64_t tbd_options)
{
    /*
     * If the platform isn't needed, skip the unnecessary parsing.
     */

    if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Ignore the version_min_* load-commands if we already found a
     * build-version load-command.
     */

    const uint64_t flags = *parse_info->flags_in;
    if (flags & F_MF_PARSE_SLC_FOUND_BUILD_VERSION) {
        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * All version_min load-commands should be the of the same cmdsize.
     */

    if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
        return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
    }

    const enum tbd_platform platform = *parse_info->platform_in;
    if (platform != TBD_PLATFORM_NONE) {
        if (platform != expected_platform) {
            const bool should_continue =
                call_callback(callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT,
                              cb_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }
    } else {
        *parse_info->platform_in = expected_platform;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_single_lc(
    struct macho_file_parse_single_lc_info *__notnull const parse_info,
    const macho_file_parse_error_callback callback,
    void *const cb_info)
{
    struct tbd_create_info *const info_in = parse_info->info_in;
    const uint64_t tbd_options = parse_info->tbd_options;

    const struct load_command load_cmd = parse_info->load_cmd;
    const uint8_t *const load_cmd_iter = parse_info->load_cmd_iter;

    switch (load_cmd.cmd) {
        case LC_BUILD_VERSION: {
            const enum macho_file_parse_result parse_platform_result =
                parse_bv_platform(parse_info,
                                  info_in,
                                  load_cmd,
                                  load_cmd_iter,
                                  callback,
                                  cb_info,
                                  tbd_options);

            if (parse_platform_result != E_MACHO_FILE_PARSE_OK) {
                return parse_platform_result;
            }

            break;
        }

        case LC_DYLD_INFO:
        case LC_DYLD_INFO_ONLY:
            /*
             * If exports aren't needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_EXPORTS) {
                break;
            }

            /*
             * All dyld_info load-commands should be of the same size.
             */

            if (load_cmd.cmdsize != sizeof(struct dyld_info_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            *parse_info->dyld_info_out =
                *(struct dyld_info_command *)load_cmd_iter;

            break;

        case LC_ID_DYLIB: {
            /*
             * If no information is needed, skip the unnecessary parsing.
             */

            const uint64_t ignore_all_mask =
                (O_TBD_PARSE_IGNORE_CURRENT_VERSION |
                 O_TBD_PARSE_IGNORE_COMPAT_VERSION |
                 O_TBD_PARSE_IGNORE_INSTALL_NAME);

            if ((tbd_options & ignore_all_mask) == ignore_all_mask) {
                *parse_info->flags_in |= F_MF_PARSE_SLC_FOUND_IDENTIFICATION;
                break;
            }

            /*
             * Make sure the dylib-command is large enough to store its basic
             * information.
             */

            if (load_cmd.cmdsize < sizeof(struct dylib_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            const struct dylib_command *const dylib_command =
                (const struct dylib_command *)load_cmd_iter;

            const uint64_t lc_parse_options = parse_info->options;
            uint32_t name_offset = dylib_command->dylib.name.offset;

            if (lc_parse_options & O_MF_PARSE_SLC_IS_BIG_ENDIAN) {
                name_offset = swap_uint32(name_offset);
            }

            /*
             * If our install-name is invalid, and our callback handles this for
             * us, we have to ignore the install-name, but still parse out the
             * current-version and compatibility-version.
             */

            bool ignore_install_name =
                (tbd_options & O_TBD_PARSE_IGNORE_INSTALL_NAME);

            const char *install_name = NULL;
            uint32_t length = 0;

            if (!ignore_install_name) {
                /*
                 * The install-name should be fully contained within the
                 * dylib-command, while not overlapping with the dylib-command's
                 * basic information.
                 */

                if (name_offset < sizeof(struct dylib_command) ||
                    name_offset >= load_cmd.cmdsize)
                {
                    const bool should_continue =
                        call_callback(callback,
                                      info_in,
                                      ERR_MACHO_FILE_PARSE_INVALID_INSTALL_NAME,
                                      cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    ignore_install_name = true;
                } else {
                    /*
                     * The install-name extends from its offset to the end of
                     * the dylib-command load-commmand.
                     */

                    const uint32_t max_length = load_cmd.cmdsize - name_offset;

                    install_name = (const char *)dylib_command + name_offset;
                    length = (uint32_t)strnlen(install_name, max_length);

                    if (length == 0) {
                        const bool should_continue =
                            call_callback(
                                callback,
                                info_in,
                                ERR_MACHO_FILE_PARSE_INVALID_INSTALL_NAME,
                                cb_info);

                        if (!should_continue) {
                            return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                        }

                        ignore_install_name = true;
                    }
                }
            }

            const struct dylib dylib = dylib_command->dylib;
            if (info_in->fields.install_name == NULL) {
                if (!(tbd_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION)) {
                    info_in->fields.current_version = dylib.current_version;
                }

                if (!(tbd_options & O_TBD_PARSE_IGNORE_COMPAT_VERSION)) {
                    const uint32_t compat_version = dylib.compatibility_version;
                    info_in->fields.compatibility_version = compat_version;
                }

                if (!ignore_install_name) {
                    if (lc_parse_options & O_MF_PARSE_SLC_COPY_STRINGS) {
                        install_name = alloc_and_copy(install_name, length);
                        if (install_name == NULL) {
                            return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                        }
                    }

                    const bool needs_quotes =
                        yaml_check_c_str(install_name, length);

                    if (needs_quotes) {
                        info_in->flags |=
                            F_TBD_CREATE_INFO_INSTALL_NAME_NEEDS_QUOTES;
                    }

                    info_in->fields.install_name = install_name;
                    info_in->fields.install_name_length = length;
                }
            } else {
                if (!(tbd_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION) &&
                    info_in->fields.current_version != dylib.current_version)
                {
                    const bool should_continue =
                        call_callback(
                            callback,
                            info_in,
                            ERR_MACHO_FILE_PARSE_CURRENT_VERSION_CONFLICT,
                            cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }
                }

                const uint32_t info_compat_version =
                    info_in->fields.compatibility_version;

                if (!(tbd_options & O_TBD_PARSE_IGNORE_COMPAT_VERSION) &&
                    info_compat_version != dylib.compatibility_version)
                {
                    const bool should_continue =
                        call_callback(
                            callback,
                            info_in,
                            ERR_MACHO_FILE_PARSE_COMPAT_VERSION_CONFLICT,
                            cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }
                }

                if (!ignore_install_name) {
                    if (info_in->fields.install_name_length != length) {
                        const bool should_continue =
                            call_callback(
                                callback,
                                info_in,
                                ERR_MACHO_FILE_PARSE_INSTALL_NAME_CONFLICT,
                                cb_info);

                        if (!should_continue) {
                            return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                        }
                    }

                    const char *const info_install_name =
                        info_in->fields.install_name;

                    if (memcmp(info_install_name, install_name, length) != 0) {
                        const bool should_continue =
                            call_callback(
                                callback,
                                info_in,
                                ERR_MACHO_FILE_PARSE_INSTALL_NAME_CONFLICT,
                                cb_info);

                        if (!should_continue) {
                            return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                        }

                        break;
                    }
                }
            }

            *parse_info->flags_in |= F_MF_PARSE_SLC_FOUND_IDENTIFICATION;
            break;
        }

        case LC_REEXPORT_DYLIB: {
            if (tbd_options & O_TBD_PARSE_IGNORE_REEXPORTS) {
                break;
            }

            /*
             * Make sure the dylib-command is large enough to store its basic
             * information.
             */

            if (load_cmd.cmdsize < sizeof(struct dylib_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            const struct dylib_command *const reexport_dylib =
                (const struct dylib_command *)load_cmd_iter;

            uint32_t reexport_offset = reexport_dylib->dylib.name.offset;
            if (parse_info->options & O_MF_PARSE_SLC_IS_BIG_ENDIAN) {
                reexport_offset = swap_uint32(reexport_offset);
            }

            /*
             * The re-export string should be fully contained within the
             * dylib-command, while not overlapping with the dylib-command's
             * basic information.
             */

            if (reexport_offset < sizeof(struct dylib_command) ||
                reexport_offset >= load_cmd.cmdsize)
            {
                return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
            }

            const char *const string =
                (const char *)reexport_dylib + reexport_offset;

            /*
             * The re-export string extends from its offset to the end of the
             * dylib-command load-commmand.
             */

            const uint32_t max_length = load_cmd.cmdsize - reexport_offset;
            const uint32_t length = (uint32_t)strnlen(string, max_length);

            if (length == 0) {
                return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
            }

            const enum macho_file_parse_result add_reexport_result =
                add_export_to_info(info_in,
                                   parse_info->arch_index,
                                   TBD_SYMBOL_TYPE_REEXPORT,
                                   string,
                                   length,
                                   tbd_options);

            if (add_reexport_result != E_MACHO_FILE_PARSE_OK) {
                return add_reexport_result;
            }

            break;
        }

        case LC_SUB_CLIENT: {
            /*
             * If no sub-clients are needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_CLIENTS) {
                break;
            }

            /*
             * Ensure that sub_client_command can fit its basic structure
             * and information.
             *
             * An exact match cannot be made as sub_client_command includes a
             * full client-string in its cmdsize.
             */

            if (load_cmd.cmdsize < sizeof(struct sub_client_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            /*
             * Ensure that the client is located fully within the load-command
             * and after the basic information of a sub_client_command
             * load-command structure.
             */

            const struct sub_client_command *const client_command =
                (const struct sub_client_command *)load_cmd_iter;

            uint32_t client_offset = client_command->client.offset;
            if (parse_info->options & O_MF_PARSE_SLC_IS_BIG_ENDIAN) {
                client_offset = swap_uint32(client_offset);
            }

            /*
             * Ensure the install-name is fully within the structure, and
             * doesn't overlap the basic structure of the client-command.
             */

            if (client_offset < sizeof(struct sub_client_command) ||
                client_offset >= load_cmd.cmdsize)
            {
                return E_MACHO_FILE_PARSE_INVALID_CLIENT;
            }

            const char *const string =
                (const char *)client_command + client_offset;

            /*
             * The client-string extends from its offset to the end of the
             * dylib-command load-commmand.
             */

            const uint32_t max_length = load_cmd.cmdsize - client_offset;
            const uint32_t length = (uint32_t)strnlen(string, max_length);

            if (length == 0) {
                return E_MACHO_FILE_PARSE_INVALID_CLIENT;
            }

            const enum macho_file_parse_result add_client_result =
                add_export_to_info(info_in,
                                   parse_info->arch_index,
                                   TBD_SYMBOL_TYPE_CLIENT,
                                   string,
                                   length,
                                   tbd_options);

            if (add_client_result != E_MACHO_FILE_PARSE_OK) {
                return add_client_result;
            }

            break;
        }

        case LC_SUB_FRAMEWORK: {
            /*
             * If no parent-umbrella is needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_PARENT_UMBRELLA) {
                break;
            }

            /*
             * Make sure the framework-command is large enough to store its
             * basic information.
             */

            if (load_cmd.cmdsize < sizeof(struct sub_framework_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            const struct sub_framework_command *const framework_command =
                (const struct sub_framework_command *)load_cmd_iter;

            const uint64_t lc_parse_options = parse_info->options;
            uint32_t umbrella_offset = framework_command->umbrella.offset;

            if (lc_parse_options & O_MF_PARSE_SLC_IS_BIG_ENDIAN) {
                umbrella_offset = swap_uint32(umbrella_offset);
            }

            /*
             * Ensure the umbrella-string is fully within the structure, and
             * doesn't overlap the basic structure of the umbrella-command.
             */

            if (umbrella_offset < sizeof(struct sub_framework_command) ||
                umbrella_offset >= load_cmd.cmdsize)
            {
                const bool should_continue =
                    call_callback(callback,
                                  info_in,
                                  ERR_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA,
                                  cb_info);

                if (!should_continue) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }

                break;
            }

            const char *const umbrella =
                (const char *)load_cmd_iter + umbrella_offset;

            /*
             * The umbrella-string is at the back of the load-command, extending
             * from the given offset to the end of the load-command.
             */

            const uint32_t max_length = load_cmd.cmdsize - umbrella_offset;
            const uint32_t length = (uint32_t)strnlen(umbrella, max_length);

            if (length == 0) {
                break;
            }

            const enum tbd_ci_add_parent_umbrella_result add_umbrella_result =
                tbd_ci_add_parent_umbrella(info_in,
                                           umbrella,
                                           length,
                                           parse_info->arch_index,
                                           tbd_options);

            switch (add_umbrella_result) {
                case E_TBD_CI_ADD_PARENT_UMBRELLA_OK:
                    break;

                case E_TBD_CI_ADD_PARENT_UMBRELLA_ALLOC_FAIL:
                case E_TBD_CI_ADD_PARENT_UMBRELLA_ARRAY_FAIL:
                    return E_MACHO_FILE_PARSE_CREATE_SYMBOL_LIST_FAIL;

                case E_TBD_CI_ADD_PARENT_UMBRELLA_INFO_CONFLICT: {
                    const bool should_continue =
                        call_callback(
                            callback,
                            info_in,
                            ERR_MACHO_FILE_PARSE_PARENT_UMBRELLA_CONFLICT,
                            cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    break;
                }
            }

            break;
        }

        case LC_SYMTAB: {
            /*
             * If exports and undefineds aren't needed, skip the unnecessary
             * parsing.
             */

            const uint64_t ignore_flags =
                (O_TBD_PARSE_IGNORE_EXPORTS | O_TBD_PARSE_IGNORE_UNDEFINEDS);

            if ((tbd_options & ignore_flags) == ignore_flags) {
                break;
            }

            /*
             * All symtab load-commands should be of the same size.
             */

            if (load_cmd.cmdsize != sizeof(struct symtab_command)) {
                return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
            }

            *parse_info->symtab_out =
                *(const struct symtab_command *)load_cmd_iter;

            break;
        }

        case LC_UUID: {
            /*
             * If uuids aren't needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_AT_AND_UUIDS) {
                break;
            }

            if (load_cmd.cmdsize != sizeof(struct uuid_command)) {
                const bool should_continue =
                    call_callback(callback,
                                  info_in,
                                  ERR_MACHO_FILE_PARSE_INVALID_UUID,
                                  cb_info);

                if (!should_continue) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }

                break;
            }

            const uint64_t flags = *parse_info->flags_in;
            const struct uuid_command *const uuid_cmd =
                (const struct uuid_command *)load_cmd_iter;

            if (flags & F_MF_PARSE_SLC_FOUND_UUID) {
                const char *const uuid_cmd_uuid = (const char *)uuid_cmd->uuid;
                const char *const uuid_str = (const char *)parse_info->uuid_in;

                if (memcmp(uuid_str, uuid_cmd_uuid, 16) != 0) {
                    const bool should_continue =
                        call_callback(callback,
                                      info_in,
                                      ERR_MACHO_FILE_PARSE_UUID_CONFLICT,
                                      cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    break;
                }
            } else {
                memcpy(parse_info->uuid_in, uuid_cmd->uuid, 16);
                *parse_info->flags_in = (flags | F_MF_PARSE_SLC_FOUND_UUID);
            }

            break;
        }

        case LC_VERSION_MIN_MACOSX: {
            const enum macho_file_parse_result parse_vm_result =
                parse_version_min_lc(parse_info,
                                     info_in,
                                     load_cmd,
                                     TBD_PLATFORM_MACOS,
                                     callback,
                                     cb_info,
                                     tbd_options);

            if (parse_vm_result != E_MACHO_FILE_PARSE_OK) {
                return parse_vm_result;
            }

            break;
        }

        case LC_VERSION_MIN_IPHONEOS: {
            const enum macho_file_parse_result parse_vm_result =
                parse_version_min_lc(parse_info,
                                     info_in,
                                     load_cmd,
                                     TBD_PLATFORM_IOS,
                                     callback,
                                     cb_info,
                                     tbd_options);

            if (parse_vm_result != E_MACHO_FILE_PARSE_OK) {
                return parse_vm_result;
            }

            break;
        }

        case LC_VERSION_MIN_WATCHOS: {
            const enum macho_file_parse_result parse_vm_result =
                parse_version_min_lc(parse_info,
                                     info_in,
                                     load_cmd,
                                     TBD_PLATFORM_WATCHOS,
                                     callback,
                                     cb_info,
                                     tbd_options);

            if (parse_vm_result != E_MACHO_FILE_PARSE_OK) {
                return parse_vm_result;
            }

            break;
        }

        case LC_VERSION_MIN_TVOS: {
            const enum macho_file_parse_result parse_vm_result =
                parse_version_min_lc(parse_info,
                                     info_in,
                                     load_cmd,
                                     TBD_PLATFORM_TVOS,
                                     callback,
                                     cb_info,
                                     tbd_options);

            if (parse_vm_result != E_MACHO_FILE_PARSE_OK) {
                return parse_vm_result;
            }

            break;
        }

        default:
            break;
    }

    return E_MACHO_FILE_PARSE_OK;
}
