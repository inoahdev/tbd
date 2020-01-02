//
//  include/target_list.h
//  tbd
//
//  Created by inoahdev on 11/21/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#ifndef TARGET_LIST_H
#define TARGET_LIST_H

#include <stdbool.h>
#include "arch_info.h"

struct target_list {
    uint64_t *data;

    /*
     * Keep space for 2 more (3 total after incuding data), so allocs aren't
     * necessary.
     */

    uint64_t stack[2];

    uint64_t alloc_count;
    uint64_t set_count;
};

enum tbd_platform;

uint64_t
replace_platform_for_target(uint64_t target, enum tbd_platform platform);

enum target_list_result {
    E_TARGET_LIST_OK,
    E_TARGET_LIST_ALLOC_FAIL
};

/*
 * Forward-declare this enum as tbd.h includes this file as well.
 */

enum target_list_result
target_list_add_target(struct target_list *__notnull list,
                       const struct arch_info *__notnull arch,
                       enum tbd_platform platform);

bool
target_list_has_target(const struct target_list *__notnull list,
                       const struct arch_info *__notnull arch,
                       enum tbd_platform platform);

uint64_t
target_list_create_target(const struct arch_info *__notnull arch,
                          enum tbd_platform platform);

static const uint64_t TARGET_ARCH_INFO_MASK = (~0ull << 4);
static const uint64_t TARGET_PLATFORM_MASK  = (~TARGET_ARCH_INFO_MASK);

/*
 * Note: target_list_get_target() does NOT verify index.
 */

void
target_list_get_target(const struct target_list *__notnull list,
                       uint64_t index,
                       const struct arch_info **__notnull arch_out,
                       enum tbd_platform *__notnull platform_out);

void
target_list_replace_platform(struct target_list *__notnull list,
                             enum tbd_platform platform);


enum target_list_result
target_list_reserve_count(struct target_list *__notnull list, uint64_t count);

void target_list_clear(struct target_list *__notnull list);
void target_list_destroy(struct target_list *__notnull list);

#endif /* TARGET_LIST_H */
