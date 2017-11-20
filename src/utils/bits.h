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
    class bits {
    public:
        explicit bits() = default;

        explicit bits(const bits &) = delete;
        explicit bits(bits &&) noexcept;

        ~bits();

        union {
            bits_integer_t integer = 0;
            bits_integer_t *pointer;
        } data;

        enum class creation_result {
            ok,
            failed_to_allocate_memory
        };

        creation_result create(bits_integer_t length);
        creation_result create_stack_max();

        creation_result create_copy(const bits &bits);

        void cast(bits_integer_t index, bool flag) noexcept;
        bool at(bits_integer_t index) const noexcept;

        bool was_created() const noexcept { return length != 0; }

        bool operator==(const bits &bits) const noexcept;
        inline bool operator!=(const bits &bits) const noexcept { return !(*this == bits); }

        bits &operator=(const bits &) = delete;
        bits &operator=(bits &&) noexcept;

    private:
        bits_integer_t length = 0;
        inline constexpr const bits_integer_t bit_size() const noexcept { return sizeof(bits_integer_t) * 8; }
    };

}