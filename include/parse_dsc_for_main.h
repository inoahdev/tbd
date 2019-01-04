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

bool
parse_shared_cache(struct tbd_for_main *global,
                   struct tbd_for_main *tbd,
                   const char *path,
                   uint64_t path_length,
                   int fd,
                   uint64_t size,
                   bool is_recursing,
                   bool print_paths,
                   uint64_t *retained_info_in);

#endif /* PARSE_DSC_FOR_MAIN_H */
