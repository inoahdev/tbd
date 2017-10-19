//
//  src/recursive/remove.cc
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 inoahdev. All rights reserved.
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
        auto end = utils::string::find_end_of_null_terminated_string(path);
        
        auto prev_path_component_end = begin;
        auto path_component_end = end;

        do {
            auto path_component_end_elmt = *path_component_end;
            *path_component_end = '\0';

            if (access(path, F_OK) != 0) {
                return result::ok;
            }
            
            if (::remove(path) != 0) {
                if (path_component_end == begin) {
                    return result::failed_to_remove_directory;
                }
                
                return result::failed_to_remove_directory;
            }

            *path_component_end = path_component_end_elmt;
            
            prev_path_component_end = path_component_end;
            path_component_end = utils::path::find_last_slash_in_front_of_pattern(begin, prev_path_component_end);
        } while (prev_path_component_end != begin);

        return result::ok;
    }
}
