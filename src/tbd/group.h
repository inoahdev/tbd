
//
//  src/tbd/group.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "symbol.h"

class group {
public:
    explicit group(const std::vector<const NXArchInfo *> &architecture_infos) noexcept;

    void add_symbol(const symbol &symbol) noexcept;
    void add_reexport(const symbol &reexport) noexcept;

    inline const std::vector<const NXArchInfo *> &architecture_infos() const noexcept { return architecture_infos_; }

    inline const std::vector<symbol> &symbols() const noexcept { return symbols_; }
    inline const std::vector<symbol> &reexports() const noexcept { return reexports_; }

    inline const bool operator==(const std::vector<const NXArchInfo *> &architecture_infos) const noexcept { return architecture_infos_ == architecture_infos; }

private:
    std::vector<const NXArchInfo *> architecture_infos_;

    std::vector<symbol> symbols_;
    std::vector<symbol> reexports_;
};
