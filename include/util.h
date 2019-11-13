//
//  include/util.h
//  tbd
//
//  Created by inoahdev on 10/24/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stdint.h>

#include "notnull.h"

/*
 * Maybe one day, add support for Windows paths.
 */

const char *
get_front_of_slashes(const char *__notnull begin, const char *__notnull iter);

const char *get_end_of_slashes(const char *__notnull path);
const char *
get_end_of_slashes_with_end(const char *__notnull path,
                            const char *__notnull end);

const char *
find_last_slash(const char *__notnull path, const char *__notnull end);

const char *
find_last_row_of_slashes(const char *__notnull path, const char *__notnull end);

const char *
remove_front_slashes(const char *__notnull string,
                     uint64_t length,
                     uint64_t *length_out);

uint64_t remove_end_slashes(const char *__notnull string, uint64_t length);

#endif /* UTIL_H */
