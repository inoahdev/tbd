//
//  include/array.h
//  tbd 
//
//  Created by inoahdev on 8/18/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef ARRAY_H
#define ARRAY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * array is a structure to hold items of the same-size.
 *
 * The item-size is not stored within the data or in the structure, so it's the
 * responsibility of the caller to provide item-size to any APIs.
 */

struct array {
    void *data;
    void *data_end;
    void *alloc_end;
};

enum array_result {
    E_ARRAY_OK,
    E_ARRAY_ALLOC_FAIL,
    E_ARRAY_NO_MATCHING_ITEM,
    E_ARRAY_INDEX_OUT_OF_BOUNDS,
    E_ARRAY_TOO_MANY_ITEMS
};

bool array_is_empty(const struct array *array);

uint64_t array_get_item_count(const struct array *array, size_t item_size);
uint64_t array_get_used_size(const struct array *array);

void *array_get_front(const struct array *array);
void *array_get_back(const struct array *array, size_t item_size);

void *
array_get_item_at_index(const struct array *array,
                        size_t item_size,
                        uint64_t index);

enum array_result
array_add_item(struct array *array,
               size_t item_size,
               const void *item,
               void **item_out);

typedef int (*array_item_comparator)(const void *array_item, const void *item);

void *
array_find_item(const struct array *array,
                size_t item_size,
                const void *item,
                array_item_comparator comparator,
                uint64_t *index_out);

enum array_result
array_add_and_unique_items_from_array(struct array *array,
                                      size_t item_size,
                                      const struct array *other,
                                      array_item_comparator comparator);

enum array_cached_index_type {
    ARRAY_CACHED_INDEX_LESS_THAN    = -1,
    ARRAY_CACHED_INDEX_EQUAL        = 0,
    ARRAY_CACHED_INDEX_GREATER_THAN = 1
};

/*
 * array_cached_index_info is a structure describing the information resulted
 * from the lookup in array_find_item_in_sorted. This is useful to avoid extra
 * lookups if adding the same item to the same array in 
 */

struct array_cached_index_info {
    uint64_t index;
    enum array_cached_index_type type;
};

void *
array_find_item_in_sorted(const struct array *array,
                          size_t item_size,
                          const void *item,
                          array_item_comparator comparator,
                          struct array_cached_index_info *index_out);

enum array_result
array_add_item_with_cached_index_info(struct array *array,
                                      size_t item_size,
                                      const void *item,
                                      struct array_cached_index_info *info,
                                      void **item_out);

enum array_result
array_sort_items_with_comparator(struct array *array,
                                 size_t item_size,
                                 array_item_comparator comparator);

enum array_result array_copy(struct array *array, struct array *array_out);

/*
 * Deallocate the array's buffer and reset the array's fields.
 */

enum array_result array_destroy(struct array *array);

#endif /* ARRAY_H */
