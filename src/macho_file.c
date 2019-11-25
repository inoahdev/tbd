//
//  src/macho_file.c
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <inttypes.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arch_info.h"
#include "mach-o/fat.h"
#include "guard_overflow.h"

#include "mach-o/loader.h"
#include "mach/machine.h"
#include "macho_file.h"
#include "macho_file_parse_load_commands.h"

#include "our_io.h"
#include "range.h"
#include "swap.h"
#include "tbd.h"

static inline bool magic_is_64_bit(const uint32_t magic) {
    switch (magic) {
        case MH_MAGIC_64:
        case MH_CIGAM_64:
            return true;

        default:
            return false;
    }
}

static inline bool
call_callback(struct tbd_create_info *__notnull const info_in,
              const enum macho_file_parse_error error,
              const macho_file_parse_error_callback callback,
              void *const cb_info)
{
    if (callback == NULL) {
        return false;
    }

    return (callback(info_in, error, cb_info));
}

static enum macho_file_parse_result
parse_thin_file(struct tbd_create_info *__notnull const info_in,
                const int fd,
                const struct range container_range,
                const struct mach_header *const header,
                const struct arch_info *const arch,
                const bool is_big_endian,
                struct macho_file_parse_extra_args extra,
                const uint64_t tbd_options,
                const uint64_t options)
{
    const uint32_t magic = header->magic;
    const bool is_64 = magic_is_64_bit(magic);

    uint64_t lc_flags = 0;
    uint32_t header_size = sizeof(struct mach_header);

    if (is_64) {
        const uint64_t container_size = range_get_size(container_range);
        if (container_size < sizeof(struct mach_header_64)) {
            return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
        }

        /*
         * 64-bit mach-o files have a different header (struct mach_header_64),
         * which only differs by having an extra uint32_t field at the end.
         */

        const uint64_t offset =
            container_range.begin + sizeof(struct mach_header_64);

        if (our_lseek(fd, offset, SEEK_SET) < 0) {
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }

        lc_flags |= F_MF_PARSE_LOAD_COMMANDS_IS_64;
        header_size = sizeof(struct mach_header_64);
    }

    if (is_big_endian) {
        lc_flags |= F_MF_PARSE_LOAD_COMMANDS_IS_BIG_ENDIAN;
    }

    const uint64_t info_flags = info_in->fields.flags;
    const uint64_t mh_flags = header->flags;

    if (info_flags != 0) {
        if (info_flags & TBD_FLAG_FLAT_NAMESPACE) {
            if (!(mh_flags & MH_TWOLEVEL)) {
                const bool should_ignore =
                    call_callback(info_in,
                                  ERR_MACHO_FILE_PARSE_FLAGS_CONFLICT,
                                  extra.callback,
                                  extra.callback_info);

                if (!should_ignore) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        }

        if (info_flags & TBD_FLAG_NOT_APP_EXTENSION_SAFE) {
            if (mh_flags & MH_APP_EXTENSION_SAFE) {
                const bool should_ignore =
                    call_callback(info_in,
                                  ERR_MACHO_FILE_PARSE_FLAGS_CONFLICT,
                                  extra.callback,
                                  extra.callback_info);

                if (!should_ignore) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        }
    } else {
        if (mh_flags & MH_TWOLEVEL) {
            info_in->fields.flags |= TBD_FLAG_FLAT_NAMESPACE;
        }

        if (!(mh_flags & MH_APP_EXTENSION_SAFE)) {
            info_in->fields.flags |= TBD_FLAG_NOT_APP_EXTENSION_SAFE;
        }
    }

    const struct range lc_available_range = {
        .begin = container_range.begin + header_size,
        .end = container_range.end,
    };

    /*
     * Ignore if arch is NULL, as arch_index would be ignored as well.
     */

    const uint64_t arch_index = (uint64_t)(arch - arch_info_get_list());
    struct mf_parse_lc_from_file_info info = {
        .fd = fd,

        .arch = arch,
        .arch_bit = (1ull << arch_index),

        .macho_range = container_range,
        .available_range = lc_available_range,

        .ncmds = header->ncmds,
        .sizeofcmds = header->sizeofcmds,
        .header_size = header_size,

        .tbd_options = tbd_options,
        .options = options,

        .flags = lc_flags
    };

    const enum macho_file_parse_result parse_load_commands_result =
        macho_file_parse_load_commands_from_file(info_in,
                                                 &info,
                                                 extra,
                                                 NULL);

    if (parse_load_commands_result != E_MACHO_FILE_PARSE_OK) {
        return parse_load_commands_result;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
verify_fat_32_arch(struct tbd_create_info *__notnull const info_in,
                   struct fat_arch *__notnull const arch,
                   const uint64_t macho_base,
                   const struct range available_range,
                   const bool is_big_endian,
                   const uint64_t tbd_options,
                   struct range *const range_out)
{
    cpu_type_t arch_cputype = arch->cputype;
    cpu_subtype_t arch_cpusubtype = arch->cpusubtype;

    uint32_t arch_offset = arch->offset;
    uint32_t arch_size = arch->size;

    if (is_big_endian) {
        arch_cputype = swap_int32(arch_cputype);
        arch_cpusubtype = swap_int32(arch_cpusubtype);

        arch_offset = swap_uint32(arch_offset);
        arch_size = swap_uint32(arch_size);

        arch->cputype = arch_cputype;
        arch->cpusubtype = arch_cpusubtype;

        arch->offset = arch_offset;
        arch->size = arch_size;
    }

    /*
     * A valid mach-o architecture should be large enough to contain a
     * mach_header.
     */

    if (arch_size < sizeof(struct mach_header)) {
        return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
    }

    uint32_t arch_end = arch_offset;
    if (guard_overflow_add(&arch_end, arch_size)) {
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    uint64_t full_arch_offset = macho_base;
    if (guard_overflow_add(&full_arch_offset, arch_offset)) {
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    uint64_t full_arch_end = macho_base;
    if (guard_overflow_add(&full_arch_end, arch_end)) {
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    const struct range arch_range = {
        .begin = full_arch_offset,
        .end = full_arch_end
    };

    if (!range_contains_other(available_range, arch_range)) {
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    const struct arch_info *arch_info = NULL;
    if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
        arch_info = arch_info_for_cputype(arch_cputype, arch_cpusubtype);
        if (arch_info == NULL) {
            return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
        }

        const struct arch_info *const arch_info_list = arch_info_get_list();

        const uint64_t arch_index = (uint64_t)(arch_info - arch_info_list);
        const uint64_t arch_bit = 1ull << arch_index;

        if (info_in->fields.archs & arch_bit) {
            return E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE;
        }

        info_in->fields.archs |= arch_bit;

        /*
         * To avoid re-lookup of arch-info, we store the pointer within the
         * cputype and cpusubtype fields.
         */

        memcpy(&arch->cputype, &arch_info, sizeof(arch_info));
    }

    if (range_out != NULL) {
        range_out->begin = arch_offset;
        range_out->end = arch_end;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static bool magic_is_thin(const uint32_t magic) {
    switch (magic) {
        case MH_MAGIC:
        case MH_CIGAM:
        case MH_MAGIC_64:
        case MH_CIGAM_64:
            return true;

        default:
            return false;
    }
}

static bool magic_is_big_endian(const uint32_t magic) {
    switch (magic) {
        case MH_CIGAM:
        case MH_CIGAM_64:
        case FAT_CIGAM:
        case FAT_CIGAM_64:
            return true;

        default:
            return false;
    }
}

static inline bool is_invalid_filetype(const uint32_t filetype) {
    switch (filetype) {
        case MH_BUNDLE:
        case MH_DYLIB:
        case MH_DYLIB_STUB:
            return false;

        default:
            return true;
    }
}

static enum macho_file_parse_result
handle_fat_32_file(struct tbd_create_info *__notnull const info_in,
                   const int fd,
                   const struct range macho_range,
                   const uint32_t nfat_arch,
                   const bool is_big_endian,
                   struct macho_file_parse_extra_args extra,
                   const uint64_t tbd_options,
                   const uint64_t options)
{
    /*
     * Calculate the total-size of the architectures given.
     */

    uint64_t archs_size = sizeof(struct fat_arch);
    if (guard_overflow_mul(&archs_size, nfat_arch)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    uint64_t total_headers_size = sizeof(struct fat_header);
    if (guard_overflow_add(&total_headers_size, archs_size)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    uint64_t available_range_start = macho_range.begin;
    if (guard_overflow_add(&available_range_start, total_headers_size)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    if (!range_contains_location(macho_range, available_range_start)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    struct fat_arch *const arch_list = malloc(archs_size);
    if (arch_list == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (our_read(fd, arch_list, archs_size) < 0) {
        free(arch_list);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    /*
     * Loop over the architectures once to verify its info, then loop over again
     * to parse.
     */

    struct fat_arch *const first_arch = arch_list;
    const struct range available_range = {
        .begin = available_range_start,
        .end = macho_range.end,
    };

    const enum macho_file_parse_result verify_first_arch_result =
        verify_fat_32_arch(info_in,
                           first_arch,
                           macho_range.begin,
                           available_range,
                           is_big_endian,
                           tbd_options,
                           NULL);

    if (verify_first_arch_result != E_MACHO_FILE_PARSE_OK) {
        free(arch_list);
        return verify_first_arch_result;
    }

    for (uint32_t i = 1; i != nfat_arch; i++) {
        struct fat_arch *const arch = arch_list + i;
        struct range arch_range = {};

        const enum macho_file_parse_result verify_arch_result =
            verify_fat_32_arch(info_in,
                               arch,
                               macho_range.begin,
                               available_range,
                               is_big_endian,
                               tbd_options,
                               &arch_range);

        if (verify_arch_result != E_MACHO_FILE_PARSE_OK) {
            free(arch_list);
            return verify_arch_result;
        }

        /*
         * Make sure the arch doesn't overlap with any previous archs.
         */

        for (uint32_t j = 0; j != i; j++) {
            struct fat_arch inner = arch_list[j];
            const struct range inner_range = {
                .begin = inner.offset,
                .end = inner.offset + inner.size
            };

            if (ranges_overlap(arch_range, inner_range)) {
                free(arch_list);
                return E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES;
            }
        }
    }

    uint32_t filetype = 0;

    bool ignore_filetype = false;
    bool parsed_one_arch = false;

    for (uint32_t i = 0; i != nfat_arch; i++) {
        const struct fat_arch arch = arch_list[i];
        const off_t arch_offset = (off_t)(macho_range.begin + arch.offset);

        if (our_lseek(fd, arch_offset, SEEK_SET) < 0) {
            free(arch_list);
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }

        struct mach_header header = {};
        if (our_read(fd, &header, sizeof(header)) < 0) {
            free(arch_list);
            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        /*
         * Swap the mach_header's fields if big-endian.
         */

        const bool arch_is_big_endian = magic_is_big_endian(header.magic);
        if (arch_is_big_endian) {
            header.cputype = swap_int32(header.cputype);
            header.cpusubtype = swap_int32(header.cpusubtype);

            header.ncmds = swap_uint32(header.ncmds);
            header.sizeofcmds = swap_uint32(header.sizeofcmds);

            header.filetype = swap_uint32(header.filetype);
            header.flags = swap_uint32(header.flags);
        } else if (!magic_is_thin(header.magic)) {
            if (options & O_MACHO_FILE_PARSE_SKIP_INVALID_ARCHITECTURES) {
                continue;
            }

            free(arch_list);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        if (!ignore_filetype) {
            if (filetype != 0) {
                if (header.filetype != filetype) {
                    const bool should_continue =
                        call_callback(info_in,
                                      ERR_MACHO_FILE_PARSE_FILETYPE_CONFLICT,
                                      extra.callback,
                                      extra.callback_info);

                    if (!should_continue) {
                        free(arch_list);
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }
                }
            } else if (is_invalid_filetype(header.filetype)) {
                if (!(options & O_MACHO_FILE_PARSE_IGNORE_WRONG_FILETYPE)) {
                    const bool should_continue =
                        call_callback(info_in,
                                      ERR_MACHO_FILE_PARSE_WRONG_FILETYPE,
                                      extra.callback,
                                      extra.callback_info);

                    if (!should_continue) {
                        free(arch_list);
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    ignore_filetype = true;
                }
            } else {
                filetype = header.filetype;
            }
        }

        /*
         * Verify that header's cpu-type matches arch's cpu-type.
         */

        const struct arch_info *arch_info = NULL;
        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
            arch_info = *(const struct arch_info **)&arch.cputype;
            if (header.cputype != arch_info->cputype) {
                free(arch_list);
                return E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO;
            }

            if (header.cpusubtype != arch_info->cpusubtype) {
                free(arch_list);
                return E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO;
            }
        }

        const struct range arch_range = {
            .begin = arch_offset,
            .end = arch_offset + arch.size
        };

        const enum macho_file_parse_result handle_arch_result =
            parse_thin_file(info_in,
                            fd,
                            arch_range,
                            &header,
                            arch_info,
                            arch_is_big_endian,
                            extra,
                            tbd_options,
                            options);

        if (handle_arch_result != E_MACHO_FILE_PARSE_OK) {
            free(arch_list);
            return handle_arch_result;
        }

        parsed_one_arch = true;
    }

    free(arch_list);

    if (!parsed_one_arch) {
        return E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
verify_fat_64_arch(struct tbd_create_info *__notnull const info_in,
                   struct fat_arch_64 *__notnull const arch,
                   const uint64_t macho_base,
                   const struct range available_range,
                   const bool is_big_endian,
                   const uint64_t tbd_options,
                   struct range *const range_out)
{
    cpu_type_t arch_cputype = arch->cputype;
    cpu_subtype_t arch_cpusubtype = arch->cpusubtype;

    uint64_t arch_offset = arch->offset;
    uint64_t arch_size = arch->size;

    if (is_big_endian) {
        arch_cputype = swap_int32(arch_cputype);
        arch_cpusubtype = swap_int32(arch_cpusubtype);

        arch_offset = swap_uint64(arch_offset);
        arch_size = swap_uint64(arch_size);

        arch->cputype = arch_cputype;
        arch->cpusubtype = arch_cpusubtype;

        arch->offset = arch_offset;
        arch->size = arch_size;
    }

    /*
     * A valid mach-o architecture should be large enough to contain a
     * mach_header.
     */

    if (arch_size < sizeof(struct mach_header)) {
        return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
    }

    /*
     * Verify that no overflow occurs when finding arch's end.
     */

    uint64_t arch_end = arch_offset;
    if (guard_overflow_add(&arch_end, arch_size)) {
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    uint64_t full_arch_offset = macho_base;
    if (guard_overflow_add(&full_arch_offset, arch_offset)) {
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    uint64_t full_arch_end = macho_base;
    if (guard_overflow_add(&full_arch_end, arch_end)) {
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    const struct range arch_range = {
        .begin = full_arch_offset,
        .end = full_arch_end
    };

    if (!range_contains_other(available_range, arch_range)) {
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
        const struct arch_info *const arch_info =
            arch_info_for_cputype(arch_cputype, arch_cpusubtype);

        if (arch_info == NULL) {
            return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
        }

        const struct arch_info *const arch_info_list = arch_info_get_list();

        const uint64_t arch_index = (uint64_t)(arch_info - arch_info_list);
        const uint64_t arch_bit = 1ull << arch_index;

        if (info_in->fields.archs & arch_bit) {
            return E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE;
        }

        info_in->fields.archs |= arch_bit;

        /*
         * To avoid re-lookup of arch-info, we store the pointer within the
         * cputype and cpusubtype fields.
         */

        memcpy(&arch->cputype, &arch_info, sizeof(arch_info));
    }

    if (range_out != NULL) {
        range_out->begin = arch_offset;
        range_out->end = arch_end;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
handle_fat_64_file(struct tbd_create_info *__notnull const info_in,
                   const int fd,
                   const struct range macho_range,
                   const uint32_t nfat_arch,
                   const bool is_big_endian,
                   struct macho_file_parse_extra_args extra,
                   const uint64_t tbd_options,
                   const uint64_t options)
{
    /*
     * Calculate the total-size of the architectures given.
     */

    uint64_t archs_size = sizeof(struct fat_arch_64);
    if (guard_overflow_mul(&archs_size, nfat_arch)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    uint64_t total_headers_size = sizeof(struct fat_header);
    if (guard_overflow_add(&total_headers_size, archs_size)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    uint64_t available_range_start = macho_range.begin;
    if (guard_overflow_add(&available_range_start, total_headers_size)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    if (!range_contains_location(macho_range, available_range_start)) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    struct fat_arch_64 *const arch_list = malloc(archs_size);
    if (arch_list == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (our_read(fd, arch_list, archs_size) < 0) {
        free(arch_list);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    /*
     * Loop over the architectures once to verify its info, then loop over again
     * to parse.
     */

    struct fat_arch_64 *const first_arch = arch_list;
    const struct range available_range = {
        .begin = available_range_start,
        .end = macho_range.end
    };

    const enum macho_file_parse_result verify_first_arch_result =
        verify_fat_64_arch(info_in,
                           first_arch,
                           macho_range.begin,
                           available_range,
                           is_big_endian,
                           tbd_options,
                           NULL);

    if (verify_first_arch_result != E_MACHO_FILE_PARSE_OK) {
        free(arch_list);
        return verify_first_arch_result;
    }

    /*
     * Before parsing, verify each architecture.
     */

    for (uint32_t i = 1; i != nfat_arch; i++) {
        struct fat_arch_64 *const arch = arch_list + i;
        struct range arch_range = {};

        const enum macho_file_parse_result verify_arch_result =
            verify_fat_64_arch(info_in,
                               arch,
                               macho_range.begin,
                               available_range,
                               is_big_endian,
                               tbd_options,
                               &arch_range);

        if (verify_arch_result != E_MACHO_FILE_PARSE_OK) {
            free(arch_list);
            return verify_arch_result;
        }

        for (uint32_t j = 0; j != i; j++) {
            struct fat_arch_64 inner = arch_list[j];
            const struct range inner_range = {
                .begin = inner.offset,
                .end = inner.offset + inner.size
            };

            if (ranges_overlap(arch_range, inner_range)) {
                free(arch_list);
                return E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES;
            }
        }
    }

    uint32_t filetype = 0;

    bool parsed_one_arch = false;
    bool ignore_filetype = false;

    for (uint32_t i = 0; i != nfat_arch; i++) {
        const struct fat_arch_64 arch = arch_list[i];
        const off_t arch_offset = (off_t)(macho_range.begin + arch.offset);

        if (our_lseek(fd, arch_offset, SEEK_SET) < 0) {
            free(arch_list);
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }

        struct mach_header header = {};
        if (our_read(fd, &header, sizeof(header)) < 0) {
            free(arch_list);
            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        /*
         * Swap mach_header's fields if big-endian as we deal only in
         * little-endian.
         */

        const bool arch_is_big_endian = magic_is_big_endian(header.magic);
        if (arch_is_big_endian) {
            header.cputype = swap_int32(header.cputype);
            header.cpusubtype = swap_int32(header.cpusubtype);

            header.ncmds = swap_uint32(header.ncmds);
            header.sizeofcmds = swap_uint32(header.sizeofcmds);

            header.filetype = swap_uint32(header.filetype);
            header.flags = swap_uint32(header.flags);
        } else if (!magic_is_thin(header.magic)) {
            if (options & O_MACHO_FILE_PARSE_SKIP_INVALID_ARCHITECTURES) {
                continue;
            }

            free(arch_list);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        if (!ignore_filetype) {
            if (filetype != 0) {
                if (header.filetype != filetype) {
                    const bool should_continue =
                        call_callback(info_in,
                                      ERR_MACHO_FILE_PARSE_FILETYPE_CONFLICT,
                                      extra.callback,
                                      extra.callback_info);

                    if (!should_continue) {
                        free(arch_list);
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }
                }
            } else if (is_invalid_filetype(header.filetype)) {
                if (!(options & O_MACHO_FILE_PARSE_IGNORE_WRONG_FILETYPE)) {
                    const bool should_continue =
                        call_callback(info_in,
                                      ERR_MACHO_FILE_PARSE_WRONG_FILETYPE,
                                      extra.callback,
                                      extra.callback_info);

                    if (!should_continue) {
                        free(arch_list);
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }

                    ignore_filetype = true;
                }
            } else {
                filetype = header.filetype;
            }
        }

        /*
         * Verify that header's cpu-type matches arch's cpu-type.
         */

        const struct arch_info *arch_info = NULL;
        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
            arch_info = *(const struct arch_info **)&arch.cputype;
            if (header.cputype != arch_info->cputype) {
                free(arch_list);
                return E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO;
            }

            if (header.cpusubtype != arch_info->cpusubtype) {
                free(arch_list);
                return E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO;
            }
        }

        const struct range arch_range = {
            .begin = arch_offset,
            .end = arch_offset + arch.size
        };

        const enum macho_file_parse_result handle_arch_result =
            parse_thin_file(info_in,
                            fd,
                            arch_range,
                            &header,
                            arch_info,
                            arch_is_big_endian,
                            extra,
                            tbd_options,
                            options);

        if (handle_arch_result != E_MACHO_FILE_PARSE_OK) {
            free(arch_list);
            return handle_arch_result;
        }

        parsed_one_arch = true;
    }

    free(arch_list);

    if (!parsed_one_arch) {
        return E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static bool magic_is_fat(const uint32_t magic) {
    switch (magic) {
        case FAT_MAGIC:
        case FAT_CIGAM:
        case FAT_MAGIC_64:
        case FAT_CIGAM_64:
            return true;

        default:
            return false;
    }
}

static bool magic_is_fat_64(const uint32_t magic) {
    switch (magic) {
        case FAT_MAGIC_64:
        case FAT_CIGAM_64:
            return true;

        default:
            return false;
    }
}

enum macho_file_parse_result
macho_file_parse_from_file(struct tbd_create_info *__notnull const info_in,
                           const int fd,
                           const uint32_t magic,
                           struct macho_file_parse_extra_args extra,
                           const uint64_t tbd_options,
                           const uint64_t options)
{
    struct range macho_range = { .begin = 0 };
    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;

    if (magic_is_fat(magic)) {
        uint32_t nfat_arch = 0;
        if (our_read(fd, &nfat_arch, sizeof(nfat_arch)) < 0) {
            if (errno == EOVERFLOW) {
                return E_MACHO_FILE_PARSE_NOT_A_MACHO;
            }

            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        if (nfat_arch == 0) {
            return E_MACHO_FILE_PARSE_NO_ARCHITECTURES;
        }

        const bool is_big_endian = magic_is_big_endian(magic);
        if (is_big_endian) {
            nfat_arch = swap_uint32(nfat_arch);
        }

        struct stat sbuf = {};
        if (fstat(fd, &sbuf) < 0) {
            return E_MACHO_FILE_PARSE_FSTAT_FAIL;
        }

        macho_range.end = sbuf.st_size;

        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
            info_in->fields.archs_count = nfat_arch;
        }

        const bool is_64 = magic_is_fat_64(magic);
        if (is_64) {
            ret = handle_fat_64_file(info_in,
                                     fd,
                                     macho_range,
                                     nfat_arch,
                                     is_big_endian,
                                     extra,
                                     tbd_options,
                                     options);
        } else {
            ret = handle_fat_32_file(info_in,
                                     fd,
                                     macho_range,
                                     nfat_arch,
                                     is_big_endian,
                                     extra,
                                     tbd_options,
                                     options);
        }

        if (ret != E_MACHO_FILE_PARSE_OK) {
            return ret;
        }

        const uint64_t ignore_missing_exports_flags =
            (O_TBD_PARSE_IGNORE_EXPORTS | O_TBD_PARSE_IGNORE_MISSING_EXPORTS);

        if ((tbd_options & ignore_missing_exports_flags) == 0) {
            if (info_in->fields.exports.item_count == 0) {
                return E_MACHO_FILE_PARSE_NO_EXPORTS;
            }
        }

        /*
         * If exports and undefineds were created with a full arch-set, they are
         * already sorted.
         */

        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
            array_sort_with_comparator(&info_in->fields.exports,
                                       sizeof(struct tbd_symbol_info),
                                       tbd_symbol_info_comparator);

            array_sort_with_comparator(&info_in->fields.undefineds,
                                       sizeof(struct tbd_symbol_info),
                                       tbd_symbol_info_comparator);
        } else {
            info_in->flags |= F_TBD_CREATE_INFO_EXPORTS_HAVE_FULL_ARCHS;
        }
    } else if (magic_is_thin(magic)) {
        struct mach_header header = { .magic = magic };
        if (our_read(fd, &header.cputype, sizeof(header) - sizeof(magic)) < 0) {
            if (errno == EOVERFLOW) {
                return E_MACHO_FILE_PARSE_NOT_A_MACHO;
            }

            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        /*
         * Swap the mach_header's fields if big-endian.
         */

        const bool is_big_endian = magic_is_big_endian(magic);
        if (is_big_endian) {
            header.cputype = swap_int32(header.cputype);
            header.cpusubtype = swap_int32(header.cpusubtype);

            header.ncmds = swap_uint32(header.ncmds);
            header.sizeofcmds = swap_uint32(header.sizeofcmds);

            header.filetype = swap_uint32(header.filetype);
            header.flags = swap_uint32(header.flags);
        }

        if (is_invalid_filetype(header.filetype)) {
            if (!(options & O_MACHO_FILE_PARSE_IGNORE_WRONG_FILETYPE)) {
                const bool should_continue =
                    call_callback(info_in,
                                  ERR_MACHO_FILE_PARSE_WRONG_FILETYPE,
                                  extra.callback,
                                  extra.callback_info);

                if (!should_continue) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        }

        struct stat sbuf = {};
        if (fstat(fd, &sbuf) < 0) {
            return E_MACHO_FILE_PARSE_FSTAT_FAIL;
        }

        macho_range.end = sbuf.st_size;

        const struct arch_info *arch = NULL;
        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS)) {
            arch = arch_info_for_cputype(header.cputype, header.cpusubtype);
            if (arch == NULL) {
                return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
            }

            const struct arch_info *const arch_info_list = arch_info_get_list();

            const uint64_t arch_index = (uint64_t)(arch - arch_info_list);
            const uint64_t arch_bit = 1ull << arch_index;

            info_in->fields.archs = arch_bit;
            info_in->fields.archs_count = 1;

            info_in->flags |= F_TBD_CREATE_INFO_EXPORTS_HAVE_FULL_ARCHS;
        }

        ret = parse_thin_file(info_in,
                              fd,
                              macho_range,
                              &header,
                              arch,
                              is_big_endian,
                              extra,
                              tbd_options,
                              options);

        if (ret != E_MACHO_FILE_PARSE_OK) {
            return ret;
        }

        const uint64_t ignore_missing_exports_flags =
            (O_TBD_PARSE_IGNORE_EXPORTS | O_TBD_PARSE_IGNORE_MISSING_EXPORTS);

        if ((tbd_options & ignore_missing_exports_flags) == 0) {
            if (info_in->fields.exports.item_count == 0) {
                return E_MACHO_FILE_PARSE_NO_EXPORTS;
            }
        }
    } else {
        return E_MACHO_FILE_PARSE_NOT_A_MACHO;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static bool magic_is_fat_32(const uint32_t magic) {
    switch (magic) {
        case FAT_MAGIC:
        case FAT_CIGAM:
            return true;

        default:
            return false;
    }
}

void macho_file_print_archs(const int fd) {
    uint32_t magic = 0;
    if (our_read(fd, &magic, sizeof(magic)) < 0) {
        fprintf(stderr,
                "Failed to read data from mach-o, error: %s\n",
                strerror(errno));

        exit(1);
    }

    const bool is_fat_64 = magic_is_fat_64(magic);
    if (is_fat_64) {
        uint32_t nfat_arch = 0;
        if (our_read(fd, &nfat_arch, sizeof(nfat_arch)) < 0) {
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        if (nfat_arch == 0) {
            fputs("File has no architectures\n", stderr);
            exit(1);
        }

        const bool is_big_endian = magic_is_big_endian(magic);
        if (is_big_endian) {
            nfat_arch = swap_uint32(nfat_arch);
        }

        /*
         * Calculate the total-size of the architectures given.
         */

        uint64_t archs_size = sizeof(struct fat_arch_64);
        if (guard_overflow_mul(&archs_size, nfat_arch)) {
            fputs("File has too many architectures\n", stderr);
            exit(1);
        }

        struct fat_arch_64 *const archs = malloc(archs_size);
        if (archs == NULL) {
            fputs("Failed to allocate space for architectures\n", stderr);
            exit(1);
        }

        if (our_read(fd, archs, archs_size) < 0) {
            free(archs);
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        for (uint32_t i = 0; i != nfat_arch; i++) {
            struct fat_arch_64 arch = archs[i];
            if (is_big_endian) {
                arch.cputype = swap_int32(arch.cputype);
                arch.cpusubtype = swap_int32(arch.cpusubtype);
            }

            const struct arch_info *const arch_info =
                arch_info_for_cputype(arch.cputype, arch.cpusubtype);

            if (arch_info == NULL) {
                fprintf(stdout,
                        "\t%" PRIu32 ". (Unsupported architecture)\r\n",
                        i + 1);
            } else {
                fprintf(stdout,
                        "\t%" PRIu32 ". %s\r\n",
                        i + 1,
                        arch_info->name);
            }
        }

        free(archs);
    } else if (magic_is_fat_32(magic)) {
        uint32_t nfat_arch = 0;
        if (our_read(fd, &nfat_arch, sizeof(nfat_arch)) < 0) {
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        if (nfat_arch == 0) {
            fputs("File has no architectures\n", stderr);
            exit(1);
        }

        const bool is_big_endian = magic_is_big_endian(magic);
        if (is_big_endian) {
            nfat_arch = swap_uint32(nfat_arch);
        }

        /*
         * Calculate the total-size of the architectures given.
         */

        uint64_t archs_size = sizeof(struct fat_arch);
        if (guard_overflow_mul(&archs_size, nfat_arch)) {
            fputs("File has too many architectures\n", stderr);
            exit(1);
        }

        struct fat_arch *const archs = malloc(archs_size);
        if (archs == NULL) {
            fputs("Failed to allocate space for architectures\n", stderr);
            exit(1);
        }

        if (our_read(fd, archs, archs_size) < 0) {
            free(archs);
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        fprintf(stdout, "%" PRIu32 " architecture(s):\n", nfat_arch);
        for (uint32_t i = 0; i != nfat_arch; i++) {
            struct fat_arch arch = archs[i];
            if (is_big_endian) {
                arch.cputype = swap_int32(arch.cputype);
                arch.cpusubtype = swap_int32(arch.cpusubtype);
            }

            const struct arch_info *const arch_info =
                arch_info_for_cputype(arch.cputype, arch.cpusubtype);

            if (arch_info == NULL) {
                fprintf(stdout,
                        "\t%" PRIu32 ". (Unsupported architecture)\r\n",
                        i + 1);
            } else {
                fprintf(stdout,
                        "\t%" PRIu32 ". %s\r\n",
                        i + 1,
                        arch_info->name);
            }
        }

        free(archs);
    } else if (magic_is_thin(magic)) {
        struct mach_header header = {};
        const uint32_t read_size =
            sizeof(header.cputype) + sizeof(header.cpusubtype);

        if (our_read(fd, &header.cputype, read_size) < 0) {
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        const bool is_big_endian = magic_is_big_endian(magic);
        if (is_big_endian) {
            header.cputype = swap_int32(header.cputype);
            header.cpusubtype = swap_int32(header.cpusubtype);
        }

        const struct arch_info *const arch_info =
            arch_info_for_cputype(header.cputype, header.cpusubtype);

        if (arch_info == NULL) {
            fputs("(Unsupported architecture)\n", stdout);
        } else {
            fprintf(stdout, "%s\n", arch_info->name);
        }
    } else {
        fprintf(stderr,
                "File is not a valid mach-o; Unrecognized magic: 0x%x\n",
                magic);

        exit(1);
    }
}
