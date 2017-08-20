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

    void cast(flags_integer_t index, bool flag) noexcept;
    bool at(flags_integer_t index) const noexcept;

    bool was_created() const noexcept { return length_ != 0; }

    bool operator==(const flags &flags) const noexcept;
    inline bool operator!=(const flags &flags) const noexcept { return !(*this == flags); }

private:
    union {
        flags_integer_t flags = 0;
        flags_integer_t *ptr;
    } flags_;

    flags_integer_t length_ = 0;
    inline constexpr flags_integer_t bit_size() const noexcept { return sizeof(flags_integer_t) * 8; }
};
