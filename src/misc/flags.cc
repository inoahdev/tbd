//
//  src/tbd/flags.cc
//  tbd
//
//  Created by administrator on 7/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>

#include <cstdint>
#include <cstdio>

#include <cstdlib>
#include <cstring>

#include "flags.h"

flags::flags(long length)
: length_(length) {
    if (length > bit_size()) {
        size_t size = length * bit_size();
        flags_.ptr = calloc(1, size);

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

void flags::cast(long index, bool result) noexcept {
    if (length_ > bit_size()) {
        auto ptr = (unsigned int *)flags_.ptr;

        const auto bit_size = this->bit_size();
        const auto bits_length = bit_size * length_;

        // As length_ can be of any size, it is not possible to
        // simply cast flags_.ptr to unsigned int *, instead, the
        // pointer must be (temporarily) advanced to allow casting
        // as unsigned int *.

        // As length_ is larger than sizeof(unsigned int)
        // It is often necessary to advance the flags-pointer
        // to the next byte to access the bit at the index the
        // caller provided.

        // To advance the flags-pointer to the right byte(s), the
        // flags-pointer is advanced by one byte until index_from_back
        // is smaller than bit_size (bit-count of unsigned int).

        while (index > bits_length) {
            index -= 8;
            ptr = (unsigned int *)((uintptr_t)ptr + 1);
        }

        if (result) {
            *ptr |= 1 << index;
        } else {
            *ptr &= ~(1 << index);
        }

    } else {
        auto flags = flags_.flags;

        if (result) {
            flags |= 1 << index;
        } else {
            flags &= ~(1 << index);
        }

        flags_.flags = flags;
    }
}

bool flags::at_index(long index) const noexcept {
    if (length_ > bit_size()) {
        auto ptr = (unsigned int *)flags_.ptr;

        const auto bit_size = this->bit_size();
        const auto bits_length = bit_size * length_;

        // As length_ can be of any size, it is not possible to
        // simply cast flags_.ptr to unsigned int *, instead, the
        // pointer must be (temporarily) advanced to allow casting
        // as unsigned int *.

        // As length_ is larger than sizeof(unsigned int)
        // It is often necessary to advance the flags-pointer
        // to the next byte to access the bit at the index the
        // caller provided.

        // To advance the flags-pointer to the right byte(s), the
        // flags-pointer is advanced by one byte until index_from_back
        // is smaller than bit_size (bit-count of unsigned int).

        while (index > bits_length) {
            index -= 8;
            ptr = (unsigned int *)((uintptr_t)ptr + 1);
        }

        return (*ptr & 1 << index) ? true : false;
    } else {
        const auto flags = flags_.flags;
        return (flags & 1 << index) ? true : false;
    }
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
