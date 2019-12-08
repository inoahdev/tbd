//
//  src/target_list.c
//  tbd
//
//  Created by inoahdev on 11/21/19.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "target_list.h"
#include "tbd.h"

uint64_t
target_list_create_target(const struct arch_info *__notnull const arch,
                          const enum tbd_platform platform)
{
    /*
     * Every arch-info is aligned on a 16-byte boundary, so the 4 LSB are free
     * for the platform.
     */

    return ((uint64_t)arch | platform);
}

static void *
realloc_to_heap(struct target_list *__notnull const list, const uint64_t new) {
    uint64_t *const data = malloc(sizeof(uint64_t) * new);
    if (data == NULL) {
        return NULL;
    }

    /*
     * The first three fields in list store the first three targets.
     */

    void *const data_end = data + list->count;
    memcpy(data, list, sizeof(uint64_t) * 3);

    list->data = data;
    list->data_end = data_end;
    list->alloc_end = data + new;

    return data_end;
}

static enum target_list_result
add_to_heap(struct target_list *__notnull const list, const uint64_t new_target)
{
    uint64_t *const data_end = list->data_end;
    uint64_t *const alloc_end = list->alloc_end;

    uint64_t *data = list->data;
    const uint64_t count = list->count;

    const uint64_t used_size = sizeof(uint64_t) * count;
    const uint64_t free_space = (uint64_t)(alloc_end - data_end);

    if (free_space == 0) {
        const uint64_t item_capacity = (uint64_t)(alloc_end - data);
        const uint64_t byte_capacity = sizeof(uint64_t) * item_capacity;

        uint64_t *const new_data = malloc(byte_capacity * 2);
        if (new_data == NULL) {
            return E_TARGET_LIST_ALLOC_FAIL;
        }

        memcpy(new_data, data, used_size);
        free(data);

        list->data = new_data;
        list->alloc_end = new_data + item_capacity;

        data = new_data;
    }

    data[count] = new_target;

    list->data_end = data + count + 1;
    list->count = count + 1;

    return E_TARGET_LIST_OK;
}

enum target_list_result
target_list_add_target(struct target_list *__notnull const list,
                       const struct arch_info *__notnull const arch,
                       const enum tbd_platform platform)
{
    const uint64_t target = target_list_create_target(arch, platform);
    switch (list->count) {
        case 0:
            list->data = (uint64_t *)target;
            list->count = 1;

            return E_TARGET_LIST_OK;

        case 1:
            list->data_end = (uint64_t *)target;
            list->count = 2;

            return E_TARGET_LIST_OK;

        case 2:
            list->alloc_end = (uint64_t *)target;
            list->count = 3;

            return E_TARGET_LIST_OK;

        case 3: {
            uint64_t *const ptr = realloc_to_heap(list, target);
            if (ptr == NULL) {
                return E_TARGET_LIST_ALLOC_FAIL;
            }

            *ptr = target;
            break;
        }

        default:
            break;
    }

    const enum target_list_result add_target_result = add_to_heap(list, target);
    if (add_target_result != E_TARGET_LIST_OK) {
        return add_target_result;
    }

    return E_TARGET_LIST_OK;
}

static bool
has_target_in_range(const uint64_t *ptr,
                    const uint64_t *const end,
                    const uint64_t item)
{
    for (; ptr != end; ptr++) {
        if (*ptr == item) {
            return true;
        }
    }

    return false;
}


bool
target_list_has_target(const struct target_list *__notnull const list,
                       const struct arch_info *__notnull const arch,
                       const enum tbd_platform platform)
{
    const uint64_t target = target_list_create_target(arch, platform);
    if (list->count < 4) {
        if (target == (uint64_t)list->data) {
            return true;
        }

        if (target == (uint64_t)list->data_end) {
            return true;
        }

        if (target == (uint64_t)list->alloc_end) {
            return true;
        }

        return false;
    }

    return has_target_in_range(list->data, list->data_end, target);
}

void
target_list_get_target(const struct target_list *__notnull const list,
                       const uint64_t index,
                       const struct arch_info **__notnull const arch_out,
                       enum tbd_platform *__notnull const platform_out)
{
    const uint64_t count = list->count;
    uint64_t target = 0;

    if (count < 4) {
        switch (index) {
            case 0:
                target = (uint64_t)list->data;
                break;

            case 1:
                target = (uint64_t)list->data_end;
                break;

            case 2:
                target = (uint64_t)list->alloc_end;
                break;

            default:
                return;
        }
    } else {
        target = list->data[index];
    }

    *arch_out = (const struct arch_info *)(target & TARGET_ARCH_INFO_MASK);
    *platform_out = (const enum tbd_platform)(target & TARGET_PLATFORM_MASK);
}

static void *
expand_heap_to_cap(struct target_list *__notnull const list,
                   const uint64_t cap)
{
    uint64_t *const data = malloc(sizeof(uint64_t) * cap);
    if (data == NULL) {
        return NULL;
    }

    /*
     * The first three fields in list store the first three targets.
     */

    void *const old_data = list->data;
    const uint64_t old_count = list->count;

    memcpy(data, old_data, sizeof(uint64_t) * old_count);

    list->data = data;
    list->data_end = data + old_count;
    list->alloc_end = data + cap;

    return data;
}

enum target_list_result
target_list_reserve_count(struct target_list *__notnull const list,
                          const uint64_t count)
{
    const uint64_t list_count = list->count;
    if (list_count < 4) {
        if (count < 4) {
            return E_TARGET_LIST_OK;
        }

        if (realloc_to_heap(list, count) == NULL) {
            return E_TARGET_LIST_ALLOC_FAIL;
        }

        return E_TARGET_LIST_OK;
    }

    if (list_count > count) {
        return E_TARGET_LIST_OK;
    }

    if (expand_heap_to_cap(list, count) == NULL) {
        return E_TARGET_LIST_ALLOC_FAIL;
    }

    return E_TARGET_LIST_OK;
}

void target_list_destroy(struct target_list *__notnull const list) {
    if (list->count > 3) {
        free(list->data);
    }

    list->data = NULL;
    list->data_end = NULL;
    list->alloc_end = NULL;
    list->count = 0;
}
