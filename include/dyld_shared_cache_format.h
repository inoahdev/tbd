//
//  include/dyld_shared_cache_format.h
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
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

struct dyld_cache_header_v5 {
    char magic[16];
    uint32_t mappingCount;
    uint32_t imagesOffset;
    uint32_t imagesCount;
    uint64_t dyldBaseAddress;
    uint64_t codeSignatureOffset;
    uint64_t codeSignatureSize;
    uint64_t slideInfoOffset;
    uint64_t slideInfoSize;
    uint64_t localSymbolsOffset;
    uint64_t localSymbolsSize;
    uint8_t  uuid[16];
    uint64_t cacheType;
    uint32_t branchPoolsOffset;
    uint32_t branchPoolsCount;
    uint64_t accelerateInfoAddr;
    uint64_t accelerateInfoSize;
    uint64_t imagesTextOffset;
    uint64_t imagesTextCount;
    uint64_t dylibsImageGroupAddr;
    uint64_t dylibsImageGroupSize;
    uint64_t otherImageGroupAddr;
    uint64_t otherImageGroupSize;
    uint64_t progClosuresAddr;
    uint64_t progClosuresSize;
    uint64_t progClosuresTrieAddr;
    uint64_t progClosuresTrieSize;
    uint32_t platform;
    uint32_t formatVersion        : 8,
             dylibsExpectedOnDisk : 1,
             simulator            : 1;
    uint64_t sharedRegionStart;
    uint64_t sharedRegionSize;
    uint64_t maxSlide;
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
