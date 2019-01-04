//
//  src/macho_file.c
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <errno.h>

#include <stdbool.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include "arch_info.h"
#include "mach-o/fat.h"

#include "guard_overflow.h"
#include "range.h"

#include "macho_file.h"
#include "macho_file_parse_load_commands.h"

#include "swap.h"

static enum macho_file_parse_result
parse_thin_file(struct tbd_create_info *const info_in,
                const int fd,
                const struct mach_header header,
                const uint64_t start,
                const uint64_t size,
                const uint64_t tbd_options,
                const uint64_t options)
{
    const bool is_64 =
        header.magic == MH_MAGIC_64 || header.magic == MH_CIGAM_64;
    
    const bool is_big_endian =
        header.magic == MH_CIGAM || header.magic == MH_CIGAM_64;

    if (is_64) {
        if (size != 0) {
            if (size < sizeof(struct mach_header_64)) {
                return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
            }
        }

        /*
         * 64-bit mach-o files have a different header (struct mach_header_64),
         * which only differs by having an extra uint32_t field at the end.
         */

        if (lseek(fd, sizeof(uint32_t), SEEK_CUR) < 0) {
            return E_MACHO_FILE_PARSE_SEEK_FAIL;
        }
    } else {
        if (!is_big_endian && header.magic != MH_MAGIC) {
            return E_MACHO_FILE_PARSE_NOT_A_MACHO;
        }
    }

    if (info_in->flags != 0) {
        if (info_in->flags & TBD_FLAG_FLAT_NAMESPACE) {
            if (!(header.flags & MH_TWOLEVEL)) {
                return E_MACHO_FILE_PARSE_CONFLICTING_FLAGS;
            }
        }

        if (info_in->flags & TBD_FLAG_NOT_APP_EXTENSION_SAFE) {
            if (header.flags & MH_APP_EXTENSION_SAFE) {
                return E_MACHO_FILE_PARSE_CONFLICTING_FLAGS;
            }
        }
    } else {
        if (header.flags & MH_TWOLEVEL) {
            info_in->flags |= MH_TWOLEVEL;
        }

        if (!(header.flags & MH_APP_EXTENSION_SAFE)) {
            info_in->flags |= TBD_FLAG_NOT_APP_EXTENSION_SAFE;
        }
    }

    const struct arch_info *const arch =
        arch_info_for_cputype(header.cputype, header.cpusubtype);

    if (arch == NULL) {
        return E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE;        
    }

    const struct arch_info *const arch_info_list = arch_info_get_list();

    const uint64_t arch_index = arch - arch_info_list;
    const uint64_t arch_bit = 1ull << arch_index;

    if (info_in->archs & arch_bit) {
        return E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE;
    }

    info_in->archs |= arch_bit;
    const enum macho_file_parse_result parse_load_commands_result =    
        macho_file_parse_load_commands_from_file(info_in,
                                                 fd,
                                                 start,
                                                 size,
                                                 arch,
                                                 arch_bit,
                                                 is_64,
                                                 is_big_endian,
                                                 header.ncmds,
                                                 header.sizeofcmds,
                                                 tbd_options,
                                                 options,
                                                 NULL);

    if (parse_load_commands_result != E_MACHO_FILE_PARSE_OK) {
        return parse_load_commands_result;
    }

    return E_MACHO_FILE_PARSE_OK;
}

static inline bool thin_magic_is_valid(const uint32_t magic) {
    return magic == MH_MAGIC || magic == MH_MAGIC_64;
}

static enum macho_file_parse_result
handle_fat_32_file(struct tbd_create_info *const info_in,
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

    if (size != 0) {
        if (total_headers_size >= size) {
            return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
        }
    }

    struct fat_arch *const archs = calloc(1, archs_size);
    if (archs == NULL) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    if (read(fd, archs, archs_size) < 0) {
        free(archs);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    /*
     * First loop over only to swap the arch-header's fields. We also do some
     * basic verification to ensure no architectures overflow.
     */

    if (is_big_endian) {
        struct fat_arch *const arch = archs;

        arch->cputype = swap_uint32(arch->cputype);
        arch->cpusubtype = swap_uint32(arch->cpusubtype);

        arch->offset = swap_uint32(arch->offset);
        arch->size = swap_uint32(arch->size);
    }

    /*
     * Before parsing, verify each architecture.
     */

    for (uint32_t i = 1; i < nfat_arch; i++) {
        struct fat_arch *const arch = archs + i;

        uint32_t arch_offset = arch->offset;
        uint32_t arch_size = arch->size;

        if (is_big_endian) {
            arch->cputype = swap_uint32(arch->cputype);
            arch->cpusubtype = swap_uint32(arch->cpusubtype);

            arch_offset = swap_uint32(arch_offset);
            arch_size = swap_uint32(arch_size);

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
         * Verify each architecture can hold at least a mach_header.
         */

        if (arch_size < sizeof(struct mach_header)) {
            free(archs);
            return E_MACHO_FILE_PARSE_SIZE_TOO_SMALL;
        }

        /*
         * Verify that no overflow occurs when finding arch's end.
         */

        uint32_t arch_end = arch_offset;
        if (guard_overflow_add(&arch_end, arch_size)) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        if (size != 0) {
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
        }

        const struct range arch_range = {
            .begin = arch_offset,
            .end = arch_offset + arch_size
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
        struct fat_arch arch = archs[i];
        if (lseek(fd, start + arch.offset, SEEK_SET) < 0) {
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

        const bool is_big_endian =
            header.magic == MH_CIGAM || header.magic == MH_CIGAM_64;

        if (is_big_endian) {
            header.cputype = swap_uint32(header.cputype);
            header.cpusubtype = swap_uint32(header.cpusubtype);

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

        if (header.cputype != arch.cputype) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        if (header.cpusubtype != arch.cpusubtype) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        const enum macho_file_parse_result handle_arch_result =
            parse_thin_file(info_in,
                            fd,
                            header,
                            start + arch.offset,
                            arch.size,
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
handle_fat_64_file(struct tbd_create_info *const info_in,
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

    if (size != 0) {
        if (total_headers_size >= size) {
            return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
        }
    }

    struct fat_arch_64 *const archs = calloc(1, archs_size);
    if (archs == NULL) {
        return E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES;
    }

    if (read(fd, archs, archs_size) < 0) {
        free(archs);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    /*
     * First loop over only to swap the arch-header's fields. We also do some
     * basic verification to ensure no architectures overflow.
     */

    if (is_big_endian) {
        struct fat_arch_64 *const arch = archs;

        arch->cputype = swap_uint32(arch->cputype);
        arch->cpusubtype = swap_uint32(arch->cpusubtype);

        arch->offset = swap_uint64(arch->offset);
        arch->size = swap_uint64(arch->size);
    }

    /*
     * Before parsing, verify each architecture.
     */

    for (uint32_t i = 1; i < nfat_arch; i++) {
        struct fat_arch_64 *const arch = archs + i;

        uint64_t arch_offset = arch->offset;
        uint64_t arch_size = arch->size;

        if (is_big_endian) {
            arch->cputype = swap_uint32(arch->cputype);
            arch->cpusubtype = swap_uint32(arch->cpusubtype);

            arch_offset = swap_uint64(arch_offset);
            arch_size = swap_uint64(arch_size);

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
         * Verify each architecture can hold at a least mach_header.
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

        if (size != 0) {
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
        }

        const struct range arch_range = {
            .begin = arch_offset,
            .end = arch_offset + arch_size
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
        struct fat_arch_64 arch = archs[i];
        if (lseek(fd, start + arch.offset, SEEK_SET) < 0) {
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

        const bool is_big_endian =
            header.magic == MH_CIGAM || header.magic == MH_CIGAM_64;

        if (is_big_endian) {
            header.cputype = swap_uint32(header.cputype);
            header.cpusubtype = swap_uint32(header.cpusubtype);

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

        if (header.cputype != arch.cputype) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        if (header.cpusubtype != arch.cpusubtype) {
            free(archs);
            return E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE;
        }

        const enum macho_file_parse_result handle_arch_result =
            parse_thin_file(info_in,
                            fd,
                            header,
                            start + arch.offset,
                            arch.size,
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
macho_file_parse_from_file(struct tbd_create_info *const info_in,
                           const int fd,
                           const uint64_t size,
                           const uint64_t tbd_options,
                           const uint64_t options)
{
    if (size < sizeof(struct fat_header)) {
        return E_MACHO_FILE_PARSE_NOT_A_MACHO;
    }

    uint32_t magic = 0;
    if (read(fd, &magic, sizeof(magic)) < 0) {
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    const bool is_fat =
        magic == FAT_MAGIC    || magic == FAT_CIGAM ||
        magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64;

    enum macho_file_parse_result ret = E_MACHO_FILE_PARSE_OK;
    if (is_fat) {
        const bool is_64 = magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64;
        const bool is_big_endian = magic == FAT_CIGAM || magic == FAT_CIGAM_64;

        uint32_t nfat_arch = 0;
        if (read(fd, &nfat_arch, sizeof(nfat_arch)) < 0) {
            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        if (nfat_arch == 0) {
            return E_MACHO_FILE_PARSE_NO_ARCHITECTURES;
        }

        if (is_big_endian) {
            nfat_arch = swap_uint32(nfat_arch);
        }

        if (is_64) {
            ret =
                handle_fat_64_file(info_in,
                                   fd,
                                   is_big_endian,
                                   nfat_arch,
                                   0,
                                   size,
                                   tbd_options,
                                   options);
        } else {
            ret =
                handle_fat_32_file(info_in,
                                   fd,
                                   is_big_endian,
                                   nfat_arch,
                                   0,
                                   size,
                                   tbd_options,
                                   options);
        }
    } else {
        const bool is_thin =
            magic == MH_MAGIC    || magic == MH_CIGAM ||
            magic == MH_MAGIC_64 || magic == MH_CIGAM_64;

        if (!is_thin) {
            return E_MACHO_FILE_PARSE_NOT_A_MACHO;
        }

        struct mach_header header = { .magic = magic };
        if (read(fd, &header.cputype, sizeof(header) - sizeof(magic)) < 0) {
            return E_MACHO_FILE_PARSE_READ_FAIL;
        }

        /*
         * Swap the mach_header's fields if big-endian.
         */

        const bool is_big_endian = magic == MH_CIGAM || magic == MH_CIGAM_64;
        if (is_big_endian) {
            header.cputype = swap_uint32(header.cputype);
            header.cpusubtype = swap_uint32(header.cpusubtype);

            header.ncmds = swap_uint32(header.ncmds);
            header.sizeofcmds = swap_uint32(header.sizeofcmds);

            header.flags = swap_uint32(header.flags);
        }

        ret =
            parse_thin_file(info_in,
                            fd,
                            header,
                            0,
                            0,
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

    /*
     * Finally sort the exports array.
     */

    const enum array_result sort_exports_result =
        array_sort_items_with_comparator(&info_in->exports,
                                         sizeof(struct tbd_export_info),
                                         tbd_export_info_comparator);

    if (sort_exports_result != E_ARRAY_OK) {
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
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

        struct fat_arch_64 *const archs = calloc(1, archs_size);
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
                arch.cputype = swap_uint32(arch.cputype);
                arch.cpusubtype = swap_uint32(arch.cpusubtype);
            }

            const struct arch_info *const arch_info =
                arch_info_for_cputype(arch.cputype, arch.cpusubtype);

            if (arch_info == NULL) {
                fprintf(stdout, "\t%d. (Unsupported architecture)\n", i + 1);
            } else {
                fprintf(stdout, "\t%d. %s\n", i + 1, arch_info->name);
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

        struct fat_arch *const archs = calloc(1, archs_size);
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

        fprintf(stdout, "%d architecture(s):\n", nfat_arch);
        for (uint32_t i = 0; i < nfat_arch; i++) {
            struct fat_arch arch = archs[i];
            if (is_big_endian) {
                arch.cputype = swap_uint32(arch.cputype);
                arch.cpusubtype = swap_uint32(arch.cpusubtype);
            }

            const struct arch_info *const arch_info =
                arch_info_for_cputype(arch.cputype, arch.cpusubtype);

            if (arch_info == NULL) {
                fprintf(stdout, "\t%d. (Unsupported architecture)\n", i + 1);
            } else {
                fprintf(stdout, "\t%d. %s\n", i + 1, arch_info->name);
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
                header.cputype = swap_uint32(header.cputype);
                header.cpusubtype = swap_uint32(header.cpusubtype);
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
