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
: length_(length) {
    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = (size_t)((double)length / (double)bit_size) + 1;
        flags_.ptr = (flags_integer_t *)calloc(1, sizeof(flags_integer_t) * size);

        if (!flags_.ptr) {
            fprintf(stderr, "Failed to allocate data of size (%ld), failing with error (%s)\n", size, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

flags::~flags() {
    if (length_ > bit_size()) {
        free(flags_.ptr);
    }
}

void flags::cast(flags_integer_t index, bool flag) noexcept {
    if (index > length_ - 1) {
        fprintf(stderr, "INTERNAL: Casting bit at index %llu from buffer with bits-length %llu", index, length_);
        exit(1);
    }

    auto shift_number = flags_integer_t(1);

    const auto bit_size = this->bit_size();
    if (length_ > bit_size) {
        if (index > bit_size - 1) {
            const auto bit_size = this->bit_size();

            // As length_ can be of any size, it is not possible to
            // simply cast flags_.ptr to flags_integer_t *, instead, the
            // pointer must be (temporarily) advanced to allow casting
            // as flags_integer_t *.

            // As length_ is larger than sizeof(flags_integer_t)
            // It is often necessary to advance the flags-pointer
            // to the next integers in the buffers to access the bit at the index the
            // caller provided.

            auto location = (flags_integer_t)((double)(index + 1) / (double)bit_size);
            auto ptr = (flags_integer_t *)((flags_integer_t)flags_.ptr + (sizeof(flags_integer_t) * location));

            index -= bit_size * location;

            if (flag) {
                *ptr |= shift_number << index;
            } else {
                *ptr &= ~(shift_number << index);
            }
        } else {
            if (flag) {
                *flags_.ptr |= shift_number << index;
            } else {
                *flags_.ptr &= ~(shift_number << index);
            }
        }
    } else {
        auto flags = flags_.flags;
        if (flag) {
            flags |= shift_number << index;
        } else {
            flags &= ~(shift_number << index);
        }

        flags_.flags = flags;
    }
}

bool flags::at(flags_integer_t index) const noexcept {
    if (index > length_ - 1) {
        fprintf(stderr, "INTERNAL: Accessing bit at index %llu from buffer with bits-length %llu", index, length_);
        exit(1);
    }

    auto flags = flags_.flags;

    const auto bit_size = this->bit_size();
    if (length_ > bit_size) {
        if (index > bit_size - 1) {
            const auto bit_size = this->bit_size();

            // As length_ can be of any size, it is not possible to
            // simply cast flags_.ptr to flags_integer_t *, instead, the
            // pointer must be (temporarily) advanced to allow casting
            // as flags_integer_t *.

            // As length_ is larger than sizeof(flags_integer_t)
            // It is often necessary to advance the flags-pointer
            // to the next byte to access the bit at the index the
            // caller provided.

            // To advance the flags-pointer to the right byte(s), the
            // flags-pointer is advanced by one byte until index_from_back
            // is smaller than bit_size (bit-count of flags_integer_t).

            auto location = (flags_integer_t)((double)(index + 1) / (double)bit_size);
            auto ptr = (flags_integer_t *)((flags_integer_t)flags_.ptr + (sizeof(flags_integer_t) * location));

            index -= bit_size * location;
            flags = *ptr;
        } else {
            flags = *flags_.ptr;
        }
    }

    const auto shift_number = flags_integer_t(1);
    return (flags & (shift_number << index)) ? true : false;
}

bool flags::operator==(const flags &flags) const noexcept {
    if (length_ != flags.length_) {
        return false;
    }

    if (length_ > bit_size()) {
        return memcmp(flags_.ptr, flags.flags_.ptr, length_);
    } else {
        return flags_.flags == flags.flags_.flags;
    }
}
