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

#include "likely.h"
#include "our_io.h"

#include "recursive.h"
#include "util.h"

static char *get_next_path_component_slash(char *__notnull const path) {
    char *iter = (char *)get_end_of_slashes(path);
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

static char *
find_next_slash_reverse(char *__notnull const path,
                        char *__notnull const last_slash)
{
    const char *const rev_end = path - 1;
    for (char *iter = (last_slash - 1); iter != rev_end; --iter) {
        const char ch = *iter;
        if (ch == '/') {
            return iter;
        }
    }

    return NULL;
}

static inline void terminate_c_str(char *__notnull const iter) {
    *iter = '\0';
}

static inline void restore_slash_c_str(char *__notnull const iter) {
    *iter = '/';
}

static int
reverse_mkdir_ignoring_last(char *__notnull const path,
                            const uint64_t path_length,
                            const mode_t mode,
                            char **const first_terminator_out)
{
    const char *const end = path + path_length;
    char *last_slash = (char *)find_last_slash(path, end);

    const char possible_end = last_slash[1];

    /*
     * We may have a slash at the back of the string, which we should ignore as
     * terminating at that point does not lead us to the path of the parent
     * directory in the full-path's hierarchy.
     */

    if (unlikely(possible_end == '\0')) {
        last_slash = (char *)get_front_of_slashes(path, last_slash);
        last_slash = find_next_slash_reverse(path, last_slash);
    }

    terminate_c_str(last_slash);
    const int first_ret = our_mkdir(path, mode);
    restore_slash_c_str(last_slash);

    if (first_ret == 0) {
        return 0;
    }

    if (first_ret < 0) {
        /*
         * If the directory already exists, we are done, as the previous mkdir
         * should have gone through.
         */

        if (errno == EEXIST) {
            return 0;
        }

        /*
         * errno is set to ENONENT when a previous path-component doesn't exist.
         *
         * Any other error is beyond our scope and we should just error-return
         * immediately.
         */

        if (unlikely(errno != ENOENT)) {
            return 1;
        }
    }

    /*
     * final_slash is a pointer to the last slash we call terminate_c_str() on
     * in the do {} while loop below.
     */

    char *const final_slash = (char *)get_front_of_slashes(path, last_slash);

    /*
     * We move backwards trying to create the hierarchy. If we fail, we move
     * yet further back, until we successfully create that directory.
     *
     * last_slash is the ending-slash of every path-component. We
     * null-terminate at last_slash to try and create a directory, and if it
     * fails, we move last-slash back.
     *
     * We then break out of the loop to then iterate forwards and
     * create the directories afterwards.
     *
     * If the directory already exists, we can simply return as it cannot happen
     * unless the second-to-last path-component already exists.
     *
     * If a directory-component doesn't exist (ENOENT), continue with the
     * iteration.
     */

    char *prev_last_slash = NULL;
    last_slash = (char *)get_front_of_slashes(path, last_slash);

    while (last_slash != path) {
        prev_last_slash = last_slash;
        last_slash = (char *)find_last_row_of_slashes(path, last_slash);

        if (unlikely(last_slash == NULL)) {
            return 1;
        }

        terminate_c_str(last_slash);
        const int ret = our_mkdir(path, mode);
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
             * If the directory already exists, all the previous mkdir calls
             * should have gone through.
             */

            if (errno == EEXIST) {
                break;
            }

            /*
             * errno is set to ENONENT when a previous path-component doesn't
             * exist.
             *
             * So if we get any other error, its due to another reason, and we
             * should just return immedietly.
            */

            if (unlikely(errno != ENOENT)) {
                if (first_terminator_out != NULL) {
                    *first_terminator_out = prev_last_slash;
                }

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

    char *slash = get_next_path_component_slash(last_slash);

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
        const int ret = our_mkdir(path, mode);
        restore_slash_c_str(slash);

        if (unlikely(ret < 0)) {
            return 1;
        }

        /*
         * Reset our slash, and if we've just created our final path-component,
         * break out and return.
         */

        if (slash == final_slash) {
            break;
        }

        slash = get_next_path_component_slash(slash);
    } while (true);

    return 0;
}

int
open_r(char *__notnull const path,
       const uint64_t length,
       const int flags,
       const mode_t mode,
       const mode_t dir_mode,
       char **const terminator_out)
{
    int fd = our_open(path, O_CREAT | flags, mode);
    if (likely(fd >= 0)) {
        return fd;
    }

    /*
     * The only error supported is ENOENT (when a directory in the hierarchy
     * doesn't exist - which is the whole point of this function).
     *
     * Other errors are beyond the scope of this function, and so we error out
     * immediately.
     */

    if (unlikely(errno != ENOENT)) {
        return -1;
    }

    if (reverse_mkdir_ignoring_last(path, length, dir_mode, terminator_out)) {
        return -1;
    }

    fd = our_open(path, O_CREAT | flags, mode);
    if (unlikely(fd < 0)) {
        return -1;
    }

    return fd;
}

int
mkdir_r(char *__notnull const path,
        const uint64_t length,
        const mode_t mode,
        char **const first_terminator_out)
{
    if (likely(our_mkdir(path, mode) == 0)) {
        return 0;
    }

    /*
     * If the directory already exists, return as a success.
     *
     * Note: To avoid multiple syscalls, we call our_mkdir() knowing that the
     * directory may already exist, preferring checking of errno over an extra
     * syscall.
     */

    if (errno == EEXIST) {
        return 0;
    }

    /*
     * The only error supported is ENOENT (when a directory in the hierarchy
     * doesn't exist, which is the whole point of this function).
     *
     * Other errors are beyond the scope of this function, and so we
     * error-return immediately.
     */

    if (unlikely(errno != ENOENT)) {
        return 1;
    }

    if (reverse_mkdir_ignoring_last(path, length, mode, first_terminator_out)) {
        return 1;
    }

    if (our_mkdir(path, mode) < 0) {
        return 1;
    }

    return 0;
}

int
remove_file_r(char *__notnull const path,
              const uint64_t length,
              char *__notnull const last)
{
    if (unlikely(our_unlink(path) != 0)) {
        return 1;
    }

    const char *const end = path + length;
    char *last_slash = (char *)find_last_row_of_slashes(last, end);

    if (last_slash == NULL) {
        return 1;
    }

    do {
        terminate_c_str(last_slash);
        const int ret = our_rmdir(path);
        restore_slash_c_str(last_slash);

        if (unlikely(ret != 0)) {
            return 1;
        }

        last_slash = (char *)find_last_row_of_slashes(path, last_slash);
    } while (last_slash != NULL);

    return 0;
}
