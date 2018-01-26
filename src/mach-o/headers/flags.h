//
//  src/mach-o/headers/flags.h
//  tbd
//
//  Created by inoahdev on 7/30/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    struct flags {
        explicit inline flags() = default;
        explicit inline flags(uint32_t value) : value(value) {};

        union {
            uint32_t value = 0;

            struct {
                bool no_undefined_references         : 1;
                bool incremental_link                : 1;
                bool dyld_link                       : 1;
                bool bind_at_load                    : 1;
                bool prebound                        : 1;
                bool split_segments                  : 1;
                bool lazy_init                       : 1;
                bool two_level_namespaces            : 1;
                bool force_flat                      : 1;
                bool no_multiple_defines             : 1;
                bool no_fix_prebinding               : 1;
                bool all_mods_bound                  : 1;
                bool subsections_via_symbols         : 1;
                bool canonicalized                   : 1;
                bool binds_to_weak                   : 1;
                bool allow_stack_execution           : 1;
                bool root_safe                       : 1;
                bool setuid_safe                     : 1;
                bool no_reexported_dylibs            : 1;
                bool position_independent_executable : 1;
                bool dead_strippable_dylib           : 1;
                bool has_thread_local_variables      : 1;
                bool app_extension_safe              : 1;
            } __attribute__((packed));
        };

        inline bool has_none() const noexcept { return this->value == 0; }
        inline void clear() noexcept { this->value = 0; }

        inline bool operator==(const flags &flags) const noexcept { return this->value == flags.value; }
        inline bool operator!=(const flags &flags) const noexcept { return this->value == flags.value; }
    };
}
