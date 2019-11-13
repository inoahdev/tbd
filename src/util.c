//
//  src/util.c
//  tbd
//
//  Created by inoahdev on 10/24/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include <stdlib.h>
#include "util.h"

static inline bool ch_is_slash(const char ch) {
    return (ch == '/');
}

const char *
get_front_of_slashes(const char *__notnull const begin,
                     const char *__notnull iter)
{
    const char *const rev_end = begin - 1;
    for (--iter; iter != rev_end; --iter) {
        if (!ch_is_slash(*iter)) {
            break;
        }
    }

    return (iter + 1);
}

const char *get_end_of_slashes(const char *__notnull const path) {
    const char *iter = path;
    for (char ch = *(++iter); ch != '\0'; ch = *(++iter)) {
        if (!ch_is_slash(ch)) {
            return iter;
        }
    }

    return NULL;
}

const char *
get_end_of_slashes_with_end(const char *__notnull const path,
                            const char *__notnull const end)
{
    const char *iter = path;
    for (; iter != end; ++iter) {
        const char ch = *iter;
        if (!ch_is_slash(ch)) {
            break;
        }
    }

    return iter;
}

const char *
find_last_slash(const char *__notnull const path,
                const char *__notnull const end)
{
    const char *iter = end - 1;
    const char *const rev_end = path - 1;

    for (; iter != rev_end; --iter) {
        if (ch_is_slash(*iter)) {
            return iter;
        }
    }

    return NULL;
}

const char *
find_last_row_of_slashes(const char *__notnull const path,
                         const char *__notnull const end)
{
    const char *iter = end - 1;
    const char *const rev_end = path - 1;

    for (; iter != rev_end; --iter) {
        if (ch_is_slash(*iter)) {
            return get_front_of_slashes(path, iter);
        }
    }

    return NULL;
}

const char *
remove_front_slashes(const char *__notnull const string,
                     const uint64_t length,
                     uint64_t *const length_out)
{
    const char *const end = string + length;
    const char *const non_space = get_end_of_slashes_with_end(string, end);

    if (length_out != NULL) {
        if (non_space == NULL) {
            *length_out = 0;
            return NULL;
        }

        const uint64_t drift = (uint64_t)(non_space - string);
        const uint64_t new_length = length - drift;

        *length_out = new_length;
    }

    return non_space;
}

uint64_t
remove_end_slashes(const char *__notnull string, const uint64_t length) {
    const char *iter = string + (length - 1);
    const char *const rev_end = string - 1;

    for (; iter != rev_end; --iter) {
        const char ch = *iter;
        if (ch_is_slash(ch)) {
            continue;
        }

        /*
         * Add one to iter to get the pointer to the beginning of the row of
         * back-slashes.
         */

        const uint64_t trimmed_length = (uint64_t)(++iter - string);
        return trimmed_length;
    }

    return 0;
}
