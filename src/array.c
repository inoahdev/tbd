//
//  src/array.c
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <stdlib.h>
#include <string.h>

#include "array.h"

void *
array_get_item_at_index(const struct array *const array,
                        const size_t item_size,
                        const uint64_t index)
{
    const uint64_t byte_index = item_size * index;
    void *const iter = &array->data[byte_index];

    if (iter > array->data_end) {
        return NULL;
    }

    return iter; 
}

void *array_get_front(const struct array *const array) {
    return array->data;
}

void *array_get_back(const struct array *const array, const size_t item_size) {
    void *const data_end = array->data_end;
    if (array->data == data_end) {
        return NULL;
    }

    return data_end - item_size;
}

bool array_is_empty(const struct array *const array) {
    return array->data == array->data_end;
}

static enum array_result
array_expand_if_necessary(struct array *const array, const uint64_t item_size) {
    const uint64_t old_capacity = array->alloc_end - array->data;
    const uint64_t used_size = array->data_end - array->data;

    if (used_size < old_capacity) {
        return E_ARRAY_OK;
    }

    uint64_t new_capacity = old_capacity * 2;
    if (new_capacity == 0) {
        new_capacity = item_size;
    }

    void *const new_data = calloc(1, new_capacity);
    if (new_data == NULL) {
        return E_ARRAY_ALLOC_FAIL;
    }

    void *const old_data = array->data;

    memcpy(new_data, array->data, used_size);
    free(old_data);

    array->data = new_data;
    array->data_end = new_data + used_size;
    array->alloc_end = new_data + new_capacity;

    return E_ARRAY_OK;
}

enum array_result
array_add_item_to_byte_index(struct array *const array,
                             const size_t item_size,
                             const void *const item,
                             const uint64_t byte_index,
                             void **const item_out)
{
    const enum array_result expand_result =
        array_expand_if_necessary(array, item_size);
    
    if (expand_result != E_ARRAY_OK) {
        return expand_result;
    }
    
    void *const position = array->data + byte_index;
    void *const data_end = array->data_end;
    
    if (position != data_end) {
        void *const next_position = position + item_size;
        const uint64_t array_move_size = data_end - position;
        
        memmove(next_position, position, array_move_size);
    }
    
    memcpy(position, item, item_size);
    array->data_end = data_end + item_size;
    
    if (item_out != NULL) {
        *item_out = position;
    }

    return E_ARRAY_OK;
}

uint64_t
array_get_item_count(const struct array *const array, const size_t item_size) {
    const uint64_t used_size = array->data_end - array->data;
    if (used_size == 0) {
        return 0;
    }

    return used_size / item_size;
}

uint64_t array_get_used_size(const struct array *const array) {
    return array->data_end - array->data;
}

enum array_result
array_add_item(struct array *const array,
               const size_t item_size,
               const void *const item,
               void **const item_out)
{
    const uint64_t byte_index = array->data_end - array->data;
    const enum array_result add_item_result =
        array_add_item_to_byte_index(array,
                                     item_size,
                                     item,
                                     byte_index,
                                     item_out);
        
    if (add_item_result != E_ARRAY_OK) {
        return add_item_result;
    }
    
    return E_ARRAY_OK;
}

enum array_result
array_add_item_to_index(struct array *const array,
                        const size_t item_size,
                        const void *const item,
                        const uint64_t index,
                        void **const item_out)
{
    const uint64_t byte_index = item_size * index;
    void *const position = array->data + byte_index;
    
    /*
     * Note: We do allow providing index of back + 1 (array->data_end).
     */
    
    if (position > array->data_end) {
        return E_ARRAY_INDEX_OUT_OF_BOUNDS;
    }
    
    const enum array_result add_item_result =
        array_add_item_to_byte_index(array,
                                     item_size,
                                     item,
                                     byte_index,
                                     item_out);
        
    if (add_item_result != E_ARRAY_OK) {
        return add_item_result;
    }
    
    return E_ARRAY_OK;
}

void *
array_find_item(const struct array *const array,
                const size_t item_size,
                const void *const item,
                const array_item_comparator comparator,
                uint64_t *const index_out)
{
    void *data_iter = array->data;
    const void *const data_end = array->data_end;

    uint64_t index = 0;

    for (; data_iter != data_end; data_iter += item_size, index++) {
        if (comparator(data_iter, item) != 0) {
            continue;
        }

        if (index_out != NULL) {
            *index_out = index;
        }
        
        return data_iter;
    }

    return NULL;
}

struct array_slice {
    uint64_t front;
    uint64_t back;
};

static uint64_t
array_slice_get_middle_index(const struct array_slice *const slice) {
    const uint64_t front = slice->front;
	const uint64_t length = slice->back - front;

    return front + (length >> 1);
}

/*
 * Use wrapper-functions here to make the code below it easier to read.
 */

static void
array_slice_set_to_lower_half(struct array_slice *const slice,
                              const uint64_t middle)
{
    slice->back = middle - 1;
}

static void
array_slice_set_to_upper_half(struct array_slice *const slice,
                              const uint64_t middle)
{
    slice->front = middle + 1;
}

static bool
array_slice_holds_one_element(const struct array_slice *const slice) {
    return slice->back - slice->front == 0;
}

static bool
array_slice_holds_two_elements(const struct array_slice *const slice) {
    return slice->back - slice->front == 1;
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

static void *
array_slice_get_sorted_array_item_for_item(
    const struct array *const array,
    struct array_slice *const slice,
    const size_t item_size,
    const void *const item,
    const array_item_comparator comparator,
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
                info_out->index = slice->front;
                info_out->type = get_cached_index_type_from_ret(compare);
            }

            return NULL;
        }

        /*
         * compare is greater than 0 when array_item is "greater than"
         * item, in which case we need to set slice to its lower half.
         *
         * Otherwise if it's less than 0, when array_item is "less than" item,
         * so we need to set slice to its upper half.
         */

        if (compare > 0) {
            /*
             * Since middle-index for a two-element slice is always
             * slice->front, we cannot go anywhere if slice->front is
             * "greater than" the item provided
             */

            if (array_slice_holds_two_elements(slice)) {
                if (info_out != NULL) {
                    info_out->index = slice->front;
                    info_out->type = ARRAY_CACHED_INDEX_LESS_THAN;
                }

                return NULL;
            }

            array_slice_set_to_lower_half(slice, index);
        } else {
            array_slice_set_to_upper_half(slice, index);
        }
    } while (true);
}

void *
array_find_item_in_sorted(const struct array *const array,
                          const size_t item_size,
                          const void *const item,
                          const array_item_comparator comparator,
                          struct array_cached_index_info *const info_out)
{
    const uint64_t used_size = array_get_used_size(array);
    if (used_size == 0) {
        if (info_out != NULL) {
            info_out->index = 0;
            info_out->type = ARRAY_CACHED_INDEX_EQUAL;
        }
        
        return NULL;
    }

    const uint64_t item_count = used_size / item_size; 
    struct array_slice slice = {
        .front = 0,
        .back = item_count - 1
    };

    void *const array_item =
        array_slice_get_sorted_array_item_for_item(array,
                                                   &slice,
                                                   item_size,
                                                   item,
                                                   comparator,
                                                   info_out);

    return array_item;
}

enum array_result
array_add_item_with_cached_index_info(
    struct array *const array,
    const size_t item_size,
    const void *const item,
    struct array_cached_index_info *const info,
    void **const item_out)
{
    const uint64_t index = info->index;
    const enum array_cached_index_type type = info->type;

    if (index == 0) {
        /*
         * If type isn't greater-than, simply have item at index 0 move up to
         * index 1 and return.
         */
        
        if (type != ARRAY_CACHED_INDEX_GREATER_THAN) {
            return array_add_item_to_index(array, item_size, item, 0, item_out);
        }

        return array_add_item_to_index(array, item_size, item, 1, item_out);
    }

    const uint64_t back_index = array_get_item_count(array, item_size) - 1;
    if (index == back_index) {
        /*
         * Since we're at the back anyways, if the item is greater, simply add
         * item to the array-buffer's end.
         */
        
        if (type == ARRAY_CACHED_INDEX_GREATER_THAN) {
            return array_add_item(array, item_size, item, NULL);
        }
        
        /*
         * If the type isn't greater-than, have the last item move up one index,
         * with the new item replacing it at its old position.
         */

        const enum array_result add_item_result =
            array_add_item_to_index(array,
                                    item_size,
                                    item,
                                    back_index,
                                    item_out);
        
        if (add_item_result != E_ARRAY_OK) {
            return add_item_result;
        }
        
        return E_ARRAY_OK;
    }
    
    if (type != ARRAY_CACHED_INDEX_GREATER_THAN) {
        /*
         * If the type isn't greater than, have our new item replacing the old
         * item at index by having the old item move to (index + 1), with the
         * new-item replacing it.
         */
        
        return array_add_item_to_index(array, item_size, item, index, item_out);
    }
    
    /*
     * Since the type is greater than, have the new item be placed at index + 1.
     */

    return array_add_item_to_index(array, item_size, item, index + 1, item_out);
}

enum array_result
array_sort_items_with_comparator(struct array *const array,
                                 const size_t item_size,
                                 const array_item_comparator comparator)
{
    const uint64_t item_count = array_get_item_count(array, item_size);
    qsort(array->data, item_count, item_size, comparator);

    return E_ARRAY_OK;
}

enum array_result
array_copy(struct array *const array, struct array *const array_out) {
    const uint64_t used_size = array_get_used_size(array);
    void *const data = calloc(1, used_size);

    if (data == NULL) {
        return E_ARRAY_OK;
    }

    void *const end = data + used_size;
    memcpy(data, array->data, used_size);
    
    array_out->data = data;
    array_out->data_end = end;
    array_out->alloc_end = end;
    
    return E_ARRAY_OK;
}

/*
 * Deallocate the array's buffer and reset the array's fields.
 */

enum array_result array_destroy(struct array *const array) {
    /*
     * free(NULL) is allowed
     */
    
    free(array->data);
    
    array->data = NULL;
    array->data_end = NULL;
    array->alloc_end = NULL;
    
    return E_ARRAY_OK;
}
