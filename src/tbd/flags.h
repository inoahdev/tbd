//
//  flags.h
//  tbd
//
//  Created by administrator on 7/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

class flags {
public:
    explicit flags() = default;
    explicit flags(long length);

    ~flags();

    void cast(long index, bool result) noexcept;
    bool at_index(long index) const noexcept;

    bool was_created() const noexcept { return length_ != 0; }

    bool operator==(const flags &flags) const noexcept;
    inline bool operator!=(const flags &flags) const noexcept { return !(*this == flags); }

private:
    union {
        unsigned int flags = 0;
        void *ptr;
    } flags_;

    long length_ = 0;
    inline constexpr unsigned long bit_size() const noexcept { return sizeof(unsigned int) * 8; }
};
