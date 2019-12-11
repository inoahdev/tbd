//
//  src/magic_buffer.c
//  tbd
//
//  Created by inoahdev on 12/8/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include <unistd.h>
#include "magic_buffer.h"

enum magic_buffer_result
magic_buffer_read_n(struct magic_buffer *__notnull const buffer,
                    const int fd,
                    const uint64_t n)
{
    const uint64_t buff_read = buffer->read;
    if (buff_read >= n) {
        return E_MAGIC_BUFFER_OK;
    }

    void *const buff = buffer->buff + buff_read;

    const uint64_t read_size = n - buff_read;
    const ssize_t read_ret = read(fd, buff, read_size);

    if (read_ret < 0) {
        return E_MAGIC_BUFFER_READ_FAIL;
    }

    buffer->read = buff_read + read_ret;
    return E_MAGIC_BUFFER_OK;
}
