//
//  include/bit_list.h
//  tbd
//
//  Created by inoahdev on 11/18/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#ifndef BIT_LIST_H
#define BIT_LIST_H

#include <stdbool.h>
#include <stdint.h>

#include "notnull.h"

struct bit_list {
    uint64_t data;

    uint64_t set_count;
    uint64_t alloc_count;
};

enum bit_list_result {
    E_BIT_LIST_OK,
    E_BIT_LIST_ALLOC_FAIL
};

enum bit_list_result
bit_list_create_with_capacity(struct bit_list *__notnull list,
                              uint64_t capacity);

uint64_t bit_list_find_first_bit(struct bit_list list);
uint64_t bit_list_find_bit_after_last(struct bit_list list, uint64_t last);

bool bit_list_is_on_heap(struct bit_list list);
bool
bit_list_equal_counts_is_equal(struct bit_list left, struct bit_list right);

uint64_t bit_list_get_for_index(struct bit_list list, uint64_t index);

uint64_t bit_list_get_for_index_on_stack(struct bit_list list, uint64_t index);
uint64_t bit_list_get_for_index_on_heap(struct bit_list list, uint64_t index);

void bit_list_set_bit(struct bit_list *__notnull list, uint64_t index);
void bit_list_set_first_n(struct bit_list *__notnull list, uint64_t n);

int bit_list_equal_counts_compare(struct bit_list left, struct bit_list right);

#endif /* BIT_LIST_H */
