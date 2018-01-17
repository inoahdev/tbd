//
//  src/utils/bits.h
//  tbd
//
//  Created by inoahdev on 7/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

#ifdef __LP64__
typedef uint64_t bits_integer_t;
#else
typedef uint32_t bits_integer_t;
#endif

namespace utils {
    struct bits {
        explicit bits() = default;
        explicit bits(bits_integer_t size) noexcept;
        
        explicit bits(const bits &) noexcept;
        explicit bits(bits &&) noexcept;

        ~bits();

        union {
            bits_integer_t integer = bits_integer_t();
            bits_integer_t *pointer;
        } data;
        
        bits_integer_t length = bits_integer_t();
        
        void create_stack_max() noexcept;
        void resize(bits_integer_t length) noexcept;
        
        void cast(bits_integer_t index, bool flag) noexcept;
        bool at(bits_integer_t index) const noexcept;

        bool was_created() const noexcept { return this->length != 0; }

        bool operator==(const bits &bits) const noexcept;
        inline bool operator!=(const bits &bits) const noexcept { return !(*this == bits); }

        bits &operator=(const bits &) = delete;
        bits &operator=(bits &&) noexcept;

    protected:
#ifdef __LP64__
        inline constexpr const bits_integer_t byte_shift() const noexcept { return 3; }
#else
        inline constexpr const bits_integer_t byte_shift() const noexcept { return 2; }
#endif
        
        inline constexpr const bits_integer_t byte_size() const noexcept { return bits_integer_t(1) << this->byte_shift(); }
        inline constexpr const bits_integer_t bit_size() const noexcept { return this->byte_size() << 3; }
        
        bits_integer_t allocation_size_from_length(bits_integer_t length);
    };

}
