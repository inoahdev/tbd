//
//  include/c_str_utils.h
//  tbd
//
//  Created by inoahdev on 11/21/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef C_STR_UTILS_H
#define C_STR_UTILS_H

#include <stdbool.h>
#include <stdint.h>

bool c_str_is_all_whitespace(const char *str);
bool c_str_is_all_digits(const char *str);

bool c_str_with_len_is_all_whitespace(const char *str, uint64_t length);

bool c_str_has_whitespace(const char *str);
bool c_str_has_prefix(const char *str, const char *prefix);

#endif /* C_STR_UTILS_H */
