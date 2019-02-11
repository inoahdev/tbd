//
//  include/handle_dsc_result.h
//  tbd
//
//  Created by inoahdev on 12/31/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef HANDLE_DSC_PARSE_RESULT
#define HANDLE_DSC_PARSE_RESULT

#include "dsc_image.h"
#include "dyld_shared_cache.h"
#include "tbd_for_main.h"

void
handle_dsc_file_parse_result(const char *path,
                             enum dyld_shared_cache_parse_result parse_result,
                             bool print_paths);

bool
handle_dsc_image_parse_result(struct tbd_for_main *global,
                              struct tbd_for_main *tbd,
                              const char *dsc_path,
                              const char *image_path,
                              enum dsc_image_parse_result parse_result,
                              bool print_paths,
                              uint64_t *retained_info_in);

void
print_dsc_image_parse_error(struct tbd_for_main *tbd,
                            const char *image_path,
                            enum dsc_image_parse_result parse_error);

#endif /* HANDLE_DSC_PARSE_RESULT */

