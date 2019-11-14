//
//  include/objc.h
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef OBJC_H
#define OBJC_H

#include <stdint.h>

enum objc_image_info_flags {
    F_OBJC_IMAGE_INFO_SUPPORTS_GC      = 1 << 1,
    F_OBJC_IMAGE_INFO_REQUIRES_GC      = 1 << 2,
    F_OBJC_IMAGE_INFO_IS_FOR_SIMULATOR = 1 << 5
};

static const uint32_t OBJC_IMAGE_INFO_SWIFT_VERSION_MASK = 0xff00;
static const uint32_t OBJC_IMAGE_INFO_SWIFT_VERSION_SHIFT = 0x8;

struct objc_image_info {
    uint32_t version;
    uint32_t flags;
};

#endif /* OBJC_H */
