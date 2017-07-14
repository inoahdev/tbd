//
//  flags.h
//  tbd
//
//  Created by administrator on 7/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <stdio.h>

class flags {
public:
    explicit flags(long length);
    ~flags();
    
    void cast(long index, bool result) noexcept;
    bool at_index(long index) const noexcept;
    
    bool operator==(const flags &flags) const noexcept;
    inline bool operator!=(const flags &flags) const noexcept { return !(*this == flags); }
    
private:
    union {
        unsigned int flags;
        void *ptr;
    } flags_;
    
    long length_;
    inline constexpr unsigned long bit_size() const noexcept { return sizeof(unsigned int) * 8; }
};
