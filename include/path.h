//
//  include/path.h
//  tbd
//
//  Created by inoahdev on 11/19/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef PATH_H
#define PATH_H

#include <stdbool.h>
#include <stdint.h>

#include "notnull.h"

/*
 * Get an absolute path from a relative path (relative to current-directory).
 * Returns either a pointer to the newly allocated string, the path provided,
 * or NULL to indicate allocation failure.
 */

char *
path_get_absolute_path(const char *__notnull path,
                       uint64_t path_length,
                       uint64_t *length_out);

char *
path_append_component_with_len(const char *__notnull path,
                               uint64_t path_length,
                               const char *__notnull component,
                               uint64_t component_length,
                               uint64_t *length_out);

char *
path_append_component_and_extension_with_len(const char *__notnull path,
                                             uint64_t path_length,
                                             const char *__notnull component,
                                             uint64_t component_length,
                                             const char *__notnull extension,
                                             uint64_t extension_length,
                                             uint64_t *length_out);

char *
path_append_two_components_and_extension_with_len(
    const char *__notnull path,
    uint64_t path_length,
    const char *__notnull first_component,
    uint64_t first_component_length,
    const char *__notnull second_component,
    uint64_t second_component_length,
    const char *__notnull extension,
    uint64_t extension_length,
    uint64_t *length_out);

const char *
path_get_front_of_row_of_slashes(const char *__notnull path,
                                 const char *__notnull iter);

const char *
path_find_last_row_of_slashes_before_end(const char *__notnull path,
                                         const char *__notnull end);

const char *
path_find_back_of_last_row_of_slashes_before_end(const char *__notnull path,
                                                 const char *__notnull end);

const char *path_get_end_of_row_of_slashes(const char *__notnull path);

const char *
path_find_last_row_of_slashes(const char *__notnull path, uint64_t path_length);

const char *
path_find_back_of_last_row_of_slashes(const char *__notnull path,
                                      const uint64_t path_length);

const char *
path_find_ending_row_of_slashes(const char *__notnull path,
                                uint64_t path_length);

bool
path_has_filename(const char *__notnull path,
                  uint64_t path_length,
                  const char *__notnull component,
                  uint64_t component_length,
                  const char **filename_out);

bool
path_has_dir_component(const char *__notnull path,
                       const char *__notnull component,
                       uint64_t component_length,
                       const char **dir_component_out);

const char *path_find_extension(const char *__notnull path, uint64_t length);

#endif /* PATH_H */
