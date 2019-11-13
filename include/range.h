//
//  include/range.h
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef RANGE_H
#define RANGE_H

#include <stdbool.h>
#include <stdint.h>

struct range {
    uint64_t begin;
    uint64_t end;
};

uint64_t range_get_size(struct range range);

bool range_contains_location(struct range range, uint64_t location);
bool range_contains_end(struct range range, uint64_t end);

bool range_contains_other(struct range left, struct range right);
bool ranges_overlap(struct range left, struct range right);

#endif /* RANGE_H */
