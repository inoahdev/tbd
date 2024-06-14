//
//  src/macho_file_parse_load_commands.c
//  tbd
//
//  Created by inoahdev on 11/20/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "arch_info.h"
#include "copy.h"
#include "guard_overflow.h"
#include "macho_file.h"
#include "objc.h"

#include "macho_file_parse_export_trie.h"
#include "macho_file_parse_single_lc.h"
#include "macho_file_parse_load_commands.h"
#include "macho_file_parse_symtab.h"

#include "our_io.h"
#include "range.h"
#include "swap.h"
#include "tbd.h"
#include "yaml.h"

static inline bool segment_has_image_info_sect(const char name[const 16]) {
    const uint64_t first = *(const uint64_t *)name;
    const uint64_t *const second_ptr = (const uint64_t *)name + 1;

    switch (first) {
        /*
         * case "__DATA" with 2 null characters.
         */

        case 71830128058207: {
            const uint64_t second = *second_ptr;
            if (second == 0) {
                return true;
            }

            break;
        }

        /*
         * case "__DATA_D".
         */

        case 4926728347494670175: {
            const uint64_t second = *second_ptr;
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
            const uint64_t second = *second_ptr;
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

            const uint64_t second = *second_ptr;
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

static inline bool
call_callback(const macho_file_parse_error_callback callback,
              struct tbd_create_info *__notnull const info_in,
              const enum macho_file_parse_callback_type type,
              void *const cb_info)
{
    if (callback != NULL) {
        if (callback(info_in, type, cb_info)) {
            return true;
        }
    }

    return false;
}

static enum macho_file_parse_result
parse_section_from_file(struct tbd_create_info *__notnull const info_in,
                        uint32_t *__notnull const swift_version_in,
                        const int fd,
                        const uint64_t base,
                        const struct range macho_available_range,
                        const uint32_t sect_offset,
                        const uint64_t sect_size,
                        const off_t position,
                        const macho_file_parse_error_callback callback,
                        void *const cb_info,
                        const struct tbd_parse_options tbd_options,
                        const struct macho_file_parse_options options)
{
    if (sect_size != sizeof(struct objc_image_info)) {
        return E_MACHO_FILE_PARSE_INVALID_SECTION;
    }

    uint32_t sect_end = sect_offset;
    if (guard_overflow_add(&sect_end, sect_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SECTION;
    }

    const struct range sect_range = {
        .begin = sect_offset,
        .end = sect_end
    };

    if (!range_contains_other(macho_available_range, sect_range)) {
        return E_MACHO_FILE_PARSE_INVALID_SECTION;
    }

    /*
     * Keep an offset to our original position, so we can seek the file back to
     * its original protection after we're done.
     */

    if (options.sect_off_absolute) {
        if (our_lseek(fd, sect_offset, SEEK_SET) < 0) {
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }
    } else {
        const off_t absolute = (off_t)(base + sect_offset);
        if (our_lseek(fd, absolute, SEEK_SET) < 0) {
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }
    }

    struct objc_image_info image_info = {};
    if (our_read(fd, &image_info, sizeof(image_info)) < 0) {
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    if (tbd_should_parse_objc_constraint(tbd_options, info_in->version)) {
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
            info_in->fields.archs.objc_constraint;

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
            info_in->fields.archs.objc_constraint = objc_constraint;
        }
    }

    if (!tbd_options.ignore_swift_version) {
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

static inline bool
should_parse_symtab(const struct macho_file_parse_options macho_options,
                    const struct tbd_parse_options tbd_options)
{
    return (!tbd_options.ignore_undefineds || macho_options.use_symbol_table);
}

static enum macho_file_parse_result
handle_uuid(struct tbd_create_info *__notnull const info_in,
            const struct arch_info *__notnull const arch,
            const enum tbd_platform platform,
            const uint8_t uuid[const 16],
            __notnull const macho_file_parse_error_callback callback,
            void *const cb_info,
            const struct macho_file_parse_slc_flags parse_slc_flags,
            const struct tbd_parse_options tbd_options)
{
    if (!parse_slc_flags.found_uuid) {
        if (!tbd_options.ignore_missing_uuids) {
            const bool should_continue =
                call_callback(callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_NO_UUID,
                              cb_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }

        return E_MACHO_FILE_PARSE_OK;
    }

    const enum tbd_ci_add_uuid_result add_uuid_result =
        tbd_ci_add_uuid(info_in, arch, platform, uuid);

    switch (add_uuid_result) {
        case E_TBD_CI_ADD_UUID_OK:
            break;

        case E_TBD_CI_ADD_UUID_ARRAY_FAIL:
            return E_MACHO_FILE_PARSE_ALLOC_FAIL;

        case E_TBD_CI_ADD_UUID_NON_UNIQUE_UUID: {
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
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
handle_targets_platform_and_uuid(
    struct tbd_create_info *__notnull const info_in,
    const struct arch_info *const arch,
    enum tbd_platform platform,
    const uint8_t uuid[const 16],
    __notnull const macho_file_parse_error_callback callback,
    void *const cb_info,
    const struct macho_file_parse_slc_flags parse_slc_flags,
    const struct tbd_parse_options tbd_options)
{
    if (!tbd_options.ignore_targets) {
        if (platform == TBD_PLATFORM_NONE) {
            const bool should_continue =
                call_callback(callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_NO_PLATFORM,
                              cb_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }

        const bool has_target =
            target_list_has_target(&info_in->fields.targets, arch, platform);

        if (has_target) {
            return E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_PLATFORM;
        }

        const enum target_list_result add_target_result =
            target_list_add_target(&info_in->fields.targets,
                                    arch,
                                    platform);

        if (add_target_result != E_TARGET_LIST_OK) {
            return E_MACHO_FILE_PARSE_CREATE_TARGET_LIST_FAIL;
        }

        const enum macho_file_parse_result handle_uuid_result =
            handle_uuid(info_in,
                        arch,
                        platform,
                        uuid,
                        callback,
                        cb_info,
                        parse_slc_flags,
                        tbd_options);

        if (handle_uuid_result != E_MACHO_FILE_PARSE_OK) {
            return handle_uuid_result;
        }
    } else if (!tbd_options.ignore_platform) {
        if (platform == TBD_PLATFORM_NONE) {
            const bool should_continue =
                call_callback(callback,
                              info_in,
                              ERR_MACHO_FILE_PARSE_NO_PLATFORM,
                              cb_info);

            if (!should_continue) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }

        tbd_ci_set_single_platform(info_in, platform);
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_load_commands_from_file(
    struct tbd_create_info *__notnull const info_in,
    const struct mf_parse_lc_from_file_info *__notnull const parse_info,
    const struct macho_file_parse_extra_args extra,
    struct macho_file_lc_info_out *const lc_info_out)
{
    const uint32_t ncmds = parse_info->ncmds;
    if (ncmds == 0) {
        return E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS;
    }

    /*
     * Verify the size and integrity of the load-commands.
     */

    uint32_t minimum_size = sizeof(struct load_command);
    if (guard_overflow_mul(&minimum_size, ncmds)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint32_t sizeofcmds = parse_info->sizeofcmds;
    if (sizeofcmds < minimum_size) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    /*
     * Ensure that sizeofcmds doesn't go past mach-o's size.
     */

    const struct range macho_range = parse_info->macho_range;
    const struct range available_range_in = parse_info->available_range;

    const uint64_t macho_size = range_get_size(macho_range);
    const uint64_t max_sizeofcmds = range_get_size(available_range_in);

    if (sizeofcmds > max_sizeofcmds) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    uint32_t relative_begin = parse_info->header_size;
    if (guard_overflow_add(&relative_begin, sizeofcmds)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const struct range relative_range = {
        .begin = relative_begin,
        .end = macho_size
    };

    if (range_get_size(relative_range) == 0) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    uint64_t available_begin = available_range_in.begin;
    if (guard_overflow_add(&available_begin, sizeofcmds)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const struct range available_range = {
        .begin = available_begin,
        .end = available_range_in.end
    };

    enum tbd_platform platform = TBD_PLATFORM_NONE;
    struct symtab_command symtab = {};

    uint32_t export_off = 0;
    uint32_t export_size = 0;
    uint8_t uuid[16] = {};

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

    const struct macho_file_parse_lc_flags flags = parse_info->flags;

    struct macho_file_parse_slc_flags parse_slc_flags = {};
    struct macho_file_parse_slc_options parse_slc_opts =
        { .copy_strings = true };

    if (flags.is_big_endian) {
        parse_slc_flags.is_big_endian = true;
    }

    const struct tbd_parse_options tbd_options = parse_info->tbd_options;
    if (!tbd_options.ignore_install_name) {
        info_in->flags.install_name_was_allocated = true;
    }

    const uint64_t arch_index = parse_info->arch_index;
    const struct macho_file_parse_options options = parse_info->options;

    struct macho_file_parse_single_lc_info parse_lc_info = {
        .info_in = info_in,

        .flags_in = &parse_slc_flags,
        .platform_in = &platform,
        .uuid_in = uuid,

        .arch_index = arch_index,

        .tbd_options = tbd_options,
        .options = parse_slc_opts,

        .export_off_out = &export_off,
        .export_size_out = &export_size,

        .symtab_out = &symtab
    };

    uint8_t *lc_iter = load_cmd_buffer;

    /*
     * Mach-o load-commands are stored right after the mach-o header.
     */

    uint64_t lc_position = available_range_in.begin;
    uint32_t size_left = sizeofcmds;

    for (uint32_t i = 0; i != ncmds; i++) {
        /*
         * Verify that we still have space for a load-command.
         */

        if (size_left < sizeof(struct load_command)) {
            free(load_cmd_buffer);
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        struct load_command load_cmd = *(struct load_command *)lc_iter;
        if (flags.is_big_endian) {
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

                const uint64_t should_ignore =
                    (tbd_options.ignore_objc_constraint &&
                     tbd_options.ignore_swift_version);

                if (should_ignore) {
                    break;
                }

                /*
                 * For the sake of leniency, we ignore segments of the wrong
                 * word-size.
                 */

                if (flags.is_64) {
                    break;
                }

                if (load_cmd.cmdsize < sizeof(struct segment_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct segment_command *const segment =
                    (const struct segment_command *)lc_iter;

                if (!segment_has_image_info_sect(segment->segname)) {
                    break;
                }

                uint32_t nsects = segment->nsects;
                if (nsects == 0) {
                    break;
                }

                if (flags.is_big_endian) {
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

                const struct section *const end = sect + nsects;
                for (; sect != end; sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    /*
                     * We have an empty section if its offset or size is zero.
                     */

                    uint32_t sect_offset = sect->offset;
                    uint32_t sect_size = sect->size;

                    if (sect_offset == 0 || sect_size == 0) {
                        continue;
                    }

                    if (flags.is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint32(sect_size);
                    }

                    const enum macho_file_parse_result parse_section_result =
                        parse_section_from_file(info_in,
                                                &swift_version,
                                                fd,
                                                macho_range.begin,
                                                relative_range,
                                                sect_offset,
                                                sect_size,
                                                (off_t)lc_position,
                                                extra.callback,
                                                extra.cb_info,
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

                const uint64_t should_ignore =
                    (tbd_options.ignore_objc_constraint &&
                     tbd_options.ignore_swift_version);

                if (should_ignore) {
                    break;
                }

                /*
                 * For the sake of leniency, we ignore segments of the wrong
                 * word-size.
                 */

                if (!flags.is_64) {
                    break;
                }

                if (load_cmd.cmdsize < sizeof(struct segment_command_64)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct segment_command_64 *const segment =
                    (const struct segment_command_64 *)lc_iter;

                if (!segment_has_image_info_sect(segment->segname)) {
                    break;
                }

                uint32_t nsects = segment->nsects;
                if (nsects == 0) {
                    break;
                }

                if (flags.is_big_endian) {
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

                const struct section_64 *sect =
                    (const struct section_64 *)(segment + 1);

                const struct section_64 *const end = sect + nsects;
                for (; sect != end; sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    /*
                     * We have an empty section if its offset or size is zero.
                     */

                    uint32_t sect_offset = sect->offset;
                    uint64_t sect_size = sect->size;

                    if (sect_offset == 0 || sect_size == 0) {
                        continue;
                    }

                    if (flags.is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint64(sect_size);
                    }

                    const enum macho_file_parse_result parse_section_result =
                        parse_section_from_file(info_in,
                                                &swift_version,
                                                fd,
                                                macho_range.begin,
                                                relative_range,
                                                sect_offset,
                                                sect_size,
                                                (off_t)lc_position,
                                                extra.callback,
                                                extra.cb_info,
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
                parse_lc_info.load_cmd = load_cmd;
                parse_lc_info.lc_iter = lc_iter;

                const enum macho_file_parse_result parse_load_command_result =
                    macho_file_parse_single_lc(&parse_lc_info,
                                               extra.callback,
                                               extra.cb_info);

                if (parse_load_command_result != E_MACHO_FILE_PARSE_OK) {
                    free(load_cmd_buffer);
                    return parse_load_command_result;
                }

                lc_position += load_cmd.cmdsize;
                break;
            }
        }

        lc_iter += load_cmd.cmdsize;
    }

    free(load_cmd_buffer);
    if (!parse_slc_flags.found_identification) {
        const bool should_continue =
            call_callback(extra.callback,
                          info_in,
                          ERR_MACHO_FILE_PARSE_NOT_A_DYNAMIC_LIBRARY,
                          extra.cb_info);

        if (!should_continue) {
            return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
        }
    }

    const enum macho_file_parse_result handle_targets_platform_uuid_result =
        handle_targets_platform_and_uuid(info_in,
                                         parse_info->arch,
                                         platform,
                                         uuid,
                                         extra.callback,
                                         extra.cb_info,
                                         parse_slc_flags,
                                         tbd_options);

    if (handle_targets_platform_uuid_result != E_MACHO_FILE_PARSE_OK) {
        return handle_targets_platform_uuid_result;
    }

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;

    bool parsed_export_trie = false;
    bool parse_symtab = true;

    if (!options.use_symbol_table) {
        if (export_off != 0 && export_size != 0) {
            if (flags.is_big_endian) {
                export_off = swap_uint32(export_off);
                export_size = swap_uint32(export_size);
            }

            if (lc_info_out != NULL) {
                lc_info_out->export_off = export_off;
                lc_info_out->export_size = export_size;
            }

            const uint64_t base_offset = macho_range.begin;
            const struct macho_file_parse_export_trie_args args = {
                .info_in = info_in,
                .available_range = available_range,

                .arch_index = arch_index,

                .is_64 = flags.is_64,
                .is_big_endian = flags.is_big_endian,

                .export_off = export_off,
                .export_size = export_size,

                .sb_buffer = extra.export_trie_sb,
                .tbd_options = tbd_options
            };

            ret = macho_file_parse_export_trie_from_file(args, fd, base_offset);
            if (ret != E_MACHO_FILE_PARSE_OK) {
                return ret;
            }

            parsed_export_trie = true;
            if (symtab.nsyms != 0) {
                parse_symtab = should_parse_symtab(options, tbd_options);
            } else {
                parse_symtab = false;
            }
        } else if (options.use_export_trie) {
            return E_MACHO_FILE_PARSE_NO_EXPORT_TRIE;
        } else if (symtab.nsyms == 0) {
            return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
        }
    } else if (symtab.nsyms == 0) {
        return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
    }

    if (parse_symtab) {
        if (flags.is_big_endian) {
            symtab.symoff = swap_uint32(symtab.symoff);
            symtab.nsyms = swap_uint32(symtab.nsyms);

            symtab.stroff = swap_uint32(symtab.stroff);
            symtab.strsize = swap_uint32(symtab.strsize);
        }

        if (lc_info_out != NULL) {
            lc_info_out->symtab = symtab;
        }

        if (options.dont_parse_exports) {
            return E_MACHO_FILE_PARSE_OK;
        }

        const uint64_t base_offset = macho_range.begin;
        const struct macho_file_parse_symtab_args args = {
            .info_in = info_in,
            .available_range = available_range,

            .arch_index = arch_index,
            .is_big_endian = flags.is_big_endian,

            .symoff = symtab.symoff,
            .nsyms = symtab.nsyms,

            .stroff = symtab.stroff,
            .strsize = symtab.strsize,

            .tbd_options = tbd_options
        };

        if (flags.is_64) {
            ret = macho_file_parse_symtab_64_from_file(&args, fd, base_offset);
        } else {
            ret = macho_file_parse_symtab_from_file(&args, fd, base_offset);
        }
    } else if (!parsed_export_trie) {
        const uint64_t ignore_missing_exports =
            (tbd_options.ignore_exports || tbd_options.ignore_missing_exports);

        /*
         * If we have either ignore_exports or ignore_missing_exports, we don't
         * have an error.
         */

        if (ignore_missing_exports) {
            return E_MACHO_FILE_PARSE_OK;
        }

        return E_MACHO_FILE_PARSE_NO_DATA;
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return ret;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
parse_section_from_map(struct tbd_create_info *__notnull const info_in,
                       const struct range map_available_range,
                       const struct range macho_available_range,
                       const uint8_t *__notnull const map,
                       const uint8_t *__notnull const macho,
                       const uint32_t sect_offset,
                       const uint64_t sect_size,
                       const macho_file_parse_error_callback callback,
                       void *const cb_info,
                       const struct tbd_parse_options tbd_options,
                       const struct macho_file_parse_options options)
{
    if (sect_size != sizeof(struct objc_image_info)) {
        return E_MACHO_FILE_PARSE_INVALID_SECTION;
    }

    uint32_t sect_end = sect_offset;
    if (guard_overflow_add(&sect_end, sect_size)) {
        return E_MACHO_FILE_PARSE_INVALID_SECTION;
    }

    const struct objc_image_info *image_info = NULL;
    const struct range sect_range = {
        .begin = sect_offset,
        .end = sect_end
    };

    if (options.sect_off_absolute) {
        if (!range_contains_other(map_available_range, sect_range)) {
            return E_MACHO_FILE_PARSE_INVALID_SECTION;
        }

        image_info = (const struct objc_image_info *)(map + sect_offset);
    } else {
        if (!range_contains_other(macho_available_range, sect_range)) {
            return E_MACHO_FILE_PARSE_INVALID_SECTION;
        }

        image_info = (const struct objc_image_info *)(macho + sect_offset);
    }

    const uint32_t flags = image_info->flags;
    if (tbd_should_parse_objc_constraint(tbd_options, info_in->version)) {
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
            info_in->fields.archs.objc_constraint;

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
            info_in->fields.archs.objc_constraint = objc_constraint;
        }
    }

    if (!tbd_options.ignore_swift_version) {
        const uint32_t existing_swift_version = info_in->fields.swift_version;
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
            info_in->fields.swift_version = image_swift_version;
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_load_commands_from_map(
    struct tbd_create_info *__notnull const info_in,
    const struct mf_parse_lc_from_map_info *__notnull const parse_info,
    struct macho_file_parse_extra_args extra,
    struct macho_file_lc_info_out *const lc_info_out)
{
    const uint32_t ncmds = parse_info->ncmds;
    if (ncmds == 0) {
        return E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS;
    }

    /*
     * Verify the size and integrity of the load-commands.
     */

    uint32_t minimum_size = sizeof(struct load_command);
    if (guard_overflow_mul(&minimum_size, ncmds)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint32_t sizeofcmds = parse_info->sizeofcmds;
    if (sizeofcmds < minimum_size) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const uint64_t macho_size = parse_info->macho_size;
    const uint32_t header_size = parse_info->header_size;
    const uint32_t max_sizeofcmds = (uint32_t)(macho_size - header_size);

    if (sizeofcmds > max_sizeofcmds) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    const struct range relative_range = {
        .begin = 0,
        .end = macho_size
    };

    struct symtab_command symtab = {};
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    uint32_t export_off = 0;
    uint32_t export_size = 0;
    uint8_t uuid[16] = {};

    const uint8_t *const macho = parse_info->macho;
    const uint8_t *lc_iter = macho + header_size;

    const struct macho_file_parse_options options = parse_info->options;
    const struct tbd_parse_options tbd_options = parse_info->tbd_options;

    struct macho_file_parse_slc_flags parse_slc_flags = {};
    struct macho_file_parse_slc_options parse_slc_opts = {};

    if (options.copy_strings_in_map) {
        if (!tbd_options.ignore_install_name) {
            info_in->flags.install_name_was_allocated = true;
        }

        parse_slc_opts.copy_strings = true;
    }

    const struct macho_file_parse_lc_flags flags = parse_info->flags;
    if (flags.is_big_endian) {
        parse_slc_flags.is_big_endian = true;
    }

    const uint8_t *const map = parse_info->map;
    const uint64_t arch_index = parse_info->arch_index;

    const struct range available_map_range = parse_info->available_map_range;
    uint32_t size_left = sizeofcmds;

    struct macho_file_parse_single_lc_info parse_lc_info = {
        .info_in = info_in,

        .flags_in = &parse_slc_flags,
        .platform_in = &platform,
        .uuid_in = uuid,

        .arch_index = arch_index,

        .tbd_options = tbd_options,
        .options = parse_slc_opts,

        .export_off_out = &export_off,
        .export_size_out = &export_size,

        .symtab_out = &symtab
    };

    for (uint32_t i = 0; i != ncmds; i++) {
        /*
         * Verify that we still have space for a load-command.
         */

        if (size_left < sizeof(struct load_command)) {
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        struct load_command load_cmd = *(const struct load_command *)lc_iter;
        if (flags.is_big_endian) {
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
         * The verification instead happens at the start of the next iteration.
         */

        switch (load_cmd.cmd) {
            case LC_SEGMENT: {
                /*
                 * Move on to the next load-command if we don't need any
                 * segment-information.
                 */

                const uint64_t should_ignore =
                    (tbd_options.ignore_objc_constraint &&
                     tbd_options.ignore_swift_version);

                if (should_ignore) {
                    break;
                }

                /*
                 * For the sake of leniency, we ignore segments of the wrong
                 * word-size.
                 */

                if (flags.is_64) {
                    break;
                }

                if (load_cmd.cmdsize < sizeof(struct segment_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct segment_command *const segment =
                    (const struct segment_command *)lc_iter;

                if (!segment_has_image_info_sect(segment->segname)) {
                    break;
                }

                uint32_t nsects = segment->nsects;
                if (nsects == 0) {
                    break;
                }

                if (flags.is_big_endian) {
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

                    if (flags.is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint32(sect_size);
                    }

                    const enum macho_file_parse_result parse_section_result =
                        parse_section_from_map(info_in,
                                               available_map_range,
                                               relative_range,
                                               map,
                                               macho,
                                               sect_offset,
                                               sect_size,
                                               extra.callback,
                                               extra.cb_info,
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

                const uint64_t should_ignore =
                    (tbd_options.ignore_objc_constraint &&
                     tbd_options.ignore_swift_version);

                if (should_ignore) {
                    break;
                }

                /*
                 * For the sake of leniency, we ignore segments of the wrong
                 * word-size.
                 */

                if (!flags.is_64) {
                    break;
                }

                if (load_cmd.cmdsize < sizeof(struct segment_command_64)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct segment_command_64 *const segment =
                    (const struct segment_command_64 *)lc_iter;

                if (!segment_has_image_info_sect(segment->segname)) {
                    break;
                }

                uint32_t nsects = segment->nsects;
                if (nsects == 0) {
                    break;
                }

                if (flags.is_big_endian) {
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

                    if (flags.is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint64(sect_size);
                    }

                    const enum macho_file_parse_result parse_section_result =
                        parse_section_from_map(info_in,
                                               available_map_range,
                                               relative_range,
                                               map,
                                               macho,
                                               sect_offset,
                                               sect_size,
                                               extra.callback,
                                               extra.cb_info,
                                               tbd_options,
                                               options);

                    if (parse_section_result != E_MACHO_FILE_PARSE_OK) {
                        return parse_section_result;
                    }
                }

                break;
            }

            default: {
                parse_lc_info.load_cmd = load_cmd;
                parse_lc_info.lc_iter = lc_iter;

                const enum macho_file_parse_result parse_load_command_result =
                    macho_file_parse_single_lc(&parse_lc_info,
                                               extra.callback,
                                               extra.cb_info);

                if (parse_load_command_result != E_MACHO_FILE_PARSE_OK) {
                    return parse_load_command_result;
                }

                break;
            }
        }

        lc_iter += load_cmd.cmdsize;
    }

    if (!parse_slc_flags.found_identification) {
        const bool should_continue =
            call_callback(extra.callback,
                          info_in,
                          ERR_MACHO_FILE_PARSE_NOT_A_DYNAMIC_LIBRARY,
                          extra.cb_info);

        if (!should_continue) {
            return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
        }
    }

    const enum macho_file_parse_result handle_targets_platform_uuid_result =
        handle_targets_platform_and_uuid(info_in,
                                         parse_info->arch,
                                         platform,
                                         uuid,
                                         extra.callback,
                                         extra.cb_info,
                                         parse_slc_flags,
                                         tbd_options);

    if (handle_targets_platform_uuid_result != E_MACHO_FILE_PARSE_OK) {
        return handle_targets_platform_uuid_result;
    }

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;

    bool parsed_export_trie = false;
    bool parse_symtab = true;

    if (!options.use_symbol_table) {
        if (export_off != 0 && export_size != 0) {
            if (tbd_options.ignore_exports) {
                return E_MACHO_FILE_PARSE_OK;
            }

            if (flags.is_big_endian) {
                export_off = swap_uint32(export_off);
                export_size = swap_uint32(export_size);
            }

            if (lc_info_out != NULL) {
                lc_info_out->export_off = export_off;
                lc_info_out->export_size = export_size;
            }

            const struct macho_file_parse_export_trie_args args = {
                .info_in = info_in,
                .available_range = parse_info->available_map_range,

                .arch_index = arch_index,

                .is_64 = flags.is_64,
                .is_big_endian = flags.is_big_endian,

                .export_off = export_off,
                .export_size = export_size,

                .sb_buffer = extra.export_trie_sb,
                .tbd_options = tbd_options
            };

            ret = macho_file_parse_export_trie_from_map(args, map);
            if (ret != E_MACHO_FILE_PARSE_OK) {
                return ret;
            }

            parsed_export_trie = true;
            if (symtab.nsyms != 0) {
                parse_symtab = should_parse_symtab(options, tbd_options);
            } else {
                parse_symtab = false;
            }
        } else if (symtab.nsyms == 0) {
            return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
        }
    } else if (symtab.nsyms == 0) {
        return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
    }

    if (parse_symtab) {
        if (flags.is_big_endian) {
            symtab.symoff = swap_uint32(symtab.symoff);
            symtab.nsyms = swap_uint32(symtab.nsyms);

            symtab.stroff = swap_uint32(symtab.stroff);
            symtab.strsize = swap_uint32(symtab.strsize);
        }

        if (lc_info_out != NULL) {
            lc_info_out->symtab = symtab;
        }

        if (options.dont_parse_exports) {
            return E_MACHO_FILE_PARSE_OK;
        }

        if (parse_symtab) {
            const struct macho_file_parse_symtab_args args = {
                .info_in = info_in,
                .available_range = parse_info->available_map_range,

                .arch_index = arch_index,
                .is_big_endian = flags.is_big_endian,

                .symoff = symtab.symoff,
                .nsyms = symtab.nsyms,

                .stroff = symtab.stroff,
                .strsize = symtab.strsize,

                .tbd_options = tbd_options
            };

            if (flags.is_64) {
                ret = macho_file_parse_symtab_64_from_map(&args, map);
            } else {
                ret = macho_file_parse_symtab_from_map(&args, map);
            }
        }
    } else if (!parsed_export_trie) {
        const uint64_t ignore_missing_exports =
            (tbd_options.ignore_exports || tbd_options.ignore_missing_exports);

        /*
         * If we have either ignore_exports or ignore_missing_exports, we don't
         * have an error.
         */

        if (ignore_missing_exports) {
            return E_MACHO_FILE_PARSE_OK;
        }

        return E_MACHO_FILE_PARSE_NO_DATA;
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return ret;
    }

    return E_MACHO_FILE_PARSE_OK;
}
