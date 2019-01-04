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

static const char needs_quotes[] = {
    ':', '{', '}', '[', ']', ',', '&', '*', '#', '?', '|', '-', '<', '>', '=',
    '!', '%', '@', '`', ' '
};

static inline bool char_needs_quotes(const char ch) {
    for (int i = 0; i < sizeof(needs_quotes); i++) {
        const char needs_quotes_ch = needs_quotes[i];
        if (ch != needs_quotes_ch) {
            continue;
        }

        return true;
    }

    return false;
}

void
yaml_check_c_str(const char *const string,
                 const uint64_t length,
                 bool *const needs_quotes_out)
{
    bool needs_quotes = false;
    for (uint64_t i = 0; i != length; i++) {
        const char ch = string[i];
        if (!char_needs_quotes(ch)) {
            continue;
        }

        needs_quotes = true;
        break;
    }

    *needs_quotes_out = needs_quotes;
}