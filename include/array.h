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

#include "notnull.h"

/*
 * array is a structure to hold items of the same-size.
 *
 * The item-size is not stored within the data or in the structure, so it's the
 * responsibility of the caller to provide item-size to any APIs.
 */

struct array {
    void *data;

    const void *data_end;
    const void *alloc_end;

    /*
     * To improve performance while using the sort APIs, store the item-count of
     * an array to avoid re-calculating.
     */

    uint64_t item_count;
};

enum array_result {
    E_ARRAY_OK,
    E_ARRAY_ALLOC_FAIL,
    E_ARRAY_NO_MATCHING_ITEM,
    E_ARRAY_INDEX_OUT_OF_BOUNDS,
    E_ARRAY_TOO_MANY_ITEMS
};

enum array_result
array_ensure_item_capacity(struct array *__notnull array,
                           size_t item_size,
                           uint64_t item_count);

uint64_t array_get_used_size(const struct array *__notnull array);

void *array_get_front(const struct array *__notnull array);
void *array_get_back(const struct array *__notnull array, size_t item_size);

void *
array_get_item_at_index(const struct array *__notnull array,
                        size_t item_size,
                        uint64_t index);

__notnull void *
array_get_item_at_index_unsafe(const struct array *__notnull array,
                               size_t item_size,
                               uint64_t index);

enum array_result
array_add_item(struct array *__notnull array,
               size_t item_size,
               const void *__notnull item,
               void **item_out);

typedef int
(*array_item_comparator)(const void *__notnull array_item,
                         const void *item);

void *
array_find_item(const struct array *__notnull array,
                size_t item_size,
                const void *__notnull item,
                __notnull array_item_comparator comparator,
                uint64_t *index_out);

enum array_result
array_add_and_unique_items_from_array(
    struct array *__notnull array,
    size_t item_size,
    const struct array *__notnull other,
    __notnull array_item_comparator comparator);

enum array_cached_index_type {
    ARRAY_CACHED_INDEX_LESS_THAN    = -1,
    ARRAY_CACHED_INDEX_EQUAL        = 0,
    ARRAY_CACHED_INDEX_GREATER_THAN = 1
};

/*
 * array_cached_index_info is a structure describing the information extracted
 * from the lookup performaced array_find_item_in_sorted.
 *
 * Recording our lookup information helps us avoid the performance penalty of
 * repeating our lookup when adding to the array.
 */

struct array_cached_index_info {
    /*
     * type stores the relationship between the array-item and the item passed
     * in by the caller.
     *
     * For example, a type of ARRAY_CACHED_INDEX_LESS_THAN means that the
     * array-item is "less than" the item passed in by the caller.
     */

    uint64_t index;
    enum array_cached_index_type type;
};

void *
array_find_item_in_sorted(const struct array *__notnull array,
                          size_t item_size,
                          const void *__notnull item,
                          __notnull array_item_comparator comparator,
                          struct array_cached_index_info *index_out);

struct array_slice {
    uint64_t front;
    uint64_t back;
};

void *
array_find_item_in_sorted_with_slice(const struct array *__notnull array,
                                     size_t item_size,
                                     struct array_slice slice,
                                     const void *__notnull item,
                                     __notnull array_item_comparator comparator,
                                     struct array_cached_index_info *info_out);

enum array_result
array_add_item_with_cached_index_info(struct array *array,
                                      size_t item_size,
                                      const void *item,
                                      struct array_cached_index_info *info,
                                      void **item_out);

enum array_result
array_copy(struct array *__notnull array, struct array *__notnull array_out);

typedef int
(*array_sort_comparator)(const void *__notnull left,
                         const void *__notnull right);

void
array_sort_with_comparator(struct array *__notnull array,
                           size_t item_size,
                           __notnull array_sort_comparator comparator);

void
array_trim_to_item_count(struct array *__notnull array,
                         size_t item_size,
                         const uint64_t item_count);

void array_clear(struct array *__notnull array);
enum array_result array_destroy(struct array *__notnull array);

#endif /* ARRAY_H */
