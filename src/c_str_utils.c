//
//  include/c_str_utils.c
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <ctype.h>
#include "c_str_utils.h"

bool c_str_is_all_whitespace(const char *const str) {
    const char *str_iter = str;
    for (char ch = *str_iter; ch != '\0'; ch = *(++str_iter)) {
        if (isspace(ch)) {
            continue;
        }

        return false;
    }

    return true;
}

bool
c_str_with_len_is_all_whitespace(const char *const str, const uint64_t length) {
    const char *str_iter = str;
    char ch = *str_iter;

    for (uint64_t i = 0; i != length; i++, ch = *(++str_iter)) {
        if (isspace(ch)) {
            continue;
        }

        return false;
    }

    return true;
}


bool c_str_has_whitespace(const char *const str) {
    const char *str_iter = str;
    for (char ch = *str_iter; ch != '\0'; ch = *(++str_iter)) {
        if (isspace(ch)) {
            return true;
        }
    }

    return false;
}
