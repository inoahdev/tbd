//
//  src/char_buffer.c
//  tbd
//
//  Created by inoahdev on 11/10/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "likely.h"
#include "char_buffer.h"

static inline uint64_t get_new_capacity(const uint64_t capacity) {
    return (capacity * 2);
}

static uint64_t
expand_for_length(struct char_buffer *__notnull cb, const uint64_t needed) {
    const uint64_t capacity = cb->capacity;
    const uint64_t wanted = cb->length + needed;

    if (unlikely(capacity == 0)) {
        return wanted;
    }

    uint64_t new_capacity = capacity;
    do {
        new_capacity = get_new_capacity(new_capacity);
    } while (new_capacity < wanted);

    return new_capacity;
}

enum char_buffer_result
cb_add_c_str(struct char_buffer *__notnull const cb,
             const char *__notnull const str,
             const uint64_t length)
{
    char *data = cb->data;

    const uint64_t cb_length = cb->length;
    const uint64_t free_count = cb->capacity - cb_length;

    if (free_count < length) {
        const uint64_t new_cap = expand_for_length(cb, length);
        char *const new_data = malloc(new_cap);

        if (new_data == NULL) {
            return E_CHAR_BUFFER_ALLOC_FAIL;
        }

        if (cb_length != 0) {
            memcpy(new_data, data, cb_length);
            free(data);
        }

        data = new_data;

        cb->data = data;
        cb->capacity = new_cap;
    }

    const uint64_t new_length = cb_length + length;
    memcpy(data + cb_length, str, length);

    cb->length = new_length;
    return E_CHAR_BUFFER_OK;
}

enum char_buffer_result
cb_reserve_capacity(struct char_buffer *__notnull const cb,
                    const uint64_t capacity)
{
    const uint64_t cb_capacity = cb->capacity;
    if (cb_capacity < capacity) {
        return E_CHAR_BUFFER_OK;
    }

    char *data = cb->data;
    uint64_t new_cap = cb_capacity;

    if (new_cap != 0) {
        do {
            new_cap = get_new_capacity(new_cap);
        } while (new_cap < capacity);
    } else {
        new_cap = capacity;
    }

    char *const new_data = malloc(new_cap);
    if (new_data == NULL) {
        return E_CHAR_BUFFER_ALLOC_FAIL;
    }

    const uint64_t cb_length = cb->length;
    if (cb_length != 0) {
        memcpy(new_data, data, cb_length);
        free(data);
    }

    cb->capacity = new_cap;
    return E_CHAR_BUFFER_OK;
}

void cb_clear(struct char_buffer *__notnull const cb) {
    cb->length = 0;
}

void cb_destroy(struct char_buffer *__notnull const cb) {
    free(cb->data);

    cb->data = NULL;
    cb->length = 0;
    cb->capacity = 0;
}
