//
//  src/macho_file_parse_load_commands.c
//  tbd
//
//  Created by inoahdev on 11/20/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <sys/types.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include "copy.h"

#include "guard_overflow.h"
#include "macho_file.h"
#include "objc.h"

#include "macho_file_parse_export_trie.h"
#include "macho_file_parse_load_commands.h"
#include "macho_file_parse_symtab.h"

#include "our_io.h"
#include "path.h"
#include "swap.h"

#include "tbd.h"
#include "yaml.h"

static inline bool segment_has_image_info_sect(const char name[16]) {
    const uint64_t first = *(const uint64_t *)name;
    switch (first) {
        /*
         * case "__DATA" with 2 null characters.
         */

        case 71830128058207: {
            const uint64_t second = *((const uint64_t *)name + 1);
            if (second == 0) {
                return true;
            }

            break;
        }

        /*
         * case "__DATA_D".
         */

        case 4926728347494670175: {
            const uint64_t second = *((const uint64_t *)name + 1);
            if (second == 1498698313) {
                /*
                 * if (second == "IRTY") with 4 null characters.
                 */

                return true;
            }

            break;
        }

        /*
         * case "__DATA_C".
         */

        case 4854670753456742239: {
            const uint64_t second = *((const uint64_t *)name + 1);
            if (second == 1414745679) {
                /*
                 * if (second == "ONST") with 4 null characters.
                 */

                return true;
            }

            break;
        }

        case 73986219138911: {
            /*
             * if name == "__OBJC".
             */

            const uint64_t second = *((const uint64_t *)name + 1);
            if (second == 0) {
                return true;
            }

            break;
        }

        default:
            break;
    }

    return false;
}

static inline bool is_image_info_section(const char name[16]) {
    const uint64_t first = *(const uint64_t *)name;
    switch (first) {
        /*
         * case "__image".
         */

        case 6874014074396041055: {
            const uint64_t second = *((const uint64_t *)name + 1);
            if (second == 1868983913) {
                /*
                 * if (second == "_info") with 3 null characters.
                 */

                return true;
            }

            break;
        }

        /*
         * case "__objc_im".
         */

        case 7592896805339094879: {
            const uint64_t second = *((const uint64_t *)name + 1);
            if (second == 8027224784786383213) {
                /*
                 * if (second == "ageinfo") with 1 null character.
                 */

                return true;
            }

            break;
        }

        default:
            break;
    }

    return false;
}

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
parse_section_from_file(struct tbd_create_info *__notnull const info_in,
                        uint32_t *__notnull const swift_version_in,
                        const int fd,
                        const struct range full_range,
                        const struct range macho_range,
                        const uint32_t sect_offset,
                        const uint64_t sect_size,
                        const off_t position,
                        const macho_file_parse_error_callback callback,
                        void *const cb_info,
                        const uint64_t tbd_options,
                        const uint64_t options)
{
    if (sect_size != sizeof(struct objc_image_info)) {
        return E_MACHO_FILE_PARSE_INVALID_SECTION;
    }

    const struct range sect_range = {
        .begin = sect_offset,
        .end = sect_offset + sect_size
    };

    if (!range_contains_other(macho_range, sect_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SECTION;
    }

    /*
     * Keep an offset to our original position, so we can seek the file back to
     * its original protection after we're done.
     */

    if (options & O_MACHO_FILE_PARSE_SECT_OFF_ABSOLUTE) {
        if (our_lseek(fd, sect_offset, SEEK_SET) < 0) {
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }
    } else {
        const off_t absolute = (off_t)(full_range.begin + sect_offset);
        if (our_lseek(fd, absolute, SEEK_SET) < 0) {
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }
    }

    struct objc_image_info image_info = {};
    if (our_read(fd, &image_info, sizeof(image_info)) < 0) {
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT)) {
        enum tbd_objc_constraint objc_constraint =
            TBD_OBJC_CONSTRAINT_RETAIN_RELEASE;

        if (image_info.flags & F_OBJC_IMAGE_INFO_REQUIRES_GC) {
            objc_constraint = TBD_OBJC_CONSTRAINT_GC;
        } else if (image_info.flags & F_OBJC_IMAGE_INFO_SUPPORTS_GC) {
            objc_constraint = TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_OR_GC;
        } else if (image_info.flags & F_OBJC_IMAGE_INFO_IS_FOR_SIMULATOR) {
            objc_constraint = TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_FOR_SIMULATOR;
        }

        const enum tbd_objc_constraint info_objc_constraint =
            info_in->fields.objc_constraint;

        if (info_objc_constraint != TBD_OBJC_CONSTRAINT_NO_VALUE) {
            if (info_objc_constraint != objc_constraint) {
                const bool should_ignore =
                    call_callback(callback,
                                  info_in,
                                  ERR_MACHO_FILE_PARSE_OBJC_CONSTRAINT_CONFLICT,
                                  cb_info);

                if (!should_ignore) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        } else {
            info_in->fields.objc_constraint = objc_constraint;
        }
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION)) {
        const uint32_t existing_swift_version = *swift_version_in;
        const uint32_t image_swift_version =
            (image_info.flags & OBJC_IMAGE_INFO_SWIFT_VERSION_MASK) >>
                OBJC_IMAGE_INFO_SWIFT_VERSION_SHIFT;

        if (existing_swift_version != 0) {
            if (existing_swift_version != image_swift_version) {
                const bool should_ignore =
                    call_callback(callback,
                                  info_in,
                                  ERR_MACHO_FILE_PARSE_SWIFT_VERSION_CONFLICT,
                                  cb_info);

                if (!should_ignore) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        } else {
            *swift_version_in = image_swift_version;
        }
    }

    if (our_lseek(fd, position, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
add_export_to_info(struct tbd_create_info *__notnull const info_in,
                   const uint64_t arch_bit,
                   const enum tbd_symbol_type type,
                   const char *__notnull const string,
                   const uint32_t length,
                   const uint64_t tbd_options)
{
    const enum tbd_ci_add_symbol_result add_export_result =
        tbd_ci_add_symbol_with_type(info_in,
                                    string,
                                    length,
                                    arch_bit,
                                    type,
                                    false,
                                    tbd_options);

    if (add_export_result != E_TBD_CI_ADD_SYMBOL_OK) {
        return E_MACHO_FILE_PARSE_CREATE_SYMBOLS_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum parse_load_commands_flags {
    F_PARSE_LOAD_COMMAND_COPY_STRINGS  = 1ull << 0,
    F_PARSE_LOAD_COMMAND_IS_BIG_ENDIAN = 1ull << 1
};

struct parse_load_command_info {
    struct tbd_create_info *info_in;
    struct tbd_uuid_info *uuid_info_in;

    const uint8_t *load_cmd_iter;
    struct load_command load_cmd;

    uint64_t arch_bit;
    uint64_t tbd_options;
    uint64_t options;

    /*
     * With the introduction of the maccatalyst platform, we can now have
     * multiple LC_VERSION_MIN_* load-commands in a single mach-o file.
     *
     * To detect the actual platform and other build-related information,
     * Apple introduced the LC_BUILD_VERSION load-command.
     *
     * We still parse LC_VERSION_MIN_* load-commands, but if we find a
     * LC_BUILD_VERSION load-command, the LC_VERSION_MIN_* load-commands are
     * ignored.
     */

    bool *found_build_version_in;
    bool *found_uuid_in;
    bool *found_identification_out;

    uint64_t flags;

    struct dyld_info_command *dyld_info_out;
    struct symtab_command *symtab_out;
};

static enum macho_file_parse_result
parse_load_command(
    const struct parse_load_command_info *__notnull const parse_info,
    const macho_file_parse_error_callback callback,
    void *const cb_info)
{
    struct tbd_create_info *const info_in = parse_info->info_in;

    const struct load_command load_cmd = parse_info->load_cmd;
    const uint8_t *const load_cmd_iter = parse_info->load_cmd_iter;

    const uint64_t lc_parse_flags = parse_info->flags;
    const uint64_t tbd_options = parse_info->tbd_options;

    switch (load_cmd.cmd) {
        case LC_BUILD_VERSION: {
            /*
             * Move on to the next load-command if the platform isn't needed.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
                break;
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
            if (lc_parse_flags & F_PARSE_LOAD_COMMAND_IS_BIG_ENDIAN) {
                platform = swap_uint32(platform);
            }

            if (platform < TBD_PLATFORM_MACOS ||
                platform > TBD_PLATFORM_WATCHOS_SIMULATOR)
            {
                const bool should_continue =
                    call_callback(callback,
                                  info_in,
                                  ERR_MACHO_FILE_PARSE_INVALID_PLATFORM,
                                  cb_info);

                if (!should_continue) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }

                break;
            }

            /*
             * For the sake of leniency, we error out only if we encounter a
             * different platform.
             */

            const enum tbd_platform info_platform = info_in->fields.platform;
            if (info_platform != TBD_PLATFORM_NONE && info_platform != platform)
            {
                if (*parse_info->found_build_version_in) {
                    const bool should_continue =
                        call_callback(callback,
                                      info_in,
                                      ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT,
                                      cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    break;
                }
            }

            info_in->fields.platform = platform;
            *parse_info->found_build_version_in = true;

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
                O_TBD_PARSE_IGNORE_CURRENT_VERSION |
                O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION |
                O_TBD_PARSE_IGNORE_INSTALL_NAME;

            if ((tbd_options & ignore_all_mask) == ignore_all_mask) {
                *parse_info->found_identification_out = true;
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

            uint32_t name_offset = dylib_command->dylib.name.offset;
            if (lc_parse_flags & F_PARSE_LOAD_COMMAND_IS_BIG_ENDIAN) {
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
                    * The install-name extends from its offset to the end of the
                    * dylib-command load-commmand.
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

                if (!(tbd_options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION)) {
                    const uint32_t compat_version = dylib.compatibility_version;
                    info_in->fields.compatibility_version = compat_version;
                }

                if (!ignore_install_name) {
                    if (lc_parse_flags & F_PARSE_LOAD_COMMAND_COPY_STRINGS) {
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

                const uint32_t compat_version = dylib.compatibility_version;
                if (!(tbd_options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION) &&
                    info_in->fields.compatibility_version != compat_version)
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

            *parse_info->found_identification_out = true;
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
            if (lc_parse_flags & F_PARSE_LOAD_COMMAND_IS_BIG_ENDIAN) {
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
                                   parse_info->arch_bit,
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
            if (lc_parse_flags & F_PARSE_LOAD_COMMAND_IS_BIG_ENDIAN) {
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
                                   parse_info->arch_bit,
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

            uint32_t umbrella_offset = framework_command->umbrella.offset;
            if (lc_parse_flags & F_PARSE_LOAD_COMMAND_IS_BIG_ENDIAN) {
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

            if (info_in->fields.parent_umbrella == NULL) {
                const char *umbrella_string = umbrella;
                if (lc_parse_flags & F_PARSE_LOAD_COMMAND_COPY_STRINGS) {
                    umbrella_string = alloc_and_copy(umbrella, length);
                    if (umbrella_string == NULL) {
                        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                    }

                    info_in->fields.parent_umbrella = umbrella_string;
                } else {
                    info_in->fields.parent_umbrella = umbrella;
                }

                const bool needs_quotes =
                    yaml_check_c_str(umbrella_string, length);

                if (needs_quotes) {
                    info_in->flags |=
                        F_TBD_CREATE_INFO_PARENT_UMBRELLA_NEEDS_QUOTES;
                }

                info_in->fields.parent_umbrella_length = length;
            } else {
                if (info_in->fields.parent_umbrella_length != length) {
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

                const char *const parent_umbrella =
                    info_in->fields.parent_umbrella;

                if (memcmp(parent_umbrella, umbrella, length) != 0) {
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

            if (tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS) {
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

            const bool found_uuid = *parse_info->found_uuid_in;
            const struct uuid_command *const uuid_cmd =
                (const struct uuid_command *)load_cmd_iter;

            if (found_uuid) {
                const char *const uuid_cmd_uuid = (const char *)uuid_cmd->uuid;
                const char *const uuid_str =
                    (const char *)parse_info->uuid_info_in->uuid;

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
                memcpy(parse_info->uuid_info_in->uuid, uuid_cmd->uuid, 16);
                *parse_info->found_uuid_in = true;
            }

            break;
        }

        case LC_VERSION_MIN_MACOSX: {
            /*
             * If the platform isn't needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
                break;
            }

            /*
             * Ignore the version_min_* load-commands if we already found a
             * build-version load-command.
             */

            const bool found_build_version =
                *parse_info->found_build_version_in;

            if (found_build_version) {
                break;
            }

            /*
             * All version_min load-commands should be the of the same cmdsize.
             */

            if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            if (info_in->fields.platform != TBD_PLATFORM_NONE) {
                if (info_in->fields.platform != TBD_PLATFORM_MACOS) {
                    const bool should_continue =
                        call_callback(callback,
                                      info_in,
                                      ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT,
                                      cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    break;
                }
            } else {
                info_in->fields.platform = TBD_PLATFORM_MACOS;
            }

            break;
        }

        case LC_VERSION_MIN_IPHONEOS: {
            /*
             * If the platform isn't needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
                break;
            }

            /*
             * Ignore the version_min_* load-commands if we already found a
             * build-version load-command.
             */

            const bool found_build_version =
                *parse_info->found_build_version_in;

            if (found_build_version) {
                break;
            }

            /*
             * All version_min load-commands should be the of the same cmdsize.
             */

            if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            const enum tbd_platform info_platform = info_in->fields.platform;
            if (info_platform != TBD_PLATFORM_NONE) {
                if (info_platform != TBD_PLATFORM_IOS) {
                    const bool should_continue =
                        call_callback(callback,
                                      info_in,
                                      ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT,
                                      cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    break;
                }
            } else {
                info_in->fields.platform = TBD_PLATFORM_IOS;
            }

            break;
        }

        case LC_VERSION_MIN_WATCHOS: {
            /*
             * If the platform isn't needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
                break;
            }

            /*
             * Ignore the version_min_* load-commands if we already found a
             * build-version load-command.
             */

            const bool found_build_version =
                *parse_info->found_build_version_in;

            if (found_build_version) {
                break;
            }

            /*
             * All version_min load-commands should be the of the same cmdsize.
             */

            if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            const enum tbd_platform info_platform = info_in->fields.platform;
            if (info_platform != TBD_PLATFORM_NONE) {
                if (info_platform != TBD_PLATFORM_WATCHOS) {
                    const bool should_continue =
                        call_callback(callback,
                                      info_in,
                                      ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT,
                                      cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    break;
                }
            } else {
                info_in->fields.platform = TBD_PLATFORM_WATCHOS;
            }

            break;
        }

        case LC_VERSION_MIN_TVOS: {
            /*
             * If the platform isn't needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
                break;
            }

            /*
             * Ignore the version_min_* load-commands if we already found a
             * build-version load-command.
             */

            const bool found_build_version =
                *parse_info->found_build_version_in;

            if (found_build_version) {
                break;
            }

            /*
             * All version_min load-commands should be the of the same cmdsize.
             */

            if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
            }

            const enum tbd_platform info_platform = info_in->fields.platform;
            if (info_platform != TBD_PLATFORM_NONE) {
                if (info_platform != TBD_PLATFORM_TVOS) {
                    const bool should_continue =
                        call_callback(callback,
                                      info_in,
                                      ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT,
                                      cb_info);

                    if (!should_continue) {
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    break;
                }
            } else {
                info_in->fields.platform = TBD_PLATFORM_TVOS;
            }

            break;
        }

        default:
            break;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static inline bool
should_parse_symtab(const uint64_t macho_options, const uint64_t tbd_options) {
    if (!(tbd_options & O_TBD_PARSE_IGNORE_UNDEFINEDS)) {
        return true;
    }

    if (macho_options & O_MACHO_FILE_PARSE_USE_SYMBOL_TABLE) {
        return true;
    }

    return false;
}

enum macho_file_parse_result
macho_file_parse_load_commands_from_file(
    struct tbd_create_info *__notnull const info_in,
    const struct mf_parse_lc_from_file_info *__notnull const parse_info,
    const struct macho_file_parse_extra_args extra,
    struct macho_file_symbol_lc_info_out *const sym_info_out)
{
    const uint32_t ncmds = parse_info->ncmds;
    if (ncmds == 0) {
        return E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS;
    }

    /*
     * Verify the size and integrity of the load-commands.
     */

    const uint32_t sizeofcmds = parse_info->sizeofcmds;
    if (sizeofcmds < sizeof(struct load_command)) {
        return E_MACHO_FILE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL;
    }

    uint32_t minimum_size = sizeof(struct load_command);
    if (guard_overflow_mul(&minimum_size, ncmds)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    if (sizeofcmds < minimum_size) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    /*
     * Ensure that sizeofcmds doesn't go past mach-o's size.
     */

    const struct range macho_range = parse_info->macho_range;
    const struct range available_range = parse_info->available_range;

    const uint64_t macho_size = range_get_size(macho_range);
    const uint64_t max_sizeofcmds = range_get_size(available_range);

    if (sizeofcmds > max_sizeofcmds) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const struct range relative_range = {
        .begin = parse_info->header_size,
        .end = macho_size
    };

    bool found_build_version = false;
    bool found_identification = false;
    bool found_uuid = false;

    struct dyld_info_command dyld_info = {};
    struct symtab_command symtab = {};
    struct tbd_uuid_info uuid_info = {};

    const struct arch_info *const arch = parse_info->arch;
    if (arch != NULL) {
        uuid_info.arch = arch;
    }

    /*
     * Allocate the entire load-commands buffer for better performance.
     */

    uint8_t *const load_cmd_buffer = (uint8_t *)malloc(sizeofcmds);
    if (load_cmd_buffer == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    const int fd = parse_info->fd;
    if (our_read(fd, load_cmd_buffer, sizeofcmds) < 0) {
        free(load_cmd_buffer);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    const uint64_t flags = parse_info->flags;

    const bool is_64 = (flags & F_MF_PARSE_LOAD_COMMANDS_IS_64);
    const bool is_big_endian = (flags & F_MF_PARSE_LOAD_COMMANDS_IS_BIG_ENDIAN);

    uint64_t parse_lc_flags = F_PARSE_LOAD_COMMAND_COPY_STRINGS;
    if (is_big_endian) {
        parse_lc_flags |= F_PARSE_LOAD_COMMAND_IS_BIG_ENDIAN;
    }

    /*
     * We end up copying our install-name and parent-umbrella, so we can setup
     * these flags beforehand.
     */

    const uint64_t alloc_flags =
        F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED |
        F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED;

    info_in->flags |= alloc_flags;

    const uint64_t arch_bit = parse_info->arch_bit;
    const uint64_t options = parse_info->options;
    const uint64_t tbd_options = parse_info->tbd_options;

    uint8_t *load_cmd_iter = load_cmd_buffer;

    /*
     * Mach-o load-commands are stored right after the mach-o header.
     */

    uint64_t lc_position = available_range.begin;
    uint32_t size_left = sizeofcmds;

    for (uint32_t i = 0; i != ncmds; i++) {
        /*
         * Verify that we still have space for a load-command.
         */

        if (size_left < sizeof(struct load_command)) {
            free(load_cmd_buffer);
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        struct load_command load_cmd = *(struct load_command *)load_cmd_iter;
        if (is_big_endian) {
            load_cmd.cmd = swap_uint32(load_cmd.cmd);
            load_cmd.cmdsize = swap_uint32(load_cmd.cmdsize);
        }

        /*
         * Verify the cmdsize by checking that a load-cmd can actually fit.
         *
         * We could also verify cmdsize meets the requirements for each type of
         * load-command, but we don't want to be too picky.
         */

        if (load_cmd.cmdsize < sizeof(struct load_command)) {
            free(load_cmd_buffer);
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        if (size_left < load_cmd.cmdsize) {
            free(load_cmd_buffer);
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        size_left -= load_cmd.cmdsize;

        /*
         * We can't check size_left here, as this could be the last
         * load-command, so we have to check at the very beginning of the loop.
         */

        switch (load_cmd.cmd) {
            case LC_SEGMENT: {
                /*
                 * If no information from a segment is needed, skip the
                 * unnecessary parsing.
                 */

                const uint64_t ignore_all_mask =
                    O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT |
                    O_TBD_PARSE_IGNORE_SWIFT_VERSION;

                if ((tbd_options & ignore_all_mask) == ignore_all_mask) {
                    break;
                }

                /*
                 * For the sake of leniency, we ignore segments of the wrong
                 * word-size.
                 */

                if (is_64) {
                    break;
                }

                if (load_cmd.cmdsize < sizeof(struct segment_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct segment_command *const segment =
                    (const struct segment_command *)load_cmd_iter;

                if (!segment_has_image_info_sect(segment->segname)) {
                    break;
                }

                uint32_t nsects = segment->nsects;
                if (nsects == 0) {
                    break;
                }

                if (is_big_endian) {
                    nsects = swap_uint32(nsects);
                }

                /*
                 * Verify the size and integrity of the sections.
                 */

                uint64_t sections_size = sizeof(struct section);
                if (guard_overflow_mul(&sections_size, nsects)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                const uint32_t max_sections_size =
                    load_cmd.cmdsize - sizeof(struct segment_command);

                if (sections_size > max_sections_size) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                uint32_t swift_version = 0;
                lc_position += sizeof(struct segment_command);

                const struct section *sect =
                    (const struct section *)(segment + 1);

                for (uint32_t j = 0; j != nsects; j++, sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    /*
                     * We have an empty section if our section-offset or
                     * section-size is zero.
                     */

                    uint32_t sect_offset = sect->offset;
                    uint32_t sect_size = sect->size;

                    if (sect_offset == 0 || sect_size == 0) {
                        continue;
                    }

                    if (is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint32(sect_size);
                    }

                    const enum macho_file_parse_result parse_section_result =
                        parse_section_from_file(info_in,
                                                &swift_version,
                                                fd,
                                                macho_range,
                                                relative_range,
                                                sect_offset,
                                                sect_size,
                                                (off_t)lc_position,
                                                extra.callback,
                                                extra.callback_info,
                                                tbd_options,
                                                options);

                    if (parse_section_result != E_MACHO_FILE_PARSE_OK) {
                        free(load_cmd_buffer);
                        return parse_section_result;
                    }

                    lc_position += sizeof(struct section);
                }

                break;
            }

            case LC_SEGMENT_64: {
                /*
                 * Move on the next load-command if no segment information is
                 * needed.
                 */

                const uint64_t ignore_all_mask =
                    O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT |
                    O_TBD_PARSE_IGNORE_SWIFT_VERSION;

                if ((tbd_options & ignore_all_mask) == ignore_all_mask) {
                    break;
                }

                /*
                 * For the sake of leniency, we ignore segments of the wrong
                 * word-size.
                 */

                if (!is_64) {
                    break;
                }

                if (load_cmd.cmdsize < sizeof(struct segment_command_64)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct segment_command_64 *const segment =
                    (const struct segment_command_64 *)load_cmd_iter;

                if (!segment_has_image_info_sect(segment->segname)) {
                    break;
                }

                uint32_t nsects = segment->nsects;
                if (nsects == 0) {
                    break;
                }

                if (is_big_endian) {
                    nsects = swap_uint32(nsects);
                }

                /*
                 * Verify the size and integrity of the sections-list.
                 */

                uint64_t sections_size = sizeof(struct section_64);
                if (guard_overflow_mul(&sections_size, nsects)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                const uint32_t max_sections_size =
                    load_cmd.cmdsize - sizeof(struct segment_command_64);

                if (sections_size > max_sections_size) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                uint32_t swift_version = 0;
                lc_position += sizeof(struct segment_command_64);

                const uint8_t *const sect_ptr = (const uint8_t *)(segment + 1);
                const struct section_64 *sect =
                    (const struct section_64 *)sect_ptr;

                for (uint32_t j = 0; j != nsects; j++, sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    /*
                     * We have an empty section if our section-offset or
                     * section-size is zero.
                     */

                    uint32_t sect_offset = sect->offset;
                    uint64_t sect_size = sect->size;

                    if (sect_offset == 0 || sect_size == 0) {
                        continue;
                    }

                    if (is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint64(sect_size);
                    }

                    const enum macho_file_parse_result parse_section_result =
                        parse_section_from_file(info_in,
                                                &swift_version,
                                                fd,
                                                macho_range,
                                                relative_range,
                                                sect_offset,
                                                sect_size,
                                                (off_t)lc_position,
                                                extra.callback,
                                                extra.callback_info,
                                                tbd_options,
                                                options);

                    if (parse_section_result != E_MACHO_FILE_PARSE_OK) {
                        free(load_cmd_buffer);
                        return parse_section_result;
                    }

                    lc_position += sizeof(struct section_64);
                }

                break;
            }

            default: {
                const struct parse_load_command_info parse_lc_info = {
                    .info_in = info_in,

                    .found_build_version_in = &found_build_version,
                    .found_uuid_in = &found_uuid,
                    .uuid_info_in = &uuid_info,

                    .arch_bit = arch_bit,

                    .load_cmd = load_cmd,
                    .load_cmd_iter = load_cmd_iter,

                    .tbd_options = tbd_options,
                    .options = options,

                    .flags = parse_lc_flags,

                    .found_identification_out = &found_identification,

                    .dyld_info_out = &dyld_info,
                    .symtab_out = &symtab
                };

                const enum macho_file_parse_result parse_load_command_result =
                    parse_load_command(&parse_lc_info,
                                       extra.callback,
                                       extra.callback_info);

                if (parse_load_command_result != E_MACHO_FILE_PARSE_OK) {
                    free(load_cmd_buffer);
                    return parse_load_command_result;
                }

                lc_position += load_cmd.cmdsize;
                break;
            }
        }

        load_cmd_iter += load_cmd.cmdsize;
    }

    free(load_cmd_buffer);
    if (!found_identification) {
        const bool should_continue =
            call_callback(extra.callback,
                          info_in,
                          ERR_MACHO_FILE_PARSE_NOT_A_DYNAMIC_LIBRARY,
                          extra.callback_info);

        if (!should_continue) {
            return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
        }
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
        if (!found_uuid) {
            if (!(tbd_options & O_TBD_PARSE_IGNORE_MISSING_UUIDS)) {
                const bool should_continue =
                    call_callback(extra.callback,
                                  info_in,
                                  ERR_MACHO_FILE_PARSE_NO_UUID,
                                  extra.callback_info);

                if (!should_continue) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        }

        /*
         * The uuid must be unique among all the uuids.
         */

        const uint8_t *const array_uuid =
            array_find_item(&info_in->fields.uuids,
                            sizeof(uuid_info),
                            &uuid_info,
                            tbd_uuid_info_is_unique_comparator,
                            NULL);

        if (array_uuid != NULL) {
            const bool should_continue =
                call_callback(extra.callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_UUID_CONFLICT,
                              extra.callback_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }

        const enum array_result add_uuid_info_result =
            array_add_item(&info_in->fields.uuids,
                           sizeof(uuid_info),
                           &uuid_info,
                           NULL);

        if (add_uuid_info_result != E_ARRAY_OK) {
            return E_MACHO_FILE_PARSE_ARRAY_FAIL;
        }
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_PLATFORM)) {
        if (info_in->fields.platform == TBD_PLATFORM_NONE) {
            const bool should_continue =
                call_callback(extra.callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_NO_PLATFORM,
                              extra.callback_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }
    }

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;

    bool parsed_dyld_info = false;
    bool parse_symtab = true;

    if (!(options & O_MACHO_FILE_PARSE_USE_SYMBOL_TABLE)) {
        if (dyld_info.export_off != 0 && dyld_info.export_size != 0) {
            if (is_big_endian) {
                dyld_info.export_off = swap_uint32(dyld_info.export_off);
                dyld_info.export_size = swap_uint32(dyld_info.export_size);
            }

            if (sym_info_out != NULL) {
                sym_info_out->dyld_info = dyld_info;
            }

            const uint64_t base_offset = macho_range.begin;
            const struct macho_file_parse_export_trie_args args = {
                .info_in = info_in,

                .arch = arch,
                .arch_bit = arch_bit,

                .available_range = available_range,

                .is_64 = is_64,
                .is_big_endian = is_big_endian,

                .dyld_info = dyld_info,
                .sb_buffer = extra.export_trie_sb,

                .tbd_options = tbd_options
            };

            ret = macho_file_parse_export_trie_from_file(args, fd, base_offset);

            parsed_dyld_info = true;
            parse_symtab = should_parse_symtab(options, tbd_options);

            if (symtab.nsyms == 0) {
                parse_symtab = false;
            }
        }
    } else if (symtab.nsyms == 0) {
        return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
    }

    if (parse_symtab) {
        if (is_big_endian) {
            symtab.symoff = swap_uint32(symtab.symoff);
            symtab.nsyms = swap_uint32(symtab.nsyms);

            symtab.stroff = swap_uint32(symtab.stroff);
            symtab.strsize = swap_uint32(symtab.strsize);
        }

        if (sym_info_out != NULL) {
            sym_info_out->symtab = symtab;
        }

        if (options & O_MACHO_FILE_PARSE_DONT_PARSE_EXPORTS) {
            return E_MACHO_FILE_PARSE_OK;
        }

        const uint64_t base_offset = macho_range.begin;
        const struct macho_file_parse_symtab_args args = {
            .info_in = info_in,
            .available_range = available_range,

            .arch_bit = arch_bit,
            .is_big_endian = is_big_endian,

            .symoff = symtab.symoff,
            .nsyms = symtab.nsyms,

            .stroff = symtab.stroff,
            .strsize = symtab.strsize,

            .tbd_options = tbd_options
        };

        if (is_64) {
            ret = macho_file_parse_symtab_64_from_file(&args, fd, base_offset);
        } else {
            ret = macho_file_parse_symtab_from_file(&args, fd, base_offset);
        }
    } else if (!parsed_dyld_info) {
        const uint64_t ignore_missing_flags =
            (O_TBD_PARSE_IGNORE_EXPORTS | O_TBD_PARSE_IGNORE_MISSING_EXPORTS);

        /*
         * If we have either O_TBD_PARSE_IGNORE_EXPORTS, or
         * O_TBD_PARSE_IGNORE_MISSING_EXPORTS, or both, we don't have an error.
         */

        if ((tbd_options & ignore_missing_flags) != 0) {
            return E_MACHO_FILE_PARSE_OK;
        }

        return E_MACHO_FILE_PARSE_NO_EXPORTS;
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return ret;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
parse_section_from_map(struct tbd_create_info *__notnull const info_in,
                       uint32_t *__notnull const existing_swift_version_in,
                       const struct range map_range,
                       const struct range macho_range,
                       const uint8_t *__notnull const map,
                       const uint8_t *__notnull const macho,
                       const uint32_t sect_offset,
                       const uint64_t sect_size,
                       const macho_file_parse_error_callback callback,
                       void *const cb_info,
                       const uint64_t tbd_options,
                       const uint64_t options)
{
    if (sect_size != sizeof(struct objc_image_info)) {
        return E_MACHO_FILE_PARSE_INVALID_SECTION;
    }

    const struct objc_image_info *image_info = NULL;
    const struct range sect_range = {
        .begin = sect_offset,
        .end = sect_offset + sect_size
    };

    if (options & O_MACHO_FILE_PARSE_SECT_OFF_ABSOLUTE) {
        if (!range_contains_other(map_range, sect_range)) {
            return E_MACHO_FILE_PARSE_INVALID_SECTION;
        }

        const void *const iter = map + sect_offset;
        image_info = (const struct objc_image_info *)iter;
    } else {
        if (!range_contains_other(macho_range, sect_range)) {
            return E_MACHO_FILE_PARSE_INVALID_SECTION;
        }

        const void *const iter = macho + sect_offset;
        image_info = (const struct objc_image_info *)iter;
    }

    const uint32_t flags = image_info->flags;

    if (!(tbd_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT)) {
        enum tbd_objc_constraint objc_constraint =
            TBD_OBJC_CONSTRAINT_RETAIN_RELEASE;

        if (flags & F_OBJC_IMAGE_INFO_REQUIRES_GC) {
            objc_constraint = TBD_OBJC_CONSTRAINT_GC;
        } else if (flags & F_OBJC_IMAGE_INFO_SUPPORTS_GC) {
            objc_constraint = TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_OR_GC;
        } else if (flags & F_OBJC_IMAGE_INFO_IS_FOR_SIMULATOR) {
            objc_constraint = TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_FOR_SIMULATOR;
        }

        const enum tbd_objc_constraint info_objc_constraint =
            info_in->fields.objc_constraint;

        if (info_objc_constraint != TBD_OBJC_CONSTRAINT_NO_VALUE) {
            if (info_objc_constraint != objc_constraint) {
                const bool should_continue =
                    call_callback(callback,
                                  info_in,
                                  ERR_MACHO_FILE_PARSE_OBJC_CONSTRAINT_CONFLICT,
                                  cb_info);

                if (!should_continue) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        } else {
            info_in->fields.objc_constraint = objc_constraint;
        }
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION)) {
        const uint32_t existing_swift_version = *existing_swift_version_in;
        const uint32_t image_swift_version =
            (flags & OBJC_IMAGE_INFO_SWIFT_VERSION_MASK) >>
                OBJC_IMAGE_INFO_SWIFT_VERSION_SHIFT;

        if (existing_swift_version != 0) {
            if (existing_swift_version != image_swift_version) {
                const bool should_continue =
                    call_callback(callback,
                                  info_in,
                                  ERR_MACHO_FILE_PARSE_SWIFT_VERSION_CONFLICT,
                                  cb_info);

                if (!should_continue) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        } else {
            *existing_swift_version_in = image_swift_version;
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_load_commands_from_map(
    struct tbd_create_info *__notnull const info_in,
    const struct mf_parse_lc_from_map_info *__notnull const parse_info,
    struct macho_file_parse_extra_args extra,
    struct macho_file_symbol_lc_info_out *const sym_info_out)
{
    const uint32_t ncmds = parse_info->ncmds;
    if (ncmds == 0) {
        return E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS;
    }

    /*
     * Verify the size and integrity of the load-commands.
     */

    const uint32_t sizeofcmds = parse_info->sizeofcmds;
    if (sizeofcmds < sizeof(struct load_command)) {
        return E_MACHO_FILE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL;
    }

    uint32_t minimum_size = sizeof(struct load_command);
    if (guard_overflow_mul(&minimum_size, ncmds)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    if (sizeofcmds < minimum_size) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint64_t flags = parse_info->flags;

    const bool is_64 = (flags & F_MF_PARSE_LOAD_COMMANDS_IS_64);
    const bool is_big_endian = (flags & F_MF_PARSE_LOAD_COMMANDS_IS_BIG_ENDIAN);

    const uint32_t header_size =
        (is_64) ?
            sizeof(struct mach_header_64) :
            sizeof(struct mach_header);

    const uint64_t macho_size = parse_info->macho_size;
    const uint32_t max_sizeofcmds = (uint32_t)(macho_size - header_size);

    if (sizeofcmds > max_sizeofcmds) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const struct range relative_range = {
        .begin = 0,
        .end = macho_size
    };

    bool found_build_version = false;
    bool found_identification = false;
    bool found_uuid = false;

    struct dyld_info_command dyld_info = {};
    struct symtab_command symtab = {};
    struct tbd_uuid_info uuid_info = { .arch = parse_info->arch };

    const uint8_t *const macho = parse_info->macho;
    const uint8_t *load_cmd_iter = macho + header_size;

    const uint64_t options = parse_info->options;
    const uint64_t tbd_options = parse_info->tbd_options;

    uint64_t parse_lc_flags = 0;
    if (options & O_MACHO_FILE_PARSE_COPY_STRINGS_IN_MAP) {
        info_in->flags |= F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED;
        info_in->flags |= F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED;

        parse_lc_flags |= F_PARSE_LOAD_COMMAND_COPY_STRINGS;
    }

    if (is_big_endian) {
        parse_lc_flags |= F_PARSE_LOAD_COMMAND_IS_BIG_ENDIAN;
    }

    const uint8_t *const map = parse_info->map;
    const uint64_t arch_bit = parse_info->arch_bit;

    const struct range available_map_range = parse_info->available_map_range;
    uint32_t size_left = sizeofcmds;

    for (uint32_t i = 0; i != ncmds; i++) {
        /*
         * Verify that we still have space for a load-command.
         */

        if (size_left < sizeof(struct load_command)) {
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        /*
         * Big-endian mach-o files will have big-endian load-commands.
         */

        struct load_command load_cmd =
            *(const struct load_command *)load_cmd_iter;

        if (is_big_endian) {
            load_cmd.cmd = swap_uint32(load_cmd.cmd);
            load_cmd.cmdsize = swap_uint32(load_cmd.cmdsize);
        }

        /*
         * Each load-command must be large enough to store the most basic
         * load-command information.
         *
         * For the sake of leniency, we don't check cmdsize further.
         */

        if (load_cmd.cmdsize < sizeof(struct load_command)) {
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        if (size_left < load_cmd.cmdsize) {
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        size_left -= load_cmd.cmdsize;

        /*
         * size_left cannot be checked here, because we don't care about
         * size_left on the last iteration.
         *
         * The verification instead happens on the next iteration.
         */

        switch (load_cmd.cmd) {
            case LC_SEGMENT: {
                /*
                 * Move on to the next load-command if we don't need any
                 * segment-information.
                 */

                const uint64_t ignore_all_mask =
                    O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT |
                    O_TBD_PARSE_IGNORE_SWIFT_VERSION;

                if ((tbd_options & ignore_all_mask) == ignore_all_mask) {
                    break;
                }

                /*
                 * For the sake of leniency, we ignore segments of the wrong
                 * word-size.
                 */

                if (is_64) {
                    break;
                }

                if (load_cmd.cmdsize < sizeof(struct segment_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct segment_command *const segment =
                    (const struct segment_command *)load_cmd_iter;

                if (!segment_has_image_info_sect(segment->segname)) {
                    break;
                }

                uint32_t nsects = segment->nsects;
                if (nsects == 0) {
                    break;
                }

                if (is_big_endian) {
                    nsects = swap_uint32(nsects);
                }

                /*
                 * Verify the size and integrity of the sections-list.
                 */

                uint64_t sections_size = sizeof(struct section);
                if (guard_overflow_mul(&sections_size, nsects)) {
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                const uint32_t max_sections_size =
                    load_cmd.cmdsize - sizeof(struct segment_command);

                if (sections_size > max_sections_size) {
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                uint32_t swift_version = 0;
                const struct section *sect =
                    (const struct section *)(segment + 1);

                for (uint32_t j = 0; j != nsects; j++, sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    /*
                     * We have an empty section if our section-offset or
                     * section-size is zero.
                     */

                    uint32_t sect_offset = sect->offset;
                    uint32_t sect_size = sect->size;

                    if (sect_offset == 0 || sect_size == 0) {
                        continue;
                    }

                    if (is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint32(sect_size);
                    }

                    const enum macho_file_parse_result parse_section_result =
                        parse_section_from_map(info_in,
                                               &swift_version,
                                               available_map_range,
                                               relative_range,
                                               map,
                                               macho,
                                               sect_offset,
                                               sect_size,
                                               extra.callback,
                                               extra.callback_info,
                                               tbd_options,
                                               options);

                    if (parse_section_result != E_MACHO_FILE_PARSE_OK) {
                        return parse_section_result;
                    }
                }

                break;
            }

            case LC_SEGMENT_64: {
                /*
                 * Move on to the next load-command if we don't need any
                 * segment-information.
                 */

                const uint64_t ignore_all_mask =
                    O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT |
                    O_TBD_PARSE_IGNORE_SWIFT_VERSION;

                if ((tbd_options & ignore_all_mask) == ignore_all_mask) {
                    break;
                }

                /*
                 * For the sake of leniency, we ignore segments of the wrong
                 * word-size.
                 */

                if (!is_64) {
                    break;
                }

                if (load_cmd.cmdsize < sizeof(struct segment_command_64)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct segment_command_64 *const segment =
                    (const struct segment_command_64 *)load_cmd_iter;

                if (!segment_has_image_info_sect(segment->segname)) {
                    break;
                }

                uint32_t nsects = segment->nsects;
                if (nsects == 0) {
                    break;
                }

                if (is_big_endian) {
                    nsects = swap_uint32(nsects);
                }

                /*
                 * Verify the size and integrity of the sections-list.
                 */

                uint64_t sections_size = sizeof(struct section_64);
                if (guard_overflow_mul(&sections_size, nsects)) {
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                const uint32_t max_sections_size =
                    load_cmd.cmdsize - sizeof(struct segment_command_64);

                if (sections_size > max_sections_size) {
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                uint32_t swift_version = 0;

                const uint8_t *const sect_ptr = (const uint8_t *)(segment + 1);
                const struct section_64 *sect =
                    (const struct section_64 *)sect_ptr;

                for (uint32_t j = 0; j != nsects; j++, sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    /*
                     * We have an empty section if our section-offset or
                     * section-size is zero.
                     */

                    uint32_t sect_offset = sect->offset;
                    uint64_t sect_size = sect->size;

                    if (sect_offset == 0 || sect_size == 0) {
                        continue;
                    }

                    if (is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint64(sect_size);
                    }

                    const enum macho_file_parse_result parse_section_result =
                        parse_section_from_map(info_in,
                                               &swift_version,
                                               available_map_range,
                                               relative_range,
                                               map,
                                               macho,
                                               sect_offset,
                                               sect_size,
                                               extra.callback,
                                               extra.callback_info,
                                               tbd_options,
                                               options);

                    if (parse_section_result != E_MACHO_FILE_PARSE_OK) {
                        return parse_section_result;
                    }
                }

                break;
            }

            default: {
                const struct parse_load_command_info parse_lc_info = {
                    .info_in = info_in,
                    .arch_bit = arch_bit,

                    .found_build_version_in = &found_build_version,
                    .found_uuid_in = &found_uuid,
                    .uuid_info_in = &uuid_info,

                    .load_cmd = load_cmd,
                    .load_cmd_iter = load_cmd_iter,

                    .tbd_options = tbd_options,
                    .options = options,

                    .flags = parse_lc_flags,

                    .found_identification_out = &found_identification,
                    .dyld_info_out = &dyld_info,
                    .symtab_out = &symtab
                };

                const enum macho_file_parse_result parse_load_command_result =
                    parse_load_command(&parse_lc_info,
                                       extra.callback,
                                       extra.callback_info);

                if (parse_load_command_result != E_MACHO_FILE_PARSE_OK) {
                    return parse_load_command_result;
                }

                break;
            }
        }

        load_cmd_iter += load_cmd.cmdsize;
    }

    if (!found_identification) {
        const bool should_continue =
            call_callback(extra.callback,
                          info_in,
                          ERR_MACHO_FILE_PARSE_NOT_A_DYNAMIC_LIBRARY,
                          extra.callback_info);

        if (!should_continue) {
            return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
        }
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
        if (!found_uuid) {
            const bool should_continue =
                call_callback(extra.callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_NO_UUID,
                              extra.callback_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }

        /*
         * The uuid must be unique among all the uuids found.
         */

        const uint8_t *const array_uuid =
            array_find_item(&info_in->fields.uuids,
                            sizeof(uuid_info),
                            &uuid_info,
                            tbd_uuid_info_is_unique_comparator,
                            NULL);

        if (array_uuid != NULL) {
            const bool should_continue =
                call_callback(extra.callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_UUID_CONFLICT,
                              extra.callback_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }

        const enum array_result add_uuid_info_result =
            array_add_item(&info_in->fields.uuids,
                           sizeof(uuid_info),
                           &uuid_info,
                           NULL);

        if (add_uuid_info_result != E_ARRAY_OK) {
            return E_MACHO_FILE_PARSE_ARRAY_FAIL;
        }
    }

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;

    bool parsed_dyld_info = false;
    bool parse_symtab = false;

    if (dyld_info.export_off != 0 && dyld_info.export_size != 0) {
        if (tbd_options & O_TBD_PARSE_IGNORE_EXPORTS) {
            return E_MACHO_FILE_PARSE_OK;
        }

        if (is_big_endian) {
            dyld_info.export_off = swap_uint32(dyld_info.export_off);
            dyld_info.export_size = swap_uint32(dyld_info.export_size);
        }

        if (sym_info_out) {
            sym_info_out->dyld_info = dyld_info;
        }

        const struct macho_file_parse_export_trie_args args = {
            .info_in = info_in,

            .arch = parse_info->arch,
            .arch_bit = arch_bit,

            .available_range = parse_info->available_map_range,

            .is_64 = is_64,
            .is_big_endian = is_big_endian,

            .dyld_info = dyld_info,
            .sb_buffer = extra.export_trie_sb,

            .tbd_options = tbd_options
        };

        ret = macho_file_parse_export_trie_from_map(args, map);

        parsed_dyld_info = true;
        parse_symtab = should_parse_symtab(options, tbd_options);
    } else {
        parse_symtab = true;
    }

    if (symtab.nsyms != 0) {
        if (is_big_endian) {
            symtab.symoff = swap_uint32(symtab.symoff);
            symtab.nsyms = swap_uint32(symtab.nsyms);

            symtab.stroff = swap_uint32(symtab.stroff);
            symtab.strsize = swap_uint32(symtab.strsize);
        }

        if (sym_info_out != NULL) {
            sym_info_out->symtab = symtab;
        }

        if (options & O_MACHO_FILE_PARSE_DONT_PARSE_EXPORTS) {
            return E_MACHO_FILE_PARSE_OK;
        }

        if (parse_symtab) {
            const struct macho_file_parse_symtab_args args = {
                .info_in = info_in,
                .available_range = parse_info->available_map_range,

                .arch_bit = arch_bit,
                .is_big_endian = is_big_endian,

                .symoff = symtab.symoff,
                .nsyms = symtab.nsyms,

                .stroff = symtab.stroff,
                .strsize = symtab.strsize,

                .tbd_options = tbd_options
            };

            if (is_64) {
                ret = macho_file_parse_symtab_64_from_map(&args, map);
            } else {
                ret = macho_file_parse_symtab_from_map(&args, map);
            }
        }
    } else if (!parsed_dyld_info) {
        const uint64_t ignore_missing_flags =
            (O_TBD_PARSE_IGNORE_EXPORTS | O_TBD_PARSE_IGNORE_MISSING_EXPORTS);

        /*
         * If we have either O_TBD_PARSE_IGNORE_EXPORTS, or
         * O_TBD_PARSE_IGNORE_MISSING_EXPORTS, or both, we don't have an error.
         */

        if ((tbd_options & ignore_missing_flags) != 0) {
            return E_MACHO_FILE_PARSE_OK;
        }

        return E_MACHO_FILE_PARSE_NO_EXPORTS;
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return ret;
    }

    return E_MACHO_FILE_PARSE_OK;
}
