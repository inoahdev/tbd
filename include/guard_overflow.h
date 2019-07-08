//
//  include/guard_overflow.h
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef GUARD_OVERFLOW_H
#define GUARD_OVERFLOW_H

#include <stdint.h>

#ifndef guard_overflow_add
#define guard_overflow_add(left_in, right) \
    __builtin_add_overflow(*left_in, right, left_in)
#endif

#ifndef guard_overflow_mul
#define guard_overflow_mul(left_in, right) \
    __builtin_mul_overflow(*left_in, right, left_in)
#endif

#endif /* GUARD_OVERFLOW_H */
