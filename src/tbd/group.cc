
//
//  src/tbd/group.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "group.h"

group::group(const std::vector<const NXArchInfo *> &architecture_infos) noexcept
: architecture_infos_(architecture_infos) {}

void group::add_symbol(const symbol &symbol) noexcept {
    auto &symbols = this->symbols_;
    auto symbols_iter = std::find(symbols.begin(), symbols.end(), symbol) ;

    if (symbols_iter == symbols.end()) {
        symbols.emplace_back(symbol);
    }
}

void group::add_reexport(const symbol &reexport) noexcept {
    auto &reexports = this->reexports_;
    auto reexports_iter = std::find(reexports.begin(), reexports.end(), reexport);

    if (reexports_iter == reexports.end()) {
        reexports.emplace_back(reexport);
    }
}
