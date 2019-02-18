//
//  src/yaml.c
//  tbd
//
//  Created by inoahdev on 11/25/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <ctype.h>
#include <stdbool.h>

#include "yaml.h"

static inline bool char_needs_quotes(const char ch) {
    switch (ch) {
        case ':':
        case '{':
        case '}':
        case '[':
        case ']':
        case ',':
        case '&':
        case '*':
        case '#':
        case '?':
        case '|':
        case '-':
        case '<':
        case '>':
        case '=':
        case '!':
        case '%':
        case '@':
        case '`':
        case ' ':
            return true;

        default:
            return false;
    }
}

bool yaml_check_c_str(const char *const string, const uint64_t length) {
    for (uint64_t i = 0; i != length; i++) {
        const char ch = string[i];
        if (!char_needs_quotes(ch)) {
            continue;
        }

        return true;
    }

    return false;
}
