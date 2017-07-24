//
//  src/tbd/symbol.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <string>
#include "flags.h"

class symbol {
public:
    explicit symbol(const char *string, bool weak, int flags_length) noexcept;
    void add_architecture(int number) noexcept;

    inline const bool weak() const noexcept { return weak_; }
    inline const char *string() const noexcept { return string_; }

    inline const flags &flags() const noexcept { return flags_; }

    inline const bool operator==(const char *string) const noexcept { return strcmp(string_, string) == 0; }
    inline const bool operator==(const symbol &symbol) const noexcept { return strcmp(string_, symbol.string_) == 0; }

    inline const bool operator!=(const char *string) const noexcept { return strcmp(string_, string) != 0; }
    inline const bool operator!=(const symbol &symbol) const noexcept { return strcmp(string_, symbol.string_) != 0; }

private:
    const char *string_;
    bool weak_;

    class flags flags_;
};
