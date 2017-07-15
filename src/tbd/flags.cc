//
//  flags.cc
//  tbd
//
//  Created by administrator on 7/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cstdio>
#include <cstdlib>

#include <cerrno>
#include <cstring>

#include "flags.h"

flags::flags(long length)
: length_(length) {
    if (length > bit_size()) {
        size_t size = length * bit_size();
        flags_.ptr = calloc(1, size);
        
        if (!flags_.ptr) {
            fprintf(stderr, "Failed to allocate data of size (%ld), failing with error (%s)\n", size, strerror(errno));
            exit(1);
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
        
        const auto bit_size = this->bit_size(); // 16
        const auto bits_length = bit_size * length_; // 32
        
        auto index_from_back = (bits_length - 1) - index; // 13
        if (index_from_back > bit_size) {
            do {
                index_from_back -= 8;
                ptr++;
            } while (index_from_back > bit_size);
        } else {
            index = bit_size - index;
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
        
        const auto bit_size = this->bit_size(); // 16
        const auto bits_length = bit_size * length_; // 32
        
        auto index_from_back = (bits_length - 1) - index; // 13
        if (index_from_back > bit_size) {
            do {
                index_from_back -= 8;
                ptr++;
            } while (index_from_back > bit_size);
        } else {
            index = bit_size - index;
        }
        
        return *ptr | 1 << index;
    } else {
        const auto flags = flags_.flags;
        return flags | 0x1;
    }
}

bool flags::operator==(const flags &flags) const noexcept {
    if (length_ != flags.length_) {
        return false;
    }
    
    if (length_ > bit_size()) {
        return memcmp(flags_.ptr, flags_.ptr, length_);
    } else {
        return flags_.flags == flags.flags_.flags;
    }
}
