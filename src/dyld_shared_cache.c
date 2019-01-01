//
//  src/dyld_shared_cache.c
//  tbd 
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <sys/mman.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "arch_info.h"
#include "dyld_shared_cache.h"

#include "guard_overflow.h"
#include "range.h"

const uint64_t dsc_magic_64 = 2319765435151317348;

enum dyld_shared_cache_parse_result
dyld_shared_cache_parse_from_file(struct dyld_shared_cache_info *const info_in,
                                  const int fd,
                                  const uint64_t size,
                                  const uint64_t options)
{
    struct dyld_cache_header header = {};
    if (size < sizeof(header)) {
        return E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE;
    }

    if (read(fd, &header, sizeof(header)) < 0) {
        return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
    }

    /*
     * Do integer-compares compare on the magic to improve performance
     */

    const uint64_t first_part = *(uint64_t *)header.magic;
    if (first_part != dsc_magic_64) {
        return E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE;
    }

    const struct arch_info *arch = NULL;
    uint64_t arch_bit = 0;

    const uint64_t second_part = *((uint64_t *)header.magic + 1);
    switch (second_part) {
        case 15261442200576032:
            /*
             * (CPU_TYPE_X86, CPU_SUBTYPE_I386_ALL)
             */

            arch = arch_info_get_list() + 20;
            arch_bit = 1ull << 20;

            break;

        case 14696481348417568:
            /*
             * (CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_ALL)
             */

            arch = arch_info_get_list() + 52;
            arch_bit = 1ull << 52;

            break;

        case 29330805708175480:
            /*
             * (CPU_TYPE_X86_64, CPU_SUBTYPE_X86_64_H)
             */

            arch = arch_info_get_list() + 53;
            arch_bit = 1ull << 53;

            break;

        case 15048386208145440:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6)
             */

            arch = arch_info_get_list() + 6;
            arch_bit = 1ull << 6;

            break;

        case 15329861184856096:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6)
             */
            
            arch = arch_info_get_list() + 5;
            arch_bit = 1ull << 5;

            break;

        case 15611336161566752:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7)
             */

            arch = arch_info_get_list() + 8;
            arch_bit = 1ull << 8;

            break;

        case 3996502057361088544:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7F)
             */

            arch = arch_info_get_list() + 9;
            arch_bit = 1ull << 9;

            break;

        case 7725773898219855904:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7K)
             */

            arch = arch_info_get_list() + 11;
            arch_bit = 1ull << 11;

            break;

        case 8302234650523279392:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V7S)
             */

            arch = arch_info_get_list() + 10;
            arch_bit = 1ull << 10;

            break;

        case 7869889086295711776:
            /*
             * (CPU_TYPE_ARM, CPU_SUBTYPE_ARM_V6M)
             */

            arch = arch_info_get_list() + 12;
            arch_bit = 1ull << 12;

            break;

        case 14696542487257120:
            /*
             * (CPU_TYPE_ARM_64, CPU_SUBTYPE_ARM_64_ALL)
             */

            arch = arch_info_get_list() + 16;
            arch_bit = 1ull << 16;

            break;

        default:
            return E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE;
    }

    /*
     * Perform basic validation of the image-array and mapping-infos array
     * offsets.
     *
     * Note: Due to the ubiquity of shared-cache headers, and any lack of
     * versioning, more stringent validation is not performed.
     */

    const struct range cache_range = {
        .begin = sizeof(header), 
        .end = size
    };

    if (!range_contains_location(cache_range, header.mappingOffset)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    if (!range_contains_location(cache_range, header.imagesOffset)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    /*
     * Validate that the mapping-infos array and images-array have no overflows.
     */

    uint64_t mappings_size = sizeof(struct dyld_cache_mapping_info);
    if (guard_overflow_mul(&mappings_size, header.mappingCount)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    uint64_t mappings_end = header.mappingOffset;
    if (guard_overflow_add(&mappings_end, mappings_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    uint64_t images_size = sizeof(struct dyld_cache_image_info);
    if (guard_overflow_mul(&images_size, header.imagesCount)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    uint64_t images_end = header.imagesOffset;
    if (guard_overflow_add(&images_end, images_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    if (!range_contains_end(cache_range, mappings_end)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;
    }

    if (!range_contains_end(cache_range, images_end)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }
    /*
     * Ensure that the total-size of the mappings and images can be quantified.
     */

    uint64_t infos_end = mappings_size;
    if (guard_overflow_add(&infos_end, images_size)) {
        return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
    }

    /*
     * Ensure that the mapping-infos array and images-array do not overlap.
     */

    const struct range mappings_range = {
        .begin = header.mappingOffset,
        .end = mappings_end
    };

    const struct range images_range = {
        .begin = header.imagesOffset,
        .end = images_end
    };

    if (ranges_overlap(mappings_range, images_range)) {
        return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_RANGES;
    }

    /*
     * Read the mapping-info array from the shared-cache.
     */

    if (lseek(fd, header.mappingOffset, SEEK_SET) < 0) {
        return E_DYLD_SHARED_CACHE_PARSE_SEEK_FAIL;
    }

    struct dyld_cache_mapping_info *const mappings = calloc(1, mappings_size);
    if (mappings == NULL) {
        return E_DYLD_SHARED_CACHE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, mappings, mappings_size) < 0) {
        free(mappings);
        return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
    }

    /*
     * Mappings are like mach-o segments, covering entire swaths of the file.
     */

    const struct range full_cache_range = {
        .begin = 0, 
        .end = size
    };

    /*
     * Verify we don't have any overlapping mappings.
     */

    for (uint32_t i = 0; i < header.mappingCount; i++) {
        struct dyld_cache_mapping_info *mapping = mappings + i;
    
        /*
         * We skip validation of mapping's address-range as its irrelevant to
         * our operations, and we aim to be lenient.
         */

        const uint64_t mapping_file_begin = mapping->fileOffset;
        uint64_t mapping_file_end = mapping_file_begin;

        if (guard_overflow_add(&mapping_file_end, mapping->size)) {
            free(mappings);
            return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_MAPPINGS;
        }

        const struct range mapping_file_range = {
            .begin = mapping_file_begin,
            .end = mapping_file_end
        };

        if (!range_contains_range(full_cache_range, mapping_file_range)) {
            free(mappings);
            return E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS;            
        }

        for (uint32_t j = 0; j < i; j++) {
            struct dyld_cache_mapping_info *const inner_mapping = mappings + j;

            const uint64_t inner_file_begin = inner_mapping->fileOffset;
            const uint64_t inner_file_end =
                inner_file_begin + inner_mapping->size;

            const struct range inner_file_range = {
                .begin = inner_file_begin,
                .end = inner_file_end  
            };

            if (ranges_overlap(mapping_file_range, inner_file_range)) {
                free(mappings);
                return E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_MAPPINGS;
            }
        }
    }

    /*
     * Read the images array from the shared-cache.
     */

    if (lseek(fd, header.imagesOffset, SEEK_SET) < 0) {
        free(mappings);
        return E_DYLD_SHARED_CACHE_PARSE_SEEK_FAIL;
    }
    
    struct dyld_cache_image_info *const images = calloc(1, images_size);
    if (images == NULL) {
        free(mappings);
        return E_DYLD_SHARED_CACHE_PARSE_ALLOC_FAIL;
    }

    if (read(fd, images, images_size) < 0) {
        free(images);
        free(mappings);

        return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
    }

    if (options & O_DYLD_SHARED_CACHE_PARSE_ZERO_IMAGE_PADS) {
        for (uint32_t i = 0; i < header.imagesCount; i++) {
            struct dyld_cache_image_info *image = images + i;

            if (options & O_DYLD_SHARED_CACHE_PARSE_VERIFY_IMAGE_PATH_OFFSETS) {
                const uint64_t path_location = image->pathFileOffset;
                if (!range_contains_location(cache_range, path_location)) {
                    return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
                }
            }
 
            image->pad = 0;             
        }
    } else if (options & O_DYLD_SHARED_CACHE_PARSE_VERIFY_IMAGE_PATH_OFFSETS) {
        for (uint32_t i = 0; i < header.imagesCount; i++) {
            struct dyld_cache_image_info *image = images + i;
            const uint64_t path_location = image->pathFileOffset;

            if (!range_contains_location(cache_range, path_location)) {
                return E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES;
            }
        }
    }

    info_in->images = images;
    info_in->images_count = header.imagesCount;

    info_in->mappings = mappings;
    info_in->mappings_count = header.mappingCount;

    info_in->arch = arch;
    info_in->arch_bit = arch_bit;

    info_in->size = size;
    return E_DYLD_SHARED_CACHE_PARSE_OK;
}

static
const char *find_null_terminator(const char *const path, const uint64_t len) {
    const char *iter = path;
    char ch = *iter;

    for (uint64_t i = 0; i != len; i++, ch = *(++iter)) {
        if (ch != '\0') {
            continue; 
        }

        return iter;
    }

    return NULL;
}

static enum dyld_shared_cache_parse_result
read_path_string(const int fd, const uint64_t max_len, char **const path_out) {
    char buf[256] = {};
    if (read(fd, buf, sizeof(buf)) < 0) {
        return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
    }

    uint64_t bytes_read = 0;
    uint64_t path_length = 0;

    do {
        const char *null_term = find_null_terminator(buf, sizeof(buf));
        if (null_term != NULL) {
            const uint64_t buf_length = null_term - buf;

            /*
             * Add to bytes_read after calculating path-length, as we only
             * accout in path_lengththe previous reads, not the current one.
             */

            path_length = bytes_read + buf_length;
            bytes_read += sizeof(buf);
    
            break;
        }

        bytes_read += sizeof(buf);

        const uint64_t bytes_remaining = max_len - bytes_read;
        if (bytes_remaining < sizeof(buf)) {
            if (read(fd, buf, bytes_remaining) < 0) {
                return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
            }
        
            null_term = find_null_terminator(buf, sizeof(buf));
            if (null_term != NULL) {
                const uint64_t buf_length = null_term - buf;

                path_length = bytes_read + buf_length;
                bytes_read += bytes_remaining; 
            } else {
                bytes_read += bytes_remaining;
                path_length = bytes_read;
            }

            break;    
        }

        if (read(fd, buf, sizeof(buf)) < 0) {
            return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
        }
    } while (true);

    if (lseek(fd, -bytes_read, SEEK_CUR) < 0) {
        return E_DYLD_SHARED_CACHE_PARSE_SEEK_FAIL;
    }

    char *const path = calloc(1, path_length + 1);
    if (path == NULL) {
        return E_DYLD_SHARED_CACHE_PARSE_ALLOC_FAIL;
    }            

    if (read(fd, path, path_length) < 0) {
        free(path);
        return E_DYLD_SHARED_CACHE_PARSE_READ_FAIL;
    }

    *path_out = path;
    return E_DYLD_SHARED_CACHE_PARSE_OK;
}

enum dyld_shared_cache_parse_result
dyld_shared_cache_iterate_images_with_callback(
    const struct dyld_shared_cache_info *const info_in,
    const int fd,
    const uint64_t start,
    const void *const item,
    const dyld_shared_cache_iterate_images_callback callback)
{
    const uint32_t images_count = info_in->images_count; 
    const uint64_t cache_size = info_in->size;

    for (uint32_t i = 0; i < images_count; i++) {
        struct dyld_cache_image_info *const image = info_in->images + i;

        const uint64_t path_file_offset = image->pathFileOffset;
        const uint64_t max_path_length = cache_size - path_file_offset;

        if (lseek(fd, start + path_file_offset, SEEK_SET) < 0) {
            return E_DYLD_SHARED_CACHE_PARSE_SEEK_FAIL;
        }

        char *path = NULL;
        const enum dyld_shared_cache_parse_result get_path_result =
            read_path_string(fd, max_path_length, &path);

        if (get_path_result != E_DYLD_SHARED_CACHE_PARSE_OK) {
            return get_path_result;
        }

        if (callback(image, path, item)) {
            continue; 
        }

        break;
    }

    return E_DYLD_SHARED_CACHE_PARSE_OK;
}

void dyld_shared_cache_info_destroy(struct dyld_shared_cache_info *const info) {
    free(info->mappings);
    free(info->images);

    info->mappings = NULL;
    info->images = NULL;

    info->arch = NULL;

    info->arch_bit = 0;
    info->size = 0;
}
