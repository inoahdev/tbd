//
//  src/main_utils/tbd_with_options.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#pragma once
#include "../mach-o/utils/tbd.h"

namespace main_utils {
    struct tbd_with_options {
        // option_masks contains both options
        // relevant to tbd and other miscellaneous
        // information for the tool's use

        enum option_masks {
            recurse_directories_at_path    = 1 << 0,
            recurse_subdirectories_at_path = 1 << 1,

            // Stop warnings from being printed
            // to the user

            ignore_warnings = 1 << 2,

            // There are supposed to be three
            // different variations - add, replace,
            // and remove, but only two bits are
            // used (replaced, removed). add is
            // used when replaced and removed
            // is 1, so having both replacing and
            // removing is not allowed

            // Replace options are set if
            // intended to replace x in tbd,
            // otherwise we add on

            replace_architectures = 1 << 3,
            replace_flags         = 1 << 4,
            replace_clients       = 1 << 5,

            // Remove options are set if intended
            // to remove x in tbd, otherwise nothing
            // happens.

            remove_architectures = 1 << 6,
            remove_flags         = 1 << 7,
            remove_clients       = 1 << 8,

            // Options provided for output

            // Preserves directories following root path
            // provided for recursion when creating an
            // output-file

            maintain_directories   = 1 << 9,
            replace_path_extension = 1 << 10,

            replaced_objc_constraint = 1 << 11
        };

        macho::utils::tbd info;

        macho::utils::tbd::creation_options creation_options;
        macho::utils::tbd::write_options write_options;

        // path is main argument provided
        // to option -p.

        std::string path;

        // If write_path is empty, that is
        // the user didn't provide an output-path,
        // tbd should reroute its output to stdout,
        // except when recursing

        std::string write_path;

        // Pointer to first option in local-options
        // Used to apply any options for writing

        int local_option_start = 0;

        // architectures is a bitset of indexes to the
        // architecture_info_table to supply as architecture
        // list

        uint64_t architectures = uint64_t();

        // containers is a bitset of indexes of
        // containers to create tbd from

        // XXX: Create code to handle custom
        // containers for tbd-create

        utils::bits containers;
        uint64_t options = uint64_t();

        // utility functions

        void apply_missing_from(const tbd_with_options &options) noexcept;
    };
}
