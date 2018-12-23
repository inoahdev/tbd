//
//  src/path.c
//  tbd
//
//  Created by inoahdev on 11/19/18.
//  Copyright © 2018 inoahdev. All rights reserved.
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

char *path_get_absolute_path_if_necessary(const char *const path) {
    const char path_front = path[0];
    if (path_front == '/') {
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
                                       strlen(path));

    return combined;
}

static inline bool path_is_slash_ch(const char ch) {
    return ch == '/';
}

const char *path_get_end_of_row_of_slashes(const char *const path) {
    const char *iter = path;
    char ch = *path;

    do {
        ch = *(++iter);
        if (ch == '\0') {
            return NULL;
        }
    } while (path_is_slash_ch(ch));

    return iter;
}

const char *path_find_last_row_of_slashes(const char *const path) {
    const char *iter = path;
    char ch = *path;

    const char *row = NULL;

    do {
        while (!path_is_slash_ch(ch)) {
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
        } while (path_is_slash_ch(ch));
    } while (true);
}

const char *path_find_ending_row_of_slashes(const char *const path) {
    const char *iter = path;
    char ch = *path;

    const char *row = NULL;

    do {
        while (!path_is_slash_ch(ch)) {
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
        } while (path_is_slash_ch(ch));
    } while (true);
}

static const char *
get_iter_before_begin_of_row_of_slashes(const char *const path,
                                        const char *const slashes)
{
    const char *iter = slashes;
    for (char ch = *(--iter); iter != path; ch = *(--iter)) {
        if (ch == '/') {
            continue;
        }

        return iter + 1;
    }

    return NULL;
}

char *
path_append_component_with_len(const char *const path,
                               const uint64_t path_length,
                               const char *const component,
                               const uint64_t component_length)
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
    } else if (component == NULL) {
        return strdup(path);
    }

    /*
     * We prefer adding a back-slash ourselves.
     */

    uint64_t path_copy_length = path_length;
    if (path_is_slash_ch(path[path_length - 1])) {
        const char *const back = &path[path_length - 1];
        const char *const end =
            get_iter_before_begin_of_row_of_slashes(path, back);
       
        if (end == NULL) {
            return strdup(component);
        }

        path_copy_length = end - path;
    }

    /*
     * We prefer either writing the back-slash ourselves, or having the end of
     * the original path be a slash, so we should remove any slash in the front
     * of component.
     */

    const char *component_iter = component;
    const char component_front = component[0];

    uint64_t component_copy_length = component_length;
    if (path_is_slash_ch(component_front)) {
        /*
         * We prefer the componet to not have a front-slash, with instead the
         * path having a back-slash, or we providing the slash ourselves.
         */
        
        component_iter = path_get_end_of_row_of_slashes(component_iter);
        if (component_iter == NULL) {
            return strdup(path);
        }

        /*
         * Calculate the "drift" the path experienced when removing the row
         * of slashes in front, and re-calculate the length for the new
         * component.
         */

        const uint64_t drift = component_iter - component;
        component_copy_length -= drift;
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

    /*
     * If needed, write the slash-separator between the original path and the
     * provided component before writing the original path and the component.
     */

    char *combined_component_iter = combined + path_copy_length;
    
    *combined_component_iter = '/';
    combined_component_iter += 1;

    memcpy(combined, path, path_copy_length);
    memcpy(combined_component_iter, component_iter, component_copy_length);

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

    return iter;
}

char *
path_append_component_and_extension_with_len(const char *const path,
                                             const uint64_t path_length,
                                             const char *const component,
                                             const uint64_t component_length,
                                             const char *const extension,
                                             const uint64_t extension_length)
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
    } else if (component == NULL) {
        return strdup(path);
    }

    /*
     * We prefer adding a back-slash ourselves.
     */

    uint64_t path_copy_length = path_length;
    if (path_is_slash_ch(path[path_length - 1])) {
        const char *const back = &path[path_length - 1];
        const char *const end =
            get_iter_before_begin_of_row_of_slashes(path, back);
       
        if (end == NULL) {
            return strdup(component);
        }

        path_copy_length = end - path;
    }

    /*
     * We prefer either writing the back-slash ourselves, or having the end of
     * the original path be a slash, so we should remove any slash in the front
     * of component.
     */

    const char *component_iter = component;
    const char component_front = component[0];

    uint64_t component_copy_length = component_length;
    if (path_is_slash_ch(component_front)) {
        /*
         * We prefer the componet to not have a front-slash, with instead the
         * path having a back-slash, or we providing the slash ourselves.
         */
        
        component_iter = path_get_end_of_row_of_slashes(component_iter);
        if (component_iter == NULL) {
            return strdup(path);
        }

        /*
         * Calculate the "drift" the path experienced when removing the row
         * of slashes in front, and re-calculate the length for the new
         * component.
         */

        const uint64_t drift = component_iter - component;
        component_copy_length -= drift;
    }

    /*
     * An extension may be provided without having a row of dots in front, which
     * needs to be accounted for.
     */

    const char *extension_copy_iter = extension;
    uint64_t extension_copy_length = extension_length;

    if (extension != NULL) {
        const char *const dot = go_to_end_of_dots(extension);
        const uint64_t drift = dot - extension;

        extension_copy_iter = dot;
        extension_copy_length -= drift;
    }

    /*
     * Add one for the back-slash on the path, and one for the extension-dot.
     */

    uint64_t combined_length =
        path_copy_length + component_copy_length + extension_copy_length + 2;

    /*
     * Add one to the length for the null-terminator.
     */

    char *const combined = calloc(1, combined_length + 1);
    if (combined == NULL) {
        return NULL;
    }

    /*
     * If needed, write the slash-separator between the original path and the
     * provided component before writing the original path and the component.
     */

    char *combined_component_iter = combined + path_copy_length;
    
    *combined_component_iter = '/';
    combined_component_iter += 1;

    memcpy(combined, path, path_copy_length);
    memcpy(combined_component_iter, component_iter, component_copy_length);

    if (extension != NULL) {
        char *combined_extension_iter =
            combined_component_iter + component_copy_length;

        /*
         * Only copy the front dot if one doesn't exists already. 
         */

        *combined_extension_iter = '.';
        combined_extension_iter += 1;

        memcpy(combined_extension_iter,
               extension_copy_iter,
               extension_copy_length);
    }

    return combined;
}