//
//  src/array.c
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "likely.h"

void *
array_get_item_at_index(const struct array *const array,
                        const size_t item_size,
                        const uint64_t index)
{
    const uint64_t byte_index = item_size * index;
    void *const iter = array->data + byte_index;

    if (unlikely(iter > array->data_end)) {
        return NULL;
    }

    return iter;
}

void *__notnull
array_get_item_at_index_unsafe(const struct array *const array,
                               const size_t item_size,
                               const uint64_t index)
{
    const uint64_t byte_index = item_size * index;
    void *const iter = array->data + byte_index;

    return iter;
}

void *array_get_front(const struct array *const array) {
    return array->data;
}

void *array_get_back(const struct array *const array, const size_t item_size) {
    const void *const data_end = array->data_end;
    if (unlikely(array->data == data_end)) {
        return NULL;
    }

    return (void *)(data_end - item_size);
}

static inline uint64_t get_new_capacity(const uint64_t old_capacity) {
    return (old_capacity * 2);
}

static enum array_result
array_grow_to_capacity(struct array *__notnull const array,
                       const uint64_t used_size,
                       const uint64_t current_capacity,
                       const uint64_t wanted_capacity)
{
    uint64_t new_capacity = current_capacity;
    if (likely(new_capacity != 0)) {
        do {
            new_capacity = get_new_capacity(new_capacity);
        } while (new_capacity < wanted_capacity);
    } else {
        new_capacity = wanted_capacity;
    }

    void *const new_data = malloc(new_capacity);
    if (unlikely(new_data == NULL)) {
        return E_ARRAY_ALLOC_FAIL;
    }

    void *const old_data = array->data;

    memcpy(new_data, old_data, used_size);
    free(old_data);

    array->data = new_data;
    array->data_end = new_data + used_size;
    array->alloc_end = new_data + new_capacity;

    return E_ARRAY_OK;
}

static enum array_result
array_expand_if_necessary(struct array *__notnull const array,
                          const uint64_t add_byte_size)
{
    void *const old_data = array->data;

    const uint64_t used_size = array_get_used_size(array);
    const uint64_t old_capacity = (uint64_t)(array->alloc_end - old_data);
    const uint64_t wanted_capacity = used_size + add_byte_size;

    if (wanted_capacity <= old_capacity) {
        return E_ARRAY_OK;
    }

    const enum array_result grow_result =
        array_grow_to_capacity(array, used_size, old_capacity, wanted_capacity);

    if (unlikely(grow_result != E_ARRAY_OK)) {
        return grow_result;
    }

    return E_ARRAY_OK;
}

enum array_result
array_ensure_item_capacity(struct array *__notnull const array,
                           const size_t item_size,
                           const uint64_t item_count)
{
    const uint64_t old_capacity = (uint64_t)(array->alloc_end - array->data);
    const uint64_t wanted_capacity = item_count * item_size;

    if (old_capacity > wanted_capacity) {
        return E_ARRAY_OK;
    }

    const uint64_t used_size = array_get_used_size(array);
    const enum array_result grow_result =
        array_grow_to_capacity(array, used_size, old_capacity, wanted_capacity);

    if (unlikely(grow_result != E_ARRAY_OK)) {
        return grow_result;
    }

    return E_ARRAY_OK;
}

enum array_result
array_add_item_to_byte_index(struct array *__notnull const array,
                             const size_t item_size,
                             const void *__notnull const item,
                             const uint64_t byte_index,
                             void **const item_out)
{
    const enum array_result expand_result =
        array_expand_if_necessary(array, item_size);

    if (unlikely(expand_result != E_ARRAY_OK)) {
        return expand_result;
    }

    void *const position = array->data + byte_index;
    const void *const data_end = array->data_end;

    if (position != data_end) {
        void *const next_position = position + item_size;
        const uint64_t move_size = (uint64_t)(data_end - position);

        memmove(next_position, position, move_size);
    }

    memcpy(position, item, item_size);

    array->data_end = data_end + item_size;
    array->item_count += 1;

    if (item_out != NULL) {
        *item_out = position;
    }

    return E_ARRAY_OK;
}

uint64_t array_get_used_size(const struct array *__notnull const array) {
    const uint64_t used_size = (uint64_t)(array->data_end - array->data);
    return used_size;
}

enum array_result
array_add_item(struct array *__notnull const array,
               const size_t item_size,
               const void *__notnull const item,
               void **const item_out)
{
    const enum array_result expand_result =
        array_expand_if_necessary(array, item_size);

    if (unlikely(expand_result != E_ARRAY_OK)) {
        return expand_result;
    }

    void *const position = (void *)array->data_end;
    memcpy(position, item, item_size);

    array->data_end = position + item_size;
    array->item_count += 1;

    if (item_out != NULL) {
        *item_out = position;
    }

    return E_ARRAY_OK;
}

enum array_result
array_add_items_from_array(struct array *__notnull const array,
                           const struct array *__notnull const src)
{
    const uint64_t used_size = array_get_used_size(src);
    const enum array_result expand_result =
        array_expand_if_necessary(array, used_size);

    if (unlikely(expand_result != E_ARRAY_OK)) {
        return expand_result;
    }

    void *const data_end = (void *)array->data_end;
    memcpy(data_end, src->data, used_size);

    array->data_end = data_end + used_size;
    array->item_count += src->item_count;

    return E_ARRAY_OK;
}

enum array_result
array_add_and_unique_items_from_array(
    struct array *__notnull const array,
    const size_t item_size,
    const struct array *__notnull const src,
    __notnull const array_item_comparator comparator)
{
    if (array->item_count == 0) {
        return array_add_items_from_array(array, src);
    }

    const void *iter = src->data;
    const void *const end = src->data_end;

    for (; iter != end; iter += item_size) {
        const void *const match =
            array_find_item(array, item_size, iter, comparator, NULL);

        if (match != NULL) {
            continue;
        }

        const enum array_result add_item_result =
            array_add_item(array, item_size, iter, NULL);

        if (add_item_result != E_ARRAY_OK) {
            return add_item_result;
        }
    }

    return E_ARRAY_OK;
}

void *
array_find_item(const struct array *__notnull const array,
                const size_t item_size,
                const void *__notnull const item,
                __notnull const array_item_comparator comparator,
                uint64_t *const index_out)
{
    void *iter = array->data;
    const uint64_t item_count = array->item_count;

    for (uint64_t index = 0; index != item_count; index++, iter += item_size) {
        if (comparator(iter, item) != 0) {
            continue;
        }

        if (index_out != NULL) {
            *index_out = index;
        }

        return iter;
    }

    return NULL;
}

/*
 * Use wrapper-functions here to make the code in
 * array_slice_get_sorted_array_item_for_item() easier to read.
 */

static uint64_t array_slice_get_middle_index(const struct array_slice slice) {
    const uint64_t length = slice.back - slice.front;
    const uint64_t middle_index = length >> 1;

    return (slice.front + middle_index);
}

static void
array_slice_set_to_lower_half(struct array_slice *__notnull const slice,
                              const uint64_t middle)
{
    slice->back = middle - 1;
}

static void
array_slice_set_to_upper_half(struct array_slice *__notnull const slice,
                              const uint64_t middle)
{
    slice->front = middle + 1;
}

static bool array_slice_holds_one_element(const struct array_slice slice) {
    return (slice.front == slice.back);
}

static bool array_slice_holds_two_elements(const struct array_slice slice) {
    const uint64_t length = slice.back - slice.front;
    return (length == 1);
}

static inline
enum array_cached_index_type get_cached_index_type_from_ret(const int ret) {
    if (ret < 0) {
        return ARRAY_CACHED_INDEX_GREATER_THAN;
    } else if (ret > 0) {
        return ARRAY_CACHED_INDEX_LESS_THAN;
    }

    return ARRAY_CACHED_INDEX_EQUAL;
}

static inline bool array_item_is_greater(const int compare_ret) {
    /*
     * compare is greater than 0 when array_item is "greater than"
     * item, in which case we need to set slice to its lower half.
     *
     * Otherwise if it's less than 0, when array_item is "less than" item,
     * so we need to set slice to its upper half.
     */

    return (compare_ret > 0);
}

static void *
array_slice_get_sorted_array_item_for_item(
    const struct array *__notnull const array,
    const size_t item_size,
    struct array_slice slice,
    const void *__notnull const item,
    __notnull const array_item_comparator comparator,
    struct array_cached_index_info *const info_out)
{
    void *const data = array->data;

    do {
        const uint64_t index = array_slice_get_middle_index(slice);

        void *const array_item = &data[index * item_size];
        const int compare = comparator(array_item, item);

        if (compare == 0) {
            if (info_out != NULL) {
                info_out->index = index;
                info_out->type = ARRAY_CACHED_INDEX_EQUAL;
            }

            return array_item;
        }

        if (array_slice_holds_one_element(slice)) {
            if (info_out != NULL) {
                info_out->index = slice.front;
                info_out->type = get_cached_index_type_from_ret(compare);
            }

            return NULL;
        }

        if (array_item_is_greater(compare)) {
            /*
             * Since middle-index for a two-element slice is always
             * slice->front, we cannot go anywhere if slice->front is already
             * "greater than" the item provided.
             */

            if (array_slice_holds_two_elements(slice)) {
                if (info_out != NULL) {
                    info_out->index = slice.front;
                    info_out->type = ARRAY_CACHED_INDEX_LESS_THAN;
                }

                return NULL;
            }

            array_slice_set_to_lower_half(&slice, index);
        } else {
            array_slice_set_to_upper_half(&slice, index);
        }
    } while (true);
}

void *
array_find_item_in_sorted(const struct array *__notnull const array,
                          const size_t item_size,
                          const void *__notnull const item,
                          __notnull const array_item_comparator comparator,
                          struct array_cached_index_info *const info_out)
{
    const uint64_t item_count = array->item_count;
    if (item_count == 0) {
        if (info_out != NULL) {
            info_out->index = 0;
            info_out->type = ARRAY_CACHED_INDEX_EQUAL;
        }

        return NULL;
    }

    const struct array_slice slice = {
        .front = 0,
        .back = item_count - 1
    };

    void *const array_item =
        array_slice_get_sorted_array_item_for_item(array,
                                                   item_size,
                                                   slice,
                                                   item,
                                                   comparator,
                                                   info_out);

    return array_item;
}

void *
array_find_item_in_sorted_with_slice(
    const struct array *__notnull const array,
    const size_t item_size,
    const struct array_slice slice,
    const void *__notnull const item,
    __notnull const array_item_comparator comparator,
    struct array_cached_index_info *const info_out)
{
    void *const array_item =
        array_slice_get_sorted_array_item_for_item(array,
                                                   item_size,
                                                   slice,
                                                   item,
                                                   comparator,
                                                   info_out);

    return array_item;
}

static enum array_result
array_add_item_to_index(struct array *__notnull const array,
                        const size_t item_size,
                        const void *__notnull const item,
                        const uint64_t index,
                        void **const item_out)
{
    const enum array_result expand_result =
        array_expand_if_necessary(array, item_size);

    if (unlikely(expand_result != E_ARRAY_OK)) {
        return expand_result;
    }

    const uint64_t byte_index = item_size * index;
    void *const position = array->data + byte_index;

    /*
     * Note: We do allow providing index of back + 1 (array->data_end).
     */

    const void *const data_end = array->data_end;
    if (position > data_end) {
        return E_ARRAY_INDEX_OUT_OF_BOUNDS;
    }

    if (position != data_end) {
        void *const next_position = position + item_size;
        const uint64_t move_size = (uint64_t)(data_end - position);

        memmove(next_position, position, move_size);
    }

    memcpy(position, item, item_size);

    array->data_end = data_end + item_size;
    array->item_count += 1;

    if (item_out != NULL) {
        *item_out = position;
    }

    return E_ARRAY_OK;
}

enum array_result
array_add_item_with_cached_index_info(
    struct array *__notnull const array,
    const size_t item_size,
    const void *__notnull const item,
    struct array_cached_index_info *__notnull const info,
    void **const item_out)
{
    uint64_t index = 0;
    switch (info->type) {
        case ARRAY_CACHED_INDEX_LESS_THAN:
        case ARRAY_CACHED_INDEX_EQUAL:
            index = info->index;
            break;

        case ARRAY_CACHED_INDEX_GREATER_THAN:
            index = info->index + 1;
            break;
    }

    const enum array_result add_item_result =
        array_add_item_to_index(array, item_size, item, index, item_out);

    if (add_item_result != E_ARRAY_OK) {
        return add_item_result;
    }

    return E_ARRAY_OK;
}

enum array_result
array_copy(struct array *__notnull const array,
           struct array *__notnull const array_out)
{
    const uint64_t used_size = array_get_used_size(array);
    void *const data = malloc(used_size);

    if (unlikely(data == NULL)) {
        return E_ARRAY_OK;
    }

    void *const end = data + used_size;
    memcpy(data, array->data, used_size);

    array_out->data = data;
    array_out->data_end = end;
    array_out->alloc_end = end;

    return E_ARRAY_OK;
}

void
array_sort_with_comparator(struct array *__notnull const array,
                           const size_t item_size,
                           __notnull const array_sort_comparator comparator)
{
    qsort(array->data, array->item_count, item_size, comparator);
}

void array_clear(struct array *__notnull const array) {
    array->data_end = array->data;
    array->item_count = 0;
}

void
array_trim_to_item_count(struct array *__notnull const array,
                         const size_t item_size,
                         const uint64_t item_count)
{
    const uint64_t byte_index = (item_count * item_size);

    array->data_end = (array->data + byte_index);
    array->item_count = item_count;
}

enum array_result array_destroy(struct array *__notnull const array) {
    /*
     * free(NULL) is allowed
     */

    free(array->data);

    array->data = NULL;
    array->data_end = NULL;
    array->alloc_end = NULL;
    array->item_count = 0;

    return E_ARRAY_OK;
}
