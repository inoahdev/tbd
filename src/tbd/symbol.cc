
//
//  src/tbd/symbol.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "symbol.h"

symbol::symbol(const std::string &string, bool weak) noexcept
: string_(string), weak_(weak) {}

void symbol::add_architecture_info(const NXArchInfo *architecture_info) noexcept {
    architecture_infos_.emplace_back(architecture_info);
}
