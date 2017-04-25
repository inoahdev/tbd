//
//  src/tbd/symbol.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/arch.h>

#include <string>
#include <vector>

class symbol {
public:
    explicit symbol(const std::string &string, bool weak) noexcept;
    void add_architecture_info(const NXArchInfo *architecture_info) noexcept;

    inline const bool weak() const noexcept { return weak_; }

    inline const std::string &string() const noexcept { return string_; }
    inline const std::vector<const NXArchInfo *> architecture_infos() const noexcept { return architecture_infos_; }

    inline const bool operator==(const std::string &string) const noexcept { return string_ == string; }
    inline const bool operator==(const symbol &symbol) const noexcept { return string_ == symbol.string_; }

private:
    std::string string_;
    std::vector<const NXArchInfo *> architecture_infos_;

    bool weak_;
};
