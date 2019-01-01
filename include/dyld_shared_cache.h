//
//  include/dyld_shared_cache.h
//  tbd 
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef DYLD_SHARED_CACHE_H
#define DYLD_SHARED_CACHE_H

#include <mach/machine.h>
#include <stdbool.h>

#include "dyld_shared_cache_format.h"

enum dyld_shared_cache_parse_options {
    O_DYLD_SHARED_CACHE_PARSE_ZERO_IMAGE_PADS           = 1 << 0,
    O_DYLD_SHARED_CACHE_PARSE_VERIFY_IMAGE_PATH_OFFSETS = 1 << 1
};

enum dyld_shared_cache_parse_result {
    E_DYLD_SHARED_CACHE_PARSE_OK,
    E_DYLD_SHARED_CACHE_PARSE_ALLOC_FAIL,

    E_DYLD_SHARED_CACHE_PARSE_SEEK_FAIL,
    E_DYLD_SHARED_CACHE_PARSE_READ_FAIL,

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

    struct dyld_cache_mapping_info *mappings;
    uint64_t mappings_count;

    const struct arch_info *arch;

    uint64_t arch_bit;
    uint64_t size;
};

enum dyld_shared_cache_parse_result
dyld_shared_cache_parse_from_file(struct dyld_shared_cache_info *info_in,
                                  int fd,
                                  uint64_t size,
                                  uint64_t options);

enum dyld_shared_cache_parse_result
dyld_shared_cache_parse_from_range(struct dyld_shared_cache_info *info_in,
                                   int fd,
                                   uint64_t start,
                                   uint64_t end,
                                   uint64_t options); 

/*
 * dyld_shared_cache_iterate_images_with_callback callback.
 * Callback takes ownership of path, its responsible for freeing it.
 *
 * Return true to continue iterating, false to stop.
 */

typedef bool 
(*dyld_shared_cache_iterate_images_callback)(
    struct dyld_cache_image_info *image,
    char *path,
    const void *item);

enum dyld_shared_cache_parse_result
dyld_shared_cache_iterate_images_with_callback(
    const struct dyld_shared_cache_info *info_in,
    int fd,
    uint64_t start,
    const void *item,
    dyld_shared_cache_iterate_images_callback callback);

void dyld_shared_cache_info_destroy(struct dyld_shared_cache_info *info);

#endif /* DYLD_SHARED_CACHE_H */
