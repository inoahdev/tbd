//
//  include/path.h
//  tbd
//
//  Created by inoahdev on 11/19/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef PATH_H
#define PATH_H

#include <stdint.h>

/*
 * Get an absolute path from a relative path (relative to current-directory).
 * Returns either a pointer to the newly allocated string, the path provided,
 * or NULL to indicate allocation failure.
 */

char *path_get_absolute_path_if_necessary(const char *path);

char *
path_append_component_with_len(const char *path,
                               uint64_t path_length,
                               const char *component,
                               uint64_t component_length);

char *
path_append_component_and_extension_with_len(const char *path,
                                             uint64_t path_length,
                                             const char *component,
                                             uint64_t component_length,
                                             const char *extension,
                                             uint64_t extension_length);

const char *
path_get_iter_before_front_of_row_of_slashes(const char *path,
                                             const char *iter);

const char *
path_get_front_of_row_of_slashes(const char *path, const char *iter);

const char *
path_find_last_row_of_slashes_before_end(const char *path, const char *end);

const char *path_get_back_of_row_of_slashes(const char *const path);
const char *path_get_end_of_row_of_slashes(const char *path);

const char *path_find_last_row_of_slashes(const char *path);
const char *path_find_ending_row_of_slashes(const char *path);

const char *
path_get_last_path_component(const char *path,
                             uint64_t path_length,
                             uint64_t *length_out);

bool
path_has_component(const char *path,
                   const char *component,
                   uint64_t component_length,
                   bool *is_hierarchy);

#endif /* PATH_H */
