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
    inline bool _create_directory(char *path) noexcept {
        return ::mkdir(path, 0755) == 0;
    }

    inline result _create_directories_ignoring_last(char *path, char *path_end, char *begin) noexcept {
        auto path_component_end = begin;

        do {
            if (path_component_end == path_end) {
                break;
            }

            auto path_component_end_elmt = *path_component_end;
            *path_component_end = '\0';

            if (!_create_directory(path)) {
                return result::failed_to_create_intermediate_directories;
            }

            *path_component_end = path_component_end_elmt;
            path_component_end = utils::path::find_next_slash_at_back_of_pattern(&path_component_end[1], path_end);
        } while (true);

        return result::ok;
    }

    result _create_directories_if_missing_ignoring_last(char *path, char **terminator, bool *path_already_exists = nullptr) noexcept {
        if (access(path, F_OK) == 0) {
            if (path_already_exists != nullptr) {
                *path_already_exists = true;
            }

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
                if (terminator != nullptr) {
                    *terminator = prev_path_component_end;
                }

                *path_component_end = path_component_end_elmt;
                return _create_directories_ignoring_last(path, end, prev_path_component_end);
            }

            prev_path_component_end = path_component_end;
            path_component_end = utils::path::find_last_slash_in_front_of_pattern(path, path_component_end);

            *prev_path_component_end = path_component_end_elmt;
        }

        return result::ok;
    }

    result perform(char *path, char **terminator) noexcept {
        auto path_already_exists = false;
        if (const auto result = _create_directories_if_missing_ignoring_last(path, terminator, &path_already_exists); path_already_exists) {
            return result;
        }

        if (!_create_directory(path)) {
            return result::failed_to_create_last_as_directory;
        }

        return result::ok;
    }

    result perform_ignorning_last(char *path, char **terminator) noexcept {
        return _create_directories_if_missing_ignoring_last(path, terminator);
    }

    result perform_with_last_as_file(char *path, char **terminator, int *last_descriptor) noexcept {
        auto path_already_exists = false;
        if (const auto result = _create_directories_if_missing_ignoring_last(path, terminator, &path_already_exists); path_already_exists) {
            return result;
        }

        const auto descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, DEFFILEMODE);
        if (descriptor == -1) {
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
