//
//  include/objc.h
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef OBJC_H
#define OBJC_H

#include <stdint.h>

enum objc_image_info_flags {
    F_OBJC_IMAGE_INFO_SUPPORTS_GC = 1 << 1,
    F_OBJC_IMAGE_INFO_REQUIRES_GC = 1 << 2,
    F_OBJC_IMAGE_INFO_OPTIMIZED_FROM_DYLD_SHARED_CACHE = 1 << 3,
    F_OBJC_IMAGE_INFO_IS_FOR_SIMULATOR = 1 << 5,
    F_OBJC_IMAGE_INFO_HAS_OBJC_CATEGORIES = 1 << 6
};

enum objc_image_info_swift_version {
    OBJC_IMAGE_INFO_SWIFT_VERSION_1 = 1,
    OBJC_IMAEG_INFO_SWIFT_VERSION_1_2,
    OBJC_IMAGE_INFO_SWIFT_VERSION_2,
    OBJC_IMAGE_INFO_SWIFT_VERSION_3
};

static const uint32_t objc_image_info_swift_version_mask = 0xff00;

struct objc_image_info {
    uint32_t version;
    uint32_t flags;
};

#endif /* OBJC_H */
