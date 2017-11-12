//
//  src/misc/bits.cc
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
#include "bits.h"

bits::creation_result bits::create(bits_integer_t length) {
    this->length = length;

    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = (size_t)((double)length / (double)bit_size) + 1;
        auto allocation_size = sizeof(bits_integer_t) * size;

        data.pointer = (bits_integer_t *)calloc(1, allocation_size);

        if (!data.pointer) {
            return creation_result::failed_to_allocate_memory;
        }
    }

    return creation_result::ok;
}

bits::creation_result bits::create_stack_max() {
    this->length = bit_size();
    return creation_result::ok;
}

bits::creation_result bits::create_copy(const bits &bits) {
    length = bits.length;

    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = static_cast<size_t>((static_cast<double>(length) / static_cast<double>(bit_size)) + 1);
        auto allocation_size = sizeof(bits_integer_t) * size;

        data.pointer = static_cast<bits_integer_t *>(malloc(allocation_size));
        if (!data.pointer) {
            return creation_result::failed_to_allocate_memory;
        }

        memcpy(data.pointer, bits.data.pointer, allocation_size);
    } else {
        data.integer = bits.data.integer;
    }

    return creation_result::ok;
}

bits::bits(bits &&bits) noexcept
: data(bits.data), length(bits.length) {
    bits.data.integer = 0;
    bits.length = 0;
}

bits::~bits() {
    if (length > bit_size()) {
        free(data.pointer);
    }
}

void bits::cast(bits_integer_t index, bool flag) noexcept {
    const auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto location = static_cast<bits_integer_t>(static_cast<double>(index + 1) / static_cast<double>(bit_size));
        auto pointer = reinterpret_cast<bits_integer_t *>(reinterpret_cast<bits_integer_t>(data.pointer) + (sizeof(bits_integer_t) * location));

        index -= bit_size * location;

        if (flag) {
            *pointer |= static_cast<bits_integer_t>(1) << index;
        } else {
            *pointer &= ~(static_cast<bits_integer_t>(1) << index);
        }
    } else {
        if (flag) {
            data.integer |= static_cast<bits_integer_t>(1) << index;
        } else {
            data.integer &= ~(static_cast<bits_integer_t>(1) << index);
        }
    }
}

bool bits::at(bits_integer_t index) const noexcept {
    auto bits = data.integer;
    const auto bit_size = this->bit_size();

    if (length > bit_size) {
        auto location = static_cast<bits_integer_t>(static_cast<double>(index + 1) / static_cast<double>(bit_size));
        auto pointer = reinterpret_cast<bits_integer_t *>(reinterpret_cast<bits_integer_t>(data.pointer) + (sizeof(bits_integer_t) * location));

        index -= bit_size * location;
        bits = *pointer;
    }

    return bits & (static_cast<bits_integer_t>(1) << index);
}

bool bits::operator==(const bits &bits) const noexcept {
    if (length != bits.length) {
        return false;
    }

    auto bit_size = this->bit_size();
    if (length > bit_size) {
        auto size = static_cast<bits_integer_t>(static_cast<double>(length) / bit_size) + 1;
        return memcmp(data.pointer, bits.data.pointer, sizeof(bits_integer_t) * size) == 0;
    }

    return data.integer == bits.data.integer;
}

bits &bits::operator=(bits &&bits) noexcept {
    length = bits.length;
    data.integer = bits.data.integer;

    bits.length = 0;
    bits.data.integer = 0;

    return *this;
}
