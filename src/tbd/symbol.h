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
    enum class type {
        reexports,
        symbols,
        weak_symbols,
        objc_classes,
        objc_ivars
    };
    
    explicit symbol(const char *string, bool weak, int flags_length, enum type type) noexcept;
    void add_architecture(int number) noexcept;

    inline const bool weak() const noexcept { return weak_; }
    inline const char *string() const noexcept { return string_; }

    inline const flags &flags() const noexcept { return flags_; }
    inline const enum type type() const noexcept { return type_; };
    
    inline const bool operator==(const char *string) const noexcept { return strcmp(string_, string) == 0; }
    inline const bool operator==(const symbol &symbol) const noexcept { return strcmp(string_, symbol.string_) == 0; }

    inline const bool operator!=(const char *string) const noexcept { return strcmp(string_, string) != 0; }
    inline const bool operator!=(const symbol &symbol) const noexcept { return strcmp(string_, symbol.string_) != 0; }

private:
    const char *string_;
    bool weak_;

    class flags flags_;
    enum type type_;
};

inline bool operator==(const enum symbol::type &lhs, const enum symbol::type &rhs) { return (int)lhs == (int)rhs; }

