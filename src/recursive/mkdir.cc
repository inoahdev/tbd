//
//  src/recursive/mkdir.cc
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 inoahdev. All rights reserved.
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

    inline result create_directories_ignoring_last(char *path, char *path_end, char *begin) noexcept {
        auto path_component_end = begin;

        do {
            auto next_path_component_begin = utils::path::find_end_of_row_of_slashes(path_component_end, path_end);
            if (next_path_component_begin == path_end) {
                break;
            }

            auto path_component_end_elmt = *path_component_end;
            *path_component_end = '\0';

            if (!_create_directory(path)) {
                return result::failed_to_create_intermediate_directories;
            }

            *path_component_end = path_component_end_elmt;
            path_component_end = utils::path::find_next_slash(next_path_component_begin, path_end);
        } while (true);

        return result::ok;
    }

    result create_directories_if_missing_ignoring_last(char *path, char **terminator) noexcept {
        auto end = utils::string::find_end_of_null_terminated_string(path);
        auto &back = *(end - 1);

        if (back == '/' || back == '\\') {
            end = utils::path::find_last_slash_in_front_of_pattern(path, end);
        }

        auto path_component_end = utils::path::find_last_slash_in_front_of_pattern(path, end);
        auto prev_path_component_end = end;

        while (path_component_end != path) {
            auto path_component_end_elmt = *path_component_end;
            *path_component_end = '\0';

            if (access(path, F_OK) == 0) {
                if (terminator != nullptr) {
                    *terminator = prev_path_component_end;
                    
                    // terminator should never point to end
                    
                    if (*terminator == end) {
                        *terminator = nullptr;
                    }
                }

                *path_component_end = path_component_end_elmt;
                return create_directories_ignoring_last(path, end, prev_path_component_end);
            }

            prev_path_component_end = path_component_end;
            path_component_end = utils::path::find_last_slash_in_front_of_pattern(path, path_component_end);

            *prev_path_component_end = path_component_end_elmt;
        }

        return result::ok;
    }

    result perform(char *path, char **terminator) noexcept {
        if (const auto result = create_directories_if_missing_ignoring_last(path, terminator); result != result::ok) {
            return result;
        }

        if (!_create_directory(path)) {
            if (errno == EEXIST) {
                return result::ok;
            }

            return result::failed_to_create_last_as_directory;
        }

        return result::ok;
    }

    result perform_ignorning_last(char *path, char **terminator) noexcept {
        return create_directories_if_missing_ignoring_last(path, terminator);
    }

    result perform_with_last_as_file(char *path, char **terminator, int *last_descriptor) noexcept {
        if (const auto result = create_directories_if_missing_ignoring_last(path, terminator); result != result::ok) {
            return result;
        }

        const auto descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
        if (descriptor == -1) {
            if (errno == EEXIST) {
                return result::ok;
            }

            return result::failed_to_create_last_as_file;
        }

        if (last_descriptor != nullptr) {
            *last_descriptor = descriptor;
        } else {
            close(descriptor);
        }

        return result::ok;
    }
}
