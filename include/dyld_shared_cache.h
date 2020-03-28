//
//  include/dyld_shared_cache.h
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#ifndef DYLD_SHARED_CACHE_H
#define DYLD_SHARED_CACHE_H

#include <stdbool.h>

#include "mach/machine.h"
#include "dyld_shared_cache_format.h"

#include "notnull.h"
#include "range.h"

struct dyld_shared_cache_parse_options {
    bool zero_image_pads : 1;
    bool verify_image_path_offsets : 1;
};

struct dyld_shared_cache_flags {
    bool unmap_map : 1;
};

enum dyld_shared_cache_parse_result {
    E_DYLD_SHARED_CACHE_PARSE_OK,
    E_DYLD_SHARED_CACHE_PARSE_ALLOC_FAIL,

    E_DYLD_SHARED_CACHE_PARSE_FSTAT_FAIL,

    E_DYLD_SHARED_CACHE_PARSE_READ_FAIL,
    E_DYLD_SHARED_CACHE_PARSE_MMAP_FAIL,

    E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE,

    E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES,
    E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS,

    E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_RANGES,
    E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_IMAGES,
    E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_MAPPINGS
};

struct dyld_shared_cache_info {
    struct dyld_cache_image_info *images;
    uint32_t images_count;

    /*
     * An absolute offset to the array of dyld_cache_mapping_info structures.
     */

    const struct dyld_cache_mapping_info *mappings;
    uint32_t mappings_count;

    uint8_t *map;
    uint64_t size;
    uint64_t arch_index;

    const struct arch_info *arch;
    struct range available_range;

    struct dyld_shared_cache_flags flags;
};

enum dyld_shared_cache_parse_result
dyld_shared_cache_parse_from_file(
    struct dyld_shared_cache_info *__notnull info_in,
    int fd,
    const char magic[16],
    struct dyld_shared_cache_parse_options options);

enum dyld_shared_cache_parse_result
dyld_shared_cache_parse_from_range(
    struct dyld_shared_cache_info *__notnull info_in,
    int fd,
    uint64_t start,
    uint64_t end,
    struct dyld_shared_cache_parse_options options);

void
dyld_shared_cache_print_list_of_images(int fd,
                                       uint64_t start,
                                       uint64_t end);

void
dyld_shared_cache_info_destroy(struct dyld_shared_cache_info *__notnull info);

#endif /* DYLD_SHARED_CACHE_H */
