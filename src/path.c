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
#include "likely.h"
#include "path.h"

static const char *current_directory = NULL;
static size_t current_directory_length = 0;

static inline bool ch_is_path_slash(const char ch) {
    return ch == '/';
}

char *
path_get_absolute_path(const char *__notnull const path,
                       uint64_t path_length,
                       uint64_t *const length_out)
{
    if (ch_is_path_slash(path[0])) {
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

    if (path_length == 0) {
        path_length = strlen(path);
    }

    char *const combined =
        path_append_component_with_len(current_directory,
                                       current_directory_length,
                                       path,
                                       path_length,
                                       length_out);

    return combined;
}

char *
path_append_component_with_len(const char *__notnull const path,
                               const uint64_t path_length,
                               const char *__notnull component,
                               const uint64_t component_length,
                               uint64_t *const length_out)
{
    /*
     * We prefer adding a back-slash ourselves.
     */

    const uint64_t path_copy_length =
        path_get_length_by_removing_end_slashes(path, path_length);

    /*
     * We prefer either writing the back-slash ourselves, or having the end of
     * the original path be a slash, so we should remove any slash in the front
     * of component.
     */

    const char *component_iter = component;
    uint64_t component_copy_length = component_length;

    if (ch_is_path_slash(component[0])) {
        /*
         * We prefer the componet to not have a front-slash, with instead the
         * path having a back-slash, or we providing the slash ourselves.
         */

        component_iter = path_get_end_of_row_of_slashes(component_iter);
        if (unlikely(component_iter == NULL)) {
            return alloc_and_copy(path, path_copy_length);
        }

        /*
         * Update the copy-length by subtracting the "drift" the component
         * experienced.
         */

        const uint64_t drift = (uint64_t)(component_iter - component);
        component_copy_length -= drift;

        /*
         * Remove any back-slashes if we have any.
         */

        component_copy_length =
            path_get_length_by_removing_end_slashes(component_iter,
                                                    component_copy_length);
    }

    if (unlikely(path_copy_length == 0)) {
        return alloc_and_copy(component_iter, component_copy_length);
    }

    /*
     * Add one for the slash-separator between path and component.
     */

    uint64_t combined_length = path_copy_length + component_copy_length + 1;

    /*
     * Add one to the length for the null-terminator.
     */

    char *const combined = malloc(combined_length + 1);
    if (unlikely(combined == NULL)) {
        return NULL;
    }

    char *combined_component_iter = combined + path_copy_length;

    /*
     * Write the slash-separator between the path and the component.
     */

    *combined_component_iter = '/';

    memcpy(combined, path, path_copy_length);
    memcpy(combined_component_iter + 1, component_iter, component_copy_length);

    if (length_out != NULL) {
        *length_out = combined_length;
    }

    combined[combined_length] = '\0';
    return combined;
}

static const char *go_to_end_of_dots(const char *__notnull const dots) {
    const char *iter = dots;
    for (char ch = *iter; ch != '\0'; ch = *(++iter)) {
        if (ch != '.') {
            return iter;
        }
    }

    return NULL;
}

char *
path_append_component_and_extension_with_len(
    const char *__notnull const path,
    const uint64_t path_length,
    const char *__notnull const component,
    const uint64_t component_length,
    const char *__notnull const extension,
    const uint64_t extension_length,
    uint64_t *const length_out)
{
    /*
     * We prefer adding a back-slash ourselves.
     */

    const uint64_t path_copy_length =
        path_get_length_by_removing_end_slashes(path, path_length);

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
        if (unlikely(component_iter == NULL)) {
            return alloc_and_copy(path, path_copy_length);
        }

        /*
         * Update our copy-length by subtracting the "drift" the string
         * experienced.
         */

        const uint64_t drift = (uint64_t)(component_iter - component);
        component_copy_length -= drift;

        /*
         * Remove any existing slashes at the back of our component.
         */

        component_copy_length =
            path_get_length_by_removing_end_slashes(component_iter,
                                                    component_copy_length);
    }

    if (unlikely(path_copy_length == 0)) {
        return alloc_and_copy(component_iter, component_copy_length);
    }

    /*
     * An extension may be provided without having a row of dots in front, which
     * needs to be accounted for.
     */

    const char *extension_copy_iter = extension;
    uint64_t extension_copy_length = 0;

    extension_copy_iter = go_to_end_of_dots(extension);
    if (extension_copy_iter != NULL) {
        const uint64_t drift = (uint64_t)(extension_copy_iter - extension);
        extension_copy_length = extension_length - drift;
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
    if (unlikely(combined == NULL)) {
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

char *
path_append_two_components_and_extension_with_len(
    const char *__notnull const path,
    const uint64_t path_length,
    const char *__notnull const first_component,
    const uint64_t first_component_length,
    const char *__notnull const second_component,
    const uint64_t second_component_length,
    const char *__notnull const extension,
    const uint64_t extension_length,
    uint64_t *const length_out)
{
    /*
     * We prefer adding a back-slash ourselves.
     */

    const uint64_t path_copy_length =
        path_get_length_by_removing_end_slashes(path, path_length);

    /*
     * We prefer either writing the back-slash ourselves, or having the end of
     * the original path be a slash, so we should remove any slash in the front
     * of component.
     */

    const char *first_component_iter = first_component;
    uint64_t first_component_copy_length = first_component_length;

    if (ch_is_path_slash(first_component[0])) {
        /*
         * We add a front-slash to the path-component, so we remove any existing
         * front-slashes here.
         */

        first_component_copy_length =
            path_get_length_by_removing_end_slashes(
                first_component_iter,
                first_component_copy_length);
    }

    const char *second_component_iter = second_component;
    uint64_t second_component_copy_length = second_component_length;

    if (ch_is_path_slash(second_component[0])) {
        second_component_copy_length =
            path_get_length_by_removing_end_slashes(
                second_component_iter,
                second_component_copy_length);
    }

    if (unlikely(path_copy_length == 0)) {
        char *result = NULL;
        if (first_component_copy_length == 0) {
            if (second_component_copy_length == 0) {
                return NULL;
            }

            *length_out = second_component_copy_length;
            result = alloc_and_copy(second_component_iter,
                                    second_component_copy_length);
        } else if (second_component_copy_length == 0) {
            *length_out = first_component_copy_length;
            result = alloc_and_copy(first_component_iter,
                                    first_component_copy_length);
        }

        return result;
    }

    if (unlikely(first_component_copy_length == 0)) {
        if (second_component_copy_length == 0) {
            return alloc_and_copy(path, path_copy_length);
        }

        char *const result =
            path_append_component_with_len(path,
                                           path_copy_length,
                                           second_component_iter,
                                           second_component_copy_length,
                                           length_out);

        return result;
    }

    if (unlikely(second_component_copy_length == 0)) {
        char *const result =
            path_append_component_with_len(path,
                                           path_copy_length,
                                           first_component_iter,
                                           first_component_copy_length,
                                           length_out);

        return result;
    }

    /*
     * An extension may be provided without having a row of dots in front, which
     * needs to be accounted for.
     */

    const char *extension_copy_iter = extension;
    uint64_t extension_copy_length = 0;

    /*
     * We prefer to add the dot ourselves.
     */

    extension_copy_iter = go_to_end_of_dots(extension);
    if (extension_copy_iter != NULL) {
        const uint64_t drift = (uint64_t)(extension_copy_iter - extension);
        extension_copy_length = extension_length - drift;
    }

    /*
     * Add one for the back-slash on the path, and one for the back-slash on the
     * first-component.
     */

    uint64_t combined_length =
        path_copy_length +
        first_component_copy_length +
        second_component_copy_length +
        2;

    /*
     * Add one for the path-extension dot, which we prefer to add ourselves.
     */

    if (extension_copy_length != 0) {
        combined_length += extension_copy_length + 1;
    }

    /*
     * Add one to the length for the null-terminator.
     */

    char *const combined = malloc(combined_length + 1);
    if (unlikely(combined == NULL)) {
        return NULL;
    }

    char *first_combined_component_iter = combined + path_copy_length;
    char *second_combined_component_iter =
        first_combined_component_iter + first_component_copy_length + 1;

    /*
     * Write the slash-separator between the path and the component.
     */

    *first_combined_component_iter = '/';
    first_combined_component_iter += 1;

    *second_combined_component_iter = '/';
    second_combined_component_iter += 1;

    memcpy(combined, path, path_copy_length);
    memcpy(first_combined_component_iter,
           first_component_iter,
           first_component_copy_length);

    memcpy(second_combined_component_iter,
           second_component_iter,
           second_component_copy_length);

    if (extension_copy_length != 0) {
        char *const combined_extension_iter =
            second_combined_component_iter + second_component_copy_length;

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
path_get_front_of_row_of_slashes(const char *__notnull const path,
                                 const char *__notnull iter)
{
    const char *const rev_end = path - 1;
    for (--iter; iter != rev_end; --iter) {
        if (!ch_is_path_slash(*iter)) {
            break;
        }
    }

    return (iter + 1);
}

const char *path_get_end_of_row_of_slashes(const char *__notnull const path) {
    const char *iter = path;
    for (char ch = *(++iter); ch != '\0'; ch = *(++iter)) {
        if (!ch_is_path_slash(ch)) {
            return iter;
        }
    }

    return NULL;
}

const char *
path_find_last_row_of_slashes(const char *__notnull const path,
                              const uint64_t path_length)
{
    const char *iter = path + (path_length - 1);
    const char *const rev_end = path - 1;

    for (; iter != rev_end; --iter) {
        if (ch_is_path_slash(*iter))  {
            return path_get_front_of_row_of_slashes(path, iter);
        }
    }

    return NULL;
}

const char *
path_find_last_slash(const char *__notnull const path,
                     const uint64_t path_length)
{
    const char *iter = path + (path_length - 1);
    const char *const rev_end = path - 1;

    for (; iter != rev_end; --iter) {
        if (ch_is_path_slash(*iter)) {
            return iter;
        }
    }

    return NULL;
}

const char *
path_find_last_row_of_slashes_before_end(const char *__notnull const path,
                                         const char *__notnull const end)
{
    const char *iter = end - 1;
    const char *const rev_end = path - 1;

    for (; iter != rev_end; --iter) {
        if (ch_is_path_slash(*iter)) {
            return path_get_front_of_row_of_slashes(path, iter);
        }
    }

    return NULL;
}

uint64_t
path_get_length_by_removing_end_slashes(const char *__notnull const string,
                                        const uint64_t length)
{
    const char *iter = string + (length - 1);
    const char *const rev_end = string - 1;

    for (; iter != rev_end; --iter) {
        if (ch_is_path_slash(*iter)) {
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

static bool
component_is_a_directory(const char *__notnull const component_end) {
    /*
     * We have a directory if component_end doesn't point to a row of slashes
     * leading to a null-terminator.
     */

    const char *const end = path_get_end_of_row_of_slashes(component_end);
    return (end != NULL);
}

static const char *get_next_slash_or_end(const char *__notnull const path) {
    const char *iter = path;
    for (char ch = *(++iter); ch != '\0'; ch = *(++iter)) {
        if (ch_is_path_slash(ch)) {
            break;
        }
    }

    return iter;
}

bool
path_has_dir_component(const char *__notnull const path,
                       const uint64_t path_length,
                       const char *__notnull const component,
                       const uint64_t component_length,
                       const char **const dir_component_out)
{
    const char *iter = path;
    if (ch_is_path_slash(*path)) {
        /*
         * If path is simply a row of slashes, we have no match unless component
         * is also a row of slashes.
         */

        iter = path_get_end_of_row_of_slashes(path);
        if (iter == NULL) {
            if (ch_is_path_slash(*component)) {
                const char *const end = path_get_end_of_row_of_slashes(path);
                if (end == NULL) {
                    return true;
                }
            }

            return false;
        }
    }

    /*
     * Add one for the slash following the component, which needs to exist
     * as we're checking for a directory.
     */

    const uint64_t full_component_length = component_length + 1;
    const char *const path_end = path + path_length;

    do {
        const uint64_t max_iter_length = (uint64_t)(path_end - iter);
        if (max_iter_length < full_component_length) {
            return false;
        }

        if (memcmp(iter, component, component_length) != 0) {
            const char *const iter_end = get_next_slash_or_end(iter);
            if (iter_end == NULL) {
                return false;
            }

            iter = path_get_end_of_row_of_slashes(iter_end);
            if (iter == NULL) {
                return false;
            }

            continue;
        }

        /*
         * We may have matched with a path-component that has our component as a
         * prefix.
         *
         * Ex: "loc" vs "local"
         */

        const char *const iter_end = iter + component_length;
        if (!ch_is_path_slash(*iter_end)) {
            continue;
        }

        if (!component_is_a_directory(iter_end)) {
            return false;
        }

        if (dir_component_out != NULL) {
            *dir_component_out = iter;
        }

        return true;
    } while (true);
}

bool
path_has_filename(const char *__notnull const path,
                  const uint64_t path_length,
                  const char *__notnull const filename,
                  const uint64_t filename_length,
                  const char **const filename_out)
{
    const char *const path_back = path + (path_length - 1);
    const char *path_iter = path_back;

    if (ch_is_path_slash(*path_back)) {
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

    /*
     * We iterate and compare from the back to save on time.
     */

    const char *filename_iter = filename + (filename_length - 1);

    char path_ch = *path_iter;
    char filename_ch = *filename_iter;

    do {
        if (path_ch != filename_ch) {
            return false;
        }

        --path_iter;
        --filename_iter;

        if (path_iter == path) {
            /*
             * We have reached the "reverse version" of the null-terminator if
             * the filename_iter is located before filename.
             */

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

uint64_t
path_get_length_by_removing_extension(const char *__notnull path,
                                      const uint64_t length)
{
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
            return length;
        }

        iter -= 1;
    }

    const char *const rev_end = path - 1;
    for (ch = *(--iter); iter != rev_end; ch = *(--iter)) {
        /*
         * If we hit a path-slash, we are about to leave the range of the last
         * path-component without a dot.
         */

        if (ch_is_path_slash(ch)) {
            break;
        }

        if (ch != '.') {
            continue;
        }

        /*
         * We haven't found an extension if the dot is at the front of the path.
         */

        if (iter == path) {
            break;
        }

        /*
         * We haven't found an extension if the path-component itself starts
         * with a dot.
         */

        ch = iter[-1];
        if (ch_is_path_slash(ch)) {
            break;
        }

        const uint64_t new_length = (uint64_t)(iter - path);
        return new_length;
    }

    return length;
}
