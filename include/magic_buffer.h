//
//  include/magic_buffer.h
//  tbd
//
//  Created by inoahdev on 12/8/19.
//  Copyright © 2019 - 2020 inoahdev. All rights reserved.
//

#ifndef MAGIC_BUFFER_H
#define MAGIC_BUFFER_H

#include <stdint.h>
#include "notnull.h"

static const uint64_t MAGIC_BUFFER_SIZE = 16;

struct magic_buffer {
    uint8_t buff[MAGIC_BUFFER_SIZE];
    uint64_t read;
};

enum magic_buffer_result {
    E_MAGIC_BUFFER_OK,
    E_MAGIC_BUFFER_READ_FAIL
};

enum magic_buffer_result
magic_buffer_read_n(struct magic_buffer *__notnull buffer, int fd, uint64_t n);

#endif /* MAGIC_BUFFER_H */
