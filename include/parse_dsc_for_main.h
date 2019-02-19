//
//  include/parse_dsc_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef PARSE_DSC_FOR_MAIN_H
#define PARSE_DSC_FOR_MAIN_H

#include "tbd_for_main.h"

/*
 * magic_in should be atleast 16 bytes large.
 */

bool
parse_shared_cache(void *magic_in,
                   uint64_t *magic_in_size_in,
                   uint64_t *retained_info_in,
                   struct tbd_for_main *global,
                   struct tbd_for_main *tbd,
                   const char *path,
                   uint64_t path_length,
                   int fd,
                   bool is_recursing,
                   bool ignore_non_cache,
                   bool print_paths);

void print_list_of_dsc_images(int fd);

#endif /* PARSE_DSC_FOR_MAIN_H */
