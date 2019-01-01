//
//  include/parse_dsc_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef PARSE_DSC_FOR_MAIN_H
#define PARSE_DSC_FOR_MAIN_H

#include "tbd_for_main.h"

bool
handle_shared_cache(struct tbd_for_main *global,
                    struct tbd_for_main *tbd,
                    const char *path,
                    int fd,
                    uint64_t size,
                    bool is_recursing);

#endif /* PARSE_DSC_FOR_MAIN_H */
