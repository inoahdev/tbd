//
//  src/stream/file/seek_type.h
//  tbd
//
//  Created by administrator on 11/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdio>

namespace stream::file {
    enum class seek_type {
        beginning = SEEK_SET,
        current = SEEK_CUR,
        end = SEEK_END
    };
}
