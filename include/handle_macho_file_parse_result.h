//
//  include/handle_macho_parse_result.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef HANDLE_MACHO_PARSE_RESULT_H
#define HANDLE_MACHO_PARSE_RESULT_H

#include "macho_file.h"
#include "tbd_for_main.h"

bool
handle_macho_file_parse_result(struct tbd_for_main *global,
                               struct tbd_for_main *tbd,
                               const char *path,
                               enum macho_file_parse_result parse_result,
                               bool print_paths,
                               uint64_t *retained_info_in);

#endif /* HANDLE_MACHO_PARSE_RESULT_H */
