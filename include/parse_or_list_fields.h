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
#include "tbd.h"

uint64_t
parse_architectures_list(int argc, const char *const *argv, int *index_in);

uint64_t parse_flags_list(int argc, const char *const *argv, int *index_in);
uint32_t parse_swift_version(const char *const arg);

enum tbd_objc_constraint parse_objc_constraint(const char *const constraint);
enum tbd_platform parse_platform(const char *const platform);
enum tbd_version parse_tbd_version(const char *const version);

void print_arch_info_list(void);
void print_objc_constraint_list(void);
void print_platform_list(void);

void print_tbd_flags_list(void);
void print_tbd_version_list(void);

#endif /* PARSE_OR_LIST_FIELDS_H */
