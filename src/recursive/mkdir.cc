//
//  src/recursive/mkdir.cc
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include <cerrno>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include "../utils/path.h"
#include "mkdir.h"

namespace recursive::mkdir {
    inline bool _create_directory(char *path) noexcept {
        return ::mkdir(path, 0755) == 0;
    }

    inline bool _create_directory_if_needed(char *path) noexcept {
        return _create_directory(path) || errno == EEXIST;
    }

    inline result create_directories_ignoring_last(char *path, char *path_end, char *begin) noexcept {
        auto path_component_end = begin;

        do {
            auto next_path_component_begin = utils::path::find_end_of_row_of_slashes(path_component_end, path_end);
            if (next_path_component_begin == path_end) {
                break;
            }

            auto path_component_end_chr = *path_component_end;
            *path_component_end = '\0';

            if (!_create_directory(path)) {
                return result::failed_to_create_intermediate_directories;
            }

            *path_component_end = path_component_end_chr;
            path_component_end = utils::path::find_next_slash(next_path_component_begin, path_end);
        } while (true);

        return result::ok;
    }

    result create_directories_if_missing_ignoring_last(char *path, char **terminator) noexcept {
        auto end = utils::string::find_end_of_null_terminated_string(path);
        auto &back = end[-1];

        // In the case where path ends with a series of slashes
        // set end to the front most slash in that series

        if (back == '/' || back == '\\') {
            end = utils::path::find_last_slash_in_front_of_pattern(path, end);
        }

        // Start from second-to-last path-component, as we
        // are after all, ignoring the last path-component

        auto path_component_end = utils::path::find_last_slash_in_front_of_pattern(path, end);
        auto prev_path_component_end = end;

        // Walk backwards, from the second-to-last
        // path-component, all the way back to the first

        // This is done in this way so as to minimize
        // syscalls on directories that most likely exist
        // (the first few path-components) versus those that
        // most likely don't exist (the last few path-components)

        while (path_component_end != path) {
            // Terminate string at path-component end,
            // so as to get the entire path of the current
            // path-component

            auto path_component_end_chr = *path_component_end;
            *path_component_end = '\0';

            if (_create_directory_if_needed(path)) {
                // if prev_path_component_end points
                // to end, it means we are at the first
                // path component, and all path-components,
                // save the last, which we are to ignore,
                // exist

                if (prev_path_component_end == end) {
                    *path_component_end = '/';
                    break;
                }

                if (terminator != nullptr) {
                    *terminator = prev_path_component_end;
                }

                *path_component_end = path_component_end_chr;
                return create_directories_ignoring_last(path, end, prev_path_component_end);
            } else if (errno != ENOENT) {
                return result::failed_to_create_intermediate_directories;
            }

            prev_path_component_end = path_component_end;
            path_component_end = utils::path::find_last_slash_in_front_of_pattern(path, path_component_end);

            *prev_path_component_end = path_component_end_chr;
        }

        return result::ok;
    }

    result perform(char *path, char **terminator) noexcept {
        if (const auto result = create_directories_if_missing_ignoring_last(path, terminator); result != result::ok) {
            return result;
        }

        if (_create_directory(path)) {
            if (terminator != nullptr) {
                if (*terminator == nullptr) {
                    *terminator = utils::string::find_end_of_null_terminated_string(path);
                }
            }

            return result::ok;
        }

        if (errno == EEXIST) {
            return result::ok;
        }

        return result::failed_to_create_last_as_directory;
    }

    result perform_ignorning_last(char *path, char **terminator) noexcept {
        return create_directories_if_missing_ignoring_last(path, terminator);
    }

    result perform_with_last_as_file(char *path, char **terminator, int *last_descriptor) noexcept {
        if (const auto result = create_directories_if_missing_ignoring_last(path, terminator); result != result::ok) {
            return result;
        }

        const auto descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
        if (descriptor != -1) {
            if (terminator != nullptr) {
                if (*terminator == nullptr) {
                    *terminator = utils::string::find_end_of_null_terminated_string(path);
                }
            }

            if (last_descriptor != nullptr) {
                *last_descriptor = descriptor;
            } else {
                close(descriptor);
            }

            return result::ok;
        }

        if (errno == EEXIST) {
            return result::ok;
        }

        return result::failed_to_create_last_as_file;
    }
}
