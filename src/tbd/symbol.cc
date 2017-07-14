
//
//  src/tbd/symbol.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "symbol.h"

symbol::symbol(const char *string, bool weak, int flags_length) noexcept
: string_(string), weak_(weak), flags_(flags_length) {}

void symbol::add_architecture(int number) noexcept {
    flags_.cast(number, true);
}
