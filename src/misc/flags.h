//
//  src/misc/flags.h
//  tbd
//
//  Created by inoahdev on 7/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

#ifdef __LP64__
typedef uint64_t flags_integer_t;
#else
typedef uint32_t flags_integer_t;
#endif

class flags {
public:
    explicit flags(flags_integer_t length);
    ~flags();

    union {
        flags_integer_t integer = 0;
        flags_integer_t *pointer;
    } bits;

    void cast(flags_integer_t index, bool flag) noexcept;
    bool at(flags_integer_t index) const noexcept;

    bool was_created() const noexcept { return length != 0; }

    bool operator==(const flags &flags) const noexcept;
    inline bool operator!=(const flags &flags) const noexcept { return !(*this == flags); }

private:
    flags_integer_t length = 0;
    inline constexpr const flags_integer_t bit_size() const noexcept { return sizeof(flags_integer_t) * 8; }
};
