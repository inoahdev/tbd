//
//  src/recursive.c
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <errno.h>
#include <fcntl.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <unistd.h>

#include "path.h"
#include "recursive.h"

static
char *find_last_slash_before_end(char *const path, const char *const end) {
    return (char *)path_find_last_row_of_slashes_before_end(path, end);
}

static char *find_next_slash_skipping_first_row(char *const path) {
    char *iter = (char *)path_get_end_of_row_of_slashes(path);
    if (iter == NULL) {
        return NULL;
    }

    char ch = '\0';

    do {
        ch = *(++iter);
        if (ch == '\0') {
            return NULL;
        }
    } while (ch != '/');

    return iter;
}

static inline
char *find_next_slash_reverse(char *const path, char *const last_slash) {
    char *const iter =
        (char *)path_find_back_of_last_row_of_slashes_before_end(path,
                                                                 last_slash);

    return iter;
}

static inline void terminate_c_str(char *const iter) {
    *iter = '\0';
}

static inline void restore_slash_c_str(char *const iter) {
    *iter = '/';
}

static int
reverse_mkdir_ignoring_last(char *const path,
                            uint64_t path_length,
                            const mode_t mode,
                            char **const first_terminator_out)
{
    char *last_slash =
        (char *)path_find_back_of_last_row_of_slashes(path, path_length);

    /*
     * We may have a slash at the back of the string, which we should ignore
     * as terminating at that point does not lead us to the path of the
     * immediate directory in the full-path's hierarchy.
     */

    const char possible_end = last_slash[1];
    if (possible_end == '\0') {
        last_slash = (char *)path_get_front_of_row_of_slashes(path, last_slash);
        last_slash = find_next_slash_reverse(path, last_slash);
    }

    terminate_c_str(last_slash);
    const int first_ret = mkdir(path, mode);
    restore_slash_c_str(last_slash);

    if (first_ret == 0) {
        return 0;
    }

    /*
     * If the directory already exists, return successfully.
     *
     * Otherwise, if another error besides ENOENT (error given when a directory
     * in the hierarchy doesn't exist), return as fail.
     */

    if (first_ret < 0) {
        /*
         * If the directory already exists, we are done, as the previous
         * mkdir should have gone through.
         */

        if (errno == EEXIST) {
            return 0;
        }

        /*
         * errno is set to ENONENT when a previous path-component doesn't
         * exist.
         *
         * So if we get any other error, its due to another reason, and we
         * should just return immedietly.
         */

        if (errno != ENOENT) {
            return 1;
        }
    }

    /*
     * Store a pointer to the final slash we'll terminate, however, store at the
     * back of any row if one exists as we move backwards when iterating and
     * when checking against this variable.
     */

    char *const final_slash =
        (char *)path_get_front_of_row_of_slashes(path, last_slash);

    /*
     * Iterate over the path-components backwayds, finding the final slash
     * within a range of the of the previous final slash, and terminate the
     * string at that slash.
     *
     * Then try creating that directory.
     * If that succeeds, break out of the loop to then iterate forwards and
     * create the directories afterwards.
     *
     * If the directory already exists, we can simply return as it cannot happen
     * unless the second-to-last path-component already exists.
     *
     * If a directory-component doesn't exist (ENOENT), continue with the
     * iteration.
     */

    last_slash = (char *)path_get_front_of_row_of_slashes(path, last_slash);
    while (last_slash != path) {
        last_slash = find_last_slash_before_end(path, last_slash);
        if (last_slash == NULL) {
            return 1;
        }

        terminate_c_str(last_slash);
        const int ret = mkdir(path, mode);
        restore_slash_c_str(last_slash);

        /*
         * We have finished with this loop when we're finally able to create a
         * directory, in the hierarchy of the full path, without error.
         *
         * Now the job is to move back forwards, creating all the directories
         * for all the path-components from after this one to the second-to-last
         * path-component.
         */

        if (ret == 0) {
            break;
        }

        if (ret < 0) {
            /*
             * If the directory already exists, we are done, as all the previous
             * mkdir calls should have gone through.
             */

            if (errno == EEXIST) {
                return 0;
            }

            /*
             * errno is set to ENONENT when a previous path-component doesn't
             * exist.
             *
             * So if we get any other error, its due to another reason, and we
             * should just return immedietly.
            */

            if (errno != ENOENT) {
                return 1;
            }
        }
    }

    if (first_terminator_out != NULL) {
        *first_terminator_out = last_slash;
    }

    /*
     * If last_slash is equal to final_slash, we have created the last
     * path-component we needed to, and should return.
     */

    if (last_slash == final_slash) {
        return 0;
    }

    /*
     * Get the next slash following the last-slash, to get the next
     * path-component after the last only that was just created.
     */

    char *slash = find_next_slash_skipping_first_row(last_slash);

    /*
     * Iterate forwards to create path-components following the final
     * path-component created in the previous loop.
     *
     * Note: We still terminate at final_slash, and only afterwards do we stop,
     * as terminating at final_slash is needed to create the second-to-last
     * path-component.
     */

    do {
        terminate_c_str(slash);
        const int ret = mkdir(path, mode);
        restore_slash_c_str(slash);

        if (ret < 0) {
            return 1;
        }

        /*
         * Reset our slash, and if we've just created our final path-component,
         * break out and return.
         */

        if (slash == final_slash) {
            break;
        }

        slash = find_next_slash_skipping_first_row(slash);
    } while (true);

    return 0;
}

int
open_r(char *const path,
       const uint64_t length,
       const int flags,
       const mode_t mode,
       const mode_t dir_mode,
       char **const terminator_out)
{
    int fd = open(path, O_CREAT | flags, mode);
    if (fd >= 0) {
        return fd;
    }

    /*
     * The only allowed error is ENOENT (when a directory in the hierarchy
     * doesn't exist, which is the whole point of this function).
     *
     * Other errors may be due to permissions error and the sort, but which are
     * beyond the scope of this function.
     */

    if (errno != ENOENT) {
        return -1;
    }

    if (reverse_mkdir_ignoring_last(path, length, dir_mode, terminator_out)) {
        return -1;
    }

    fd = open(path, O_CREAT | flags, mode);
    if (fd < 0) {
        return -1;
    }

    return fd;
}

int
mkdir_r(char *const path,
        const uint64_t length,
        const mode_t mode,
        char **const first_terminator_out)
{
    if (mkdir(path, mode) == 0) {
        return 0;
    }

    /*
     * If the directory already exists, return as a success.
     *
     * Note: To avoid multiple syscalls, we call mkdir() knowing that the
     * directory may already exist, preferring to check errno instead.
     */

    if (errno == EEXIST) {
        return 0;
    }

    /*
     * The only allowed error is ENOENT (when a directory in the hierarchy
     * doesn't exist, which is the whole point of this function).
     *
     * Other errors may be due to permissions error and the sort, but which are
     * beyond the scope of this function.
     */

    if (errno != ENOENT) {
        return 1;
    }

    if (reverse_mkdir_ignoring_last(path, length, mode, first_terminator_out)) {
        return 1;
    }

    if (mkdir(path, mode) < 0) {
        return 1;
    }

    return 0;
}

int
remove_partial_r(char *const path, const uint64_t length, char *const from) {
    if (remove(path) != 0) {
        return 1;
    }

    const uint64_t from_delta = (uint64_t)(from - path);
    const uint64_t from_length = length - from_delta;

    char *last_slash = (char *)path_find_last_row_of_slashes(from, from_length);
    if (last_slash == NULL) {
        return 1;
    }

    do {
        terminate_c_str(last_slash);
        const int ret = remove(path);
        restore_slash_c_str(last_slash);

        if (ret != 0) {
            return 1;
        }

        last_slash = find_last_slash_before_end(from, last_slash);
    } while (last_slash != NULL);

    return 0;
}
