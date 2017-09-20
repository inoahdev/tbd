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
    explicit flags() = default;

    explicit flags(const flags &) = delete;
    explicit flags(flags &&) noexcept;

    ~flags();

    union {
        flags_integer_t integer = 0;
        flags_integer_t *pointer;
    } bits;

    enum class creation_result {
        ok,
        failed_to_allocate_memory
    };

    creation_result create(flags_integer_t length);
    creation_result create_copy(const flags &flags);

    void cast(flags_integer_t index, bool flag) noexcept;
    bool at(flags_integer_t index) const noexcept;

    bool was_created() const noexcept { return length != 0; }

    bool operator==(const flags &flags) const noexcept;
    inline bool operator!=(const flags &flags) const noexcept { return !(*this == flags); }

    flags &operator=(const flags &) = delete;
    flags &operator=(flags &&) noexcept;

private:
    flags_integer_t length = 0;
    inline constexpr const flags_integer_t bit_size() const noexcept { return sizeof(flags_integer_t) * 8; }
};
