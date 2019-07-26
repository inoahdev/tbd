//
//  src/macho_file.c
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <inttypes.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mach-o/fat.h"

#include "guard_overflow.h"
#include "range.h"

#include "macho_file.h"
#include "macho_file_parse_load_commands.h"

#include "swap.h"

#define SORT_NAME tbd_export_info
#define SORT_TYPE struct tbd_export_info
#define SORT_CMP(x,y) tbd_export_info_compare(&x, &y)

#include "deps/sort/sort.h"

static enum macho_file_parse_result
parse_thin_file(struct tbd_create_info *__notnull const info_in,
                const int fd,
                const uint64_t start,
                const uint64_t size,
                const struct mach_header header,
                const struct arch_info *const arch,
                const bool is_big_endian,
                const uint64_t tbd_options,
                const uint64_t options)
{
    const bool is_64 =
        header.magic == MH_MAGIC_64 || header.magic == MH_CIGAM_64;

    if (is_64) {
        if (size < sizeof(struct mach_header_64)) {
            return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
        }

        /*
         * 64-bit mach-o files have a different header (struct mach_header_64),
         * which only differs by having an extra uint32_t field at the end.
         */

        const uint64_t offset = start + sizeof(struct mach_header_64);
        if (lseek(fd, offset, SEEK_SET) < 0) {
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }
    } else {
        if (!is_big_endian) {
            if (header.magic != MH_MAGIC) {
                return E_MACHO_FILE_PARSE_NOT_A_MACHO;
            }
        }
    }

    if (info_in->flags_field != 0) {
        if (info_in->flags_field & TBD_FLAG_FLAT_NAMESPACE) {
            if (!(header.flags & MH_TWOLEVEL)) {
                return E_MACHO_FILE_PARSE_CONFLICTING_FLAGS;
            }
        }

        if (info_in->flags_field & TBD_FLAG_NOT_APP_EXTENSION_SAFE) {
            if (header.flags & MH_APP_EXTENSION_SAFE) {
                return E_MACHO_FILE_PARSE_CONFLICTING_FLAGS;
            }
        }
    } else {
        if (header.flags & MH_TWOLEVEL) {
            info_in->flags_field |= TBD_FLAG_FLAT_NAMESPACE;
        }

        if (!(header.flags & MH_APP_EXTENSION_SAFE)) {
            info_in->flags_field |= TBD_FLAG_NOT_APP_EXTENSION_SAFE;
        }
    }

    const uint32_t header_size =
        (is_64) ?
            sizeof(struct mach_header_64) :
            sizeof(struct mach_header);

    const uint64_t arch_index = (uint64_t)(arch - arch_info_get_list());
    struct mf_parse_lc_from_file_info info = {
        .fd = fd,

        .arch = arch,
        .arch_bit = 1ull << arch_index,

        .is_64 = is_64,
        .is_big_endian = is_big_endian,

        .ncmds = header.ncmds,
        .sizeofcmds = header.sizeofcmds,

        .full_range.begin = start,
        .full_range.end = start + size,

        .available_range.begin = start + header_size,
        .available_range.end = info.full_range.end,

        .tbd_options = tbd_options,
        .options = options
    };

    const enum macho_file_parse_result parse_load_commands_result =
        macho_file_parse_load_commands_from_file(info_in, &info, NULL);

    if (parse_load_commands_result != E_MACHO_FILE_PARSE_OK) {
        return parse_load_commands_result;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static inline bool thin_magic_is_valid(const uint32_t magic) {
    return magic == MH_MAGIC || magic == MH_MAGIC_64;
}

static enum macho_file_parse_result
handle_fat_32_file(struct tbd_create_info *__notnull const info_in,
                   const int fd,
                   const bool is_big_endian,
                   const uint32_t nfat_arch,
                   const uint64_t start,
                   const uint64_t size,
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

    if (total_headers_size >= size) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    struct fat_arch *const archs = malloc(archs_size);
    if (archs == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, archs, archs_size) < 0) {
        free(archs);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    /*
     * First loop over only to swap the arch-header's fields. We also do some
     * basic verification to ensure no architectures overflow.
     */

    struct fat_arch *const first_arch = archs;

    cpu_type_t first_arch_cputype = first_arch->cputype;
    cpu_subtype_t first_arch_cpusubtype = first_arch->cpusubtype;

    uint32_t first_arch_offset = first_arch->offset;
    uint32_t first_arch_size = first_arch->size;

    if (is_big_endian) {
        first_arch_cputype = swap_int32(first_arch_cputype);
        first_arch_cpusubtype = swap_int32(first_arch_cpusubtype);

        first_arch_offset = swap_uint32(first_arch_offset);
        first_arch_size = swap_uint32(first_arch_size);

        first_arch->cputype = first_arch_cputype;
        first_arch->cpusubtype = first_arch_cpusubtype;

        first_arch->offset = first_arch_offset;
        first_arch->size = first_arch_size;
    }

    /*
     * Ensure the arch's mach-o isn't within the fat-header or the
     * arch-headers.
     */

    if (first_arch_offset < total_headers_size) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    /*
     * Verify each architecture is a valid mach-o by ensuring that it
     * can hold at the least a mach_header.
     */

    if (first_arch_size < sizeof(struct mach_header)) {
        free(archs);
        return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
    }

    /*
     * Verify that the architecture is not located beyond end of file.
     */

    if (first_arch_offset >= size) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    /*
     * Verify that the architecture is fully within the given size.
     */

    const uint64_t first_arch_end = first_arch_offset + first_arch_size;
    if (first_arch_end > size) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    /*
     * Verify that the architecture can fully exist within the range of the
     * provided file.
     */

    uint64_t first_real_arch_offset = start;
    if (guard_overflow_add(&first_real_arch_offset, first_arch_offset)) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    uint64_t first_real_arch_end = start;
    if (guard_overflow_add(&first_real_arch_end, first_arch_end)) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS)) {
        const struct arch_info *const first_arch_info =
            arch_info_for_cputype(first_arch_cputype, first_arch_cpusubtype);

        if (first_arch_info == NULL) {
            free(archs);
            return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
        }

        const struct arch_info *const arch_info_list = arch_info_get_list();
        const uint64_t first_arch_index =
            (uint64_t)(first_arch_info - arch_info_list);

        const uint64_t first_arch_bit = 1ull << first_arch_index;
        if (info_in->archs & first_arch_bit) {
            free(archs);
            return E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE;
        }

        info_in->archs |= first_arch_bit;

        /*
         * To avoid re-lookup of arch-info, we store the pointer within the
         * cputype and cpusubtype fields.
         */

        memcpy(&first_arch->cputype, &first_arch_info, sizeof(first_arch_info));
    }

    /*
     * Before parsing, verify each architecture.
     */

    for (uint32_t i = 1; i < nfat_arch; i++) {
        struct fat_arch *const arch = archs + i;

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
         * Ensure the arch's mach-o isn't within the fat-header or the
         * arch-headers.
         */

        if (arch_offset < total_headers_size) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify each architecture is a valid mach-o by ensuring that it
         * can hold at the least a mach_header.
         */

        if (arch_size < sizeof(struct mach_header)) {
            free(archs);
            return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
        }

        /*
         * Verify that the architecture is not located beyond end of file.
         */

        if (arch_offset >= size) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify that the architecture is fully within the given size.
         */

        const uint64_t arch_end = arch_offset + arch_size;
        if (arch_end > size) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify that the architecture can fully exist within the range of the
         * provided file.
         */

        uint64_t real_arch_offset = start;
        if (guard_overflow_add(&real_arch_offset, arch_offset)) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        uint64_t real_arch_end = start;
        if (guard_overflow_add(&real_arch_end, arch_end)) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        const struct arch_info *arch_info = NULL;
        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS)) {
            arch_info = arch_info_for_cputype(arch_cputype, arch_cpusubtype);
            if (arch_info == NULL) {
                free(archs);
                return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
            }

            const struct arch_info *const arch_info_list = arch_info_get_list();

            const uint64_t arch_index = (uint64_t)(arch_info - arch_info_list);
            const uint64_t arch_bit = 1ull << arch_index;

            if (info_in->archs & arch_bit) {
                free(archs);
                return E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE;
            }

            info_in->archs |= arch_bit;

            /*
             * To avoid re-lookup of arch-info, we store the pointer within the
             * cputype and cpusubtype fields.
             */

            memcpy(&arch->cputype, &arch_info, sizeof(arch_info));
        }

        const struct range arch_range = {
            .begin = arch_offset,
            .end = arch_end
        };

        for (uint32_t j = 0; j < i; j++) {
            struct fat_arch inner = archs[j];
            const struct range inner_range = {
                .begin = inner.offset,
                .end = inner.offset + inner.size
            };

            if (ranges_overlap(arch_range, inner_range)) {
                free(archs);
                return E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES;
            }
        }
    }

    bool parsed_one_arch = false;
    for (uint32_t i = 0; i < nfat_arch; i++) {
        const struct fat_arch arch = archs[i];
        const off_t arch_offset = (off_t)(start + arch.offset);

        if (lseek(fd, arch_offset, SEEK_SET) < 0) {
            free(archs);
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }

        struct mach_header header = {};
        if (read(fd, &header, sizeof(header)) < 0) {
            free(archs);
            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        /*
         * Swap the mach_header's fields if big-endian.
         */

        const bool arch_is_big_endian =
            header.magic == MH_CIGAM || header.magic == MH_CIGAM_64;

        if (arch_is_big_endian) {
            header.cputype = swap_int32(header.cputype);
            header.cpusubtype = swap_int32(header.cpusubtype);

            header.ncmds = swap_uint32(header.ncmds);
            header.sizeofcmds = swap_uint32(header.sizeofcmds);

            header.flags = swap_uint32(header.flags);
        } else if (!thin_magic_is_valid(header.magic)) {
            if (options & O_MACHO_FILE_PARSE_SKIP_INVALID_ARCHITECTURES) {
                continue;
            }

            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify that header's cpu-type matches arch's cpu-type.
         */

        const struct arch_info *arch_info = NULL;
        if (tbd_options & O_TBD_PARSE_IGNORE_ARCHS) {
            if (header.cputype != arch.cputype) {
                free(archs);
                return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
            }

            if (header.cpusubtype != arch.cpusubtype) {
                free(archs);
                return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
            }
        } else {
            arch_info = *(const struct arch_info **)&arch.cputype;
            if (header.cputype != arch_info->cputype) {
                free(archs);
                return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
            }

            if (header.cpusubtype != arch_info->cpusubtype) {
                free(archs);
                return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
            }
        }

        const enum macho_file_parse_result handle_arch_result =
            parse_thin_file(info_in,
                            fd,
                            (uint64_t)arch_offset,
                            arch.size,
                            header,
                            arch_info,
                            arch_is_big_endian,
                            tbd_options,
                            options);

        if (handle_arch_result != E_MACHO_FILE_PARSE_OK) {
            free(archs);
            return handle_arch_result;
        }

        parsed_one_arch = true;
    }

    free(archs);

    if (!parsed_one_arch) {
        return E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static enum macho_file_parse_result
handle_fat_64_file(struct tbd_create_info *__notnull const info_in,
                   const int fd,
                   const bool is_big_endian,
                   const uint32_t nfat_arch,
                   const uint64_t start,
                   const uint64_t size,
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

    if (total_headers_size >= size) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    struct fat_arch_64 *const archs = malloc(archs_size);
    if (archs == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, archs, archs_size) < 0) {
        free(archs);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    /*
     * First loop over only to swap the arch-header's fields. We also do some
     * basic verification to ensure no architectures overflow.
     */

    struct fat_arch_64 *const first_arch = archs;

    cpu_type_t first_arch_cputype = first_arch->cputype;
    cpu_subtype_t first_arch_cpusubtype = first_arch->cpusubtype;

    uint64_t first_arch_offset = first_arch->offset;
    uint64_t first_arch_size = first_arch->size;

    if (is_big_endian) {
        first_arch_cputype = swap_int32(first_arch_cputype);
        first_arch_cpusubtype = swap_int32(first_arch_cpusubtype);

        first_arch_offset = swap_uint64(first_arch_offset);
        first_arch_size = swap_uint64(first_arch_size);

        first_arch->cputype = first_arch_cputype;
        first_arch->cpusubtype = first_arch_cpusubtype;

        first_arch->offset = first_arch_offset;
        first_arch->size = first_arch_size;
    }

    /*
     * Ensure the arch's mach-o isn't within the fat-header or the
     * arch-headers.
     */

    if (first_arch_offset < total_headers_size) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    /*
     * Verify each architecture is a valid mach-o by ensuring that it
     * can hold at the least a mach_header.
     */

    if (first_arch_size < sizeof(struct mach_header)) {
        free(archs);
        return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
    }

    /*
     * Verify that no overflow occurs when finding arch's end.
     */

    uint64_t first_arch_end = first_arch_offset;
    if (guard_overflow_add(&first_arch_end, first_arch_size)) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    /*
     * Verify that the architecture is not located beyond end of file.
     */

    if (first_arch_offset > size) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    /*
     * Verify that the architecture is fully within the given size.
     */

    if (first_arch_end > size) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    /*
     * Verify that the architecture can fully exist within the range of the
     * provided file.
     */

    uint64_t first_real_arch_offset = start;
    if (guard_overflow_add(&first_real_arch_offset, first_arch_offset)) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    uint64_t first_real_arch_end = start;
    if (guard_overflow_add(&first_real_arch_end, first_arch_end)) {
        free(archs);
        return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
    }

    if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS)) {
        const struct arch_info *const first_arch_info =
            arch_info_for_cputype(first_arch->cputype, first_arch->cpusubtype);

        if (first_arch_info == NULL) {
            free(archs);
            return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
        }

        const struct arch_info *const arch_info_list = arch_info_get_list();
        const uint64_t first_arch_index =
            (uint64_t)(first_arch_info - arch_info_list);

        const uint64_t first_arch_bit = 1ull << first_arch_index;
        if (info_in->archs & first_arch_bit) {
            free(archs);
            return E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE;
        }

        info_in->archs |= first_arch_bit;

        /*
         * To avoid re-lookup of arch-info, we store the pointer within the
         * cputype and cpusubtype fields.
         */

        memcpy(&first_arch->cputype, &first_arch_info, sizeof(first_arch_info));
    }

    /*
     * Before parsing, verify each architecture.
     */

    for (uint32_t i = 1; i < nfat_arch; i++) {
        struct fat_arch_64 *const arch = archs + i;

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
         * Ensure the arch's mach-o isn't within the fat-header or the
         * arch-headers.
         */

        if (arch_offset < total_headers_size) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify each architecture is a valid mach-o by ensuring that it
         * can hold at the least a mach_header.
         */

        if (arch_size < sizeof(struct mach_header)) {
            free(archs);
            return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
        }

        /*
         * Verify that no overflow occurs when finding arch's end.
         */

        uint64_t arch_end = arch_offset;
        if (guard_overflow_add(&arch_end, arch_size)) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify that the architecture is not located beyond end of file.
         */

        if (arch_offset > size) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify that the architecture is fully within the given size.
         */

        if (arch_end > size) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify that the architecture can fully exist within the range of the
         * provided file.
         */

        uint64_t real_arch_offset = start;
        if (guard_overflow_add(&real_arch_offset, arch_offset)) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        uint64_t real_arch_end = start;
        if (guard_overflow_add(&real_arch_end, arch_end)) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS)) {
            const struct arch_info *const arch_info =
                arch_info_for_cputype(arch_cputype, arch_cpusubtype);

            if (arch_info == NULL) {
                free(archs);
                return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
            }

            const struct arch_info *const arch_info_list = arch_info_get_list();

            const uint64_t arch_index = (uint64_t)(arch_info - arch_info_list);
            const uint64_t arch_bit = 1ull << arch_index;

            if (info_in->archs & arch_bit) {
                free(archs);
                return E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE;
            }

            info_in->archs |= arch_bit;
        }

        const struct range arch_range = {
            .begin = arch_offset,
            .end = arch_end
        };

        for (uint32_t j = 0; j < i; j++) {
            struct fat_arch_64 inner = archs[j];
            const struct range inner_range = {
                .begin = inner.offset,
                .end = inner.offset + inner.size
            };

            if (ranges_overlap(arch_range, inner_range)) {
                free(archs);
                return E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES;
            }
        }
    }

    bool parsed_one_arch = false;
    for (uint32_t i = 0; i < nfat_arch; i++) {
        const struct fat_arch_64 arch = archs[i];
        const off_t arch_offset = (off_t)(start + arch.offset);

        if (lseek(fd, arch_offset, SEEK_SET) < 0) {
            free(archs);
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }

        struct mach_header header = {};
        if (read(fd, &header, sizeof(header)) < 0) {
            free(archs);
            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        /*
         * Swap mach_header's fields if big-endian as we deal only in
         * little-endian.
         */

        const bool arch_is_big_endian =
            header.magic == MH_CIGAM || header.magic == MH_CIGAM_64;

        if (arch_is_big_endian) {
            header.cputype = swap_int32(header.cputype);
            header.cpusubtype = swap_int32(header.cpusubtype);

            header.ncmds = swap_uint32(header.ncmds);
            header.sizeofcmds = swap_uint32(header.sizeofcmds);

            header.flags = swap_uint32(header.flags);
        } else if (!thin_magic_is_valid(header.magic)) {
            if (options & O_MACHO_FILE_PARSE_SKIP_INVALID_ARCHITECTURES) {
                continue;
            }

            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        /*
         * Verify that header's cpu-type matches arch's cpu-type.
         */

        const struct arch_info *arch_info = NULL;
        if (tbd_options & O_TBD_PARSE_IGNORE_ARCHS) {
            if (header.cputype != arch.cputype) {
                free(archs);
                return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
            }

            if (header.cpusubtype != arch.cpusubtype) {
                free(archs);
                return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
            }
        } else {
            arch_info = *(const struct arch_info **)&arch.cputype;
            if (header.cputype != arch_info->cputype) {
                free(archs);
                return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
            }

            if (header.cpusubtype != arch_info->cpusubtype) {
                free(archs);
                return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
            }
        }

        const enum macho_file_parse_result handle_arch_result =
            parse_thin_file(info_in,
                            fd,
                            (uint64_t)arch_offset,
                            arch.size,
                            header,
                            arch_info,
                            arch_is_big_endian,
                            tbd_options,
                            options);

        if (handle_arch_result != E_MACHO_FILE_PARSE_OK) {
            free(archs);
            return handle_arch_result;
        }

        parsed_one_arch = true;
    }

    free(archs);

    if (!parsed_one_arch) {
        return E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_from_file(struct tbd_create_info *__notnull const info_in,
                           const int fd,
                           const uint32_t magic,
                           const uint64_t tbd_options,
                           const uint64_t options)
{
    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    const bool is_fat =
        magic == FAT_MAGIC    || magic == FAT_CIGAM ||
        magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64;

    if (is_fat) {
        uint32_t nfat_arch = 0;
        if (read(fd, &nfat_arch, sizeof(nfat_arch)) < 0) {
            if (errno == EOVERFLOW) {
                return E_MACHO_FILE_PARSE_NOT_A_MACHO;
            }

            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        if (nfat_arch == 0) {
            return E_MACHO_FILE_PARSE_NO_ARCHITECTURES;
        }

        const bool is_big_endian = magic == FAT_CIGAM || magic == FAT_CIGAM_64;
        if (is_big_endian) {
            nfat_arch = swap_uint32(nfat_arch);
        }

        struct stat sbuf = {};
        if (fstat(fd, &sbuf) < 0) {
            return E_MACHO_FILE_PARSE_FSTAT_FAIL;
        }

        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS)) {
            info_in->archs_count = nfat_arch;
        }

        const bool is_64 = magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64;
        const uint64_t file_size = (uint64_t)sbuf.st_size;

        if (is_64) {
            ret = handle_fat_64_file(info_in,
                                     fd,
                                     is_big_endian,
                                     nfat_arch,
                                     0,
                                     file_size,
                                     tbd_options,
                                     options);
        } else {
            ret = handle_fat_32_file(info_in,
                                     fd,
                                     is_big_endian,
                                     nfat_arch,
                                     0,
                                     file_size,
                                     tbd_options,
                                     options);
        }

        if (ret != E_MACHO_FILE_PARSE_OK) {
            return ret;
        }

        if (!(tbd_options & O_TBD_PARSE_IGNORE_MISSING_EXPORTS)) {
            if (array_is_empty(&info_in->exports)) {
                return E_MACHO_FILE_PARSE_NO_EXPORTS;
            }
        }

        if (!(tbd_options & O_TBD_PARSE_EXPORTS_HAVE_FULL_ARCHS)) {
            /*
             * Finally sort the exports array.
             */

            tbd_export_info_quick_sort(info_in->exports.data,
                                       info_in->exports.item_count);
        } else {
            info_in->flags |= F_TBD_CREATE_INFO_EXPORTS_HAVE_FULL_ARCHS;
        }
    } else {
        const bool is_thin =
            magic == MH_MAGIC || magic == MH_CIGAM ||
            magic == MH_MAGIC_64 || magic == MH_CIGAM_64;

        if (!is_thin) {
            return E_MACHO_FILE_PARSE_NOT_A_MACHO;
        }

        struct mach_header header = { .magic = magic };
        if (read(fd, &header.cputype, sizeof(header) - sizeof(magic)) < 0) {
            if (errno == EOVERFLOW) {
                return E_MACHO_FILE_PARSE_NOT_A_MACHO;
            }

            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        struct stat sbuf = {};
        if (fstat(fd, &sbuf) < 0) {
            return E_MACHO_FILE_PARSE_FSTAT_FAIL;
        }

        const uint64_t file_size = (uint64_t)sbuf.st_size;
        const bool is_big_endian = magic == MH_CIGAM || magic == MH_CIGAM_64;

        /*
         * Swap the mach_header's fields if big-endian.
         */

        if (is_big_endian) {
            header.cputype = swap_int32(header.cputype);
            header.cpusubtype = swap_int32(header.cpusubtype);

            header.ncmds = swap_uint32(header.ncmds);
            header.sizeofcmds = swap_uint32(header.sizeofcmds);

            header.flags = swap_uint32(header.flags);
        }

        const struct arch_info *const arch =
            arch_info_for_cputype(header.cputype, header.cpusubtype);

        if (arch == NULL) {
            return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;
        }

        const struct arch_info *const arch_info_list = arch_info_get_list();

        const uint64_t arch_index = (uint64_t)(arch - arch_info_list);
        const uint64_t arch_bit = 1ull << arch_index;

        if (!(tbd_options & O_TBD_PARSE_IGNORE_ARCHS)) {
            info_in->archs |= arch_bit;
            info_in->archs_count = 1;
            info_in->flags |= F_TBD_CREATE_INFO_EXPORTS_HAVE_FULL_ARCHS;
        }

        ret = parse_thin_file(info_in,
                              fd,
                              0,
                              file_size,
                              header,
                              arch,
                              is_big_endian,
                              tbd_options,
                              options);

        if (ret != E_MACHO_FILE_PARSE_OK) {
            return ret;
        }

        if (!(tbd_options & O_TBD_PARSE_IGNORE_MISSING_EXPORTS)) {
            if (array_is_empty(&info_in->exports)) {
                return E_MACHO_FILE_PARSE_NO_EXPORTS;
            }
        }
    }

    return E_MACHO_FILE_PARSE_OK;
}

void macho_file_print_archs(const int fd) {
    uint32_t magic = 0;
    if (read(fd, &magic, sizeof(magic)) < 0) {
        fprintf(stderr,
                "Failed to read data from mach-o, error: %s\n",
                strerror(errno));

        exit(1);
    }

    const bool is_fat_64 = magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64;
    if (is_fat_64) {
        uint32_t nfat_arch = 0;
        if (read(fd, &nfat_arch, sizeof(nfat_arch)) < 0) {
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        if (nfat_arch == 0) {
            fputs("File has no architectures\n", stderr);
            exit(1);
        }

        const bool is_big_endian = magic == FAT_CIGAM_64;
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

        if (read(fd, archs, archs_size) < 0) {
            free(archs);
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        for (uint32_t i = 0; i < nfat_arch; i++) {
            struct fat_arch_64 arch = archs[i];
            if (is_big_endian) {
                arch.cputype = swap_int32(arch.cputype);
                arch.cpusubtype = swap_int32(arch.cpusubtype);
            }

            const struct arch_info *const arch_info =
                arch_info_for_cputype(arch.cputype, arch.cpusubtype);

            if (arch_info == NULL) {
                fprintf(stdout,
                        "\t%" PRIu32 ". (Unsupported architecture)\n",
                        i + 1);
            } else {
                fprintf(stdout, "\t%" PRIu32 ". %s\n", i + 1, arch_info->name);
            }
        }

        free(archs);
    } else if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
        uint32_t nfat_arch = 0;
        if (read(fd, &nfat_arch, sizeof(nfat_arch)) < 0) {
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        if (nfat_arch == 0) {
            fputs("File has no architectures\n", stderr);
            exit(1);
        }

        const bool is_big_endian = magic == FAT_CIGAM;
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

        if (read(fd, archs, archs_size) < 0) {
            free(archs);
            fprintf(stderr,
                    "Failed to read data from mach-o, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        fprintf(stdout, "%" PRIu32 " architecture(s):\n", nfat_arch);
        for (uint32_t i = 0; i < nfat_arch; i++) {
            struct fat_arch arch = archs[i];
            if (is_big_endian) {
                arch.cputype = swap_int32(arch.cputype);
                arch.cpusubtype = swap_int32(arch.cpusubtype);
            }

            const struct arch_info *const arch_info =
                arch_info_for_cputype(arch.cputype, arch.cpusubtype);

            if (arch_info == NULL) {
                fprintf(stdout,
                        "\t%" PRIu32 ". (Unsupported architecture)\n",
                        i + 1);
            } else {
                fprintf(stdout, "\t%" PRIu32 ". %s\n", i + 1, arch_info->name);
            }
        }

        free(archs);
    } else {
        const bool is_thin =
            magic == MH_MAGIC    || magic == MH_CIGAM ||
            magic == MH_MAGIC_64 || magic == MH_CIGAM_64;

        if (is_thin) {
            struct mach_header header = {};
            const uint32_t read_size =
                sizeof(header.cputype) + sizeof(header.cpusubtype);

            if (read(fd, &header.cputype, read_size) < 0) {
                fprintf(stderr,
                        "Failed to read data from mach-o, error: %s\n",
                        strerror(errno));

                exit(1);
            }

            const bool is_big_endian =
                magic == MH_CIGAM || magic == MH_CIGAM_64;

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
}
