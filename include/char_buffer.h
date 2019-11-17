//
//  include/char_buffer.h
//  tbd
//
//  Created by inoahdev on 11/10/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#ifndef CHAR_BUFFER_H
#define CHAR_BUFFER_H

#include <stdlib.h>
#include "notnull.h"

struct char_buffer {
    char *data;

    uint64_t length;
    uint64_t capacity;
};

enum char_buffer_result {
    E_CHAR_BUFFER_OK,
    E_CHAR_BUFFER_ALLOC_FAIL
};

enum char_buffer_result
cb_add_c_str(struct char_buffer *__notnull sb,
             const char *__notnull str,
             uint64_t length);

enum char_buffer_result
cb_reserve_capacity(struct char_buffer *__notnull cb, uint64_t capacity);

void cb_clear(struct char_buffer *__notnull sb);
void cb_destroy(struct char_buffer *__notnull sb);

#endif /* CHAR_BUFFER_H */
