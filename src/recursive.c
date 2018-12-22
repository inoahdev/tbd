//
//  src/mkdir_r.c
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 inoahdev. All rights reserved.
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

static char *find_beginning_of_row_of_slashes(char *const begin, char *iter) {
    for (char ch = *(--iter); iter != begin; ch = *(--iter)) {
        if (ch != '/') {
            return iter + 1;
        }
    }

    return NULL;
}

static
char *find_last_slash_before_end(char *const path, const char *const end) {
    char *iter = (char *)(end - 1);
    for (char ch = *iter; iter != path; ch = *(--iter)) {
        if (ch == '/') {
            return find_beginning_of_row_of_slashes(path, iter);
        }
    }

    return NULL;
}

static char *find_next_slash(char *const path) {
    char *iter = path;
    for (char ch = *(++iter); ch != '\0'; ch = *(++iter)) {
        if (ch != '/') {
            continue;
        }
            
        /*
         * Skip past a row of slashes, if one does exist.
         */
            
        for (ch = *(++iter); ch != '\0'; ch = *(++iter)) {
            if (ch != '/') {
                break;
            }
        }

        return iter - 1;
    }

    return NULL;
}

static int
reverse_mkdir_ignoring_last(char *const path,
                            const mode_t mode,
                            char **const first_terminator_out)
{
    char *last_slash = (char *)path_find_last_row_of_slashes(path);

    /*
     * We may have a slash at the end of the string, which must be removed, so
     * we can properly iterate backwards the slashes that seperate components. 
     */

    int first_ret = 0;
    if (last_slash[1] == '\0') {
        last_slash = find_beginning_of_row_of_slashes(path, last_slash);
        
        *last_slash = '\0';
        first_ret = mkdir(path, mode);
        *last_slash = '/';

        last_slash = find_last_slash_before_end(path, last_slash);
    } else {
        *last_slash = '\0';
        first_ret = mkdir(path, mode);
        *last_slash = '/';
    }

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
        if (errno == EEXIST) {
            return 0;
        }

        if (errno != ENOENT) {
            return 1;
        }
    }

    /*
     * Store a pointer to this slash mentioned above, which we will use later to
     * stop right before parsing the last-path-component.
     */

    char *const final_slash = last_slash;

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

    while (last_slash != path) {
        last_slash = find_last_slash_before_end(path, last_slash);
        if (last_slash == NULL) {
            return 1;
        }

        *last_slash = '\0';
        
        const int ret = mkdir(path, mode);
        if (ret == 0) {
            *last_slash = '/';
            break;
        }
        
        if (ret < 0) {
            /*
             * If the directory already exists, we are done, as the previous
             * mkdir should have gone through.
             */

            if (errno == EEXIST) {
                *last_slash = '/';
                return 0;
            }

            /*
             * errno is set to ENONENT when a previous path-component doesn't
             * exist. So if we get any other error, its due to another reason,
             * and we just should return immedietly.
             */

            if (errno != ENOENT) {
                *last_slash = '/';
                return 1;
            }
        }

        *last_slash = '/';
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

    char *slash = find_next_slash(last_slash);

    /*
     * Iterate forwards to create path-components following the final
     * path-component created in the previous loop.
     * 
     * Note: We still terminate at final_slash, and only afterwards do we stop,
     * as terminating at final_slash is needed to create the second-to-last 
     * path-component.
     */

    do {
        *slash = '\0';

        const int ret = mkdir(path, mode);
        if (ret < 0) {
            *slash = '\0';
            return 1;
        }

        /*
         * Reset our slash, and if we've just created our final path-component,
         * break out and return.
         */

        *slash = '/';
        if (slash == final_slash) {
            break;
        }

        slash = find_next_slash(slash);
    } while (true);

    return 0;
}

int
open_r(char *const path,
       const mode_t flags,
       const mode_t mode,
       const mode_t dir_mode,
       char **const first_terminator_out)
{
    int fd = open(path, O_CREAT | flags, mode);
    if (fd >= 0) {
        return fd;
    }

    if (errno != ENOENT) {
        return -1;
    }

    if (reverse_mkdir_ignoring_last(path, dir_mode, first_terminator_out)) {
        return -1;
    }

    fd = open(path, O_CREAT | flags, mode);
    if (fd < 0) {
        return -1;
    }

    return fd;
}

int remove_parial_r(char *const path, char *const from) {
    if (remove(path) != 0) {
        return 1;
    }

    char *last_slash = (char *)path_find_last_row_of_slashes(from);

    do {
        if (last_slash == NULL) {
            return 1;
        }

        *last_slash = '\0';
        
        const int ret = remove(path);
        if (ret != 0) {
            return 1;
        }

        *last_slash = '/';
        last_slash = find_last_slash_before_end(from, last_slash);
    } while (true);

    return 0;
}