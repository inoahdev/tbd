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

        struct options {
            explicit inline options() = default;
            explicit inline options(uint64_t value) : value(value) {}

            union {
                uint64_t value = 0;

                struct {
                    bool recurse_directories_at_path    : 1;
                    bool recurse_subdirectories_at_path : 1;

                    // Stop warnings from being printed
                    // to the user

                    bool ignore_warnings : 1;

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

                    bool replace_architectures : 1;
                    bool replace_flags         : 1;
                    bool replace_clients       : 1;

                    // Remove options are set if intended
                    // to remove x in tbd, otherwise nothing
                    // happens.

                    bool remove_architectures : 1;
                    bool remove_flags         : 1;
                    bool remove_clients       : 1;

                    // Options provided for output

                    // Preserves directories following root path
                    // provided for recursion when creating an
                    // output-file

                    bool maintain_directories   : 1;
                    bool replace_path_extension : 1;

                    bool replaced_objc_constraint : 1;
                } __attribute__((packed));
            };

            inline bool has_none() const noexcept { return this->value == 0; }
            inline void clear() noexcept { this->value = 0; }

            inline bool operator==(const options &options) const noexcept { return this->value == options.value; }
            inline bool operator!=(const options &options) const noexcept { return this->value != options.value; }
        } options;

        // utility functions

        void apply_missing_from(const tbd_with_options &options) noexcept;
        void apply_local_options(int argc, const char *argv[]) noexcept;
    };
}
