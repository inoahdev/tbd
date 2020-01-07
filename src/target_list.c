//
//  src/target_list.c
//  tbd
//
//  Created by inoahdev on 11/21/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "target_list.h"
#include "tbd.h"

uint64_t
replace_platform_for_target(const uint64_t target,
                            const enum tbd_platform platform)
{
    const uint64_t arch = (target & TARGET_ARCH_INFO_MASK);
    const uint64_t new_target = (arch | platform);

    return new_target;
}

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

    data[0] = (uint64_t)list->data;
    data[1] = list->stack[0];
    data[2] = list->stack[1];

    list->data = data;
    list->alloc_count = new;

    void *const data_end = data + list->set_count;
    return data_end;
}

static enum target_list_result
add_to_heap(struct target_list *__notnull const list, const uint64_t new_target)
{
    uint64_t *data = list->data;

    const uint64_t set_count = list->set_count;
    const uint64_t free_count = list->alloc_count - set_count;

    if (free_count == 0) {
        const uint64_t cap = set_count + free_count;
        const uint64_t new_cap = cap * 2;

        uint64_t *const new_data = malloc(new_cap);
        if (new_data == NULL) {
            return E_TARGET_LIST_ALLOC_FAIL;
        }

        memcpy(new_data, data, sizeof(uint64_t) * set_count);
        free(data);

        list->data = new_data;
        list->alloc_count = new_cap;

        data = new_data;
    }

    data[set_count] = new_target;
    list->set_count = set_count + 1;

    return E_TARGET_LIST_OK;
}

enum target_list_result
target_list_add_target(struct target_list *__notnull const list,
                       const struct arch_info *__notnull const arch,
                       const enum tbd_platform platform)
{
    const uint64_t target = target_list_create_target(arch, platform);
    if (list->alloc_count == 0) {
        switch (list->set_count) {
            case 0:
                list->data = (uint64_t *)target;
                list->set_count = 1;

                return E_TARGET_LIST_OK;

            case 1:
                list->stack[0] = target;
                list->set_count = 2;

                return E_TARGET_LIST_OK;

            case 2:
                list->stack[1] = target;
                list->set_count = 3;

                return E_TARGET_LIST_OK;

            case 3: {
                uint64_t *const ptr = realloc_to_heap(list, 6);
                if (ptr == NULL) {
                    return E_TARGET_LIST_ALLOC_FAIL;
                }

                *ptr = target;
                break;
            }

            default:
                break;
        }
    }

    const enum target_list_result add_target_result = add_to_heap(list, target);
    if (add_target_result != E_TARGET_LIST_OK) {
        return add_target_result;
    }

    return E_TARGET_LIST_OK;
}

static bool
has_target_in_range(const uint64_t *ptr,
                    const uint64_t count,
                    const uint64_t target)
{
    const uint64_t *const end = ptr + count;
    for (; ptr != end; ptr++) {
        if (*ptr == target) {
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
    if (list->alloc_count == 0) {
        if (target == (uint64_t)list->data) {
            return true;
        }

        if (target == list->stack[0]) {
            return true;
        }

        if (target == list->stack[1]) {
            return true;
        }

        return false;
    }

    return has_target_in_range(list->data, list->set_count, target);
}

void
target_list_get_target(const struct target_list *__notnull const list,
                       const uint64_t index,
                       const struct arch_info **__notnull const arch_out,
                       enum tbd_platform *__notnull const platform_out)
{
    uint64_t target = 0;
    if (list->alloc_count == 0) {
        switch (index) {
            case 0:
                target = (uint64_t)list->data;
                break;

            case 1:
                target = list->stack[0];
                break;

            case 2:
                target = list->stack[1];
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

static inline uint64_t *
replace_platform_cast(uint64_t *__notnull const target,
                      const enum tbd_platform platform)
{
    return (uint64_t *)replace_platform_for_target((uint64_t)target, platform);
}

static inline void
replace_platform_ptr(uint64_t *__notnull const ptr,
                     const enum tbd_platform platform)
{
    *ptr = replace_platform_for_target(*ptr, platform);
}

void
target_list_replace_platform(struct target_list *__notnull const list,
                             const enum tbd_platform platform)
{
    const uint64_t set_count = list->set_count;
    if (list->alloc_count == 0) {
        switch (set_count) {
            case 0:
                return;

            case 1:
                list->data = replace_platform_cast(list->data, platform);
                return;

            case 2:
                list->data = replace_platform_cast(list->data, platform);
                replace_platform_ptr(&list->stack[0], platform);

                return;

            case 3:
                list->data = replace_platform_cast(list->data, platform);

                replace_platform_ptr(&list->stack[0], platform);
                replace_platform_ptr(&list->stack[1], platform);

                return;

            default:
                break;
        }
    }

    uint64_t *target = list->data;
    const uint64_t *const end = target + set_count;

    for (; target != end; target++) {
        replace_platform_ptr(target, platform);
    }
}

static void *
expand_heap_to_cap(struct target_list *__notnull const list, const uint64_t cap)
{
    uint64_t *const data = malloc(sizeof(uint64_t) * cap);
    if (data == NULL) {
        return NULL;
    }

    /*
     * The first three fields in list store the first three targets.
     */

    void *const old_data = list->data;
    const uint64_t old_count = list->set_count;

    memcpy(data, old_data, sizeof(uint64_t) * old_count);
    free(old_data);

    list->data = data;
    list->alloc_count = cap;

    return data;
}

enum target_list_result
target_list_reserve_count(struct target_list *__notnull const list,
                          const uint64_t count)
{
    if (count < 4) {
        return E_TARGET_LIST_OK;
    }

    const uint64_t alloc_count = list->alloc_count;
    if (alloc_count > count) {
        return E_TARGET_LIST_OK;
    }

    if (alloc_count == 0) {
        if (realloc_to_heap(list, count) == NULL) {
            return E_TARGET_LIST_ALLOC_FAIL;
        }
    } else {
        if (expand_heap_to_cap(list, count) == NULL) {
            return E_TARGET_LIST_ALLOC_FAIL;
        }
    }

    return E_TARGET_LIST_OK;
}

void target_list_clear(struct target_list *__notnull const list) {
    if (list->alloc_count == 0) {
        list->data = NULL;

        list->stack[0] = 0;
        list->stack[1] = 0;
    }

    list->set_count = 0;
}

void target_list_destroy(struct target_list *__notnull const list) {
    if (list->alloc_count != 0) {
        free(list->data);
    }

    list->data = NULL;

    list->stack[0] = 0;
    list->stack[1] = 0;

    list->set_count = 0;
    list->alloc_count = 0;
}
