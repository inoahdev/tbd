//
//  src/recursive/mkdir.cc
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include "../utils/path.h"
#include "mkdir.h"

namespace recursive::mkdir {
    enum class last_as {
        none,
        file,
        directory
    };

    template <last_as type>
    inline result _perform_without_check(char *path, char *end, char *component_end, int *last_descriptor = nullptr) {
        auto path_component_end = component_end;

        do {
            auto path_component_end_elmt = *path_component_end;
            *path_component_end = '\0';

            if (path_component_end == end) {
                if constexpr (type == last_as::none) {
                    *path_component_end = path_component_end_elmt;
                    break;
                }

                if constexpr (type == last_as::file) {
                    auto descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
                    *path_component_end = path_component_end_elmt;

                    if (descriptor == -1) {
                        return result::failed_to_create_last_as_file;
                    }

                    if (last_descriptor != nullptr) {
                        *last_descriptor = descriptor;
                    } else {
                        close(descriptor);
                    }

                    break;
                }

                if constexpr (type == last_as::directory) {
                    if (::mkdir(path, 0755) != 0) {
                        return result::failed_to_create_last_as_directory;
                    }

                    *path_component_end = path_component_end_elmt;
                    break;
                }
            }

            if (::mkdir(path, 0755) != 0) {
                return result::failed_to_create_intermediate_directories;
            }

            *path_component_end = path_component_end_elmt;
            path_component_end = utils::path::find_next_slash_at_back_of_pattern(&path_component_end[1], end);
        } while (true);

        return result::ok;
    }

    template <last_as type>
    inline result _perform(char *path, char **iter, int *last_descriptor = nullptr) {
        if (access(path, F_OK) == 0) {
            return result::ok;
        }

        auto end = utils::string::find_end_of_null_terminated_string(path);
        auto back = end - 1;

        if (*back == '/' || *back == '\\') {
            end = utils::path::find_last_slash_in_front_of_pattern(path, end);
        }

        auto path_component_end = end;
        auto prev_path_component_end = end;

        while (path_component_end != path) {
            auto path_component_end_elmt = *path_component_end;
            *path_component_end = '\0';

            if (access(path, F_OK) == 0) {
                if (iter != nullptr) {
                    *iter = prev_path_component_end;
                }

                *path_component_end = path_component_end_elmt;
                return _perform_without_check<type>(path, end, prev_path_component_end, last_descriptor);
            }

            prev_path_component_end = path_component_end;
            path_component_end = utils::path::find_last_slash_in_front_of_pattern(path, path_component_end);

            *prev_path_component_end = path_component_end_elmt;
        }

        return result::ok;
    }

    result perform(char *path, char **iter) {
        return _perform<last_as::directory>(path, iter);
    }

    result perform_ignorning_last(char *path, char **iter) {
        return _perform<last_as::none>(path, iter);
    }

    result perform_with_last_as_file(char *path, char **iter, int *last_descriptor) {
        return _perform<last_as::file>(path, iter, last_descriptor);
    }
}
