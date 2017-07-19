
//
//  src/tbd/group.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <vector>
#include "flags.h"

class group {
public:
    explicit group() noexcept = default;
    explicit group(const flags &flags) noexcept;

    inline void increment_symbol_count() noexcept { symbols_count_++; }
    inline void increment_reexport_count() noexcept { reexports_count_++; }

    inline const flags &flags() const noexcept { return flags_; }

    inline unsigned int symbols_count() const noexcept { return symbols_count_; }
    inline unsigned int reexports_count() const noexcept { return reexports_count_; }

    inline const bool operator==(const class flags &flags) const noexcept { return flags_ == flags; }
    inline const bool operator==(const group &group) const noexcept { return flags_ == group.flags_; }

    inline const bool operator!=(const class flags &flags) const noexcept { return flags_ != flags; }
    inline const bool operator!=(const group &group) const noexcept { return flags_ != group.flags_; }

private:
    class flags flags_;

    unsigned int symbols_count_ = 0;
    unsigned int reexports_count_ = 0;
};
