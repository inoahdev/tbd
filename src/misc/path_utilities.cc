//
//  src/misc/path_utilities.cc
//  tbd
//
//  Created by inoahdev on 9/9/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "path_utilities.h"

namespace path {
    char *find_next_slash(char *string) {
        auto iter = string;
        while (*iter != '\0' && *iter != '/' && *iter != '\\') {
            iter++;
        }
        
        while (*iter != '\0' && (iter[1] == '/' || iter[1] == '\\')) {
            iter++;
        }

        if (!*iter) {
            return nullptr;
        }

        return iter;
    }

    char *find_last_slash(char *string) {
        auto iter = string;
        while (true) {
            if (*iter == '\0') {
                return nullptr;
            }

            if (*iter == '/' || *iter == '\\') {
                break;
            }

            ++iter;
        }

        return iter;
    }

    bool ends_with_slash(char *string) {
        while (*string != '\0') {
            string++;
        }

        return string[-1] == '/';
    }

}
