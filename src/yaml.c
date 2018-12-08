//
//  src/yaml.c
//  tbd
//
//  Created by inoahdev on 11/25/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <stdbool.h>
#include "yaml.h"

static const char needs_quotes[] = {
    ':', '{', '}', '[', ']', ',', '&', '*', '#', '?', '|', '-', '<', '>', '=',
    '!', '%', '@', '`'
};

static bool yaml_needs_quotes(const char *const string) {
    const char *iter = string;
    for (char ch = *iter; ch != '\0'; ch = *(++iter)) {
        for (int i = 0; i < sizeof(needs_quotes); i++) {
            if (ch == needs_quotes[i]) {
                return true;
            }
        }
    }

    return false;
}

int yaml_write_string(FILE *const file, const char *const string) {
    if (yaml_needs_quotes(string)) {
        if (fprintf(file, "\'%s\'", string) < 0) {
            return 1;
        }

        return 0;
    }

    if (fprintf(file, "%s", string) < 0) {
        return 1;
    }

    return 0;
}

int
yaml_write_string_with_len(FILE *const file,
                           const char *const string,
                           const uint32_t len)
{
    if (yaml_needs_quotes(string)) {
        if (fputc('\'', file) == EOF) {
            return 1;
        }

        if (fwrite(string, len, 1, file) != 1) {
            return 1;
        }

        if (fputc('\'', file) == EOF) {
            return 1;
        }

        return 0;
    }

    if (fwrite(string, len, 1, file) != 1) {
        return 1;
    }

    return 0;
}