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
#include "likely.h"

char *
alloc_and_copy(const char *__notnull const string, const uint64_t length) {
    char *const copy = malloc(length + 1);
    if (unlikely(copy == NULL)) {
        return NULL;
    }

    memcpy(copy, string, length);
    copy[length] = '\0';

    return copy;
}
