//
//  include/dyld_shared_cache_format.h
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef DYLD_SHARED_CACHE_FORMAT_H
#define DYLD_SHARED_CACHE_FORMAT_H

#include <stdint.h>

/*
 * From dyld/dyld3/shared-cache/dyld_cache_format.h in apple's dyld source.
 */

struct dyld_cache_header {
    char magic[16];
    uint32_t mappingOffset;
    uint32_t mappingCount;
    uint32_t imagesOffset;
    uint32_t imagesCount;
    uint64_t dyldBaseAddress;
};

struct dyld_cache_mapping_info {
    uint64_t address;
    uint64_t size;
    uint64_t fileOffset;
    uint32_t maxProt;
    uint32_t initProt;
};

struct dyld_cache_image_info {
    uint64_t address;
    uint64_t modTime;
    uint64_t inode;
    uint32_t pathFileOffset;
    uint32_t pad;
};

#endif /* DYLD_SHARED_CACHE_FORMAT_H */
