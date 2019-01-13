//
//  src/path.c
//  tbd
//
//  Created by inoahdev on 11/19/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <errno.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include "path.h"

static const char *current_directory = NULL;
static size_t current_directory_length = 0;

static inline bool ch_is_path_slash(const char ch) {
    return ch == '/';
}

char *path_get_absolute_path_if_necessary(const char *const path) {
    const char path_front = path[0];
    if (ch_is_path_slash(path_front)) {
        return (char *)path;
    }

    if (current_directory == NULL) {
        current_directory = getcwd(NULL, 0);
        if (current_directory == NULL) {
            fprintf(stderr,
                    "Failed to retrieve current-directory, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        current_directory_length = strlen(current_directory);
    }

    char *const combined =
        path_append_component_with_len(current_directory,
                                       current_directory_length,
                                       path,
                                       strlen(path),
                                       NULL);

    return combined;
}

const char *
path_get_iter_before_front_of_row_of_slashes(const char *const path,
                                             const char *iter)
{
    char ch = *iter;

    do {
        --iter;
        if (iter < path) {
            return NULL;
        }

        ch = *iter;
    } while (ch_is_path_slash(ch));

    return iter;
}

const char *
path_get_front_of_row_of_slashes(const char *const path, const char *iter) {
    char ch = *iter;

    do {
        --iter;
        if (iter < path) {
            return NULL;
        }

        ch = *iter;
    } while (ch_is_path_slash(ch));

    return iter + 1;
}

const char *path_get_back_of_row_of_slashes(const char *const path) {
    const char *iter = path;
    char ch = *path;

    do {
        ch = *(++iter);
        if (ch == '\0') {
            return NULL;
        }
    } while (ch_is_path_slash(ch));

    return iter - 1;
}

const char *path_get_end_of_row_of_slashes(const char *const path) {
    const char *iter = path;
    char ch = *path;

    do {
        ch = *(++iter);
        if (ch == '\0') {
            return NULL;
        }
    } while (ch_is_path_slash(ch));

    return iter;
}

const char *path_get_next_slash(const char *const path) {
    const char *iter = path;
    for (char ch = *iter; ch != '\0'; ch = *(++iter)) {
        if (ch_is_path_slash(ch)) {
            return iter;
        }
    }

    return NULL;
}

const char *path_get_next_slash_or_end(const char *const path) {
    const char *iter = path;
    char ch = *iter;

    if (ch == '\0') {
        return NULL;
    }

    do {
        if (ch_is_path_slash(ch)) {
            return iter;
        }

        ch = *(++iter);
    } while (ch != '\0');

    return iter;
}

const char *path_find_last_row_of_slashes(const char *const path) {
    const char *iter = path;
    char ch = *path;

    if (ch == '\0') {
        return NULL;
    }

    const char *row = NULL;

    do {
        while (!ch_is_path_slash(ch)) {
            ch = *(++iter);
            if (ch == '\0') {
                return row;
            }
        }

        row = iter;

        do {
            ch = *(++iter);
            if (ch == '\0') {
                return row;
            }
        } while (ch_is_path_slash(ch));
    } while (true);
}

const char *
path_find_last_row_of_slashes_before_end(const char *const path,
                                         const char *const end)
{
    const char *iter = path;
    char ch = *path;

    if (ch == '\0') {
        return NULL;
    }

    const char *row = NULL;

    do {
        while (!ch_is_path_slash(ch)) {
            ++iter;
            if (iter == end) {
                return row;
            }

            ch = *iter;
        }

        row = iter;

        do {
            ++iter;
            if (iter == end) {
                return row;
            }

            ch = *iter;
        } while (ch_is_path_slash(ch));
    } while (true);
}

const char *path_find_ending_row_of_slashes(const char *const path) {
    const char *iter = path;
    char ch = *path;

    const char *row = NULL;

    do {
        while (!ch_is_path_slash(ch)) {
            ch = *(++iter);
            if (ch == '\0') {
                /*
                 * Return NULL as the previous characters encountered were not
                 * slashes.
                 */

                return NULL;
            }
        }

        row = iter;

        do {
            ch = *(++iter);
            if (ch == '\0') {
                return row;
            }
        } while (ch_is_path_slash(ch));
    } while (true);
}

static uint64_t
get_length_by_trimming_back_slashes(const char *const string,
                                    const uint64_t length)
{
    const char back_ch = string[length - 1];
    if (!ch_is_path_slash(back_ch)) {
        return length;
    }

    const char *iter = &string[length - 1];
    for (char ch = *(--iter); iter != string; ch = *(--iter)) {
        if (ch_is_path_slash(ch)) {
            continue;
        }

        /*
         * Add one to iter to get the pointer to the beginning of the row of
         * back-slashes.
         */

        return (iter + 1) - string;
    }

    return 0;
}

char *
path_append_component_with_len(const char *const path,
                               const uint64_t path_length,
                               const char *const component,
                               const uint64_t component_length,
                               uint64_t *const length_out)
{
    /*
     * Handle cases where either the path or component (or both) are NULL.
     * Note: strings returned must be alloacted.
     */

    if (path == NULL) {
        if (component == NULL) {
            return NULL;
        }

        return strdup(component);
    }

    /*
     * We prefer adding a back-slash ourselves.
     */

    const uint64_t path_copy_length =
        get_length_by_trimming_back_slashes(path, path_length);

    if (component == NULL) {
        return strndup(path, path_copy_length);
    }

    /*
     * We prefer either writing the back-slash ourselves, or having the end of
     * the original path be a slash, so we should remove any slash in the front
     * of component.
     */

    const char *component_iter = component;
    const char component_front = component[0];

    uint64_t component_copy_length = component_length;
    if (ch_is_path_slash(component_front)) {
        /*
         * We prefer the componet to not have a front-slash, with instead the
         * path having a back-slash, or we providing the slash ourselves.
         */
        
        component_iter = path_get_end_of_row_of_slashes(component_iter);
        if (component_iter == NULL) {
            return strndup(path, path_copy_length);
        }

        /*
         * Calculate the "drift" the path experienced when removing the row
         * of slashes in front, and re-calculate the length for the new
         * component.
         */

        const uint64_t drift = component_iter - component;
        component_copy_length -= drift;
    }

    if (path_copy_length == 0) {
        return strndup(component_iter, component_copy_length);
    } 

    /*
     * Add one for the back-slash on the path.
     */

    uint64_t combined_length = path_copy_length + component_copy_length + 1;

    /*
     * Add one to the length for the null-terminator.
     */

    char *const combined = calloc(1, combined_length + 1);
    if (combined == NULL) {
        return NULL;
    }

    char *combined_component_iter = combined + path_copy_length;
    
    /*
     * Write the slash-separator between the path and the component.
     */

    *combined_component_iter = '/';
    combined_component_iter += 1;

    memcpy(combined, path, path_copy_length);
    memcpy(combined_component_iter, component_iter, component_copy_length);

    if (length_out != NULL) {
        *length_out = combined_length;
    }

    return combined;
}

static const char *go_to_end_of_dots(const char *const dots) {
    const char *iter = dots;
    for (char ch = *iter; ch != '\0'; ch = *(++iter)) {
        if (ch == '.') {
            continue;
        }

        return iter;
    }

    return NULL;
}

char *
path_append_component_and_extension_with_len(const char *const path,
                                             const uint64_t path_length,
                                             const char *const component,
                                             const uint64_t component_length,
                                             const char *const extension,
                                             const uint64_t extension_length,
                                             uint64_t *const length_out)
{
    /*
     * Handle cases where either the path or component (or both) are NULL.
     * Note: strings returned must be alloacted.
     */

    if (path == NULL) {
        if (component == NULL) {
            return NULL;
        }

        return strdup(component);
    }

    /*
     * We prefer adding a back-slash ourselves.
     */

    const uint64_t path_copy_length =
        get_length_by_trimming_back_slashes(path, path_length);

    if (component == NULL) {
        return strndup(path, path_copy_length);
    }

    /*
     * We prefer either writing the back-slash ourselves, or having the end of
     * the original path be a slash, so we should remove any slash in the front
     * of component.
     */

    const char *component_iter = component;
    const char component_front = component[0];

    uint64_t component_copy_length = component_length;
    if (ch_is_path_slash(component_front)) {
        /*
         * We prefer the componet to not have a front-slash, with instead the
         * path having a back-slash, or we providing the slash ourselves.
         */
        
        component_iter = path_get_end_of_row_of_slashes(component_iter);
        if (component_iter == NULL) {
            return strndup(path, path_copy_length);
        }

        /*
         * Calculate the "drift" the path experienced when removing the row
         * of slashes in front, and re-calculate the length for the new
         * component.
         */

        const uint64_t drift = component_iter - component;
        component_copy_length -= drift;
    }

    if (path_copy_length == 0) {
        return strndup(component_iter, component_copy_length);
    }

    /*
     * An extension may be provided without having a row of dots in front, which
     * needs to be accounted for.
     */

    const char *extension_copy_iter = extension;
    uint64_t extension_copy_length = extension_length;

    if (extension != NULL) {
        extension_copy_iter = go_to_end_of_dots(extension);
        if (extension_copy_iter != NULL) {
            const uint64_t drift = extension_copy_iter - extension;
            extension_copy_length -= drift;
        } else {
            extension_copy_length = 0;
        }
    }

    /*
     * Add one for the back-slash on the path.
     */

    uint64_t combined_length =
        path_copy_length + component_copy_length + extension_copy_length + 1;

    /*
     * Add one for the extension-dot (if we have one).
     */

    if (extension_copy_length != 0) {
        combined_length += 1;
    }

    /*
     * Add one for the null-terminator.
     */

    char *const combined = calloc(1, combined_length + 1);
    if (combined == NULL) {
        return NULL;
    }

    char *combined_component_iter = combined + path_copy_length;

    /*
     * Write the slash-separator between the path and the component.
     */
    
    *combined_component_iter = '/';
    combined_component_iter += 1;

    memcpy(combined, path, path_copy_length);
    memcpy(combined_component_iter, component_iter, component_copy_length);

    if (extension_copy_length != 0) {
        char *combined_extension_iter =
            combined_component_iter + component_copy_length;

        /*
         * Add a dot before actually writing out the extension.
         */

        *combined_extension_iter = '.';
        combined_extension_iter += 1;

        memcpy(combined_extension_iter,
               extension_copy_iter,
               extension_copy_length);
    }

    if (length_out != NULL) {
        *length_out = combined_length;
    }

    return combined;
}

const char *
path_get_last_path_component(const char *const path,
                             const uint64_t path_length,
                             uint64_t *const length_out)
{
    const char *component_end = &path[path_length];
    const char back_ch = path[path_length - 1];
    
    if (ch_is_path_slash(back_ch)) {
        const char *const back = &path[path_length - 1];
        component_end =
            path_get_iter_before_front_of_row_of_slashes(path, back);
    
        /*
         * If we get NULL, the entire path-string is just path-slashes.
         */

        if (component_end == NULL) {
            return NULL;
        }
    }

    /*
     * To get the beginning of the last-path-component, find the last row of
     * slashes, before any row of slashes that end the string (if present).
     */

    const char *component_begin =
        path_find_last_row_of_slashes_before_end(path, component_end);

    component_begin = path_get_end_of_row_of_slashes(component_begin);
    *length_out = component_end - component_begin;

    return component_begin;
}

static bool component_is_in_hierarchy(const char *const component_end) {
    const char *const next_slash = path_get_next_slash(component_end);
    if (next_slash != NULL) {
        /*
         * We may have hit a row of slashes at the very end of the
         * path-string.
         */

        const char *const end = path_get_end_of_row_of_slashes(next_slash);
        if (end != NULL) {
            return true;
        }
    }

    return false;
}

bool
path_has_component(const char *const path,
                   const char *const component,
                   const uint64_t component_length,
                   const bool allow_in_hierarchy)
{
    const char path_front = path[0];
    if (strcmp(component, "/") == 0) {
        if (ch_is_path_slash(path_front)) {
            return true;
        }
    }

    const char *iter_begin = path;
    if (ch_is_path_slash(path_front)) {
        /*
         * If path is simply a row of slashes, we have no match unless component
         * is also a row of slashes.
         */

        iter_begin = path_get_end_of_row_of_slashes(path);
        if (iter_begin == NULL) {
            const char component_front = component[0];
            if (ch_is_path_slash(component_front)) {
                const char *const end = path_get_end_of_row_of_slashes(path);
                if (end == NULL) {
                    return true;
                }
            }

            return false;
        }
    }

    const char *iter_end = path_get_next_slash_or_end(iter_begin);

    do {
        const uint64_t iter_length = iter_end - iter_begin;
        if (component_length == iter_length) {
            if (strncmp(iter_begin, component, iter_length) == 0) {
                if (component_is_in_hierarchy(iter_end)) {
                    if (allow_in_hierarchy) {
                        return true;
                    }
                } else {
                    return true;
                }
            }
        }

        iter_begin = path_get_end_of_row_of_slashes(iter_end);
        if (iter_begin == NULL) {
            return NULL;
        }

        iter_end = path_get_next_slash_or_end(iter_begin);
        if (iter_end == NULL) {
            return NULL;
        }
    } while (true);

    return false;
}
