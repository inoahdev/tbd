//
//  src/msic/recursively.cc
//  tbd
//
//  Created by inoahdev on 4/16/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include "path_utilities.h"
#include "recursively.h"

enum recursively_create_directories_from_file_path_inner_options : uint64_t {
    create_last_as_file      = 1 << 0,
    create_last_as_directory = 1 << 1
};

char *recursively_create_directories_from_file_path_inner(char *path, char *begin, char *end, char *slash, uint64_t options, int *last_descriptor = nullptr) {
    auto path_component_begin = begin;
    auto path_component_end = slash;

    do {
        if (path_component_end == end) {
            if (options & create_last_as_file) {
                auto descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
                if (descriptor == -1) {
                    fprintf(stderr, "Failed to create file (at path %s), failing with error: %s\n", path, strerror(errno));
                    exit(1);
                }

                if (last_descriptor != nullptr) {
                    *last_descriptor = descriptor;
                } else {
                    close(descriptor);
                }
            }

            if (options & create_last_as_directory) {
                if (mkdir(path, 0755) != 0) {
                    fprintf(stderr, "Failed to create directory (at path %s) with mode (0755), failing with error: %s\n", path, strerror(errno));
                    exit(1);
                }
            }

            break;
        }

        *path_component_end = '\0';

        if (mkdir(path, 0755) != 0) {
            fprintf(stderr, "Failed to create directory (at path %s) with mode (0755), failing with error: %s\n", path, strerror(errno));
            exit(1);
        }

        *path_component_end = '/';

        path_component_begin = path::find_end_of_row_of_slashes(path_component_end, end);
        path_component_end = path::find_next_slash(path_component_begin, end);
    } while (true);

    return slash;
}

char *recursively_create_directories_from_file_path_ignoring_last(char *path, size_t index) {
    auto begin = path::find_end_of_row_of_slashes(&path[index]);
    auto end = &path[strlen(path)];

    if (begin == end) {
        return nullptr;
    }

    // Move to front of terminating slashes

    auto path_reverse_iter = end;
    if (*(path_reverse_iter - 1) == '/' || *(path_reverse_iter - 1) == '\\') {
        do {
            --path_reverse_iter;
        } while (path_reverse_iter != begin && (*(path_reverse_iter - 1) == '/' || *(path_reverse_iter - 1) == '\\'));
    }

    auto path_component_begin = begin;
    while (path_component_begin != path_reverse_iter) {
        auto path_component_end = path::find_next_slash(path_component_begin, path_reverse_iter);
        auto path_component_end_char = *path_component_end;

        *path_component_end = '\0';

        if (access(path, F_OK) != 0) {
            *path_component_end = path_component_end_char;
            recursively_create_directories_from_file_path_inner(path, path_component_begin, path_reverse_iter, path_component_end, 0);

            return path_component_begin;
        }

        *path_component_end = path_component_end_char;
        path_component_begin = path::find_end_of_row_of_slashes(path_component_end, path_reverse_iter);
    }

    return nullptr;
}

char *recursively_create_directories_from_file_path_creating_last_as_file(char *path, size_t index, int *last_descriptor) {
    auto begin = path::find_end_of_row_of_slashes(&path[index]);
    auto end = &path[strlen(path)];

    if (begin == end) {
        return nullptr;
    }

    // Move to front of terminating slashes

    auto path_reverse_iter = end;
    if (*(path_reverse_iter - 1) == '/' || *(path_reverse_iter - 1) == '\\') {
        do {
            --path_reverse_iter;
        } while (path_reverse_iter != begin && (*(path_reverse_iter - 1) == '/' || *(path_reverse_iter - 1) == '\\'));
    }

    auto path_component_begin = begin;
    while (path_component_begin != path_reverse_iter) {
        auto path_component_end = path::find_next_slash(path_component_begin, path_reverse_iter);
        auto path_component_end_char = *path_component_end;

        *path_component_end = '\0';

        if (access(path, F_OK) != 0) {
            *path_component_end = path_component_end_char;
            recursively_create_directories_from_file_path_inner(path, path_component_begin, path_reverse_iter, path_component_end, create_last_as_file, last_descriptor);

            return path_component_begin;
        }

        *path_component_end = path_component_end_char;
        path_component_begin = path::find_end_of_row_of_slashes(path_component_end, path_reverse_iter);
    }

    return nullptr;
}

char *recursively_create_directories_from_file_path_creating_last_as_directory(char *path, size_t index) {
    auto begin = path::find_end_of_row_of_slashes(&path[index]);
    auto end = &path[strlen(path)];

    if (begin == end) {
        return nullptr;
    }

    // Move to front of terminating slashes

    auto path_reverse_iter = end;
    if (*(path_reverse_iter - 1) == '/' || *(path_reverse_iter - 1) == '\\') {
        do {
            --path_reverse_iter;
        } while (path_reverse_iter != begin && (*(path_reverse_iter - 1) == '/' || *(path_reverse_iter - 1) == '\\'));
    }

    auto path_component_begin = begin;
    while (path_component_begin != path_reverse_iter) {
        auto path_component_end = path::find_next_slash(path_component_begin, path_reverse_iter);
        auto path_component_end_char = *path_component_end;

        *path_component_end = '\0';

        if (access(path, F_OK) != 0) {
            *path_component_end = path_component_end_char;
            recursively_create_directories_from_file_path_inner(path, path_component_begin, path_reverse_iter, path_component_end, create_last_as_directory);

            return path_component_begin;
        }

        *path_component_end = path_component_end_char;
        path_component_begin = path::find_end_of_row_of_slashes(path_component_end, path_reverse_iter);
    }

    return nullptr;
}

void recursively_remove_directories_from_file_path(char *path, char *begin, char *end) {
     // Make sure end is null-terminated

    if (!end) {
        end = begin;
        while (*end != '\0') {
            end++;
        }
    }

    auto path_component_end = end;
    if (*(path_component_end - 1) == '/' || *(path_component_end - 1) == '\\') {
        do {
            --path_component_end;
        } while (path_component_end != begin && (*(path_component_end - 1) == '/' || *(path_component_end - 1) == '\\'));
    }

    auto path_component_end_char = *path_component_end;
    *path_component_end = '\0';

    if (access(path, F_OK) != 0) {
        return;
    }

    if (remove(path) != 0) {
        fprintf(stderr, "Failed to remove object (at path %s), failing with error: %s\n", path, strerror(errno));
        exit(1);
    }

    auto prev_path_component_end = path_component_end;

    *path_component_end = path_component_end_char;
    path_component_end = path::find_last_slash(begin, path_component_end);

    while (path_component_end != prev_path_component_end) {
        path_component_end_char = *path_component_end;
        *path_component_end = '\0';

        if (remove(path) != 0) {
            fprintf(stderr, "Failed to remove object (at path %s), failing with error: %s\n", path, strerror(errno));
            exit(1);
        }

        *path_component_end = path_component_end_char;
        prev_path_component_end = path_component_end;

        path_component_end = path::find_last_slash(begin, prev_path_component_end);
    }
}
