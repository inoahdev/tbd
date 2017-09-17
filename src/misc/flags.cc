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

#include <utility>

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

flags::flags(const flags &flags) noexcept :
length(flags.length) {
    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = (size_t)((double)length / (double)bit_size) + 1;
        auto allocation_size = sizeof(flags_integer_t) * size;

        bits.pointer = (flags_integer_t *)malloc(allocation_size);
        if (!bits.pointer) {
            fprintf(stderr, "Failed to allocate data of size (%ld), failing with error (%s)\n", size, strerror(errno));
            exit(1);
        }

        memcpy(bits.pointer, flags.bits.pointer, allocation_size);
    } else {
        bits.integer = flags.bits.integer;
    }
}

flags::flags(flags &&flags) noexcept
: bits(flags.bits) {
    flags.bits.integer = 0;
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
        auto location = (flags_integer_t)((double)(index + 1) / (double)bit_size);
        auto pointer = (flags_integer_t *)((flags_integer_t)bits.pointer + (sizeof(flags_integer_t) * location));

        index -= bit_size * location;

        if (flag) {
            *pointer |= (flags_integer_t)1 << index;
        } else {
            *pointer &= ~((flags_integer_t)1 << index);
        }
    } else {
        if (flag) {
            bits.integer |= (flags_integer_t)1 << index;
        } else {
            bits.integer &= ~((flags_integer_t)1 << index);
        }
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
        auto location = (flags_integer_t)((double)(index + 1) / (double)bit_size);
        auto pointer = (flags_integer_t *)((flags_integer_t)bits.pointer + (sizeof(flags_integer_t) * location));

        index -= bit_size * location;
        flags = *pointer;
    }

    return flags & ((flags_integer_t)1 << index);
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

flags &flags::operator=(const flags &flags) noexcept {
    length = flags.length;

    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = (size_t)((double)length / (double)bit_size) + 1;
        auto allocation_size = sizeof(flags_integer_t) * size;

        bits.pointer = (flags_integer_t *)malloc(allocation_size);
        if (!bits.pointer) {
            fprintf(stderr, "Failed to allocate data of size (%ld), failing with error (%s)\n", size, strerror(errno));
            exit(1);
        }

        memcpy(bits.pointer, flags.bits.pointer, allocation_size);
    } else {
        bits.integer = flags.bits.integer;
    }

    return *this;
}

flags &flags::operator=(flags &&flags) noexcept {
    length = flags.length;
    bits.integer = flags.bits.integer;

    flags.length = 0;
    flags.bits.integer = 0;

    return *this;
}
