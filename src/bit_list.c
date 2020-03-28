//
//  src/bit_list.c
//  tbd
//
//  Created by inoahdev on 11/18/19.
//  Copyright © 2019 - 2020 inoahdev. All rights reserved.
//

#include <sys/types.h>

#include <stdint.h>
#include <stdlib.h>
#include <strings.h>

#include "bit_list.h"
#include "likely.h"

enum bit_list_result
bit_list_create_with_capacity(struct bit_list *__notnull const list,
                              const uint64_t capacity)
{
    /*
     * We can only hold 63 bits on the stack.
     */

    const uint64_t integer_count = (capacity >> 6);
    if (integer_count == 0) {
        return E_BIT_LIST_OK;
    }

    const uint64_t byte_capacity = (sizeof(uint64_t) * integer_count);
    uint64_t *const data = malloc(byte_capacity);

    if (data == NULL) {
        return E_BIT_LIST_ALLOC_FAIL;
    }

    list->data = (uint64_t)data | 1;
    list->alloc_count += 1;

    return E_BIT_LIST_OK;
}

static uint64_t find_first_bit_stack(uint64_t stack, const uint64_t start) {
    stack >>= start;

    const uint64_t loc = ffsll(stack);
    if (loc != 0) {
        return (loc - 1);
    }

    return UINT64_MAX;
}

static uint64_t
find_first_bit_heap(const uint64_t *ptr,
                    const uint64_t *const end,
                    uint64_t start)
{
    if (start > 63) {
        const uint64_t mask = (1ull << 6) - 1;
        const uint64_t remainder = (start & mask);

        ptr += (start >> 6);
        start >>= remainder;
    }

    for (; ptr != end; ptr++) {
        const uint64_t loc = ffsll(*ptr);
        if (loc != 0) {
            return (loc - 1);
        }
    }

    return UINT64_MAX;
}

static inline uint64_t *get_bits_ptr(const struct bit_list list) {
    return (uint64_t *)(list.data & ~1ull);
}

static inline uint64_t get_set_integer_count(const struct bit_list list) {
    const uint64_t mask = ((1ull << 6) - 1);
    const uint64_t remainder = (list.set_count & mask);

    if (remainder != 0) {
        return (remainder + 1);
    }

    return remainder;
}

static inline int bit_list_is_on_heap(const struct bit_list list) {
    return (list.data & 1);
}

uint64_t bit_list_find_first_bit(const struct bit_list list) {
    if (likely(bit_list_is_on_heap(list) == 0)) {
        return find_first_bit_stack(list.data, 1);
    }

    const uint64_t set_integer_count = get_set_integer_count(list);

    const uint64_t *const ptr = get_bits_ptr(list);
    const uint64_t *const end = ptr + set_integer_count;

    return find_first_bit_heap(ptr, end, 0);
}

uint64_t
bit_list_find_bit_after_last(const struct bit_list list, const uint64_t last) {
    if (likely(bit_list_is_on_heap(list) == 0)) {
        /*
         * Add one for the LSB bit, and one to move past the last bit.
         */

        return find_first_bit_stack(list.data, last + 2);
    }

    const uint64_t set_integer_count = get_set_integer_count(list);

    const uint64_t *const ptr = get_bits_ptr(list);
    const uint64_t *const end = ptr + set_integer_count;

    /*
     * Only add one as we don't have to worry about the LSB flag.
     */

    return find_first_bit_heap(ptr, end, last + 1);
}

static int
compare_int_ptrs(const uint64_t *__notnull l_ptr,
                 const uint64_t *__notnull r_ptr,
                 const uint64_t *__notnull const l_end)
{
    for (; l_ptr != l_end; l_ptr++, r_ptr++) {
        const uint64_t l_data = *l_ptr;
        const uint64_t r_data = *r_ptr;

        if (l_data > r_data) {
            return 1;
        } else if (l_data < r_data) {
            return -1;
        }
    }

    return 0;
}

int
bit_list_equal_counts_compare(const struct bit_list left,
                              const struct bit_list right)
{
    if (likely(bit_list_is_on_heap(left) == 0)) {
        if (left.data > right.data) {
            return 1;
        } else if (left.data < right.data) {
            return -1;
        }

        return 0;
    }

    const uint64_t set_integer_count = get_set_integer_count(left);

    const uint64_t *const l_ptr = (const uint64_t *)left.data;
    const uint64_t *const r_ptr = (const uint64_t *)right.data;

    return compare_int_ptrs(l_ptr, r_ptr, l_ptr + set_integer_count);
}

static bool
int_ptrs_is_equal(const uint64_t *__notnull l_ptr,
                  const uint64_t *__notnull r_ptr,
                  const uint64_t *__notnull const l_end)
{
    for (; l_ptr != l_end; l_ptr++, r_ptr++) {
        const uint64_t l_data = *l_ptr;
        const uint64_t r_data = *r_ptr;

        if (l_data == r_data) {
            return true;
        }
    }

    return false;
}

bool
bit_list_equal_counts_is_equal(const struct bit_list left,
                               const struct bit_list right)
{
    if (likely(bit_list_is_on_heap(left) == 0)) {
        return (left.data == right.data);
    }

    const uint64_t set_integer_count = get_set_integer_count(left);

    const uint64_t *const l_ptr = (const uint64_t *)left.data;
    const uint64_t *const r_ptr = (const uint64_t *)right.data;

    return int_ptrs_is_equal(l_ptr, r_ptr, l_ptr + set_integer_count);
}

uint64_t
bit_list_get_for_index(const struct bit_list list, const uint64_t index) {
    if (likely(bit_list_is_on_heap(list) == 0)) {
        return bit_list_get_for_index_on_stack(list, index);
    }

    return bit_list_get_for_index_on_heap(list, index);
}

uint64_t
bit_list_get_for_index_on_stack(const struct bit_list list,
                                const uint64_t index)
{
    const uint64_t bit_index = (1ull << (index + 1));
    return (list.data & bit_index);
}

uint64_t
bit_list_get_for_index_on_heap(const struct bit_list list, const uint64_t index)
{
    const uint64_t *const info = get_bits_ptr(list);

    const uint64_t byte_index = (index >> 6);
    const uint64_t *const ptr = info + byte_index;

    const uint64_t bit_index_mask = (1ull << 6) - 1;
    const uint64_t bit_index = (index & bit_index_mask);
    const uint64_t mask = (1ull << bit_index);

    return (*ptr & mask);
}

void
bit_list_set_bit(struct bit_list *__notnull const list, const uint64_t index) {
    if (unlikely(bit_list_is_on_heap(*list))) {
        uint64_t *const info = get_bits_ptr(*list);

        const uint64_t byte_index = (index >> 6);
        uint64_t *const ptr = info + byte_index;

        const uint64_t bit_index_mask = (1ull << 6) - 1;
        const uint64_t bit_index = (index & bit_index_mask);

        const uint64_t integer = *ptr;
        const uint64_t mask = (1ull << bit_index);

        if (!(integer & mask)) {
            *ptr = (integer | mask);
            list->set_count += 1;
        }
    } else {
        const uint64_t integer = list->data;
        const uint64_t mask = (1ull << (index + 1));

        if (!(integer & mask)) {
            list->data = (integer | mask);
            list->set_count += 1;
        }
    }
}

static inline uint64_t get_mask_for_first_n(const uint64_t n) {
    return (~0ull >> (64 - n));
}

void
bit_list_set_first_n(struct bit_list *__notnull const list, const uint64_t n) {
    if (unlikely(bit_list_is_on_heap(*list))) {
        uint64_t *ptr = get_bits_ptr(*list);
        if (n >= 64) {
            uint64_t i = n;
            do {
                *ptr |= ~0ull;
                ptr++;

                i -= 64;
                if (i >= 64) {
                    continue;
                }

                *ptr |= get_mask_for_first_n(i);
                break;
            } while (true);
        } else {
            *ptr |= get_mask_for_first_n(n);
        }
    } else {
        /*
         * Shift by one as the LSB is used by bit_list.
         */

        const uint64_t mask = (get_mask_for_first_n(n) << 1);
        list->data |= mask;
    }

    list->set_count = n;
}

void bit_list_clear(struct bit_list *__notnull const list) {
    list->set_count = 0;
}

void bit_list_destroy(struct bit_list *__notnull const list) {
    if (unlikely(bit_list_is_on_heap(*list))) {
        free(get_bits_ptr(*list));
    }
}
