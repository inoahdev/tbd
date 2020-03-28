//
//  src/macho_file.c
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <errno.h>
#include <inttypes.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "mach/machine.h"
#include "mach-o/fat.h"
#include "guard_overflow.h"

#include "macho_file.h"
#include "macho_file_parse_load_commands.h"

#include "our_io.h"
#include "swap.h"
#include "target_list.h"
#include "tbd.h"

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

struct macho_magic_info {
    uint32_t magic;
    uint32_t other;
};

enum macho_file_open_result
macho_file_open(struct macho_file *__notnull const macho,
                struct magic_buffer *__notnull const buffer,
                const int fd,
                struct range range)
{
    const enum magic_buffer_result read_magic_result =
        magic_buffer_read_n(buffer, fd, sizeof(struct macho_magic_info));

    if (read_magic_result != E_MAGIC_BUFFER_OK) {
        return E_MACHO_FILE_OPEN_READ_FAIL;
    }

    uint32_t nfat_arch = 1;
    struct mach_header header = {};

    const struct macho_magic_info *const magic_info =
        (const struct macho_magic_info *)buffer->buff;

    const uint32_t magic = magic_info->magic;
    if (magic_is_fat(magic)) {
        nfat_arch = magic_info->other;
        if (magic_is_big_endian(magic)) {
            nfat_arch = swap_uint32(nfat_arch);
        }
    } else if (magic_is_thin(magic)) {
        header.magic = magic;
        header.cputype = (cpu_type_t)magic_info->other;

        const uint32_t read_size = sizeof(header) - sizeof(magic_info);
        if (our_read(fd, &header.cpusubtype, read_size) < 0) {
            if (errno == EOVERFLOW) {
                return E_MACHO_FILE_OPEN_NOT_A_MACHO;
            }

            return E_MACHO_FILE_OPEN_READ_FAIL;
        }

        /*
         * Swap the mach_header's fields if big-endian.
         */

        if (magic_is_big_endian(magic)) {
            header.cputype = swap_int32(header.cputype);
            header.cpusubtype = swap_int32(header.cpusubtype);

            header.ncmds = swap_uint32(header.ncmds);
            header.sizeofcmds = swap_uint32(header.sizeofcmds);

            header.filetype = swap_uint32(header.filetype);
            header.flags = swap_uint32(header.flags);
        }
    } else {
        return E_MACHO_FILE_OPEN_NOT_A_MACHO;
    }

    if (range.end == 0) {
        struct stat sbuf = {};
        if (fstat(fd, &sbuf) != 0) {
            return E_MACHO_FILE_OPEN_FSTAT_FAIL;
        }

        const uint64_t real_size = sbuf.st_size - range.begin;
        if (real_size < sizeof(struct mach_header)) {
            return E_MACHO_FILE_OPEN_NOT_A_MACHO;
        }

        range.end = real_size;
    }

    macho->fd = fd;
    macho->magic = magic;
    macho->nfat_arch = nfat_arch;
    macho->header = header;
    macho->range = range;

    return E_MACHO_FILE_OPEN_OK;
}

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
              const enum macho_file_parse_callback_type type,
              const macho_file_parse_error_callback callback,
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
parse_thin_file(struct tbd_create_info *__notnull const info_in,
                const int fd,
                const struct range container_range,
                const struct mach_header *const header,
                const struct arch_info *const arch,
                struct macho_file_parse_extra_args extra,
                const bool is_big_endian,
                const uint64_t arch_index,
                const struct tbd_parse_options tbd_options,
                const struct macho_file_parse_options options)
{
    const uint32_t magic = header->magic;
    const bool is_64 = magic_is_64_bit(magic);

    struct macho_file_parse_lc_flags lc_flags = {};
    uint32_t header_size = sizeof(struct mach_header);

    if (is_64) {
        const uint64_t container_size = range_get_size(container_range);
        if (container_size < sizeof(struct mach_header_64)) {
            return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
        }

        /*
         * 64-bit mach-o files have a different header (struct mach_header_64),
         * which only adds an extra uint32_t field to the end of struct
         * mach_header.
         */

        const uint64_t offset =
            container_range.begin + sizeof(struct mach_header_64);

        if (our_lseek(fd, offset, SEEK_SET) < 0) {
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }

        lc_flags.is_64 = true;
        header_size = sizeof(struct mach_header_64);
    }

    if (is_big_endian) {
        lc_flags.is_big_endian = true;
    }

    const struct tbd_flags info_flags = info_in->fields.flags;
    const uint64_t mh_flags = header->flags;

    if (info_flags.value != 0) {
        if (info_flags.flat_namespace) {
            if (!(mh_flags & MH_TWOLEVEL)) {
                const bool should_ignore =
                    call_callback(info_in,
                                  ERR_MACHO_FILE_PARSE_FLAGS_CONFLICT,
                                  extra.callback,
                                  extra.cb_info);

                if (!should_ignore) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        }

        if (info_flags.not_app_extension_safe) {
            if (mh_flags & MH_APP_EXTENSION_SAFE) {
                const bool should_ignore =
                    call_callback(info_in,
                                  ERR_MACHO_FILE_PARSE_FLAGS_CONFLICT,
                                  extra.callback,
                                  extra.cb_info);

                if (!should_ignore) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        }
    } else {
        if (mh_flags & MH_TWOLEVEL) {
            info_in->fields.flags.flat_namespace = true;
        }

        if (!(mh_flags & MH_APP_EXTENSION_SAFE)) {
            info_in->fields.flags.not_app_extension_safe = true;
        }
    }

    if (mh_flags & MH_NLIST_OUTOFSYNC_WITH_DYLDINFO) {
        if (options.use_symbol_table) {
            const bool should_ignore =
                call_callback(info_in,
                              WARN_MACHO_FILE_SYMBOL_TABLE_OUTOFSYNC,
                              extra.callback,
                              extra.cb_info);

            if (!should_ignore) {
                return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
            }
        }
    }

    const struct range lc_available_range = {
        .begin = container_range.begin + header_size,
        .end = container_range.end,
    };

    /*
     * Ignore if arch is NULL, as arch_index would be ignored as well.
     */

    struct mf_parse_lc_from_file_info info = {
        .fd = fd,

        .arch = arch,
        .arch_index = arch_index,

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
verify_fat_32_arch(struct fat_arch *__notnull const arch,
                   const uint64_t macho_base,
                   const struct range available_range,
                   const bool is_big_endian,
                   const struct tbd_parse_options options,
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
    if (!options.ignore_targets) {
        arch_info = arch_info_for_cputype(arch_cputype, arch_cpusubtype);
        if (arch_info == NULL) {
            return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
        }

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
                   const struct tbd_parse_options tbd_options,
                   const struct macho_file_parse_options options)
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
        verify_fat_32_arch(first_arch,
                           macho_range.begin,
                           available_range,
                           is_big_endian,
                           tbd_options,
                           NULL);

    if (verify_first_arch_result != E_MACHO_FILE_PARSE_OK) {
        free(arch_list);
        return verify_first_arch_result;
    }

    struct fat_arch *arch = first_arch + 1;
    const struct fat_arch *const end = arch_list + nfat_arch;

    for (; arch != end; arch++) {
        struct range arch_range = {};
        const enum macho_file_parse_result verify_arch_result =
            verify_fat_32_arch(arch,
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

        const struct fat_arch *inner = arch_list;
        for (; inner != arch; inner++) {
            const uint64_t inner_offset = inner->offset;
            const struct range inner_range = {
                .begin = inner_offset,
                .end = inner_offset + inner->size
            };

            if (ranges_overlap(arch_range, inner_range)) {
                free(arch_list);
                return E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES;
            }
        }
    }

    uint32_t arch_index = 0;
    uint32_t filetype = 0;

    bool ignore_filetype = false;
    bool parsed_one_arch = false;

    for (arch = arch_list; arch != end; arch++, arch_index++) {
        const off_t arch_offset = (off_t)(macho_range.begin + arch->offset);
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
            if (options.skip_invalid_archs) {
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
                                      extra.cb_info);

                    if (!should_continue) {
                        free(arch_list);
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }
                }
            } else if (is_invalid_filetype(header.filetype)) {
                if (!options.ignore_wrong_filetype) {
                    const bool should_continue =
                        call_callback(info_in,
                                      ERR_MACHO_FILE_PARSE_WRONG_FILETYPE,
                                      extra.callback,
                                      extra.cb_info);

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
        if (!tbd_options.ignore_targets) {
            arch_info = *(const struct arch_info **)&arch->cputype;
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
            .end = arch_offset + arch->size
        };

        const enum macho_file_parse_result handle_arch_result =
            parse_thin_file(info_in,
                            fd,
                            arch_range,
                            &header,
                            arch_info,
                            extra,
                            arch_is_big_endian,
                            arch_index,
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
verify_fat_64_arch(struct fat_arch_64 *__notnull const arch,
                   const uint64_t macho_base,
                   const struct range available_range,
                   const bool is_big_endian,
                   const struct tbd_parse_options tbd_options,
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

    if (!tbd_options.ignore_targets) {
        const struct arch_info *const arch_info =
            arch_info_for_cputype(arch_cputype, arch_cpusubtype);

        if (arch_info == NULL) {
            return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
        }

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
                   const struct tbd_parse_options tbd_options,
                   const struct macho_file_parse_options options)
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
        verify_fat_64_arch(first_arch,
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

    struct fat_arch_64 *arch = first_arch + 1;
    const struct fat_arch_64 *const end = arch_list + nfat_arch;

    for (; arch != end; arch++) {
        struct range arch_range = {};
        const enum macho_file_parse_result verify_arch_result =
            verify_fat_64_arch(arch,
                               macho_range.begin,
                               available_range,
                               is_big_endian,
                               tbd_options,
                               &arch_range);

        if (verify_arch_result != E_MACHO_FILE_PARSE_OK) {
            free(arch_list);
            return verify_arch_result;
        }

        struct fat_arch_64 *inner = arch_list;
        for (; inner != arch; inner++) {
            const uint64_t inner_offset = inner->offset;
            const struct range inner_range = {
                .begin = inner_offset,
                .end = inner_offset + inner->size
            };

            if (ranges_overlap(arch_range, inner_range)) {
                free(arch_list);
                return E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES;
            }
        }
    }

    uint32_t arch_index = 0;
    uint32_t filetype = 0;

    bool parsed_one_arch = false;
    bool ignore_filetype = false;

    for (arch = arch_list; arch != end; arch++, arch_index++) {
        const off_t arch_offset = (off_t)(macho_range.begin + arch->offset);
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
            if (options.skip_invalid_archs) {
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
                                      extra.cb_info);

                    if (!should_continue) {
                        free(arch_list);
                        return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                    }
                }
            } else if (is_invalid_filetype(header.filetype)) {
                if (!options.ignore_wrong_filetype) {
                    const bool should_continue =
                        call_callback(info_in,
                                      ERR_MACHO_FILE_PARSE_WRONG_FILETYPE,
                                      extra.callback,
                                      extra.cb_info);

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
        if (!tbd_options.ignore_targets) {
            arch_info = *(const struct arch_info **)&arch->cputype;
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
            .end = arch_offset + arch->size
        };

        const enum macho_file_parse_result handle_arch_result =
            parse_thin_file(info_in,
                            fd,
                            arch_range,
                            &header,
                            arch_info,
                            extra,
                            arch_is_big_endian,
                            arch_index,
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
                           struct macho_file *__notnull const macho,
                           struct macho_file_parse_extra_args extra,
                           const struct tbd_parse_options tbd_options,
                           const struct macho_file_parse_options options)
{
    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;

    const int fd = macho->fd;
    const uint32_t magic = macho->magic;
    const uint32_t nfat_arch = macho->nfat_arch;

    if (magic_is_fat(magic)) {
        const enum tbd_ci_set_target_count_result set_count_result =
            tbd_ci_set_target_count(info_in, nfat_arch);

        if (set_count_result != E_TBD_CI_SET_TARGET_COUNT_OK) {
            return E_MACHO_FILE_PARSE_ALLOC_FAIL;
        }

        if (magic_is_fat_64(magic)) {
            ret = handle_fat_64_file(info_in,
                                     fd,
                                     macho->range,
                                     nfat_arch,
                                     magic_is_big_endian(magic),
                                     extra,
                                     tbd_options,
                                     options);
        } else {
            ret = handle_fat_32_file(info_in,
                                     fd,
                                     macho->range,
                                     nfat_arch,
                                     magic_is_big_endian(magic),
                                     extra,
                                     tbd_options,
                                     options);
        }

        if (ret != E_MACHO_FILE_PARSE_OK) {
            return ret;
        }

        const bool ignore_missing_exports =
            (tbd_options.ignore_exports || tbd_options.ignore_missing_exports);

        if (!ignore_missing_exports) {
            const struct array *const metadata = &info_in->fields.metadata;
            const struct array *const symbols = &info_in->fields.symbols;

            if (metadata->item_count == 0 && symbols->item_count == 0) {
                return E_MACHO_FILE_PARSE_NO_DATA;
            }
        }

        /*
         * If exports and undefineds were created with a full arch-set, they are
         * already sorted.
         */

        if (tbd_options.ignore_targets) {
            info_in->flags.uses_full_targets = true;
        } else {
            tbd_ci_sort_info(info_in);
        }
    } else {
        const struct mach_header header = macho->header;
        if (is_invalid_filetype(header.filetype)) {
            if (!options.ignore_wrong_filetype) {
                const bool should_continue =
                    call_callback(info_in,
                                  ERR_MACHO_FILE_PARSE_WRONG_FILETYPE,
                                  extra.callback,
                                  extra.cb_info);

                if (!should_continue) {
                    return E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK;
                }
            }
        }

        const struct arch_info *arch = NULL;
        if (!tbd_options.ignore_targets) {
            arch = arch_info_for_cputype(header.cputype, header.cpusubtype);
            if (arch == NULL) {
                return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
            }
        }

        ret = parse_thin_file(info_in,
                              fd,
                              macho->range,
                              &header,
                              arch,
                              extra,
                              magic_is_big_endian(magic),
                              0,
                              tbd_options,
                              options);

        if (ret != E_MACHO_FILE_PARSE_OK) {
            return ret;
        }

        const bool ignore_missing_exports =
            (tbd_options.ignore_exports || tbd_options.ignore_missing_exports);

        if (!ignore_missing_exports) {
            const struct array *const metadata = &info_in->fields.metadata;
            const struct array *const symbols = &info_in->fields.symbols;

            if (metadata->item_count == 0 && symbols->item_count == 0) {
                return E_MACHO_FILE_PARSE_NO_DATA;
            }
        }

        info_in->flags.uses_full_targets = true;
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
            fputs("File has too many architectures to fit inside file\n",
                  stderr);

            exit(1);
        }

        struct fat_arch_64 *const arch_list = malloc(archs_size);
        if (arch_list == NULL) {
            fputs("Failed to allocate space for architectures\n", stderr);
            exit(1);
        }

        if (our_read(fd, arch_list, archs_size) < 0) {
            free(arch_list);
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        fprintf(stdout, "%" PRIu32 " architecture(s):\n", nfat_arch);

        struct fat_arch_64 *arch = arch_list;
        const struct fat_arch_64 *const end = arch + nfat_arch;

        for (uint32_t arch_index = 0; arch != end; arch++, arch_index++) {
            cpu_type_t cputype = arch->cputype;
            cpu_subtype_t cpusubtype = arch->cpusubtype;

            if (is_big_endian) {
                cputype = swap_int32(cputype);
                cpusubtype = swap_int32(cpusubtype);
            }

            const struct arch_info *const arch_info =
                arch_info_for_cputype(cputype, cpusubtype);

            if (arch_info != NULL) {
                fprintf(stdout,
                        "\t%" PRIu32 ". %s\r\n",
                        arch_index + 1,
                        arch_info->name);
            } else {
                fprintf(stdout,
                        "\t%" PRIu32 ". (Unsupported architecture)\r\n",
                        arch_index + 1);
            }
        }

        free(arch_list);
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

        struct fat_arch *const arch_list = malloc(archs_size);
        if (arch_list == NULL) {
            fputs("Failed to allocate space for architectures\n", stderr);
            exit(1);
        }

        if (our_read(fd, arch_list, archs_size) < 0) {
            free(arch_list);
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        fprintf(stdout, "%" PRIu32 " architecture(s):\n", nfat_arch);

        struct fat_arch *arch = arch_list;
        const struct fat_arch *const end = arch + nfat_arch;

        for (uint32_t arch_index = 0; arch != end; arch++, arch_index++) {
            cpu_type_t cputype = arch->cputype;
            cpu_subtype_t cpusubtype = arch->cpusubtype;

            if (is_big_endian) {
                cputype = swap_int32(cputype);
                cpusubtype = swap_int32(cpusubtype);
            }

            const struct arch_info *const arch_info =
                arch_info_for_cputype(cputype, cpusubtype);

            if (arch_info != NULL) {
                fprintf(stdout,
                        "\t%" PRIu32 ". %s\r\n",
                        arch_index + 1,
                        arch_info->name);
            } else {
                fprintf(stdout,
                        "\t%" PRIu32 ". (Unsupported architecture)\r\n",
                        arch_index + 1);
            }
        }

        free(arch_list);
    } else if (magic_is_thin(magic)) {
        struct {
            cpu_type_t cputype;
            cpu_subtype_t cpusubtype;
        } info;

        if (our_read(fd, &info, sizeof(info)) < 0) {
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        const bool is_big_endian = magic_is_big_endian(magic);
        if (is_big_endian) {
            info.cputype = swap_int32(info.cputype);
            info.cpusubtype = swap_int32(info.cpusubtype);
        }

        const struct arch_info *const arch_info =
            arch_info_for_cputype(info.cputype, info.cpusubtype);

        if (arch_info != NULL) {
            fprintf(stdout, "%s\n", arch_info->name);
        } else {
            fputs("(Unsupported architecture)\n", stdout);
        }
    } else {
        fprintf(stderr,
                "File is not a valid mach-o; Unrecognized magic: 0x%x\n",
                magic);

        exit(1);
    }
}
