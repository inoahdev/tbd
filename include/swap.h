//
//  include/swap.h
//  tbd
//
//  Created by inoahdev on 11/20/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef SWAP_H
#define SWAP_H

#include <stdint.h>

uint16_t swap_uint16(uint16_t num);
uint32_t swap_uint32(uint32_t num);
uint64_t swap_uint64(uint64_t num);

/*
 * For type-safety, we support separate functions to swap int16_t and
 * int32_t, although their impls are the same as their unsigned counterparts.
 */

int16_t swap_int16(int16_t num);
int32_t swap_int32(int32_t num);

#endif /* SWAP_H */
