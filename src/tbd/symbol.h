//
//  src/tbd/symbol.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <mach-o/arch.h>

#include <string>
#include <vector>

class symbol {
public:
    explicit symbol(const char *string, bool weak) noexcept;

    void add_architecture_info(const NXArchInfo *architecture_info) noexcept;

    inline const bool weak() const noexcept { return weak_; }

    inline const char *string() const noexcept { return string_; }
    inline const std::vector<const NXArchInfo *> architecture_infos() const noexcept { return architecture_infos_; }

    inline const bool operator==(const char *string) const noexcept { return strcmp(string_, string) == 0; }
    inline const bool operator==(const symbol &symbol) const noexcept { return strcmp(string_, symbol.string_) == 0; }

private:
    const char *string_;
    std::vector<const NXArchInfo *> architecture_infos_;

    bool weak_;
};
