//
//  src/range.c
//  tbd
//
//  Created by inoahdev on 12/29/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include "range.h"

bool
range_contains_location(const struct range range, const uint64_t location) {
    return (location >= range.begin && location < range.end);
}

bool range_contains_end(const struct range range, const uint64_t end) {
    return (end > range.begin && end <= range.end);
}

bool range_contains_other(const struct range left, const struct range right) {
    if (!range_contains_location(left, right.begin)) {
        return false;
    }

    if (!range_contains_end(left, right.end)) {
        return false;
    }

    return true;
}

bool ranges_overlap(struct range left, struct range right) {
    /*
     * right's begin is inside left's range.
     */

    if (range_contains_location(left, right.begin)) {
        return true;
    }

    /*
     * right's end is inside left's range.
     */

    if (range_contains_end(left, right.end)) {
        return true;
    }

    /*
     * right completely contains left.
     */

    return (right.begin < left.begin && right.end > left.end);
}
