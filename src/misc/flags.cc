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

flags::creation_result flags::create(flags_integer_t length) {
    this->length = length;

    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = (size_t)((double)length / (double)bit_size) + 1;
        auto allocation_size = sizeof(flags_integer_t) * size;

        bits.pointer = (flags_integer_t *)calloc(1, allocation_size);

        if (!bits.pointer) {
            return creation_result::failed_to_allocate_memory;
        }
    }

    return creation_result::ok;
}

flags::creation_result flags::create_copy(const flags &flags) {
    length = flags.length;

    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = (size_t)((double)length / (double)bit_size) + 1;
        auto allocation_size = sizeof(flags_integer_t) * size;

        bits.pointer = (flags_integer_t *)malloc(allocation_size);
        if (!bits.pointer) {
            return creation_result::failed_to_allocate_memory;
        }

        memcpy(bits.pointer, flags.bits.pointer, allocation_size);
    } else {
        bits.integer = flags.bits.integer;
    }

    return creation_result::ok;
}

flags::flags(flags &&flags) noexcept
: bits(flags.bits), length(flags.length) {
    flags.bits.integer = 0;
    flags.length = 0;
}

flags::~flags() {
    if (length > bit_size()) {
        free(bits.pointer);
    }
}

void flags::cast(flags_integer_t index, bool flag) noexcept {
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

flags &flags::operator=(flags &&flags) noexcept {
    length = flags.length;
    bits.integer = flags.bits.integer;

    flags.length = 0;
    flags.bits.integer = 0;

    return *this;
}
