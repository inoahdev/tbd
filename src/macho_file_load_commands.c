//
//  src/macho_fd_load_commands.c
//  tbd
//
//  Created by inoahdev on 11/20/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <errno.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "c_str_utils.h"
#include "objc.h"

#include "macho_file_load_commands.h"
#include "macho_file_symbols.h"

#include "path.h"
#include "swap.h"

static enum macho_file_parse_result 
parse_objc_constraint(struct tbd_create_info *const info,
                      const uint32_t flags)
{
    enum tbd_objc_constraint objc_constraint =
        TBD_OBJC_CONSTRAINT_RETAIN_RELEASE;

    if (flags & F_OBJC_IMAGE_INFO_REQUIRES_GC) {
        objc_constraint = TBD_OBJC_CONSTRAINT_GC;
    } else if(flags & F_OBJC_IMAGE_INFO_SUPPORTS_GC) {
        objc_constraint = TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_OR_GC;
    } else if (flags & F_OBJC_IMAGE_INFO_IS_FOR_SIMULATOR) {
        objc_constraint =
            TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_FOR_SIMULATOR;
    }

    if (info->objc_constraint != 0) {
        if (info->objc_constraint != objc_constraint) {
            return E_MACHO_FILE_PARSE_CONFLICTING_OBJC_CONSTRAINT;
        }
    } else {
        info->objc_constraint = objc_constraint;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
add_export_to_info(struct tbd_create_info *const info,
                   const uint64_t arch_bit,
                   const enum tbd_export_type type,
                   char *const string)
{
    const struct tbd_export_info export_info = {
        .archs = arch_bit,
        .length = strlen(string),
        .string = string,
        .type = type
    };

    struct array *const exports = &info->exports;
    struct array_cached_index_info cached_info = {};

    struct tbd_export_info *const existing_info =
        array_find_item_in_sorted(exports,
                                  sizeof(export_info),
                                  &export_info,
                                  tbd_export_info_no_archs_comparator,
                                  &cached_info);

    if (existing_info != NULL) {
        existing_info->archs |= arch_bit;
        free(string);

        return E_MACHO_FILE_PARSE_OK;
    }

    const enum array_result add_export_info_result =
        array_add_item_with_cached_index_info(exports,
                                              sizeof(export_info),
                                              &export_info,
                                              &cached_info,
                                              NULL);

    if (add_export_info_result != E_ARRAY_OK) {
        free(string);
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_load_commands(struct tbd_create_info *const info,
                               const struct arch_info *const arch,
                               const uint64_t arch_bit,
                               const int fd,
                               const uint64_t start,
                               const uint64_t size,
                               const bool is_64,
                               const bool is_big_endian,
                               const uint32_t ncmds,
                               const uint32_t sizeofcmds,
                               const uint64_t parse_options,
                               const uint64_t options)
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

    /*
     * Check for a possible overflow when multipying.
     */

    const uint32_t minimum_size = sizeof(struct load_command) * ncmds;
    if (minimum_size / sizeof(struct load_command) != ncmds) {
        return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
    }

    /*
     * If a container-size has been provided, ensure that sizeofcmds doesn't go
     * past it.
     */

    if (size != 0) {
        uint32_t header_size = sizeof(struct mach_header);
        if (is_64) {
            header_size = sizeof(struct mach_header_64);
        }

        const uint32_t remaining_size = size - header_size;
        if (sizeofcmds > remaining_size) {
            return E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS;
        }
    }

    bool found_identification = false;
    bool found_platform = false;
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

    for (uint32_t i = 0; i < ncmds; i++) {
        /*
         * Big-endian mach-o fds have load-commands whose information is also
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

        switch (load_cmd.cmd) {
            case LC_BUILD_VERSION: {
                /*
                 * If the platform isn't needed, skip the unnecessary parsing.
                 */

                if (parse_options & O_TBD_PARSE_IGNORE_PLATFORM) {
                    break;
                }

                /*
                 * All build_version load-commands should be the of the same
                 * cmdsize.
                 */

                if (load_cmd.cmdsize != sizeof(struct build_version_command)) {
                    break;
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

                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
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

                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
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

                if (info->platform != 0) {
                    if (info->platform != build_version_platform) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                    }
                } else {
                    info->platform = build_version_platform;
                }
    
                found_platform = true;
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

                if (parse_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION &&
                    parse_options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION &&
                    parse_options & O_TBD_PARSE_IGNORE_INSTALL_NAME)
                {
                    found_identification = true;
                    break;
                }
                
                /*
                 * Ensure that dylib_command can fit its basic structure and
                 * information. Note that dylib_command includes a install-name
                 * string in its cmdsize.
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
                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
                        found_identification = true;
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
                }

                if (name_offset >= load_cmd.cmdsize) {
                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
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

                const uint32_t install_name_length =
                    load_cmd.cmdsize - name_offset;

                char *const install_name = calloc(1, install_name_length + 1);
                if (install_name == NULL) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                }

                const char *const install_name_ptr =
                    (const char *)dylib_command + name_offset;

                memcpy(install_name, install_name_ptr, install_name_length);

                /*
                 * Do a quick check here to ensure that install_name is in fact
                 * filled with *non-whitespace* characters.
                 */

                if (c_str_is_all_whitespace(install_name)) {
                    free(install_name);

                    /*
                     * If we're ignoring invalid fields, simply goto the next
                     * load-command.
                     */

                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
                        found_identification = true;
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME;
                }

                /*
                 * Verify that the information we have received is the same as 
                 * the information we may have received earlier.
                 *
                 * Note: Although this information should only be provided once,
                 * we are pretty lenient, so we don't enforce this.
                 */

                const struct dylib dylib = dylib_command->dylib;
                if (info->install_name != NULL) {
                    if (info->current_version != dylib.current_version) {
                        free(install_name);
                        free(load_cmd_buffer);

                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    const uint32_t compatibility_version =
                        dylib.compatibility_version;

                    if (compatibility_version != dylib.compatibility_version) {
                        free(install_name);
                        free(load_cmd_buffer);

                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    if (strcmp(info->install_name, install_name) != 0) {
                        free(install_name);
                        free(load_cmd_buffer);

                        return E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION;
                    }

                    free(install_name);
                } else {
                    if (!(parse_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION)) {
                        info->current_version = dylib.current_version;
                    }

                    const bool ignore_compatibility_version =
                        options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;

                    if (!ignore_compatibility_version) {
                        info->compatibility_version =
                            dylib.compatibility_version;
                    }

                    if (!(parse_options & O_TBD_PARSE_IGNORE_INSTALL_NAME)) {
                        info->install_name = install_name;
                    }
                }

                found_identification = true;
                break;
            }

            case LC_REEXPORT_DYLIB: {
                if (parse_options & O_TBD_PARSE_IGNORE_REEXPORTS) {
                    break;
                }

                const struct dylib_command *const reexport_dylib =
                    (const struct dylib_command *)load_cmd_iter;
 
                uint32_t reexport_offset = reexport_dylib->dylib.name.offset;
                if (is_big_endian) {
                    reexport_offset = swap_uint32(reexport_offset);
                }

                if (reexport_offset < sizeof(struct dylib_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
                }

                if (reexport_offset >= load_cmd.cmdsize) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_REEXPORT;
                }

                const uint32_t reexport_length =
                    load_cmd.cmdsize - reexport_offset;

                char *const reexport_string = calloc(1, reexport_length + 1);
                if (reexport_string == NULL) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                }

                const char *const reexport_ptr =
                    (const char *)reexport_dylib + reexport_offset;
                
                memcpy(reexport_string, reexport_ptr, reexport_length);

                /*
                 * Do a quick check here to ensure that the reexport-string is 
                 * in fact filled with *non-whitespace* characters.
                 */

                if (c_str_is_all_whitespace(reexport_string)) {
                    free(reexport_string);
                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                const enum macho_file_parse_result add_reexport_result =
                    add_export_to_info(info,
                                       arch_bit,
                                       TBD_EXPORT_TYPE_REEXPORT,
                                       reexport_string);

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

                if ((parse_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT) &&
                    (parse_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION))
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

                /*
                 * We're looking for the objc image-info section, located in
                 * either the __DATA, __DATA_DIRTY, __DATA_CONST, __OBJC
                 * segments.
                 */

                const struct segment_command *const segment =
                    (const struct segment_command *)load_cmd_iter;

                const char *const segname = segment->segname;
                if (strncmp(segname, "__DATA", 16) != 0 &&
                    strncmp(segname, "__DATA_DIRTY", 16) != 0 &&
                    strncmp(segname, "__DATA_CONST", 16) != 0 &&
                    strncmp(segname, "__OBJC", 16) != 0)
                {
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

                const uint32_t sections_size = sizeof(struct section) * nsects;

                /*
                 * Ensure no overflow occurs when calculating the total sections
                 * size.
                 */

                if (sections_size / sizeof(struct section) != nsects) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                const uint32_t max_sections_size =
                    load_cmd.cmdsize - sizeof(struct segment_command);

                if (sections_size > max_sections_size) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                const uint8_t *const sect_ptr =
                    load_cmd_iter + sizeof(struct segment_command);

                const struct section *sect = (struct section *)sect_ptr;
                for (uint32_t i = 0; i < nsects; i++, sect++) {
                    const char *const sectname = sect->sectname;
                    if (strncmp(sectname, "__image_info", 16) != 0 &&
                        strncmp(sectname, "__objc_imageinfo", 16) != 0)
                    {
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

                    /*
                     * This is an ugly hack, but we don't really have any other
                     * choice for this right now.
                     */

                    if (size != 0) {
                        if (sect_offset > size) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_INVALID_SECTION;
                        }

                        if (sect_offset + sect_size > size) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_INVALID_SECTION;
                        }

                        if (lseek(fd, start + sect_offset, SEEK_SET) < 0) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_SEEK_FAIL;
                        }
                    } else {
                        if (lseek(fd, start + sect_offset, SEEK_SET) < 0) {
                            free(load_cmd_buffer);

                            /*
                             * It's possible the fd is too small as we haven't
                             * checked if the offset is contained within the
                             * fd.
                             */

                            if (errno == EINVAL) {
                                return E_MACHO_FILE_PARSE_INVALID_SECTION;
                            }
 
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
                        parse_objc_constraint(info, image_info.flags);
            
                    if (parse_constraint_result != E_MACHO_FILE_PARSE_OK) {
                        if (!(options & O_MACHO_FILE_IGNORE_INVALID_FIELDS)) {
                            free(load_cmd_buffer);
                            return parse_constraint_result;
                        }
                    }

                    const uint32_t mask = objc_image_info_swift_version_mask;
                    const uint32_t swift_version =
                        (image_info.flags & mask) >> 8;

                    if (info->swift_version != 0) {
                        if (info->swift_version != swift_version) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    } else {
                        info->swift_version = swift_version;
                    }

                    /*
                     * Don't break here to ensure no other image-info section
                     * has different information.
                     */
                }

                break;
            }

            case LC_SEGMENT_64: {
                /*
                 * If no information from a segment is needed, skip the
                 * unnecessary parsing.
                 */

                if ((parse_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT) &&
                    (parse_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION))
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

                /*
                 * We're looking for the objc image-info section, located in
                 * either the __DATA, __DATA_DIRTY, __DATA_CONST, __OBJC
                 * segments.
                 */

                const struct segment_command_64 *const segment =
                    (const struct segment_command_64 *)load_cmd_iter;

                const char *const segname = segment->segname;
                if (strncmp(segname, "__DATA", 16) != 0 &&
                    strncmp(segname, "__DATA_DIRTY", 16) != 0 &&
                    strncmp(segname, "__DATA_CONST", 16) != 0 &&
                    strncmp(segname, "__OBJC", 16) != 0)
                {
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

                const uint32_t sections_size = sizeof(struct section) * nsects;

                /*
                 * Ensure no overflow occurs when calculating the total sections
                 * size.
                 */

                if (sections_size / sizeof(struct section) != nsects) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                const uint32_t max_sections_size =
                    load_cmd.cmdsize - sizeof(struct segment_command_64);

                if (sections_size > max_sections_size) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS;
                }

                const uint8_t *const sect_ptr =
                    load_cmd_iter + sizeof(struct segment_command_64);

                const struct section_64 *sect = (struct section_64 *)sect_ptr;
                for (uint32_t i = 0; i < nsects; i++, sect++) {
                    const char *const sectname = sect->sectname;
                    if (strncmp(sectname, "__image_info", 16) != 0 &&
                        strncmp(sectname, "__objc_imageinfo", 16) != 0)
                    {
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

                    /*
                     * This is an ugly hack, but we don't really have any other
                     * choice for this right now.
                     */

                    if (size != 0) {
                        if (sect_offset > size) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_INVALID_SECTION;
                        }

                        if (sect_offset + sect_size > size) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_INVALID_SECTION;
                        }

                        if (lseek(fd, start + sect_offset, SEEK_SET) < 0) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_SEEK_FAIL;
                        }
                    } else {
                        if (lseek(fd, start + sect_offset, SEEK_SET) < 0) {
                            free(load_cmd_buffer);

                            /*
                             * It's possible the fd is too small as we haven't
                             * checked if the offset is contained within the
                             * fd.
                             */

                            if (errno == EINVAL) {
                                return E_MACHO_FILE_PARSE_INVALID_SECTION;
                            }
 
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
                        parse_objc_constraint(info, image_info.flags);
            
                    if (parse_constraint_result != E_MACHO_FILE_PARSE_OK) {
                        if (!(options & O_MACHO_FILE_IGNORE_INVALID_FIELDS)) {
                            free(load_cmd_buffer);
                            return parse_constraint_result;
                        }
                    }

                    const uint32_t mask = objc_image_info_swift_version_mask;
                    const uint32_t swift_version =
                        (image_info.flags & mask) >> 8;

                    if (info->swift_version != 0) {
                        if (info->swift_version != swift_version) {
                            free(load_cmd_buffer);
                            return E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION;
                        }
                    } else {
                        info->swift_version = swift_version;
                    }

                    /*
                     * Don't break here to ensure no other image-info section
                     * has different information.
                     */
                }

                break;
            }

            case LC_SUB_CLIENT: {
                /*
                 * If no sub-clients are needed, skip the unnecessary parsing.
                 */

                if (parse_options & O_TBD_PARSE_IGNORE_CLIENTS) {
                    break;
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

                if (client_offset < sizeof(struct sub_client_command)) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                if (client_offset >= load_cmd.cmdsize) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                /*
                 * Ensure that the umbrella-string offset is not within the
                 * basic structure, and not outside of the load-command.
                 */

                const uint32_t client_string_length =
                    load_cmd.cmdsize - client_offset;

                char *const client_string = calloc(1, client_string_length + 1);
                if (client_string == NULL) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                }

                const char *const client_ptr =
                    (const char *)client_command + client_offset;

                memcpy(client_string, client_ptr, client_string_length);

                /*
                 * Do a quick check here to ensure that the client-string is in
                 * fact filled with *non-whitespace* characters.
                 */

                if (c_str_is_all_whitespace(client_string)) {
                    free(client_string);
                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                const enum macho_file_parse_result add_client_result =
                    add_export_to_info(info,
                                       arch_bit,
                                       TBD_EXPORT_TYPE_CLIENT,
                                       client_string);

                if (add_client_result != E_MACHO_FILE_PARSE_OK) {
                    free(client_string);
                    free(load_cmd_buffer);

                    return add_client_result;
                }

                break;
            }
            
            case LC_SUB_FRAMEWORK: {
                /*
                 * If no sub-umbrella are needed, skip the unnecessary parsing.
                 */

                if (parse_options & O_TBD_PARSE_IGNORE_PARENT_UMBRELLA) {
                    break;
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
                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
                        break;
                    }

                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA;
                }

                if (umbrella_offset >= load_cmd.cmdsize) {
                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
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

                const uint32_t umbrella_string_length =
                    load_cmd.cmdsize - umbrella_offset;

                char *const umbrella_string =
                    calloc(1, umbrella_string_length + 1);

                if (umbrella_string == NULL) {
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_ALLOC_FAIL;
                }

                const char *const umbrella_ptr =
                    (const char *)load_cmd_iter + umbrella_offset;

                memcpy(umbrella_string, umbrella_ptr, umbrella_string_length);

                /*
                 * Do a quick check here to ensure that the umbrella-string is
                 * in fact filled with *non-whitespace* characters.
                 */

                if (c_str_is_all_whitespace(umbrella_string)) {
                    free(umbrella_string);
                    if (options & O_MACHO_FILE_IGNORE_INVALID_FIELDS) {
                        break;
                    }
                    
                    free(load_cmd_buffer);
                    return E_MACHO_FILE_PARSE_INVALID_CLIENT;
                }

                if (info->parent_umbrella != NULL) {
                    if (strcmp(info->parent_umbrella, umbrella_string) != 0) {
                        free(umbrella_string);
                        free(load_cmd_buffer);

                        return E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA;
                    }
                
                    free(umbrella_string);
                } else {
                    info->parent_umbrella = umbrella_string;
                }

                break;
            }

            case LC_SYMTAB: {
                /*
                 * If symbols aren't needed, skip the unnecessary parsing.
                 */

                if (parse_options & O_TBD_PARSE_IGNORE_SYMBOLS) {
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

                if (parse_options & O_TBD_PARSE_IGNORE_UUID) {
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

                    if (strncmp(uuid_str, uuid_cmd_uuid, 16) != 0) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
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

                if (parse_options & O_TBD_PARSE_IGNORE_PLATFORM) {
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

                if (info->platform != 0) {
                    if (info->platform != TBD_PLATFORM_MACOS) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                    }
                } else {
                    info->platform = TBD_PLATFORM_MACOS;
                }

                break;
            }

            case LC_VERSION_MIN_IPHONEOS: {
                /*
                 * If the platform isn't needed, skip the unnecessary parsing.
                 */

                if (parse_options & O_TBD_PARSE_IGNORE_PLATFORM) {
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

                if (info->platform != 0) {
                    if (info->platform != TBD_PLATFORM_IOS) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                    }
                } else {
                    info->platform = TBD_PLATFORM_IOS;
                }

                break;
            }

            case LC_VERSION_MIN_WATCHOS: {
                /*
                 * If the platform isn't needed, skip the unnecessary parsing.
                 */

                if (parse_options & O_TBD_PARSE_IGNORE_PLATFORM) {
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

                if (info->platform != 0) {
                    if (info->platform != TBD_PLATFORM_WATCHOS) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                    }
                } else {
                    info->platform = TBD_PLATFORM_WATCHOS;
                }

                break;
            }

            case LC_VERSION_MIN_TVOS: {
                /*
                 * If the platform isn't needed, skip the unnecessary parsing.
                 */

                if (parse_options & O_TBD_PARSE_IGNORE_PLATFORM) {
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

                if (info->platform != 0) {
                    if (info->platform != TBD_PLATFORM_TVOS) {
                        free(load_cmd_buffer);
                        return E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM;
                    }
                } else {
                    info->platform = TBD_PLATFORM_TVOS;
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

    if (!(parse_options & O_TBD_PARSE_IGNORE_SYMBOLS)) {
        if (symtab.cmd != LC_SYMTAB) {
            return E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE;
        }
    }

    if (!(parse_options & O_TBD_PARSE_IGNORE_UUID)) {
        if (!found_uuid) {
            return E_MACHO_FILE_PARSE_NO_UUID;
        }
    }   
    
    /*
     * Ensure that the uuid found is unique among all other containers before
     * adding to the fd's uuid arrays.
     */

    const uint8_t *const array_uuid =
        array_find_item(&info->uuids,
                        sizeof(uuid_info),
                        &uuid_info,
                        tbd_uuid_info_comparator,
                        NULL);

    if (array_uuid != NULL) {
        return E_MACHO_FILE_PARSE_CONFLICTING_UUID;
    }
    
    const enum array_result add_uuid_info_result =
        array_add_item(&info->uuids, sizeof(uuid_info), &uuid_info, NULL);

    if (add_uuid_info_result != E_ARRAY_OK) {
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
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

    /*
     * Verify the symbol-table's information.
     */

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    if (is_64) {
        ret =
            macho_file_parse_symbols_64(info,
                                        fd,
                                        arch_bit,
                                        start,
                                        size,
                                        is_big_endian,
                                        symtab.symoff,
                                        symtab.nsyms,
                                        symtab.stroff,
                                        symtab.strsize,
                                        parse_options,
                                        options);
    } else {
        ret =
            macho_file_parse_symbols(info,
                                     fd,
                                     arch_bit,
                                     start,
                                     size,
                                     is_big_endian,
                                     symtab.symoff,
                                     symtab.nsyms,
                                     symtab.stroff,
                                     symtab.strsize,
                                     parse_options,
                                     options);
    }

    if (ret != E_MACHO_FILE_PARSE_OK) {
        return ret;
    }

    if (array_is_empty(&info->exports)) {
        return E_MACHO_FILE_PARSE_NO_EXPORTS;
    }

    return E_MACHO_FILE_PARSE_OK;
}
