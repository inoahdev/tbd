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

#include "copy.h"
#include "path.h"

static const char *current_directory = NULL;
static size_t current_directory_length = 0;

static inline bool ch_is_path_slash(const char ch) {
    return ch == '/';
}

char *
path_get_absolute_path_if_necessary(const char *const path,
                                    const uint64_t path_length,
                                    uint64_t *const length_out)
{
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
                                       path_length,
                                       length_out);

    return combined;
}

const char *
path_get_front_of_row_of_slashes(const char *const path, const char *iter) {
    /*
     * If we're already at the front of the entire string, simply return the
     * string.
     */

    if (iter == path) {
        return iter;
    }

    char ch = '\0';

    do {
        --iter;
        if (iter < path) {
            return NULL;
        }

        ch = *iter;
    } while (ch_is_path_slash(ch));

    return iter + 1;
}

const char *path_get_end_of_row_of_slashes(const char *const path) {
    const char *iter = path;
    char ch = '\0';

    do {
        ch = *(++iter);
        if (ch == '\0') {
            return NULL;
        }
    } while (ch_is_path_slash(ch));

    return iter;
}

const char *
path_find_last_row_of_slashes(const char *const path,
                              const uint64_t path_length)
{
    const char *back = path + (path_length - 1);
    for (; back >= path; back--) {
        const char ch = *back;
        if (ch_is_path_slash(ch)) {
            return path_get_front_of_row_of_slashes(path, back);
        }
    }

    return NULL;
}

const char *
path_find_back_of_last_row_of_slashes(const char *const path,
                                      const uint64_t path_length)
{
    const char *back = path + (path_length - 1);
    for (; back >= path; back--) {
        const char ch = *back;
        if (!ch_is_path_slash(ch)) {
            continue;
        }

        return back;
    }

    return NULL;
}

const char *
path_find_last_row_of_slashes_before_end(const char *const path,
                                         const char *const end)
{
    const char *iter = end;
    for (char ch = *(--iter); iter >= path; ch = *(--iter)) {
        if (ch_is_path_slash(ch)) {
            return path_get_front_of_row_of_slashes(path, iter);
        }
    }

    return NULL;
}

const char *
path_find_back_of_last_row_of_slashes_before_end(const char *const path,
                                                 const char *const end)
{
    const char *iter = end;
    for (char ch = *(--iter); iter >= path; ch = *(--iter)) {
        if (ch_is_path_slash(ch)) {
            return iter;
        }
    }

    return NULL;
}

const char *
path_find_ending_row_of_slashes(const char *const path, const uint64_t length) {
    const char *const back = path + (length - 1);
    const char ch = *back;

    if (!ch_is_path_slash(ch)) {
        return NULL;
    }

    return path_get_front_of_row_of_slashes(path, back);
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
    for (char ch = *(--iter); iter >= string; ch = *(--iter)) {
        if (ch_is_path_slash(ch)) {
            continue;
        }

        /*
         * Add one to iter to get the pointer to the beginning of the row of
         * back-slashes.
         */

        const uint64_t trimmed_length = (uint64_t)(++iter - string);
        return trimmed_length;
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
     * We prefer adding a back-slash ourselves.
     */

    const uint64_t path_copy_length =
        get_length_by_trimming_back_slashes(path, path_length);

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
            return alloc_and_copy(path, path_copy_length);
        }

        /*
         * Calculate the "drift" the path experienced when removing the row
         * of slashes in front, and re-calculate the length for the new
         * component.
         */

        const uint64_t drift = (uint64_t)(component_iter - component);
        component_copy_length -= drift;
    }

    if (path_copy_length == 0) {
        return alloc_and_copy(component_iter, component_copy_length);
    }

    /*
     * Add one for the back-slash on the path.
     */

    uint64_t combined_length = path_copy_length + component_copy_length + 1;

    /*
     * Add one to the length for the null-terminator.
     */

    char *const combined = malloc(combined_length + 1);
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

    combined[combined_length] = '\0';
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
     * We prefer adding a back-slash ourselves.
     */

    const uint64_t path_copy_length =
        get_length_by_trimming_back_slashes(path, path_length);

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
            return alloc_and_copy(path, path_copy_length);
        }

        /*
         * Calculate the "drift" the path experienced when removing the row
         * of slashes in front, and re-calculate the length for the new
         * component.
         */

        const uint64_t drift = (uint64_t)(component_iter - component);
        component_copy_length -= drift;
    }

    if (path_copy_length == 0) {
        return alloc_and_copy(component_iter, component_copy_length);
    }

    /*
     * An extension may be provided without having a row of dots in front, which
     * needs to be accounted for.
     */

    const char *extension_copy_iter = extension;
    uint64_t extension_copy_length = 0;

    if (extension != NULL) {
        extension_copy_iter = go_to_end_of_dots(extension);
        if (extension_copy_iter != NULL) {
            const uint64_t drift = (uint64_t)(extension_copy_iter - extension);
            extension_copy_length = extension_length - drift;
        }
    }

    /*
     * Add one for the back-slash on the path.
     */

    uint64_t combined_length = path_copy_length + component_copy_length + 1;

    /*
     * Add one for the extension-dot.
     */

    if (extension_copy_length != 0) {
        combined_length += extension_copy_length + 1;
    }

    /*
     * Add one for the null-terminator.
     */

    char *const combined = malloc(combined_length + 1);
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
        char *const combined_extension_iter =
            combined_component_iter + component_copy_length;

        /*
         * Add one for the extension-dot.
         */

        memcpy(combined_extension_iter + 1,
               extension_copy_iter,
               extension_copy_length);

        *combined_extension_iter = '.';
    }

    if (length_out != NULL) {
        *length_out = combined_length;
    }

    combined[combined_length] = '\0';
    return combined;
}

const char *
path_get_last_path_component(const char *const path,
                             const uint64_t path_length,
                             uint64_t *const length_out)
{
    const char path_front_ch = *path;
    if (!ch_is_path_slash(path_front_ch)) {
        *length_out = path_length;
        return path;
    }

    const char *component_end = &path[path_length];
    const char back_ch = path[path_length - 1];

    if (ch_is_path_slash(back_ch)) {
        const char *const back = component_end - 1;
        component_end = path_get_front_of_row_of_slashes(path, back);

        /*
         * If we get a pointer back to path, the path-string is just slashes.
         */

        if (component_end == path) {
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
    *length_out = (uint64_t)(component_end - component_begin);

    return component_begin;
}

const char *
path_get_next_component(const char *const component,
                        const uint64_t component_length)
{
    const char *const next_component = component + component_length;
    if (ch_is_path_slash(*next_component)) {
        return path_get_end_of_row_of_slashes(next_component);
    }

    return next_component;
}

static const char *get_next_slash(const char *const path) {
    const char *iter = path;
    for (char ch = *iter; ch != '\0'; ch = *(++iter)) {
        if (ch_is_path_slash(ch)) {
            return iter;
        }
    }

    return NULL;
}

static bool component_is_a_directory(const char *const component_end) {
    /*
     * We have a directory if there's no null-terminator at component-end, and
     * component_end doesn't point to a row of slashes leading to a
     * null-terminator.
     */

    const char *const next_slash = get_next_slash(component_end);
    if (next_slash != NULL) {
        const char *const end = path_get_end_of_row_of_slashes(next_slash);
        if (end != NULL) {
            return true;
        }
    }

    return false;
}

static const char *get_next_slash_or_end(const char *const path) {
    const char *iter = path;
    char ch = *iter;

    if (ch == '\0') {
        return iter;
    }

    do {
        if (ch_is_path_slash(ch)) {
            break;
        }

        ch = *(++iter);
    } while (ch != '\0');

    return iter;
}

bool
path_has_dir_component(const char *const path,
                       const char *const component,
                       const uint64_t component_length,
                       const char **const dir_component_out)
{
    const char *iter_begin = path;
    if (ch_is_path_slash(path[0])) {
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

    const char *iter_end = get_next_slash_or_end(iter_begin);

    do {
        const uint64_t iter_length = (uint64_t)(iter_end - iter_begin);
        if (component_length == iter_length) {
            if (memcmp(iter_begin, component, iter_length) == 0) {
                if (component_is_a_directory(iter_end)) {
                    if (dir_component_out != NULL) {
                        *dir_component_out = iter_begin;
                    }

                    return true;
                }

                return false;
            }
        }

        iter_begin = path_get_end_of_row_of_slashes(iter_end);
        if (iter_begin == NULL) {
            return NULL;
        }

        iter_end = get_next_slash_or_end(iter_begin);
    } while (true);

    return false;
}

bool
path_has_filename(const char *const path,
                  const uint64_t path_length,
                  const char *const filename,
                  const uint64_t filename_length,
                  const char **const filename_out)
{
    const char *const path_back = path + (path_length - 1);
    const char *path_iter = path_back;

    if (*path_back == '/') {
        const char *const row_front =
            path_get_front_of_row_of_slashes(path, path_back);

        /*
         * If the first character is a slash, then the entire string is a row of
         * slashes.
         */

        if (row_front == path) {
            return false;
        }

        path_iter = row_front - 1;
    }

    const char *filename_iter = filename + (filename_length - 1);

    char path_ch = *path_iter;
    char filename_ch = *filename_iter;

    do {
        if (path_ch != filename_ch) {
            return false;
        }

        --path_iter;
        --filename_iter;

        /*
         * We have reached the "reverse version" of the null-terminator if the
         * filename_iter is located before filename.
         */

        if (path_iter == path) {
            if (filename_iter < filename) {
                if (filename_out != NULL) {
                    *filename_out = path_iter;
                }

                return true;
            }

            return false;
        }

        path_ch = *path_iter;
        if (path_ch == '/') {
            if (filename_iter < filename) {
                if (filename_out != NULL) {
                    *filename_out = path_iter + 1;
                }

                return true;
            }

            return false;
        }

        if (filename_iter < filename) {
            return false;
        }

        filename_ch = *filename_iter;
    } while (true);

    return true;
}

const char *
path_find_extension(const char *const path, const uint64_t length) {
    const char *const back = path + (length - 1);
    const char *iter = back;

    /*
     * If we have a row of slashes at the back of the path-string, remove them
     * from our parsing range.
     */

    char ch = *iter;
    if (ch_is_path_slash(ch)) {
        iter = path_get_front_of_row_of_slashes(path, iter);
        if (iter == path) {
            return NULL;
        }

        iter -= 1;
        ch = *iter;
    }

    for (ch = *(--iter); iter >= path; ch = *(--iter)) {
        /*
         * If we hit a path-slash, we are about to leave the range of the last
         * path-component without a dot.
         */

        if (ch_is_path_slash(ch)) {
            return NULL;
        }

        if (ch != '.') {
            continue;
        }

        /*
         * We haven't found an extension if the dot is at the front of the path.
         */

        if (iter == path) {
            return NULL;
        }

        /*
         * We haven't found an extension if the path-component itself starts
         * with a dot.
         */

        ch = *(iter - 1);
        if (ch_is_path_slash(ch)) {
            return NULL;
        }

        return iter;
    }

    return NULL;
}
