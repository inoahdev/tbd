//
//  include/parse_or_list_fields.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef PARSE_OR_LIST_FIELDS_H
#define PARSE_OR_LIST_FIELDS_H

#include <stdint.h>

#include "notnull.h"
#include "tbd.h"

uint64_t
parse_architectures_list(int index,
                         int argc,
                         const char *const *__notnull argv,
                         int *__notnull count_out,
                         int *__notnull index_out);

uint32_t
parse_flags_list(int index,
                 int argc,
                 const char *const *__notnull argv,
                 int *__notnull index_out);

uint32_t parse_swift_version(const char *__notnull const arg);

enum tbd_objc_constraint
parse_objc_constraint(const char *__notnull constraint);

enum tbd_platform parse_platform(const char *__notnull platform);
enum tbd_version parse_tbd_version(const char *__notnull version);

int64_t parse_packed_version(const char *__notnull const version);

void print_arch_info_list(void);
void print_objc_constraint_list(void);
void print_platform_list(void);

void print_tbd_flags_list(void);
void print_tbd_version_list(void);

#endif /* PARSE_OR_LIST_FIELDS_H */
