//
//  src/recursive/remove.cc
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include <cstdio>
#include <unistd.h>

#include "../utils/path.h"
#include "remove.h"

namespace recursive::remove {
    result perform(char *path) {
        return perform(path, path);
    }

    result perform(char *path, char *begin) {
        // To perform a recursive-remove, we start from
        // the last path-component and iterate backwards
        // until we reach a path-component beginning at
        // caller argument "begin"

        auto end = utils::string::find_end_of_null_terminated_string(path);

        // Use a "prev_path_component_end" variable to store
        // the previous path-component end. This is needed
        // because path-component end is allowed to hit begin
        // but not be below it, so we use "prev_path_component_end"
        // to check if we have already been at begin and are
        // now, at least supposed to be, lower

        auto path_component_end = end;
        auto prev_path_component_end = path_component_end;

        do {
            // Terminate path at path-component end to get
            // full path of the current path-component

            auto path_component_end_chr = *path_component_end;
            *path_component_end = '\0';

            if (access(path, F_OK) != 0) {
                if (path_component_end == begin) {
                    return result::directory_doesnt_exist;
                }

                return result::sub_directory_doesnt_exist;
            }

            if (::remove(path) != 0) {
                if (path_component_end == begin) {
                    return result::failed_to_remove_directory;
                }

                return result::failed_to_remove_subdirectories;
            }

            *path_component_end = path_component_end_chr;

            prev_path_component_end = path_component_end;
            path_component_end = utils::path::find_last_slash_in_front_of_pattern(begin, path_component_end);
        } while (prev_path_component_end != begin);

        return result::ok;
    }
}
