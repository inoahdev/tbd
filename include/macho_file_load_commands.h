//
//  include/macho_file_load_commands.h
//  tbd
//
//  Created by inoahdev on 11/20/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef MACHO_FILE_LOAD_COMMANDS_H
#define MACHO_FILE_LOAD_COMMANDS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "arch_info.h"
#include "macho_file.h"

enum macho_file_parse_result 
macho_file_parse_load_commands(struct tbd_create_info *const info,
                               const struct arch_info *const arch,
                               const uint64_t arch_bit,
                               const int fd,
                               const uint64_t start,
                               const uint64_t size,
                               const bool is_64,
                               const bool is_big_endian,
                               const uint32_t ncmds,
                               const uint32_t sizeofcmds,
                               const uint64_t create_options,
                               const uint64_t options);

#endif /* MACHO_FILE_LOAD_COMMANDS_H */
