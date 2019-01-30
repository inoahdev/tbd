//
//  src/dyld_shared_cache.c
//  tbd 
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <sys/mman.h>
#include <sys/stat.h>

#include <errno.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "arch_info.h"
#include "dyld_shared_cache.h"

#include "guard_overflow.h"
#include "range.h"

static const uint64_t dsc_magic_64 = 2319765435151317348;

static int 
get_arch_info_from_magic(const char magic[16],
                         const struct arch_info **const arch_info_out,
                         uint64_t *const arch_bit_out)
{
    const uint64_t first_part = *(const uint64_t *)magic;
    if (first_part != dsc_magic_64) {
        return E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE;
    }

    const struct arch_info *arch = NULL;
    uint64_t arch_bit = 0;

    const uint64_t second_part = *((const uint64_t *)magic + 1);
    switch (second_part) {
        case 15261442200576032:
            /*
             * (CPU_TYPE_X86, CPU_SUBTYPE_I386_ALL).
             */

            arch = arch_info_get_list() + 6;
            arch_bit = 1ull << 6;

            break;

        case 14696481348417568:
            /*
             * (CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_ALL).
             */

            arch = arch_info_get_list() + 48;
            arch_bit = 1ull << 48;

            break;

        case 29330805708175480:
            /*
             * (CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_H).
             */

            arch = arch_info_get_list() + 49;
            arch_bit = 1ull << 49;

            break;

        case 15048386208145440:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V5TEJ).
             */

            arch = arch_info_get_list() + 20;
            arch_bit = 1ull << 20;

            break;

        case 15329861184856096:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6).
             */
            
            arch = arch_info_get_list() + 19;
            arch_bit = 1ull << 19;

            break;

        case 15611336161566752:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7).
             */

            arch = arch_info_get_list() + 22;
            arch_bit = 1ull << 22;

            break;

        case 3996502057361088544:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7F).
             */

            arch = arch_info_get_list() + 23;
            arch_bit = 1ull << 23;

            break;

        case 7725773898219855904:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7K).
             */

            arch = arch_info_get_list() + 25;
            arch_bit = 1ull << 25;

            break;

        case 8302234650523279392:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S).
             */

            arch = arch_info_get_list() + 24;
            arch_bit = 1ull << 24;

            break;

        case 7869889086295711776:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6M).
             */

            arch = arch_info_get_list() + 26;
            arch_bit = 1ull << 26;

            break;

        case 14696542487257120:
            /*
             * (CPU_TYPE_ARM_64, CPU_SUBTYPE_ARM_64_ALL).
             */

            arch = arch_info_get_list() + 50;
            arch_bit = 1ull << 50;

            break;

        default:
            return 1;
    }

    *arch_info_out = arch;
    *arch_bit_out = arch_bit; 

    return 0;
}

enum dyld_shared_cache_parse_result
dyld_shared_cache_parse_from_file(struct dyld_shared_cache_info *const info_in,
                                  const int fd,
                                  const char magic[16],
                                  const uint64_t options)
{
    /*
     * For performance, check magic and verify header first before mapping file
     * to memory.
     */

    const struct arch_info *arch = NULL;
    uint64_t arch_bit = 0;

    if (get_arch_info_from_magic(magic, &arch, &arch_bit)) {
        return E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE;
    }

    struct dyld_cache_header header = {};
    memcpy(&header.magic, magic, 16);

    if (read(fd, &header.mappingOffset, sizeof(header) - 16) < 0) {
        if (errno == EOVERFLOW) {
            return E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE;
        }

        return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
    }

    struct stat sbuf = {};
    if (fstat(fd, &sbuf) < 0) {
        return E_DYLD_SHARED_CACHE_PARSE_FSTAT_FAIL;
    }

    const uint64_t dsc_size = (uint64_t)sbuf.st_size;
    const struct range available_cache_range = {
        .begin = sizeof(struct dyld_cache_header), 
        .end = dsc_size
    };

    /*
     * Perform basic validation of the image-array and mapping-infos array
     * offsets.
     *
     * Note: Due to the ubiquity of shared-cache headers, and any lack of
     * versioning, more stringent validation is not performed.
     */

    const uint32_t mapping_offset = header.mappingOffset;
    const uint32_t images_offset = header.imagesOffset;    

    if (!range_contains_location(available_cache_range, mapping_offset)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    if (!range_contains_location(available_cache_range, images_offset)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    /*
     * Validate that the mapping-infos array and images-array have no overflows.
     */

    const uint32_t mapping_count = header.mappingCount;
    const uint32_t images_count = header.imagesCount;

    /*
     * Get the size of the mapping-infos table by multipying the mapping-count
     * and the size of a mapping-info
     * 
     * Since sizeof(struct dyld_cache_mapping_info) is a power of 2 (32), use a
     * shift by 5 instead.
     */

    uint64_t mapping_size = mapping_count;
    if (guard_overflow_shift(&mapping_size, 5)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    uint64_t mapping_end = mapping_offset;
    if (guard_overflow_add(&mapping_end, mapping_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    /*
     * Get the size of the image-infos table by multipying the images-count
     * and the size of a image-info
     * 
     * Since sizeof(struct dyld_cache_image_info) is a power of 2 (32), use a
     * shift by 5 instead.
     */

    uint64_t images_size = images_count;
    if (guard_overflow_shift(&images_size, 5)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    /*
     * Verify that both the mapping-infos array and the images-array are
     * completely within the cache-file.
     */

    uint64_t images_end = images_offset;
    if (guard_overflow_add(&images_end, images_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    if (!range_contains_end(available_cache_range, mapping_end)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    if (!range_contains_end(available_cache_range, images_end)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    /*
     * Ensure that the total-size of the mappings and images can be quantified.
     */

    uint64_t infos_end = mapping_size;
    if (guard_overflow_add(&infos_end, images_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    /*
     * Ensure that the mapping-infos array and images-array do not overlap.
     */

    const struct range mappings_range = {
        .begin = mapping_offset,
        .end = mapping_end
    };

    const struct range images_range = {
        .begin = images_offset,
        .end = images_end
    };

    if (ranges_overlap(mappings_range, images_range)) {
        return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_RANGES;
    }

    /*
     * Map file to memory only at the last moment, after all checking has been
     * done to ensure this is a valid dyld_shared_cache file.
     */

    uint8_t *const map =
        mmap(0, dsc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);

    if (map == MAP_FAILED) {
        return E_DYLD_SHARED_CACHE_PARSE_MMAP_FAIL;
    }

    const struct dyld_cache_mapping_info *const mappings =
        (const struct dyld_cache_mapping_info *)(map + mapping_offset);

    /*
     * Mappings are like mach-o segments, covering entire swaths of the file.
     */

    const struct range full_cache_range = {
        .begin = 0, 
        .end = (uint64_t)sbuf.st_size
    };

    /*
     * Verify we don't have any overlapping mappings.
     */

    for (uint32_t i = 0; i < mapping_count; i++) {
        const struct dyld_cache_mapping_info *const mapping = mappings + i;
    
        /*
         * We skip validation of mapping's address-range as its irrelevant to
         * our operations, and because we aim to be lenient.
         */

        const uint64_t mapping_file_begin = mapping->fileOffset;
        uint64_t mapping_file_end = mapping_file_begin;

        if (guard_overflow_add(&mapping_file_end, mapping->size)) {
            munmap(map, dsc_size);
            return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_MAPPINGS;
        }

        const struct range mapping_file_range = {
            .begin = mapping_file_begin,
            .end = mapping_file_end
        };

        if (!range_contains_range(full_cache_range, mapping_file_range)) {
            munmap(map, dsc_size);
            return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;            
        }

        /*
         * Check the previous mappings, which conveniently have already gone
         * through verification in this loop before, for any overlaps with the
         * current mapping.
         */

        for (uint32_t j = 0; j < i; j++) {
            const struct dyld_cache_mapping_info *const inner_mapping =
                mappings + j;

            const uint64_t inner_file_begin = inner_mapping->fileOffset;
            const uint64_t inner_file_end =
                inner_file_begin + inner_mapping->size;

            const struct range inner_file_range = {
                .begin = inner_file_begin,
                .end = inner_file_end  
            };

            if (ranges_overlap(mapping_file_range, inner_file_range)) {
                munmap(map, dsc_size);
                return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_MAPPINGS;
            }
        }
    }

    struct dyld_cache_image_info *const images =
        (struct dyld_cache_image_info *)(map + images_offset);

    /*
     * Perform the image operations if need be. Since the images array is quite
     * large (Usually >1000 images), we aim to do everything in only one loop.
     */

    if (options & O_DYLD_SHARED_CACHE_PARSE_ZERO_IMAGE_PADS) {
        for (uint32_t i = 0; i < images_count; i++) {
            struct dyld_cache_image_info *const image = images + i;

            if (options & O_DYLD_SHARED_CACHE_PARSE_VERIFY_IMAGE_PATH_OFFSETS) {
                const uint32_t location = image->pathFileOffset;
                if (!range_contains_location(available_cache_range, location)) {
                    munmap(map, dsc_size);
                    return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
                }
            }
 
            image->pad = 0;             
        }
    } else if (options & O_DYLD_SHARED_CACHE_PARSE_VERIFY_IMAGE_PATH_OFFSETS) {
        for (uint32_t i = 0; i < images_count; i++) {
            struct dyld_cache_image_info *image = images + i;
            const uint32_t location = image->pathFileOffset;

            if (!range_contains_location(available_cache_range, location)) {
                munmap(map, dsc_size);
                return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
            }
        }
    }

    info_in->images = images;
    info_in->images_count = images_count;

    info_in->mappings = mappings;
    info_in->mappings_count = mapping_count;

    info_in->arch = arch;
    info_in->arch_bit = arch_bit;

    info_in->map = map;
    info_in->size = dsc_size;

    info_in->flags |= F_DYLD_SHARED_CACHE_UNMAP_MAP;
    return E_DYLD_SHARED_CACHE_PARSE_OK;
}

enum dyld_shared_cache_parse_result
dyld_shared_cache_iterate_images_with_callback(
    const struct dyld_shared_cache_info *const info_in,
    void *const item,
    const dyld_shared_cache_iterate_images_callback callback)
{
    const uint8_t *const map = info_in->map;
    const uint32_t images_count = info_in->images_count; 

    for (uint32_t i = 0; i < images_count; i++) {
        struct dyld_cache_image_info *const image = info_in->images + i;

        const uint32_t path_file_offset = image->pathFileOffset;
        const char *const path = (const char *)(map + path_file_offset);

        if (callback(image, path, item)) {
            continue; 
        }

        break;
    }

    return E_DYLD_SHARED_CACHE_PARSE_OK;
}

void dyld_shared_cache_info_destroy(struct dyld_shared_cache_info *const info) {
    if (info->flags & F_DYLD_SHARED_CACHE_UNMAP_MAP) {
        munmap(info->map, info->size);
    }

    info->map = NULL;
    info->size = 0;

    info->mappings = NULL;
    info->images = NULL;

    info->arch = NULL;
    info->arch_bit = 0;
}
