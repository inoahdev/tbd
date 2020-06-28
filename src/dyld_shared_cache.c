//
//  src/dyld_shared_cache.c
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arch_info.h"
#include "dyld_shared_cache.h"
#include "dyld_shared_cache_format.h"
#include "guard_overflow.h"
#include "our_io.h"
#include "range.h"

static const uint64_t dsc_magic_64_normal = 2319765435151317348;
static const uint64_t dsc_magic_64_arm64_32 = 7003509047616633188;

static int
get_arch_info_from_magic(const char magic[const 16],
                         const struct arch_info **__notnull const arch_info_out)
{
    const struct arch_info *arch = NULL;

    const uint64_t first_part = *(const uint64_t *)magic;
    const uint64_t second_part = *((const uint64_t *)magic + 1);

    switch (second_part) {
        case 15261442200576032:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_X86, CPU_SUBTYPE_I386_ALL).
             */

            arch = arch_info_get_list() + 6;
            break;

        case 14696481348417568:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_ALL).
             */

            arch = arch_info_get_list() + 49;
            break;

        case 29330805708175480:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_H).
             */

            arch = arch_info_get_list() + 50;
            break;

        case 15048386208145440:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V5TEJ).
             */

            arch = arch_info_get_list() + 20;
            break;

        case 15329861184856096:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6).
             */

            arch = arch_info_get_list() + 19;
            break;

        case 15611336161566752:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7).
             */

            arch = arch_info_get_list() + 22;
            break;

        case 3996502057361088544:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7F).
             */

            arch = arch_info_get_list() + 23;
            break;

        case 7725773898219855904:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7K).
             */

            arch = arch_info_get_list() + 25;
            break;

        case 8302234650523279392:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S).
             */

            arch = arch_info_get_list() + 24;
            break;

        case 7869889086295711776:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6M).
             */

            arch = arch_info_get_list() + 26;
            break;

        case 14696542487257120:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM64, CPU_SUBTYPE_ARM_64_ALL).
             */

            arch = arch_info_get_list() + 51;
            break;

        case 28486381016867104:
            if (first_part != dsc_magic_64_normal) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM64, CPU_SUBTYPE_ARM64E).
             */

            arch = arch_info_get_list() + 53;
            break;

        case 14130232826424690:
            if (first_part != dsc_magic_64_arm64_32) {
                return 1;
            }

            /*
             * (CPU_TYPE_ARM64_32, CPU_SUBTYPE_ARM_64_ALL).
             */

            arch = arch_info_get_list() + 56;
            break;

        default:
            return 1;
    }

    *arch_info_out = arch;
    return 0;
}

enum dyld_shared_cache_parse_result
dyld_shared_cache_parse_from_file(
    struct dyld_shared_cache_info *__notnull const info_in,
    const int fd,
    const char magic[16],
    const struct dyld_shared_cache_parse_options options)
{
    /*
     * Avoid mapping file until after verifying file-magic and headers.
     */

    const struct arch_info *arch = NULL;
    if (get_arch_info_from_magic(magic, &arch)) {
        return E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE;
    }

    struct dyld_cache_header header = {};
    if (our_read(fd, &header.mappingOffset, sizeof(header) - 16) < 0) {
        if (errno == EOVERFLOW) {
            return E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE;
        }

        return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
    }

    /*
     * Validate that the mapping-infos array and images-array have no overflows.
     */

    uint64_t mappings_size = sizeof(struct dyld_cache_mapping_info);
    if (guard_overflow_mul(&mappings_size, header.mappingCount)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    uint64_t mappings_off_end = header.mappingOffset;
    if (guard_overflow_add(&mappings_off_end, mappings_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    uint64_t images_size = sizeof(struct dyld_cache_image_info);
    if (guard_overflow_mul(&images_size, header.imagesCount)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    uint64_t images_off_end = header.imagesOffset;
    if (guard_overflow_add(&images_off_end, images_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    struct stat sbuf = {};
    if (fstat(fd, &sbuf) < 0) {
        return E_DYLD_SHARED_CACHE_PARSE_FSTAT_FAIL;
    }

    const uint64_t dsc_size = (uint64_t)sbuf.st_size;
    const struct range no_main_header_range = {
        .begin = sizeof(struct dyld_cache_header),
        .end = dsc_size
    };

    /*
     * Ensure that the mapping-infos array and images-array do not overlap.
     */

    const struct range mappings_range = {
        .begin = header.mappingOffset,
        .end = mappings_off_end
    };

    const struct range images_range = {
        .begin = header.imagesOffset,
        .end = images_off_end
    };

    if (ranges_overlap(mappings_range, images_range)) {
        return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_RANGES;
    }

    /*
     * Perform basic validation of the image-array and mapping-infos array
     * offsets.
     *
     * Note: Due to the ubiquity of shared-cache headers, and any lack of
     * versioning, more stringent validation is not performed.
     */

    if (!range_contains_other(no_main_header_range, mappings_range)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    if (!range_contains_other(no_main_header_range, images_range)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    /*
     * Ensure that the total-size of the mappings and images can be quantified.
     */

    uint64_t total_infos_size = mappings_size;
    if (guard_overflow_add(&total_infos_size, images_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    /*
     * After validating all our fields, we finally map the dyld_shared_cache
     * file to memory.
     *
     * We map with write protections so we can use extra fields (like a
     * dyld_cache_image_info's pad field) to save on memory.
     *
     * We also map with MAP_PRIVATE to prevent writing on the original file
     * itself.
     */

    uint8_t *const map =
        mmap(0, dsc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    if (map == MAP_FAILED) {
        return E_DYLD_SHARED_CACHE_PARSE_MMAP_FAIL;
    }

    const struct dyld_cache_mapping_info *const mapping_list =
        (const struct dyld_cache_mapping_info *)(map + header.mappingOffset);

    /*
     * We use full_cache_range to verify our dsc-mappings.
     *
     * Our mappings are comparable to mach-o segments, recording the information
     * of large swaths of file and address-space in the entire dyld_shared_cache
     * file.
     */

    const struct range full_cache_range = {
        .begin = 0,
        .end = dsc_size
    };

    /*
     * Verify we don't have any overlapping mappings.
     */

    const struct dyld_cache_mapping_info *mapping = mapping_list;
    const struct dyld_cache_mapping_info *const mappings_end =
        mapping + header.mappingCount;

    for (; mapping != mappings_end; mapping++) {
        const uint64_t mapping_file_begin = mapping->fileOffset;

        /*
         * We skip validation of mapping's address-range we don't use it, and
         * because we aim to be lenient.
         */

        uint64_t mapping_file_end = mapping_file_begin;
        if (guard_overflow_add(&mapping_file_end, mapping->size)) {
            munmap(map, dsc_size);
            return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_MAPPINGS;
        }

        const struct range mapping_file_range = {
            .begin = mapping_file_begin,
            .end = mapping_file_end
        };

        if (!range_contains_other(full_cache_range, mapping_file_range)) {
            munmap(map, dsc_size);
            return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
        }

        /*
         * Check the previous mappings, which conveniently have already gone
         * through verification in this loop before, for any overlaps with the
         * current mapping.
         */

        const struct dyld_cache_mapping_info *inner = mapping_list;
        for (; inner != mapping; inner++) {
            const uint64_t inner_file_begin = inner->fileOffset;
            const uint64_t inner_file_end = inner_file_begin + inner->size;

            const struct range inner_file_range = {
                .begin = inner_file_begin,
                .end = inner_file_end
            };

            if (!ranges_overlap(mapping_file_range, inner_file_range)) {
                continue;
            }

            munmap(map, dsc_size);
            return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_MAPPINGS;
        }
    }

    /*
     * Create an "available range" where other data structures may lie without
     * overlapping with dyld_shared_cache file structures.
     */

    struct range available_range = {
        .begin = images_off_end,
        .end = dsc_size
    };

    if (available_range.begin < mappings_off_end) {
        available_range.begin = mappings_off_end;
    }

    struct dyld_cache_image_info *const image_list =
        (struct dyld_cache_image_info *)(map + header.imagesOffset);

    /*
     * Perform the image operations if need be.
     *
     * Since the images array is quite large (Usually >1000 images), we aim to
     * do everything in only one loop.
     */

    if (options.zero_image_pads) {
        struct dyld_cache_image_info *image = image_list;
        const struct dyld_cache_image_info *const images_end =
            image + header.imagesCount;

        if (options.verify_image_path_offsets) {
            for (; image != images_end; image++) {
                const uint32_t location = image->pathFileOffset;
                if (!range_contains_location(available_range, location)) {
                    munmap(map, dsc_size);
                    return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
                }

                image->pad = 0;
            }
        } else {
            for (; image != images_end; image++) {
                image->pad = 0;
            }
        }
    } else if (options.verify_image_path_offsets) {
        struct dyld_cache_image_info *image = image_list;
        const struct dyld_cache_image_info *const images_end =
            image + header.imagesCount;

        for (; image != images_end; image++) {
            const uint32_t location = image->pathFileOffset;
            if (range_contains_location(available_range, location)) {
                continue;
            }

            munmap(map, dsc_size);
            return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
        }
    }

    info_in->images = image_list;
    info_in->images_count = header.imagesCount;

    info_in->mappings = mapping_list;
    info_in->mappings_count = header.mappingCount;

    info_in->arch = arch;

    info_in->map = map;
    info_in->size = dsc_size;

    info_in->available_range = available_range;
    info_in->flags.unmap_map = true;

    return E_DYLD_SHARED_CACHE_PARSE_OK;
}

void
dyld_shared_cache_info_destroy(
    struct dyld_shared_cache_info *__notnull const info)
{
    if (info->flags.unmap_map) {
        munmap(info->map, info->size);
    }

    info->map = NULL;
    info->size = 0;

    info->mappings = NULL;
    info->images = NULL;

    info->arch = NULL;
}
