//
//  src/copy.c
//  tbd
//
//  Created by inoahdev on 02/15/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "copy.h"

char *alloc_and_copy(const char *const string, const uint64_t length) {
    char *const copy = calloc(1, length + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, string, length);
    return copy;
}
