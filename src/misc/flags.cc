//
//  src/misc/flags.cc
//  tbd
//
//  Created by inoahdev on 7/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>

#include <cstdint>
#include <cstdio>

#include <cstdlib>
#include <cstring>

#include "flags.h"

flags::flags(flags_integer_t length)
: length(length) {
    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = (size_t)((double)length / (double)bit_size) + 1;
        auto allocation_size = sizeof(flags_integer_t) * size;

        bits.pointer = (flags_integer_t *)calloc(1, allocation_size);

        if (!bits.pointer) {
            fprintf(stderr, "Failed to allocate data of size (%ld), failing with error (%s)\n", size, strerror(errno));
            exit(1);
        }
    }
}

flags::flags(const flags &flags) {
    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = (size_t)((double)length / (double)bit_size) + 1;
        auto allocation_size = sizeof(flags_integer_t) * size;

        bits.pointer = (flags_integer_t *)malloc(allocation_size);
        if (!bits.pointer) {
            fprintf(stderr, "Failed to allocate data of size (%ld), failing with error (%s)\n", size, strerror(errno));
            exit(1);
        }

        memcpy(flags.bits.pointer, flags.bits.pointer, allocation_size);
    } else {
        bits.integer = flags.bits.integer;
    }
}

flags::~flags() {
    if (length > bit_size()) {
        free(bits.pointer);
    }
}

void flags::cast(flags_integer_t index, bool flag) noexcept {
    if (index > length - 1) {
        fprintf(stderr, "INTERNAL: Casting bit at index %llu from buffer with bits-length %llu", index, length);
        exit(1);
    }

    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        // As length can be of any size, it is not possible to
        // simply cast bits.pointer to flags_integer_t *, instead, the
        // pointer must be (temporarily) advanced to allow casting
        // as flags_integer_t *.

        // As length is larger than sizeof(flags_integer_t)
        // It is often necessary to advance the flags-pointer
        // to the next integers in the buffers to access the bit at the index the
        // caller provided.

        auto location = (flags_integer_t)((double)(index + 1) / (double)bit_size);
        auto pointer = (flags_integer_t *)((flags_integer_t)bits.pointer + (sizeof(flags_integer_t) * location));

        index -= bit_size * location;

        if (flag) {
            *pointer |= (flags_integer_t)1 << index;
        } else {
            *pointer &= ~((flags_integer_t)1 << index);
        }
    } else {
        auto flags = bits.integer;
        if (flag) {
            flags |= (flags_integer_t)1 << index;
        } else {
            flags &= ~((flags_integer_t)1 << index);
        }

        bits.integer = flags;
    }
}

bool flags::at(flags_integer_t index) const noexcept {
    if (index > length - 1) {
        fprintf(stderr, "INTERNAL: Accessing bit at index %llu from buffer with bits-length %llu", index, length);
        exit(1);
    }

    auto flags = bits.integer;

    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        // As length can be of any size, it is not possible to
        // simply cast bits.pointer to flags_integer_t *, instead, the
        // pointer must be (temporarily) advanced to allow casting
        // as flags_integer_t *.

        // As length is larger than sizeof(flags_integer_t)
        // It is often necessary to advance the flags-pointer
        // to the next byte to access the bit at the index the
        // caller provided.

        // To advance the flags-pointer to the right byte(s), the
        // flags-pointer is advanced by one byte until index_from_back
        // is smaller than bit_size (bit-count of flags_integer_t).

        auto location = (flags_integer_t)((double)(index + 1) / (double)bit_size);
        auto pointer = (flags_integer_t *)((flags_integer_t)bits.pointer + (sizeof(flags_integer_t) * location));

        index -= bit_size * location;
        flags = *pointer;
    }

    return (flags & ((flags_integer_t)1 << index)) ? true : false;
}

bool flags::operator==(const flags &flags) const noexcept {
    if (length != flags.length) {
        return false;
    }

    if (length > bit_size()) {
        return memcmp(bits.pointer, flags.bits.pointer, length);
    } else {
        return bits.integer == flags.bits.integer;
    }
}
