//
//  src/main_utils/parse_architectures_list.cc
//  tbd
//
//  Created by inoahdev on 1/25/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <cstdio>
#include <cstdlib>

#include "../mach-o/architecture_info.h"
#include "parse_architectures_list.h"

namespace main_utils {
    uint64_t parse_architectures_list(int &index, int argc, const char *argv[]) {
        if (index == argc) {
            fputs("Please provide a list of architectures\n", stderr);
            exit(1);
        }

        auto architectures = uint64_t();
        do {
            const auto &architecture_string = argv[index];
            const auto &architecture_string_front = architecture_string[0];

            // Quickly filter out an option or path instead of a (relatively)
            // expensive call to macho::architecture_info_from_name().

            if (architecture_string_front == '-' ||
                architecture_string_front == '/' ||
                architecture_string_front == '\\')
            {
                // If the architectures vector is empty, the user did not provide any architectures
                // but did provided the architecture option, which requires at least one architecture
                // being provided.

                if (!architectures) {
                    fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)"
                          "\n",
                          stderr);
                    exit(1);
                }

                break;
            }

            const auto architecture_info_table_index = macho::architecture_info_index_from_name(architecture_string);
            if (architecture_info_table_index == -1) {
                // macho::architecture_info_index_from_name() returning -1 can be the result of one
                // of two scenarios, The string stored in architecture being invalid, such as being
                // misspelled, or the string being the path object inevitably following the architecture
                // argument.

                // If the architectures vector is empty, the user did not provide any architectures
                // but did provide the architecture option, which requires at least one architecture
                // being provided.

                if (!architectures) {
                    fprintf(stderr, "Unrecognized architecture with name: %s\n", architecture_string);
                    exit(1);
                }

                break;
            }

            architectures |= (1ull << architecture_info_table_index);
            index++;
        } while (index < argc);

        // As the caller of this function is in a for loop,
        // the index is incremented once again once this function
        // returns. To make up for this, decrement index by 1.

        index--;
        return architectures;
    }
}
