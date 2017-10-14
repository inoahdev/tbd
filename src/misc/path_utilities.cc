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

        if (!*iter) {
            return nullptr;
        }

        return iter;
    }

    const char *find_next_slash(const char *string) {
        auto iter = string;
        while (*iter != '\0' && *iter != '/' && *iter != '\\') {
            iter++;
        }

        if (!*iter) {
            return nullptr;
        }

        return iter;
    }

    char *find_next_unique_slash(char *string) {
        auto iter = string;
        while (*iter != '\0' && *iter != '/' && *iter != '\\') {
            iter++;
        }

        if (*iter != '\0') {
            while (iter[1] != '\0' && (iter[1] == '/' || iter[1] == '\\')) {
                iter++;
            }

            if (iter[1] == '\0') {
                return nullptr;
            }
        }

        return iter;
    }

    const char *find_next_unique_slash(const char *string) {
        auto iter = string;
        while (*iter != '\0' && *iter != '/' && *iter != '\\') {
            iter++;
        }

        if (*iter != '\0') {
            while (iter[1] != '\0' && (iter[1] == '/' || iter[1] == '\\')) {
                iter++;
            }

            if (iter[1] == '\0') {
                return nullptr;
            }
        }

        return iter;
    }
}
