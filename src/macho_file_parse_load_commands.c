//
//  src/macho_fd_load_commands.c
//  tbd
//
//  Created by inoahdev on 11/20/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <errno.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "guard_overflow.h"
#include "objc.h"

#include "macho_file_parse_load_commands.h"
#include "macho_file_parse_symbols.h"

#include "path.h"
#include "swap.h"

#include "yaml.h"

static bool segment_has_image_info_sect(const char name[16]) {
    const uint64_t first = *(uint64_t *)name;
    switch (first) {
        /*
         * case "__DATA" with 2 null characters.
         */

        case 71830128058207: {
            /*
             * if name == "__DATA".
             */

            const uint64_t second = *((uint64_t *)name + 1);
            if (second == 0) {
                return true; 
            }

            break;
        }

        /*
         * case "__DATA_D".
         */

        case 4926728347494670175: {
            const uint64_t second = *((uint64_t *)name + 1);
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
            const uint64_t second = *((uint64_t *)name + 1);
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

            const uint64_t second = *((uint64_t *)name + 1);
            if (second == 0) {
                return true; 
            }

            break;
        }
            
    }

    return false;
}

static bool is_image_info_section(const char name[16]) {
    const uint64_t first = *(uint64_t *)name;
    switch (first) {
        /*
         * case "__image".
         */

        case 6874014074396041055: {
            const uint64_t second = *((uint64_t *)name + 1);
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
            const uint64_t second = *((uint64_t *)name + 1);
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
parse_objc_constraint(struct tbd_create_info *const info_in,
                      const uint32_t flags)
{
    enum tbd_objc_constraint objc_constraint =
        TBD_OBJC_CONSTRAINT_RETAIN_RELEASE;

    if (flags & F_OBJC_IMAGE_INFO_REQUIRES_GC) {
        objc_constraint = TBD_OBJC_CONSTRAINT_GC;
    } else if(flags & F_OBJC_IMAGE_INFO_SUPPORTS_GC) {
        objc_constraint = TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_OR_GC;
    } else if (flags & F_OBJC_IMAGE_INFO_IS_FOR_SIMULATOR) {
        objc_constraint = TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_FOR_SIMULATOR;
    }

    if (info_in->objc_constraint != 0) {
        if (info_in->objc_constraint != objc_constraint) {
            return E_MACHO_FILE_PARSE_CONFLICTING_OBJC_CONSTRAINT;
        }
    } else {
        info_in->objc_constraint = objc_constraint;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
add_export_to_info(struct tbd_create_info *const info_in,
                   const uint64_t arch_bit,
                   const enum tbd_export_type type,
                   const char *const string,
                   const uint32_t string_length,
                   const bool needs_quotes)
{
    struct tbd_export_info export_info = {
        .archs = arch_bit,
        .archs_count = 1,
        .length = string_length,
        .string = (char *)string,
        .type = type
    };

    if (needs_quotes) {
        export_info.flags |= F_TBD_EXPORT_INFO_STRING_NEEDS_QUOTES;
    }

    struct array *const exports = &info_in->exports;
    struct array_cached_index_info cached_info = {};

    struct tbd_export_info *const existing_info =
        array_find_item_in_sorted(exports,
                                  sizeof(export_info),
                                  &export_info,
                                  tbd_export_info_no_archs_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        const uint64_t archs = existing_info->archs;
        if (!(archs & arch_bit)) {
            existing_info->archs = archs | arch_bit;
            existing_info->archs_count += 1;
        }

        return E_MACHO_FILE_PARSE_OK;
    }

    /*
     * Copy the provided string as the original string comes from the large
     * load-command buffer which will soon be freed.
     */

    export_info.string = strndup(export_info.string, export_info.length);
    if (export_info.string == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
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

enum macho_file_parse_result
macho_file_parse_load_commands_from_file(
    struct tbd_create_info *const info_in,
    const int fd,
    const uint64_t start,
    const uint64_t size,
    const struct arch_info *const arch,
    const uint64_t arch_bit,
    const bool is_64,
    const bool is_big_endian,
    const uint32_t ncmds,
    const uint32_t sizeofcmds,
    const uint64_t tbd_options,
    const uint64_t options,
    struct symtab_command *const symtab_out)
{
    if (ncmds == 0) {
        return E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS; 
    }
    
    /*
     * Verify the size and integrity of the load-commands.
     */

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
     * If a container-size has been provided, ensure that sizeofcmds doesn't go
     * past it.
     */

    if (size != 0) {
        uint32_t header_size = sizeof(struct mach_header);
        if (is_64) {
            header_size += sizeof(uint32_t);
        }

        const uint32_t remaining_size = size - header_size;
        if (sizeofcmds > remaining_size) {
            return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
        }
    }

    bool found_identification = false;
    bool found_uuid = false;

    struct symtab_command symtab = {};
    struct tbd_uuid_info uuid_info = { .arch = arch };

    /*
     * Allocate the entire load-commands buffer to allow fast parsing.
     */

    uint8_t *const load_cmd_buffer = calloc(1, sizeofcmds);
    if (load_cmd_buffer == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }
    
    if (read(fd, load_cmd_buffer, sizeofcmds) < 0) {
        free(load_cmd_buffer);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }
    
    /*
     * Iterate over the load-commands, which is located directly after the
     * mach-o header.
     */

    uint8_t *load_cmd_iter = load_cmd_buffer;
    uint32_t size_left = sizeofcmds;

    for (uint32_t i = 0; i != ncmds; i++) {
        /*
         * Verify that we still have space for a load-command.
         */

        if (size_left < sizeof(struct load_command)) {
            free(load_cmd_buffer);
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        /*
         * Big-endian mach-o files have load-commands whose information is also
         * big-endian.
         */
        
        struct load_command load_cmd = *(struct load_command *)load_cmd_iter;
        if (is_big_endian) {
            load_cmd.cmd = swap_uint32(load_cmd.cmd);
            load_cmd.cmdsize = swap_uint32(load_cmd.cmdsize);
        }

        /*
         * Verify the cmdsize by checking that a load-cmd can actually fit.
         * More verification can be done here, but we don't aim to be too picky.
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
            case LC_BUILD_VERSION: {
                /*
                 * If the platform isn't needed, skip the unnecessary parsing.
                 */

                if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
                    break;
                }

                /*
                 * All build_version load-commands should be at least large
                 * enough to hold a build_version_command.
                 *
                 * Note: build_version_command has an array of build-tool
                 * structures directly following, so the cmdsize will not always
                 * match.
                 */

                if (load_cmd.cmdsize < sizeof(struct build_version_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                /*
                 * Ensure that the given platform is valid.
                 */
                
                struct build_version_command *const build_version =
                    (struct build_version_command *)load_cmd_iter;

                uint32_t build_version_platform = build_version->platform;           
                if (is_big_endian) {
                    build_version_platform =
                        swap_uint32(build_version_platform);
                }

                if (build_version_platform < TBD_PLATFORM_MACOS) {
                    /*
                     * If we're ignoring invalid fields, simply goto the next
                     * load-command.
                     */

                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_PLATFORM;
                }

                if (build_version_platform > TBD_PLATFORM_TVOS) {
                    /*
                     * If we're ignoring invalid fields, simply goto the next
                     * load-command.
                     */

                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_PLATFORM;
                }

                /*
                 * Verify that if a platform was found earlier, the platform
                 * matches the given platform here.
                 *
                 * Note: Although this information should only be provided once,
                 * we are pretty lenient, so we don't enforce this.
                 */

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != build_version_platform) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = build_version_platform;
                }
    
                break;
            }

            case LC_ID_DYLIB: {
                /*
                 * We could check here if an LC_ID_DYLIB was already found for
                 * the same container, but for the sake of leniency, we don't.
                 */

                /*
                 * If no information is needed, skip the unnecessary parsing.
                 */

                if (tbd_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION &&
                    tbd_options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION &&
                    tbd_options & O_TBD_PARSE_IGNORE_INSTALL_NAME)
                {
                    found_identification = true;
                    break;
                }
                
                /*
                 * Ensure that dylib_command can fit its basic structure and
                 * information.
                 * 
                 * An exact match cannot be made as dylib_command includes a
                 * full install-name string in its cmdsize.
                 */

                if (load_cmd.cmdsize < sizeof(struct dylib_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }
                
                const struct dylib_command *const dylib_command =
                    (const struct dylib_command *)load_cmd_iter;

                uint32_t name_offset = dylib_command->dylib.name.offset;
                if (is_big_endian) {
                    name_offset = swap_uint32(name_offset);
                }

                /*
                 * Ensure that the install-name offset is not within the basic
                 * structure, and not outside of the load-command.
                 */

                if (name_offset < sizeof(struct dylib_command)) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        found_identification = true;
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
                }

                if (name_offset >= load_cmd.cmdsize) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        found_identification = true;
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
                }

                /*
                 * The install-name is at the back of the load-command,
                 * extending from the given offset to the end of the
                 * load-command.
                 */

                const char *const name_ptr =
                    (const char *)dylib_command + name_offset;

                const uint32_t max_length = load_cmd.cmdsize - name_offset;
                const uint32_t length = strnlen(name_ptr, max_length);

                if (length == 0) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        found_identification = true;
                        break;
                    }
                    
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
                }

                /*
                 * Do a quick check here to ensure that install_name is a valid
                 * yaml-string (with some additional boundaries).
                 */

                bool needs_quotes = false;
                yaml_check_c_str(name_ptr, length, &needs_quotes);

                if (needs_quotes) {
                    info_in->flags |=
                        F_TBD_CREATE_INFO_INSTALL_NAME_NEEDS_QUOTES;
                }
                
                /*
                 * Verify that the information we have received is the same as 
                 * the information we may have received earlier.
                 *
                 * Note: Although this information should only be provided once,
                 * we are pretty lenient, so we don't enforce this.
                 */

                const struct dylib dylib = dylib_command->dylib;
                if (info_in->install_name != NULL) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (ignore_conflicting_fields) {
                        found_identification = true;
                        break;
                    }

                    if (info_in->current_version != dylib.current_version) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    const uint32_t compatibility_version =
                        dylib.compatibility_version;

                    if (compatibility_version != dylib.compatibility_version) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    if (info_in->install_name_length != length) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    if (memcmp(info_in->install_name, name_ptr, length) != 0) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }
                } else {
                    if (!(tbd_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION)) {
                        info_in->current_version = dylib.current_version;
                    }

                    const bool ignore_compatibility_version =
                        options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;

                    if (!ignore_compatibility_version) {
                        info_in->compatibility_version =
                            dylib.compatibility_version;
                    }

                    if (!(tbd_options & O_TBD_PARSE_IGNORE_INSTALL_NAME)) {
                        char *const install_name = strndup(name_ptr, length);
                        if (install_name == NULL) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                        }

                        info_in->install_name = install_name;
                        info_in->install_name_length = length;
                    }
                }

                found_identification = true;
                break;
            }

            case LC_REEXPORT_DYLIB: {
                if (tbd_options & O_TBD_PARSE_IGNORE_REEXPORTS) {
                    break;
                }

                /*
                 * Ensure that dylib_command can fit its basic structure and
                 * information.
                 * 
                 * An exact match cannot be made as dylib_command includes the
                 * full re-export string in its cmdsize.
                 */

                if (load_cmd.cmdsize < sizeof(struct dylib_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct dylib_command *const reexport_dylib =
                    (const struct dylib_command *)load_cmd_iter;
 
                uint32_t reexport_offset = reexport_dylib->dylib.name.offset;
                if (is_big_endian) {
                    reexport_offset = swap_uint32(reexport_offset);
                }

                /*
                 * Ensure that the reexport-string is not within the
                 * basic structure, and not outside of the load-command itself.
                 */

                if (reexport_offset < sizeof(struct dylib_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
                }

                if (reexport_offset >= load_cmd.cmdsize) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
                }

                const char *const reexport_string =
                    (const char *)reexport_dylib + reexport_offset;

                const uint32_t max_length = load_cmd.cmdsize - reexport_offset;
                const uint32_t length = strnlen(reexport_string, max_length);
                
                if (length == 0) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
                }

                /*
                 * Do a quick check here to ensure that the re-export is a
                 * valid yaml-string (with some additional boundaries).
                 */

                bool needs_quotes = false;
                yaml_check_c_str(reexport_string, length, &needs_quotes);

                const enum macho_file_parse_result add_reexport_result =
                    add_export_to_info(info_in,
                                       arch_bit,
                                       TBD_EXPORT_TYPE_REEXPORT,
                                       reexport_string,
                                       length,
                                       needs_quotes);

                if (add_reexport_result != E_MACHO_FILE_PARSE_OK) {
                    free(load_cmd_buffer);
                    return add_reexport_result;
                }

                break;
            }

            case LC_SEGMENT: {
                /*
                 * If no information from a segment is needed, skip the
                 * unnecessary parsing.
                 */

                if ((tbd_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT) &&
                    (tbd_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION))
                {
                    break;
                }

                /*
                 * Verify we have the right segment for the right word-size
                 * (32-bit vs 64-bit).
                 *
                 * We just ignore this as having the wrong segment-type doesn't
                 * matter for us.
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
                const uint8_t *const sect_ptr =
                    load_cmd_iter + sizeof(struct segment_command);

                const struct section *sect = (struct section *)sect_ptr;
                for (uint32_t i = 0; i < nsects; i++, sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    uint32_t sect_offset = sect->offset;
                    uint32_t sect_size = sect->size;

                    if (is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint32(sect_size);
                    }

                    if (sect_size != sizeof(struct objc_image_info)) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_INVALID_SECTION;
                    }

                    /*
                     * Keep an offset of our original position, which we'll seek
                     * back to alter, so we can return safetly back to the for
                     * loop.
                     */

                    const uint32_t original_pos = lseek(fd, 0, SEEK_CUR);

                    if (options & O_MACHO_FILE_PARSE_SECT_OFF_ABSOLUTE) {
                        if (lseek(fd, sect_offset, SEEK_SET) < 0) {
                            free(load_cmd_buffer);

                            if (size != 0) {
                                /*
                                 * It's possible the fd is too small as we
                                 * haven't checked if the offset is contained
                                 * within the fd.
                                 */

                                if (errno == EINVAL) {
                                    return E_MACHO_FILE_PARSE_INVALID_SECTION;
                                }
                            }

                            return E_MACHO_FILE_PARSE_SEEK_FAIL;
                        }
                    } else {
                        if (size != 0) {
                            if (sect_offset >= size) {
                                free(load_cmd_buffer);
                                return E_MACHO_FILE_PARSE_INVALID_SECTION;
                            }

                            const uint64_t sect_end =
                                sect_offset + sizeof(struct objc_image_info);

                            if (sect_end > size) {
                                free(load_cmd_buffer);
                                return E_MACHO_FILE_PARSE_INVALID_SECTION;
                            }
                        }

                        if (lseek(fd, start + sect_offset, SEEK_SET) < 0) {
                            return E_MACHO_FILE_PARSE_SEEK_FAIL;
                        }
                    }

                    struct objc_image_info image_info = {};
                    if (read(fd, &image_info, sizeof(image_info)) < 0) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_READ_FAIL;
                    }
                    
                    /*
                     * Seek back to our original-position to continue recursing
                     * the load-commands.
                     */

                    if (lseek(fd, original_pos, SEEK_SET) < 0) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_SEEK_FAIL;
                    }

                    /*
                     * Parse the objc-constraint and ensure it's the same as
                     * the one discovered for another containers.
                     */
    
                    const enum macho_file_parse_result parse_constraint_result =
                        parse_objc_constraint(info_in, image_info.flags);
            
                    if (parse_constraint_result != E_MACHO_FILE_PARSE_OK) {
                        const bool ignore_conflicting_fields =
                            options &
                            O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                        if (!ignore_conflicting_fields) {
                            free(load_cmd_buffer);
                            return parse_constraint_result;
                        }
                    }

                    const uint32_t mask = objc_image_info_swift_version_mask;
                    const uint32_t image_swift_version =
                        (image_info.flags & mask) >> 8;

                    if (swift_version != 0) {
                        if (swift_version != image_swift_version) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    } else {
                        swift_version = image_swift_version;
                    }

                    /*
                     * Don't break here to ensure no other image-info section
                     * has different information.
                     */
                }

                if (info_in->swift_version != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->swift_version != swift_version) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    }
                } else {
                    info_in->swift_version = swift_version;
                }

                break;
            }

            case LC_SEGMENT_64: {
                /*
                 * If no information from a segment is needed, skip the
                 * unnecessary parsing.
                 */

                if ((tbd_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT) &&
                    (tbd_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION))
                {
                    break;
                }

                /*
                 * Verify we have the right segment for the right word-size
                 * (32-bit vs 64-bit).
                 *
                 * We just ignore this as having the wrong segment-type doesn't
                 * matter for us.
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
                 * Verify the size and integrity of the sections.
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
                const uint8_t *const sect_ptr =
                    load_cmd_iter + sizeof(struct segment_command_64);

                const struct section_64 *sect = (struct section_64 *)sect_ptr;
                for (uint32_t i = 0; i < nsects; i++, sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    uint32_t sect_offset = sect->offset;
                    uint32_t sect_size = sect->size;

                    if (is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint32(sect_size);
                    }

                    if (sect_size != sizeof(struct objc_image_info)) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_INVALID_SECTION;
                    }

                    /*
                     * Keep an offset of our original position, which we'll seek
                     * back to alter, so we can return safetly back to the for
                     * loop.
                     */

                    const uint32_t original_pos = lseek(fd, 0, SEEK_CUR);

                    if (options & O_MACHO_FILE_PARSE_SECT_OFF_ABSOLUTE) {
                        if (lseek(fd, sect_offset, SEEK_SET) < 0) {
                            free(load_cmd_buffer);

                            if (size != 0) {
                                /*
                                 * It's possible the fd is too small as we
                                 * haven't checked if the offset is contained
                                 * within the fd.
                                 */

                                if (errno == EINVAL) {
                                    return E_MACHO_FILE_PARSE_INVALID_SECTION;
                                }
                            }

                            return E_MACHO_FILE_PARSE_SEEK_FAIL;
                        }
                    } else {
                        if (size != 0) {
                            if (sect_offset >= size) {
                                free(load_cmd_buffer);
                                return E_MACHO_FILE_PARSE_INVALID_SECTION;
                            }

                            const uint64_t sect_end =
                                sect_offset + sizeof(struct objc_image_info);

                            if (sect_end > size) {
                                free(load_cmd_buffer);
                                return E_MACHO_FILE_PARSE_INVALID_SECTION;
                            }
                        }

                        if (lseek(fd, start + sect_offset, SEEK_SET) < 0) {
                            return E_MACHO_FILE_PARSE_SEEK_FAIL;
                        }
                    }

                    struct objc_image_info image_info = {};
                    if (read(fd, &image_info, sizeof(image_info)) < 0) {
                        free(load_cmd_buffer);

                        /*
                         * It's possible the fd is too small as we haven't
                         * checked if the image_info can be fully contained
                         * within.
                         */

                        if (errno == EOVERFLOW) {
                            return E_MACHO_FILE_PARSE_INVALID_SECTION;
                        }

                        return E_MACHO_FILE_PARSE_READ_FAIL;
                    }

                    /*
                     * Seek back to our original-position to continue recursing
                     * the load-commands.
                     */

                    if (lseek(fd, original_pos, SEEK_SET) < 0) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_SEEK_FAIL;
                    }

                    /*
                     * Parse the objc-constraint and ensure it's the same as
                     * the one discovered for another containers.
                     */
    
                    const enum macho_file_parse_result parse_constraint_result =
                        parse_objc_constraint(info_in, image_info.flags);
            
                    if (parse_constraint_result != E_MACHO_FILE_PARSE_OK) {
                        const bool ignore_conflicting_fields =
                            options &
                            O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                        if (!ignore_conflicting_fields) {
                            free(load_cmd_buffer);
                            return parse_constraint_result;
                        }
                    }

                    const uint32_t mask = objc_image_info_swift_version_mask;
                    const uint32_t image_swift_version =
                        (image_info.flags & mask) >> 8;

                    if (swift_version != 0) {
                        if (swift_version != image_swift_version) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    } else {
                        swift_version = image_swift_version;
                    }

                    /*
                     * Don't break here to ensure no other image-info section
                     * has different information.
                     */
                }

                if (info_in->swift_version != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->swift_version != swift_version) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    }
                } else {
                    info_in->swift_version = swift_version;
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
                 * An exact match cannot be made as sub_client_command includes
                 * a full client-string in its cmdsize.
                 */

                if (load_cmd.cmdsize < sizeof(struct sub_client_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                /*
                 * Ensure that the client is located fully within the
                 * load-command and after the basic information of a
                 * sub_client_command load-command structure.
                 */

                const struct sub_client_command *const client_command =
                    (const struct sub_client_command *)load_cmd_iter;

                uint32_t client_offset = client_command->client.offset;
                if (is_big_endian) {
                    client_offset = swap_uint32(client_offset);
                }

                /*
                 * Ensure that the client-string offset is not within the
                 * basic structure, and not outside of the load-command.
                 */

                if (client_offset < sizeof(struct sub_client_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                if (client_offset >= load_cmd.cmdsize) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                const char *const string =
                    (const char *)client_command + client_offset;

                const uint32_t max_length = load_cmd.cmdsize - client_offset;
                const uint32_t length = strnlen(string, max_length);

                if (length == 0) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                /*
                 * Do a quick check here to ensure that the client-string is a
                 * valid yaml-string (with some additional boundaries).
                 */

                bool needs_quotes = false;
                yaml_check_c_str(string, length, &needs_quotes);

                const enum macho_file_parse_result add_client_result =
                    add_export_to_info(info_in,
                                       arch_bit,
                                       TBD_EXPORT_TYPE_CLIENT,
                                       string,
                                       length,
                                       needs_quotes);

                if (add_client_result != E_MACHO_FILE_PARSE_OK) {
                    free(load_cmd_buffer);
                    return add_client_result;
                }

                break;
            }
            
            case LC_SUB_FRAMEWORK: {
                /*
                 * If no sub-umbrella are needed, skip the unnecessary parsing.
                 */

                if (tbd_options & O_TBD_PARSE_IGNORE_PARENT_UMBRELLA) {
                    break;
                }

                /*
                 * Ensure that sub_framework_command can fit its basic structure
                 * and information.
                 * 
                 * An exact match cannot be made as sub_framework_command
                 * includes a full umbrella-string in its cmdsize.
                 */

                if (load_cmd.cmdsize < sizeof(struct sub_framework_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct sub_framework_command *const framework_command =
                    (const struct sub_framework_command *)load_cmd_iter;

                uint32_t umbrella_offset = framework_command->umbrella.offset;
                if (is_big_endian) {
                    umbrella_offset = swap_uint32(umbrella_offset);
                } 

                /*
                 * Ensure that the umbrella-string offset is not within the
                 * basic structure, and not outside of the load-command.
                 */

                if (umbrella_offset < sizeof(struct sub_framework_command)) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
                }

                if (umbrella_offset >= load_cmd.cmdsize) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
                }

                /*
                 * The umbrella-string is at the back of the load-command,
                 * extending from the given offset to the end of the
                 * load-command.
                 */

                const char *const umbrella =
                    (const char *)load_cmd_iter + umbrella_offset;

                const uint32_t max_length = load_cmd.cmdsize - umbrella_offset;
                const uint32_t length = strnlen(umbrella, max_length);

                if (length == 0) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }
                    
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
                }

                /*
                 * Do a quick check here to ensure that parent-umbrella is a
                 * valid yaml-string (with some additional boundaries).
                 */

                bool needs_quotes = false;
                yaml_check_c_str(umbrella, length, &needs_quotes);

                if (needs_quotes) {
                    info_in->flags |=
                        F_TBD_CREATE_INFO_PARENT_UMBRELLA_NEEDS_QUOTES;
                }

                if (info_in->parent_umbrella != NULL) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (ignore_conflicting_fields) {
                        break;
                    }

                    if (info_in->parent_umbrella_length != length) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA;
                    }

                    const char *const parent_umbrella =
                        info_in->parent_umbrella;

                    if (memcmp(parent_umbrella, umbrella, length) != 0) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA;
                    }
                } else {
                    char *const umbrella_string = strndup(umbrella, length);
                    if (umbrella_string == NULL) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                    }

                    info_in->parent_umbrella = umbrella_string;
                    info_in->parent_umbrella_length = length;
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
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE;
                }

                /*
                 * Fill in the symtab's load-command info to mark that we did
                 * indeed fill in the symtab's info fields.
                 */
                
                symtab = *(struct symtab_command *)load_cmd_iter;
                break;
            }

            case LC_UUID: {
                /*
                 * If uuids aren't needed, skip the unnecessary parsing.
                 */

                if (tbd_options & O_TBD_PARSE_IGNORE_UUID) {
                    break;
                }

                if (load_cmd.cmdsize != sizeof(struct uuid_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_UUID;
                }
                
                const struct uuid_command *const uuid_cmd =
                    (const struct uuid_command *)load_cmd_iter;

                if (found_uuid) {
                    const char *const uuid_str = (const char *)uuid_info.uuid;
                    const char *const uuid_cmd_uuid =
                        (const char *)uuid_cmd->uuid;

                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (memcmp(uuid_str, uuid_cmd_uuid, 16) != 0) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
                        }
                    }
                } else {
                    memcpy(uuid_info.uuid, uuid_cmd->uuid, 16);
                    found_uuid = true; 
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
                 * All version_min load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != TBD_PLATFORM_MACOS) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = TBD_PLATFORM_MACOS;
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
                 * All version_min load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != TBD_PLATFORM_IOS) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = TBD_PLATFORM_IOS;
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
                 * All version_min load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != TBD_PLATFORM_WATCHOS) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = TBD_PLATFORM_WATCHOS;
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
                 * All version_min load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                    free(load_cmd_buffer);                    
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != TBD_PLATFORM_TVOS) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = TBD_PLATFORM_TVOS;
                }

                break;
            }

            default:
                break;
        }
    
        load_cmd_iter += load_cmd.cmdsize;
    }

    free(load_cmd_buffer);

    if (!found_identification) {
        return E_MACHO_FILE_PARSE_NO_IDENTIFICATION;
    }
    
    info_in->flags |= F_TBD_CREATE_INFO_STRINGS_WERE_COPIED;

    /*
     * Ensure that the uuid found is unique among all other containers before
     * adding to the fd's uuid arrays.
     */

    const uint8_t *const array_uuid =
        array_find_item(&info_in->uuids,
                        sizeof(uuid_info),
                        &uuid_info,
                        tbd_uuid_info_comparator,
                        NULL);

    if (array_uuid != NULL) {
        return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
    }
    
    const enum array_result add_uuid_info_result =
        array_add_item(&info_in->uuids, sizeof(uuid_info), &uuid_info, NULL);

    if (add_uuid_info_result != E_ARRAY_OK) {
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_PLATFORM)) {
        if (info_in->platform == 0) {
            return E_MACHO_FILE_PARSE_NO_PLATFORM;
        }
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_SYMBOLS)) {
        if (symtab.cmd != LC_SYMTAB) {
            return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
        }
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_UUID)) {
        if (!found_uuid) {
            return E_MACHO_FILE_PARSE_NO_UUID;
        }
    }

    /*
     * Retrieve the symbol and string-table info via the symtab_command.
     */

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

    /*
     * Verify the symbol-table's information.
     */

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    if (is_64) {
        ret =
            macho_file_parse_symbols_64_from_file(info_in,
                                                  fd,
                                                  start,
                                                  size,
                                                  arch_bit,
                                                  is_big_endian,
                                                  symtab.symoff,
                                                  symtab.nsyms,
                                                  symtab.stroff,
                                                  symtab.strsize,
                                                  tbd_options,
                                                  options);
    } else {
        ret =
            macho_file_parse_symbols_from_file(info_in,
                                               fd,
                                               start,
                                               size,
                                               arch_bit,
                                               is_big_endian,
                                               symtab.symoff,
                                               symtab.nsyms,
                                               symtab.stroff,
                                               symtab.strsize,
                                               tbd_options,
                                               options);
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return ret;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_load_commands_from_map(struct tbd_create_info *const info_in,
                                        const uint8_t *const map,
                                        const uint64_t map_size,
                                        const uint8_t *const macho,
                                        const uint64_t macho_size,
                                        const struct arch_info *const arch,
                                        const uint64_t arch_bit,
                                        const bool is_64,
                                        const bool is_big_endian,
                                        const uint32_t ncmds,
                                        const uint32_t sizeofcmds,
                                        const uint64_t tbd_options,
                                        const uint64_t options,
                                        struct symtab_command *const symtab_out)
{
    if (ncmds == 0) {
        return E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS; 
    }
    
    /*
     * Verify the size and integrity of the load-commands.
     */

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
     * Ensure that sizeofcmds doesn't go past the end of map.
     */

    uint32_t header_size = sizeof(struct mach_header);
    if (is_64) {
        header_size += sizeof(uint32_t);
    }

    const uint32_t remaining_size = macho_size - header_size;
    if (sizeofcmds > remaining_size) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    bool found_identification = false;
    bool found_uuid = false;

    struct symtab_command symtab = {};
    struct tbd_uuid_info uuid_info = { .arch = arch };

    /*
     * Allocate the entire load-commands buffer to allow fast parsing.
     */

    const uint8_t *load_cmd_iter = macho + header_size;
    
    /*
     * Iterate over the load-commands, which is located directly after the
     * mach-o header.
     */

    uint64_t size = macho_size;
    if (options & O_MACHO_FILE_PARSE_SECT_OFF_ABSOLUTE) {
        size = map_size;
    }

    uint32_t size_left = sizeofcmds;
    for (uint32_t i = 0; i != ncmds; i++) {
        /*
         * Verify that we still have space for a load-command.
         */

        if (size_left < sizeof(struct load_command)) {
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        /*
         * Big-endian mach-o files have load-commands whose information is also
         * big-endian.
         */
        
        struct load_command load_cmd = *(struct load_command *)load_cmd_iter;
        if (is_big_endian) {
            load_cmd.cmd = swap_uint32(load_cmd.cmd);
            load_cmd.cmdsize = swap_uint32(load_cmd.cmdsize);
        }

        /*
         * Verify the cmdsize by checking that a load-cmd can actually fit.
         * More verification can be done here, but we don't aim to be too picky.
         */

        if (load_cmd.cmdsize < sizeof(struct load_command)) {
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        if (size_left < load_cmd.cmdsize) {
            return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
        }

        size_left -= load_cmd.cmdsize;

        /*
         * We can't check size_left here, as this could be the last
         * load-command, so we have to check at the very beginning of the loop.
         */

        switch (load_cmd.cmd) {
            case LC_BUILD_VERSION: {
                /*
                 * If the platform isn't needed, skip the unnecessary parsing.
                 */

                if (tbd_options & O_TBD_PARSE_IGNORE_PLATFORM) {
                    break;
                }

                /*
                 * All build_version load-commands should be at least large
                 * enough to hold a build_version_command.
                 *
                 * Note: build_version_command has an array of build-tool
                 * structures directly following, so the cmdsize will not always
                 * match.
                 */

                if (load_cmd.cmdsize < sizeof(struct build_version_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                /*
                 * Ensure that the given platform is valid.
                 */
                
                struct build_version_command *const build_version =
                    (struct build_version_command *)load_cmd_iter;

                uint32_t build_version_platform = build_version->platform;           
                if (is_big_endian) {
                    build_version_platform =
                        swap_uint32(build_version_platform);
                }

                if (build_version_platform < TBD_PLATFORM_MACOS) {
                    /*
                     * If we're ignoring invalid fields, simply goto the next
                     * load-command.
                     */

                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    return E_MACHO_FILE_PARSE_INVALID_PLATFORM;
                }

                if (build_version_platform > TBD_PLATFORM_TVOS) {
                    /*
                     * If we're ignoring invalid fields, simply goto the next
                     * load-command.
                     */

                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    return E_MACHO_FILE_PARSE_INVALID_PLATFORM;
                }

                /*
                 * Verify that if a platform was found earlier, the platform
                 * matches the given platform here.
                 *
                 * Note: Although this information should only be provided once,
                 * we are pretty lenient, so we don't enforce this.
                 */

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != build_version_platform) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = build_version_platform;
                }
    
                break;
            }

            case LC_ID_DYLIB: {
                /*
                 * We could check here if an LC_ID_DYLIB was already found for
                 * the same container, but for the sake of leniency, we don't.
                 */

                /*
                 * If no information is needed, skip the unnecessary parsing.
                 */

                if (tbd_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION &&
                    tbd_options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION &&
                    tbd_options & O_TBD_PARSE_IGNORE_INSTALL_NAME)
                {
                    found_identification = true;
                    break;
                }
                
                /*
                 * Ensure that dylib_command can fit its basic structure and
                 * information.
                 * 
                 * An exact match cannot be made as dylib_command includes a
                 * full install-name string in its cmdsize.
                 */

                if (load_cmd.cmdsize < sizeof(struct dylib_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }
                
                const struct dylib_command *const dylib_command =
                    (const struct dylib_command *)load_cmd_iter;

                uint32_t name_offset = dylib_command->dylib.name.offset;
                if (is_big_endian) {
                    name_offset = swap_uint32(name_offset);
                }

                /*
                 * Ensure that the install-name offset is not within the basic
                 * structure, and not outside of the load-command.
                 */

                if (name_offset < sizeof(struct dylib_command)) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        found_identification = true;
                        break;
                    }

                    return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
                }

                if (name_offset >= load_cmd.cmdsize) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        found_identification = true;
                        break;
                    }

                    return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
                }

                /*
                 * The install-name is at the back of the load-command,
                 * extending from the given offset to the end of the
                 * load-command.
                 */

                const char *install_name =
                    (const char *)dylib_command + name_offset;

                const uint32_t max_length = load_cmd.cmdsize - name_offset;
                const uint32_t length = strnlen(install_name, max_length);

                if (length == 0) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        found_identification = true;
                        break;
                    }
                    
                    return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
                }

                /*
                 * Do a quick check here to ensure that install_name is a valid
                 * yaml-string (with some additional boundaries).
                 */

                bool needs_quotes = false;
                yaml_check_c_str(install_name, length, &needs_quotes);

                if (needs_quotes) {
                    info_in->flags |=
                        F_TBD_CREATE_INFO_INSTALL_NAME_NEEDS_QUOTES;
                }
                
                /*
                 * Verify that the information we have received is the same as 
                 * the information we may have received earlier.
                 *
                 * Note: Although this information should only be provided once,
                 * we are pretty lenient, so we don't enforce this.
                 */

                const struct dylib dylib = dylib_command->dylib;
                if (info_in->install_name != NULL) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (ignore_conflicting_fields) {
                        found_identification = true;
                        break;
                    }

                    if (info_in->current_version != dylib.current_version) {
                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    const uint32_t compatibility_version =
                        dylib.compatibility_version;

                    if (compatibility_version != dylib.compatibility_version) {
                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    if (info_in->install_name_length != length) {
                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    const char *const info_name = info_in->install_name;
                    if (memcmp(info_name, install_name, length) != 0) {
                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }
                } else {
                    if (!(tbd_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION)) {
                        info_in->current_version = dylib.current_version;
                    }

                    const bool ignore_compatibility_version =
                        options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;

                    if (!ignore_compatibility_version) {
                        info_in->compatibility_version =
                            dylib.compatibility_version;
                    }

                    if (!(tbd_options & O_TBD_PARSE_IGNORE_INSTALL_NAME)) {
                        if (options & O_MACHO_FILE_PARSE_COPY_STRINGS_IN_MAP) {
                            install_name = strndup(install_name, length);
                            if (install_name == NULL) {
                                return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                            }
                        }

                        info_in->install_name = install_name;
                        info_in->install_name_length = length;
                    }
                }

                found_identification = true;
                break;
            }

            case LC_REEXPORT_DYLIB: {
                if (tbd_options & O_TBD_PARSE_IGNORE_REEXPORTS) {
                    break;
                }

                /*
                 * Ensure that dylib_command can fit its basic structure and
                 * information.
                 * 
                 * An exact match cannot be made as dylib_command includes the
                 * full re-export string in its cmdsize.
                 */

                if (load_cmd.cmdsize < sizeof(struct dylib_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct dylib_command *const reexport_dylib =
                    (const struct dylib_command *)load_cmd_iter;
 
                uint32_t reexport_offset = reexport_dylib->dylib.name.offset;
                if (is_big_endian) {
                    reexport_offset = swap_uint32(reexport_offset);
                }

                /*
                 * Ensure that the reexport-string is not within the
                 * basic structure, and not outside of the load-command itself.
                 */

                if (reexport_offset < sizeof(struct dylib_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
                }

                if (reexport_offset >= load_cmd.cmdsize) {
                    return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
                }

                const char *reexport_string =
                    (const char *)reexport_dylib + reexport_offset;

                const uint32_t max_length = load_cmd.cmdsize - reexport_offset;
                const uint32_t length = strnlen(reexport_string, max_length);
                
                if (length == 0) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
                }

                /*
                 * Do a quick check here to ensure that the re-export is a
                 * valid yaml-string (with some additional boundaries).
                 */

                bool needs_quotes = false;
                yaml_check_c_str(reexport_string, length, &needs_quotes);

                const enum macho_file_parse_result add_reexport_result =
                    add_export_to_info(info_in,
                                       arch_bit,
                                       TBD_EXPORT_TYPE_REEXPORT,
                                       reexport_string,
                                       length,
                                       needs_quotes);

                if (add_reexport_result != E_MACHO_FILE_PARSE_OK) {
                    return add_reexport_result;
                }

                break;
            }

            case LC_SEGMENT: {
                /*
                 * If no information from a segment is needed, skip the
                 * unnecessary parsing.
                 */

                if ((tbd_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT) &&
                    (tbd_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION))
                {
                    break;
                }

                /*
                 * Verify we have the right segment for the right word-size
                 * (32-bit vs 64-bit).
                 *
                 * We just ignore this as having the wrong segment-type doesn't
                 * matter for us.
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
                 * Verify the size and integrity of the sections.
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
                const uint8_t *const sect_ptr =
                    load_cmd_iter + sizeof(struct segment_command);

                const struct section *sect = (struct section *)sect_ptr;
                for (uint32_t i = 0; i < nsects; i++, sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    uint32_t sect_offset = sect->offset;
                    uint32_t sect_size = sect->size;

                    if (is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint32(sect_size);
                    }

                    if (sect_size != sizeof(struct objc_image_info)) {
                        return E_MACHO_FILE_PARSE_INVALID_SECTION;
                    }

                    if (sect_offset >= size) {
                        return E_MACHO_FILE_PARSE_INVALID_SECTION;
                    }

                    const uint64_t sect_end =
                        sect_offset + sizeof(struct objc_image_info);
                    
                    if (sect_end > size) {
                        return E_MACHO_FILE_PARSE_INVALID_SECTION;
                    }

                    const struct objc_image_info *image_info =
                        (const struct objc_image_info *)(macho + sect_offset);

                    if (options & O_MACHO_FILE_PARSE_SECT_OFF_ABSOLUTE) {
                        image_info =
                            (const struct objc_image_info *)(map + sect_offset);
                    }
                    
                    /*
                     * Parse the objc-constraint and ensure it's the same as
                     * the one discovered for another containers.
                     */
    
                    const uint32_t flags = image_info->flags;
                    const enum macho_file_parse_result parse_constraint_result =
                        parse_objc_constraint(info_in, flags);
            
                    if (parse_constraint_result != E_MACHO_FILE_PARSE_OK) {
                        const bool ignore_conflicting_fields =
                            options &
                            O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                        if (!ignore_conflicting_fields) {
                            return parse_constraint_result;
                        }
                    }

                    const uint32_t mask = objc_image_info_swift_version_mask;
                    const uint32_t image_swift_version = (flags & mask) >> 8;

                    if (swift_version != 0) {
                        if (swift_version != image_swift_version) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    } else {
                        swift_version = image_swift_version;
                    }

                    /*
                     * Don't break here to ensure no other image-info section
                     * has different information.
                     */
                }

                if (info_in->swift_version != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->swift_version != swift_version) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    }
                } else {
                    info_in->swift_version = swift_version;
                }

                break;
            }

            case LC_SEGMENT_64: {
                /*
                 * If no information from a segment is needed, skip the
                 * unnecessary parsing.
                 */

                if ((tbd_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT) &&
                    (tbd_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION))
                {
                    break;
                }

                /*
                 * Verify we have the right segment for the right word-size
                 * (32-bit vs 64-bit).
                 *
                 * We just ignore this as having the wrong segment-type doesn't
                 * matter for us.
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
                 * Verify the size and integrity of the sections.
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
                const uint8_t *const sect_ptr =
                    load_cmd_iter + sizeof(struct segment_command_64);

                const struct section_64 *sect =
                    (const struct section_64 *)sect_ptr;
                
                for (uint32_t i = 0; i < nsects; i++, sect++) {
                    if (!is_image_info_section(sect->sectname)) {
                        continue;
                    }

                    uint32_t sect_offset = sect->offset;
                    uint32_t sect_size = sect->size;

                    if (is_big_endian) {
                        sect_offset = swap_uint32(sect_offset);
                        sect_size = swap_uint32(sect_size);
                    }

                    if (sect_size != sizeof(struct objc_image_info)) {
                        return E_MACHO_FILE_PARSE_INVALID_SECTION;
                    }

                    if (sect_offset >= size) {
                        return E_MACHO_FILE_PARSE_INVALID_SECTION;
                    }

                    const uint64_t sect_end =
                        sect_offset + sizeof(struct objc_image_info);
                    
                    if (sect_end > size) {
                        return E_MACHO_FILE_PARSE_INVALID_SECTION;
                    }

                    const struct objc_image_info *image_info =
                        (const struct objc_image_info *)(macho + sect_offset);

                    if (options & O_MACHO_FILE_PARSE_SECT_OFF_ABSOLUTE) {
                        image_info =
                            (const struct objc_image_info *)(map + sect_offset);
                    }
                    
                    /*
                     * Parse the objc-constraint and ensure it's the same as
                     * the one discovered for another containers.
                     */
    
                    const uint32_t flags = image_info->flags;
                    const enum macho_file_parse_result parse_constraint_result =
                        parse_objc_constraint(info_in, flags);
            
                    if (parse_constraint_result != E_MACHO_FILE_PARSE_OK) {
                        const bool ignore_conflicting_fields =
                            options &
                            O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                        if (!ignore_conflicting_fields) {
                            return parse_constraint_result;
                        }
                    }

                    const uint32_t mask = objc_image_info_swift_version_mask;
                    const uint32_t image_swift_version = (flags & mask) >> 8;

                    if (swift_version != 0) {
                        if (swift_version != image_swift_version) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    } else {
                        swift_version = image_swift_version;
                    }

                    /*
                     * Don't break here to ensure no other image-info section
                     * has different information.
                     */
                }

                if (info_in->swift_version != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->swift_version != swift_version) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    }
                } else {
                    info_in->swift_version = swift_version;
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
                 * An exact match cannot be made as sub_client_command includes
                 * a full client-string in its cmdsize.
                 */

                if (load_cmd.cmdsize < sizeof(struct sub_client_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                /*
                 * Ensure that the client is located fully within the
                 * load-command and after the basic information of a
                 * sub_client_command load-command structure.
                 */

                const struct sub_client_command *const client_command =
                    (const struct sub_client_command *)load_cmd_iter;

                uint32_t client_offset = client_command->client.offset;
                if (is_big_endian) {
                    client_offset = swap_uint32(client_offset);
                }

                /*
                 * Ensure that the client-string offset is not within the
                 * basic structure, and not outside of the load-command.
                 */

                if (client_offset < sizeof(struct sub_client_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                if (client_offset >= load_cmd.cmdsize) {
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                const char *const client_string =
                    (const char *)client_command + client_offset;

                const uint32_t max_length = load_cmd.cmdsize - client_offset;
                const uint32_t length = strnlen(client_string, max_length);

                if (length == 0) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                /*
                 * Do a quick check here to ensure that the client-string is a
                 * valid yaml-string (with some additional boundaries).
                 */

                bool needs_quotes = false;
                yaml_check_c_str(client_string, length, &needs_quotes);

                const enum macho_file_parse_result add_client_result =
                    add_export_to_info(info_in,
                                       arch_bit,
                                       TBD_EXPORT_TYPE_CLIENT,
                                       client_string,
                                       length,
                                       needs_quotes);

                if (add_client_result != E_MACHO_FILE_PARSE_OK) {
                    return add_client_result;
                }

                break;
            }
            
            case LC_SUB_FRAMEWORK: {
                /*
                 * If no sub-umbrella are needed, skip the unnecessary parsing.
                 */

                if (tbd_options & O_TBD_PARSE_IGNORE_PARENT_UMBRELLA) {
                    break;
                }

                /*
                 * Ensure that sub_framework_command can fit its basic structure
                 * and information.
                 * 
                 * An exact match cannot be made as sub_framework_command
                 * includes a full umbrella-string in its cmdsize.
                 */

                if (load_cmd.cmdsize < sizeof(struct sub_framework_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                const struct sub_framework_command *const framework_command =
                    (const struct sub_framework_command *)load_cmd_iter;

                uint32_t umbrella_offset = framework_command->umbrella.offset;
                if (is_big_endian) {
                    umbrella_offset = swap_uint32(umbrella_offset);
                } 

                /*
                 * Ensure that the umbrella-string offset is not within the
                 * basic structure, and not outside of the load-command.
                 */

                if (umbrella_offset < sizeof(struct sub_framework_command)) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
                }

                if (umbrella_offset >= load_cmd.cmdsize) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
                }

                /*
                 * The umbrella-string is at the back of the load-command,
                 * extending from the given offset to the end of the
                 * load-command.
                 */

                const char *umbrella =
                    (const char *)load_cmd_iter + umbrella_offset;

                const uint32_t max_length = load_cmd.cmdsize - umbrella_offset;
                const uint32_t length = strnlen(umbrella, max_length);

                if (length == 0) {
                    if (options & O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS) {
                        break;
                    }
                    
                    return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
                }

                /*
                 * Do a quick check here to ensure that parent-umbrella is a
                 * valid yaml-string (with some additional boundaries).
                 */

                bool needs_quotes = false;
                yaml_check_c_str(umbrella, length, &needs_quotes);

                if (needs_quotes) {
                    info_in->flags |=
                        F_TBD_CREATE_INFO_PARENT_UMBRELLA_NEEDS_QUOTES;
                }

                if (info_in->parent_umbrella != NULL) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (ignore_conflicting_fields) {
                        break;
                    }

                    if (info_in->parent_umbrella_length != length) {
                        return E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA;
                    }

                    const char *const parent_umbrella =
                        info_in->parent_umbrella;

                    if (memcmp(parent_umbrella, umbrella, length) != 0) {
                        return E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA;
                    }
                } else {
                    if (options & O_MACHO_FILE_PARSE_COPY_STRINGS_IN_MAP) {
                        umbrella = strndup(umbrella, length);
                        if (umbrella == NULL) {
                            return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                        }
                    }

                    info_in->parent_umbrella = umbrella;
                    info_in->parent_umbrella_length = length;
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

                /*
                 * Fill in the symtab's load-command info to mark that we did
                 * indeed fill in the symtab's info fields.
                 */
                
                symtab = *(struct symtab_command *)load_cmd_iter;
                break;
            }

            case LC_UUID: {
                /*
                 * If uuids aren't needed, skip the unnecessary parsing.
                 */

                if (tbd_options & O_TBD_PARSE_IGNORE_UUID) {
                    break;
                }

                if (load_cmd.cmdsize != sizeof(struct uuid_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_UUID;
                }
                
                const struct uuid_command *const uuid_cmd =
                    (const struct uuid_command *)load_cmd_iter;

                if (found_uuid) {
                    const char *const uuid_str = (const char *)uuid_info.uuid;
                    const char *const uuid_cmd_uuid =
                        (const char *)uuid_cmd->uuid;

                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (memcmp(uuid_str, uuid_cmd_uuid, 16) != 0) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
                        }
                    }
                } else {
                    memcpy(uuid_info.uuid, uuid_cmd->uuid, 16);
                    found_uuid = true; 
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
                 * All version_min load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != TBD_PLATFORM_MACOS) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = TBD_PLATFORM_MACOS;
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
                 * All version_min load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != TBD_PLATFORM_IOS) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = TBD_PLATFORM_IOS;
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
                 * All version_min load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != TBD_PLATFORM_WATCHOS) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = TBD_PLATFORM_WATCHOS;
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
                 * All version_min load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct version_min_command)) {
                    return E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND;
                }

                if (info_in->platform != 0) {
                    const bool ignore_conflicting_fields =
                        options & O_MACHO_FILE_PARSE_IGNORE_CONFLICTING_FIELDS;

                    if (!ignore_conflicting_fields) {
                        if (info_in->platform != TBD_PLATFORM_TVOS) {
                            return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                        }
                    }
                } else {
                    info_in->platform = TBD_PLATFORM_TVOS;
                }

                break;
            }

            default:
                break;
        }
    
        load_cmd_iter += load_cmd.cmdsize;
    }

    if (!found_identification) {
        return E_MACHO_FILE_PARSE_NO_IDENTIFICATION;
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_UUID)) {
        if (!found_uuid) {
            return E_MACHO_FILE_PARSE_NO_UUID;
        }
    }   

    if (options & O_MACHO_FILE_PARSE_COPY_STRINGS_IN_MAP) {
        info_in->flags |= F_TBD_CREATE_INFO_STRINGS_WERE_COPIED;
    }
    
    /*
     * Ensure that the uuid found is unique among all other containers before
     * adding to the fd's uuid arrays.
     */

    const uint8_t *const array_uuid =
        array_find_item(&info_in->uuids,
                        sizeof(uuid_info),
                        &uuid_info,
                        tbd_uuid_info_comparator,
                        NULL);

    if (array_uuid != NULL) {
        return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
    }
    
    const enum array_result add_uuid_info_result =
        array_add_item(&info_in->uuids, sizeof(uuid_info), &uuid_info, NULL);

    if (add_uuid_info_result != E_ARRAY_OK) {
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
    }

    if (symtab.cmd != LC_SYMTAB) {
        if (tbd_options & O_TBD_PARSE_IGNORE_SYMBOLS) {
            return E_MACHO_FILE_PARSE_OK;
        }

        if (tbd_options & O_TBD_PARSE_IGNORE_MISSING_EXPORTS) {
            return E_MACHO_FILE_PARSE_OK;
        }

        return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
    }

    /*
     * Retrieve the symbol and string-table info via the symtab_command.
     */

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

    /*
     * Verify the symbol-table's information.
     */

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    if (is_64) {
        ret =
            macho_file_parse_symbols_64_from_map(info_in,
                                                 map,
                                                 size,
                                                 arch_bit,
                                                 is_big_endian,
                                                 symtab.symoff,
                                                 symtab.nsyms,
                                                 symtab.stroff,
                                                 symtab.strsize,
                                                 tbd_options,
                                                 options);
    } else {
        ret =
            macho_file_parse_symbols_from_map(info_in,
                                              map,
                                              size,
                                              arch_bit,
                                              is_big_endian,
                                              symtab.symoff,
                                              symtab.nsyms,
                                              symtab.stroff,
                                              symtab.strsize,
                                              tbd_options,
                                              options);
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return ret;
    }

    return E_MACHO_FILE_PARSE_OK;
}
