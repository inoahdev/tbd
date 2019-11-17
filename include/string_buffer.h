//
//  include/string_buffer.h
//  tbd
//
//  Created by inoahdev on 11/10/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#include <stdlib.h>
#include "notnull.h"

struct string_buffer {
    char *data;

    uint64_t length;
    uint64_t capacity;
};

enum string_buffer_result {
    E_STRING_BUFFER_OK,
    E_STRING_BUFFER_ALLOC_FAIL
};

enum string_buffer_result
sb_add_c_str(struct string_buffer *__notnull sb,
             const char *__notnull str,
             uint64_t length);

enum string_buffer_result
sb_reserve_space(struct string_buffer *__notnull sb, uint64_t capacity);

void sb_clear(struct string_buffer *__notnull sb);
void sb_destroy(struct string_buffer *__notnull sb);

#endif /* STRING_BUFFER_H */
