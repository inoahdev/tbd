//
//  src/main_utils/recursive_remove_with_terminator.cc
//  tbd
//
//  Created by inoahdev on 1/25/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <cstdio>
#include "../recursive/remove.h"

namespace main_utils {
    void recursively_remove_with_terminator(char *path, char *terminator, bool should_print_paths) {
        if (terminator == nullptr) {
            return;
        }

        switch (recursive::remove::perform(path, terminator)) {
            case recursive::remove::result::ok:
                break;

            case recursive::remove::result::failed_to_remove_directory:
                if (should_print_paths) {
                    // Terminate at terminator to get original created directory

                    *terminator = '\0';
                    fprintf(stderr, "Failed to remove created-directory (at path %s)\n", path);
                } else {
                    fputs("Failed to remove created-directory at provided output-path\n", stderr);
                }

                break;

            case recursive::remove::result::directory_doesnt_exist:
                if (should_print_paths) {
                    // Terminate at terminator to get original created directory

                    *terminator = '\0';
                    fprintf(stderr, "[Internal Error] Created directory doesn't exist (at path %s)\n", path);
                } else {
                    fputs("[Internal Error] Created directory doesn't exist (at path %s)\n", stderr);
                }

                break;

            case recursive::remove::result::failed_to_remove_subdirectories:
                if (should_print_paths) {
                    // Terminate at terminator to get original created directory
                    *terminator = '\0';

                    fprintf(stderr, "Failed to remove created sub-directories (of directory at path %s)\n", path);
                    *terminator = '/';

                    fprintf(stderr, " at path: %s\n", path);
                } else {
                    fprintf(stderr, "Failed to remove created sub-directories of provided output-path %s)\n", path);
                }

                break;

            case recursive::remove::result::sub_directory_doesnt_exist:
                // Terminate at terminator to get original created directory
                *terminator = '\0';

                fprintf(stderr, "[Internal Error] Created sub-directories (of directory at path %s)\n", path);
                *terminator = '/';

                fprintf(stderr, " (at path %s) doesn't exist\n", path);
                break;

        }
    }
}
