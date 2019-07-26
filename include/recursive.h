//
//  include/recursive.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef RECURSIVE_H
#define RECURSIVE_H

#include <sys/types.h>

#include <stdio.h>
#include <stdint.h>

#include "notnull.h"

int
open_r(char *path,
       uint64_t path_length,
       int flags,
       mode_t mode,
       mode_t dir_mode,
       char **first_terminator_out);

int
mkdir_r(char *path,
        uint64_t path_length,
        mode_t mode,
        char **first_terminator_out);

/*
 * Remove the files and all directories above it until we reach last.
 *
 * last is a pointer to the last-path component to be removed. Null-terminating
 * last will give the path to the first directory created.
 */

int
remove_file_r(char *__notnull path, uint64_t path_length, char *__notnull last);

#endif /* RECURSIVE_H */
