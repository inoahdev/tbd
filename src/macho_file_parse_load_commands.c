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
#include "objc.h"

#include "macho_file_parse_load_commands.h"
#include "macho_file_parse_symbols.h"

#include "our_io.h"
#include "path.h"
#include "swap.h"

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

    if (our_lseek(fd, position, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

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
            return E_MACHO_FILE_PARSE_CONFLICTING_OBJC_CONSTRAINT;
        }
    } else {
        info_in->fields.objc_constraint = objc_constraint;
    }

    const uint32_t existing_swift_version = *swift_version_in;
    const uint32_t image_swift_version =
        (image_info.flags & OBJC_IMAGE_INFO_SWIFT_VERSION_MASK) >>
            OBJC_IMAGE_INFO_SWIFT_VERSION_SHIFT;

    if (existing_swift_version != 0) {
        if (existing_swift_version != image_swift_version) {
            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
        }
    } else {
        *swift_version_in = image_swift_version;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
add_export_to_info(struct tbd_create_info *__notnull const info_in,
                   const uint64_t arch_bit,
                   const enum tbd_export_type type,
                   const char *__notnull const string,
                   const uint32_t length,
                   const uint64_t tbd_options)
{
    struct tbd_export_info export_info = {
        .archs = arch_bit,
        .archs_count = 1,
        .length = length,
        .string = (char *)string,
        .type = type
    };

    if (tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS) {
        export_info.archs = info_in->fields.archs;
        export_info.archs_count = info_in->fields.archs_count;
    }

    struct array *const exports = &info_in->fields.exports;
    struct array_cached_index_info cached_info = {};

    struct tbd_export_info *const existing_info =
        array_find_item_in_sorted(exports,
                                  sizeof(export_info),
                                  &export_info,
                                  tbd_export_info_no_archs_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        if (tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS) {
            return E_MACHO_FILE_PARSE_OK;
        }

        const uint64_t archs = existing_info->archs;
        if (!(archs & arch_bit)) {
            existing_info->archs = archs | arch_bit;
            existing_info->archs_count += 1;
        }

        return E_MACHO_FILE_PARSE_OK;
    }

    export_info.string = alloc_and_copy(export_info.string, export_info.length);
    if (export_info.string == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    const bool needs_quotes =
        yaml_check_c_str(export_info.string, export_info.length);

    if (needs_quotes) {
        export_info.flags |= F_TBD_EXPORT_INFO_STRING_NEEDS_QUOTES;
    }

    const enum array_result add_export_info_result =
        array_add_item_with_cached_index_info(exports,
                                              sizeof(export_info),
                                              &export_info,
                                              &cached_info,
                                              NULL);

    if (add_export_info_result != E_ARRAY_OK) {
        free(export_info.string);
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}

struct parse_load_command_info {
    struct tbd_create_info *info_in;
    struct tbd_uuid_info *uuid_info_in;

    const uint8_t *load_cmd_iter;
    struct load_command load_cmd;

    bool copy_strings;
    bool is_big_endian;

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

    struct symtab_command *symtab_out;
};

static inline
enum macho_file_parse_result
parse_load_command(const struct parse_load_command_info parse_info) {
    struct tbd_create_info *const info_in = parse_info.info_in;
    const struct load_command load_cmd = parse_info.load_cmd;

    const uint64_t options = parse_info.options;
    const uint64_t tbd_options = parse_info.tbd_options;

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
                (const struct build_version_command *)parse_info.load_cmd_iter;

            uint32_t platform = build_version->platform;
            if (parse_info.is_big_endian) {
                platform = swap_uint32(platform);
            }

            if (platform < TBD_PLATFORM_MACOS ||
                platform > TBD_PLATFORM_WATCHOS_SIMULATOR)
            {
                /*
                 * Move on to the next load-commmand if we have to ignore
                 * invalid fields.
                 */

                if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                    break;
                }

                return E_MACHO_FILE_PARSE_INVALID_PLATFORM;
            }

            /*
             * For the sake of leniency, we error out only if we encounter
             * a different platform.
             */

            const enum tbd_platform info_platform = info_in->fields.platform;
            if (info_platform != TBD_PLATFORM_NONE &&
                info_platform != platform)
            {
                const bool found_build_version =
                    *parse_info.found_build_version_in;

                if (found_build_version) {
                    return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                }
            }

            info_in->fields.platform = platform;
            *parse_info.found_build_version_in = true;

            break;
        }

        case LC_ID_DYLIB: {
            /*
             * If no information is needed, skip the unnecessary parsing.
             */

            const uint64_t ignore_all_mask =
                O_TBD_PARSE_IGNORE_CURRENT_VERSION |
                O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION |
                O_TBD_PARSE_IGNORE_INSTALL_NAME;

            if ((tbd_options & ignore_all_mask) == ignore_all_mask) {
                *parse_info.found_identification_out = true;
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
                (const struct dylib_command *)parse_info.load_cmd_iter;

            uint32_t name_offset = dylib_command->dylib.name.offset;
            if (parse_info.is_big_endian) {
                name_offset = swap_uint32(name_offset);
            }

            /*
             * The install-name should be fully contained within the
             * dylib-command, while not overlapping with the dylib-command's
             * basic information.
             */

            if (name_offset < sizeof(struct dylib_command) ||
                name_offset >= load_cmd.cmdsize)
            {
                if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                    *parse_info.found_identification_out = true;
                    break;
                }

                return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
            }

            /*
             * The install-name extends from its offset to the end of the
             * dylib-command load-commmand.
             */

            const char *const name =
                (const char *)dylib_command + name_offset;

            const uint32_t max_length = load_cmd.cmdsize - name_offset;
            const uint32_t length = (uint32_t)strnlen(name, max_length);

            if (length == 0) {
                if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                    *parse_info.found_identification_out = true;
                    break;
                }

                return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
            }

            const struct dylib dylib = dylib_command->dylib;
            if (info_in->fields.install_name == NULL) {
                if (!(tbd_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION)) {
                    info_in->fields.current_version = dylib.current_version;
                }

                if (!(options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION)) {
                    const uint32_t compat_version = dylib.compatibility_version;
                    info_in->fields.compatibility_version = compat_version;
                }

                if (!(tbd_options & O_TBD_PARSE_IGNORE_INSTALL_NAME)) {
                    const char *install_name = name;
                    if (parse_info.copy_strings) {
                        install_name = alloc_and_copy(name, length);
                        if (install_name == NULL) {
                            return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                        }


                        info_in->fields.install_name = install_name;
                    } else {
                        info_in->fields.install_name = name;
                    }

                    const bool needs_quotes =
                        yaml_check_c_str(install_name, length);

                    if (needs_quotes) {
                        info_in->flags |=
                            F_TBD_CREATE_INFO_INSTALL_NAME_NEEDS_QUOTES;
                    }

                    info_in->fields.install_name_length = length;
                }
            } else {
                if (info_in->fields.current_version != dylib.current_version) {
                    return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                }

                const uint32_t compat_version = dylib.compatibility_version;
                if (info_in->fields.compatibility_version != compat_version) {
                    return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                }

                if (info_in->fields.install_name_length != length) {
                    return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                }

                if (memcmp(info_in->fields.install_name, name, length) != 0) {
                    return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                }
            }

            *parse_info.found_identification_out = true;
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
                (const struct dylib_command *)parse_info.load_cmd_iter;

            uint32_t reexport_offset = reexport_dylib->dylib.name.offset;
            if (parse_info.is_big_endian) {
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
                if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                    break;
                }

                return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
            }

            const enum macho_file_parse_result add_reexport_result =
                add_export_to_info(info_in,
                                   parse_info.arch_bit,
                                   TBD_EXPORT_TYPE_REEXPORT,
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
                (const struct sub_client_command *)parse_info.load_cmd_iter;

            uint32_t client_offset = client_command->client.offset;
            if (parse_info.is_big_endian) {
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
                if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                    break;
                }

                return E_MACHO_FILE_PARSE_INVALID_CLIENT;
            }

            const enum macho_file_parse_result add_client_result =
                add_export_to_info(info_in,
                                   parse_info.arch_bit,
                                   TBD_EXPORT_TYPE_CLIENT,
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
                (const struct sub_framework_command *)parse_info.load_cmd_iter;

            uint32_t umbrella_offset = framework_command->umbrella.offset;
            if (parse_info.is_big_endian) {
                umbrella_offset = swap_uint32(umbrella_offset);
            }

            /*
             * Ensure the umbrella-string is fully within the structure, and
             * doesn't overlap the basic structure of the umbrella-command.
             */

            if (umbrella_offset < sizeof(struct sub_framework_command) ||
                umbrella_offset >= load_cmd.cmdsize)
            {
                if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                    break;
                }

                return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
            }

            const char *const umbrella =
                (const char *)parse_info.load_cmd_iter + umbrella_offset;

            /*
             * The umbrella-string is at the back of the load-command, extending
             * from the given offset to the end of the load-command.
             */

            const uint32_t max_length = load_cmd.cmdsize - umbrella_offset;
            const uint32_t length = (uint32_t)strnlen(umbrella, max_length);

            if (length == 0) {
                if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                    break;
                }

                return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
            }

            if (info_in->fields.parent_umbrella == NULL) {
                const char *umbrella_string = umbrella;
                if (parse_info.copy_strings) {
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
                const char *const parent_umbrella =
                    info_in->fields.parent_umbrella;

                if (memcmp(parent_umbrella, umbrella, length) != 0) {
                    return E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA;
                }
            }

            break;
        }

        case LC_SYMTAB: {
            /*
             * If symbols aren't needed, skip the unnecessary parsing.
             */

            if (tbd_options & O_TBD_PARSE_IGNORE_SYMBOLS) {
                break;
            }

            /*
             * All symtab load-commands should be of the same size.
             */

            if (load_cmd.cmdsize != sizeof(struct symtab_command)) {
                return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
            }

            *parse_info.symtab_out =
                *(const struct symtab_command *)parse_info.load_cmd_iter;

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
                return E_MACHO_FILE_PARSE_INVALID_UUID;
            }

            const bool found_uuid = *parse_info.found_uuid_in;
            const struct uuid_command *const uuid_cmd =
                (const struct uuid_command *)parse_info.load_cmd_iter;

            if (found_uuid) {
                const char *const uuid_cmd_uuid = (const char *)uuid_cmd->uuid;
                const char *const uuid_str =
                    (const char *)parse_info.uuid_info_in->uuid;

                if (memcmp(uuid_str, uuid_cmd_uuid, 16) != 0) {
                    return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
                }
            } else {
                memcpy(parse_info.uuid_info_in->uuid, uuid_cmd->uuid, 16);
                *parse_info.found_uuid_in = true;
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

            const bool found_build_version = *parse_info.found_build_version_in;
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
                    return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
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

            const bool found_build_version = *parse_info.found_build_version_in;
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
                    return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
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

            const bool found_build_version = *parse_info.found_build_version_in;
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
                    return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
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

            const bool found_build_version = *parse_info.found_build_version_in;
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
                    return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
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

enum macho_file_parse_result
macho_file_parse_load_commands_from_file(
    struct tbd_create_info *__notnull const info_in,
    const struct mf_parse_lc_from_file_info *__notnull const parse_info,
    struct symtab_command *const symtab_out)
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

    const struct range full_range = parse_info->full_range;
    const struct range available_range = parse_info->available_range;

    const uint64_t macho_size = full_range.end - full_range.begin;
    const uint32_t max_sizeofcmds =
        (uint32_t)(available_range.end - available_range.begin);

    if (sizeofcmds > max_sizeofcmds) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint64_t header_size = available_range.begin - full_range.begin;
    const struct range relative_range = {
        .begin = header_size,
        .end = macho_size
    };

    bool found_build_version = false;
    bool found_identification = false;
    bool found_uuid = false;

    struct symtab_command symtab = {};
    struct tbd_uuid_info uuid_info = {};

    const struct arch_info *const arch = parse_info->arch;
    if (arch != NULL) {
        uuid_info.arch = arch;
    }

    /*
     * Allocate the entire load-commands buffer for better performance.
     */

    uint8_t *const load_cmd_buffer = malloc(sizeofcmds);
    if (load_cmd_buffer == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    const int fd = parse_info->fd;
    if (our_read(fd, load_cmd_buffer, sizeofcmds) < 0) {
        free(load_cmd_buffer);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    /*
     * We end up copying our install-name and parent-umbrella, so we can setup
     * these flags beforehand.
     */

    const uint64_t alloc_flags =
        F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED |
        F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED;

    info_in->flags |= alloc_flags;

    const bool is_64 = parse_info->is_64;
    const bool is_big_endian = parse_info->is_big_endian;

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
                                                full_range,
                                                relative_range,
                                                sect_offset,
                                                sect_size,
                                                (off_t)lc_position,
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
                                                full_range,
                                                relative_range,
                                                sect_offset,
                                                sect_size,
                                                (off_t)lc_position,
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
                    .copy_strings = true,

                    .load_cmd = load_cmd,
                    .load_cmd_iter = load_cmd_iter,

                    .is_big_endian = is_big_endian,
                    .tbd_options = tbd_options,
                    .options = options,

                    .found_identification_out = &found_identification,
                    .symtab_out = &symtab
                };

                const enum macho_file_parse_result parse_load_command_result =
                    parse_load_command(parse_lc_info);

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
        return E_MACHO_FILE_PARSE_NO_IDENTIFICATION;
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
        if (!found_uuid) {
            return E_MACHO_FILE_PARSE_NO_UUID;
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
            return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
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
        if (info_in->fields.platform == 0) {
            return E_MACHO_FILE_PARSE_NO_PLATFORM;
        }
    }

    if (symtab.cmd != LC_SYMTAB) {
        const uint64_t ignore_missing_flags =
            (O_TBD_PARSE_IGNORE_SYMBOLS | O_TBD_PARSE_IGNORE_MISSING_EXPORTS);

        /*
         * If we have either O_TBD_PARSE_IGNORE_SYMBOLS, or
         * O_TBD_PARSE_IGNORE_MISSING_EXPORTS, or both, we don't have an error.
         */

        if ((tbd_options & ignore_missing_flags) != 0) {
            return E_MACHO_FILE_PARSE_OK;
        }

        return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
    }

    if (is_big_endian) {
        symtab.symoff = swap_uint32(symtab.symoff);
        symtab.nsyms = swap_uint32(symtab.nsyms);

        symtab.stroff = swap_uint32(symtab.stroff);
        symtab.strsize = swap_uint32(symtab.strsize);
    }

    if (symtab_out != NULL) {
        *symtab_out = symtab;
    }

    if (options & O_MACHO_FILE_PARSE_DONT_PARSE_SYMBOL_TABLE) {
        return E_MACHO_FILE_PARSE_OK;
    }

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    const struct macho_file_parse_symbols_args args = {
        .info_in = info_in,

        .full_range = full_range,
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
        ret = macho_file_parse_symbols_64_from_file(args, fd);
    } else {
        ret = macho_file_parse_symbols_from_file(args, fd);
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
            return E_MACHO_FILE_PARSE_CONFLICTING_OBJC_CONSTRAINT;
        }
    } else {
        info_in->fields.objc_constraint = objc_constraint;
    }

    const uint32_t existing_swift_version = *existing_swift_version_in;
    const uint32_t image_swift_version =
        (flags & OBJC_IMAGE_INFO_SWIFT_VERSION_MASK) >>
            OBJC_IMAGE_INFO_SWIFT_VERSION_SHIFT;

    if (existing_swift_version != 0) {
        if (existing_swift_version != image_swift_version) {
            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
        }
    } else {
        *existing_swift_version_in = image_swift_version;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_load_commands_from_map(
    struct tbd_create_info *__notnull const info_in,
    const struct mf_parse_lc_from_map_info *__notnull const parse_info,
    struct symtab_command *const symtab_out)
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

    const bool is_64 = parse_info->is_64;
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

    struct symtab_command symtab = {};
    struct tbd_uuid_info uuid_info = { .arch = parse_info->arch };

    const uint8_t *const macho = parse_info->macho;

    const uint8_t *load_cmd_iter = macho + header_size;
    const uint64_t options = parse_info->options;

    if (options & O_MACHO_FILE_PARSE_COPY_STRINGS_IN_MAP) {
        info_in->flags |= F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED;
        info_in->flags |= F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED;
    }

    const bool is_big_endian = parse_info->is_big_endian;
    const uint64_t tbd_options = parse_info->tbd_options;

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
                    if (sect_offset == 0) {
                        continue;
                    }

                    uint32_t sect_size = sect->size;
                    if (sect_size == 0) {
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
                    if (sect_offset == 0) {
                        continue;
                    }

                    uint64_t sect_size = sect->size;
                    if (sect_size == 0) {
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

                    .is_big_endian = is_big_endian,
                    .tbd_options = tbd_options,
                    .options = options,

                    .copy_strings =
                        (options & O_MACHO_FILE_PARSE_COPY_STRINGS_IN_MAP),

                    .found_identification_out = &found_identification,
                    .symtab_out = &symtab
                };

                const enum macho_file_parse_result parse_load_command_result =
                    parse_load_command(parse_lc_info);

                if (parse_load_command_result != E_MACHO_FILE_PARSE_OK) {
                    return parse_load_command_result;
                }

                break;
            }
        }

        load_cmd_iter += load_cmd.cmdsize;
    }

    if (!found_identification) {
        return E_MACHO_FILE_PARSE_NO_IDENTIFICATION;
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
        if (!found_uuid) {
            return E_MACHO_FILE_PARSE_NO_UUID;
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
            return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
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

    if (symtab.cmd != LC_SYMTAB) {
        const uint64_t ignore_missing_flags =
            (O_TBD_PARSE_IGNORE_SYMBOLS | O_TBD_PARSE_IGNORE_MISSING_EXPORTS);

        /*
         * If we have either O_TBD_PARSE_IGNORE_SYMBOLS, or
         * O_TBD_PARSE_IGNORE_MISSING_EXPORTS, or both, we don't have an error.
         */

        if ((tbd_options & ignore_missing_flags) != 0) {
            return E_MACHO_FILE_PARSE_OK;
        }

        return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
    }

    if (is_big_endian) {
        symtab.symoff = swap_uint32(symtab.symoff);
        symtab.nsyms = swap_uint32(symtab.nsyms);

        symtab.stroff = swap_uint32(symtab.stroff);
        symtab.strsize = swap_uint32(symtab.strsize);
    }

    if (symtab_out != NULL) {
        *symtab_out = symtab;
    }

    if (options & O_MACHO_FILE_PARSE_DONT_PARSE_SYMBOL_TABLE) {
        return E_MACHO_FILE_PARSE_OK;
    }

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    const struct macho_file_parse_symbols_args args = {
        .info_in = info_in,
        .available_range = available_map_range,

        .arch_bit = arch_bit,
        .is_big_endian = is_big_endian,

        .symoff = symtab.symoff,
        .nsyms = symtab.nsyms,

        .stroff = symtab.stroff,
        .strsize = symtab.strsize
    };

    if (is_64) {
        ret = macho_file_parse_symbols_64_from_map(args, map);
    } else {
        ret = macho_file_parse_symbols_from_map(args, map);
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return ret;
    }

    return E_MACHO_FILE_PARSE_OK;
}
