//
//  src/utils/bits.cc
//  tbd
//
//  Created by inoahdev on 7/10/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include <cerrno>

#include <cstdint>
#include <cstdio>

#include <cstdlib>
#include <cstring>
#include <utility>

#include "bits.h"

namespace utils {
    bits::bits(bits_integer_t length) noexcept :
    length(length) {
        const auto bit_size = this->bit_size();
        if (this->length > bit_size) {
            const auto size = this->allocation_size_from_length(this->length);
            data.pointer = new bits_integer_t[size];
        }
    }

    void bits::create_stack_max() noexcept {
        this->length = bits_integer_t(1) << (this->byte_shift() + 3);
    }

    void bits::resize(bits_integer_t length) noexcept {
        const auto bit_size = this->bit_size();
        const auto old_allocation_size = this->allocation_size_from_length(this->length);

        if (this->length > bit_size) {
            const auto new_allocation_size = this->allocation_size_from_length(length);

            if (new_allocation_size != old_allocation_size) {
                auto pointer = new bits_integer_t[new_allocation_size];
                if (!old_allocation_size) {
                    memcpy(pointer, &data.integer, this->byte_size());
                } else {
                    memcpy(pointer, data.pointer, old_allocation_size < new_allocation_size ? old_allocation_size : new_allocation_size);
                    delete[] data.pointer;
                }
                
                data.pointer = pointer;
            }
        } else {
            auto integer = data.integer;
            if (old_allocation_size != 0) {
                integer = *data.pointer;
                delete[] data.pointer;
            }

            data.integer = integer;
        }

        this->length = length;
    }

    bits::bits(const bits &bits) noexcept
    : length(bits.length) {
        const auto bit_size = this->bit_size();
        if (bits.length > bit_size) {
            const auto bit_shift = (this->byte_shift() + 3);

            auto size = this->length >> bit_shift;
            auto remainder_bits = (bits_integer_t(~0) >> (bit_size - bit_shift));

            if (this->length & remainder_bits) {
                size++;
            }

            data.pointer = new bits_integer_t[size];
            memcpy(data.pointer, bits.data.pointer, sizeof(bits_integer_t) * size);
        } else {
            data.integer = bits.data.integer;
        }
    }

    bits::bits(bits &&bits) noexcept
    : data(bits.data), length(bits.length) {
        bits.data.integer = bits_integer_t();
        bits.length = bits_integer_t();
    }

    bits::~bits() {
        if (this->length > this->bit_size()) {
            delete[] data.pointer;
        }
    }

    void bits::cast(bits_integer_t index, bool flag) noexcept {
        auto data = &this->data.integer;
        const auto bit_size = this->bit_size();

        if (this->length > bit_size) {
            const auto location = (index + 1) >> (this->byte_shift() + 3);

            data = this->data.pointer + location;
            index -= bit_size * location;
        }

        if (flag) {
            *data |= static_cast<bits_integer_t>(1) << index;
        } else {
            *data &= ~(static_cast<bits_integer_t>(1) << index);
        }
    }

    bool bits::at(bits_integer_t index) const noexcept {
        const auto bit_size = this->bit_size();
        if (this->length > bit_size) {
            const auto location = (index + 1) >> (this->byte_shift() + 3);
            const auto pointer = this->data.pointer + location;

            index -= bit_size * location;

            return *pointer & index;
        }

        return this->data.integer & (bits_integer_t(1) << index);
    }

    bits_integer_t bits::allocation_size_from_length(bits_integer_t length) {
        const auto bit_shift = (this->byte_shift() + 3);

        auto size = this->length >> bit_shift;
        auto remainder_bits = (bits_integer_t(~0) >> (this->bit_size() - bit_shift));

        if (this->length & remainder_bits) {
            size++;
        }

        return size;
    }

    bool bits::operator==(const bits &bits) const noexcept {
        if (this->length != bits.length) {
            return false;
        }

        auto bit_size = this->bit_size();
        if (this->length > bit_size) {
            const auto bit_shift = (this->byte_shift() + 3);

            auto size = this->length >> bit_shift;
            auto remainder_bits = (bits_integer_t(~0) >> (bit_size - bit_shift));

            if (this->length & remainder_bits) {
                size++;
            }

            return memcmp(data.pointer, bits.data.pointer, sizeof(bits_integer_t) * size) == 0;
        }

        return this->data.integer == bits.data.integer;
    }

    bits &bits::operator=(bits &&bits) noexcept {
        this->length = bits.length;
        this->data.integer = bits.data.integer;

        bits.length = bits_integer_t();
        bits.data.integer = bits_integer_t();

        return *this;
    }
}
