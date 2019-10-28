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
 *
 * If path_length is zero, path_get_absolute_path() will calculate the
 * path-length.
 *
 * length_out is only written to if an absolute path is created.
 *
 * Returns either a pointer to the newly allocated string, the path provided,
 * or NULL to indicate allocation failure.
 */

char *
path_get_absolute_path(const char *__notnull path,
                       uint64_t path_length,
                       uint64_t *length_out);

char *
path_append_component(const char *__notnull path,
                      uint64_t path_length,
                      const char *__notnull component,
                      uint64_t component_length,
                      uint64_t *length_out);

char *
path_append_comp_and_ext(const char *__notnull path,
                         uint64_t path_length,
                         const char *__notnull component,
                         uint64_t component_length,
                         const char *__notnull extension,
                         uint64_t extension_length,
                         uint64_t *length_out);

char *
path_append_two_comp_and_ext(const char *__notnull path,
                             uint64_t path_length,
                             const char *__notnull first_component,
                             uint64_t first_component_length,
                             const char *__notnull second_component,
                             uint64_t second_component_length,
                             const char *__notnull extension,
                             uint64_t extension_length,
                             uint64_t *length_out);

bool
path_has_dir_component(const char *__notnull path,
                       uint64_t path_length,
                       const char *__notnull component,
                       uint64_t component_length,
                       const char **dir_component_out);

bool
path_has_filename(const char *__notnull path,
                  uint64_t path_length,
                  const char *__notnull component,
                  uint64_t component_length,
                  const char **filename_out);

uint64_t path_remove_extension(const char *__notnull path, uint64_t length);

#endif /* PATH_H */
