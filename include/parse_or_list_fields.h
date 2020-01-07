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

struct target_list
parse_architectures_list(int index,
                         int argc,
                         char *const *__notnull argv,
                         int *__notnull index_out);

struct tbd_flags
parse_flags_list(int index,
                 int argc,
                 char *const *__notnull argv,
                 int *__notnull index_out);

enum tbd_objc_constraint
parse_objc_constraint(const char *__notnull constraint);

enum tbd_platform parse_platform(const char *__notnull platform);

uint32_t parse_swift_version(const char *__notnull const arg);

struct target_list
parse_targets_list(int index,
                   int argc,
                   char *const *__notnull argv,
                   int *__notnull index_out);

int64_t parse_packed_version(const char *__notnull const version);
enum tbd_version parse_tbd_version(const char *__notnull version);

void print_arch_info_list(void);
void print_objc_constraint_list(void);
void print_platform_list(void);

void print_tbd_flags_list(void);
void print_tbd_version_list(void);

#endif /* PARSE_OR_LIST_FIELDS_H */
