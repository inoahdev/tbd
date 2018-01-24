//
//  src/main_utils/tbd_user_input.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <iostream>
#include "tbd_with_options.h"

namespace main_utils {
    void request_input(std::string &string, const char *prompt, const std::initializer_list<const char *> &acceptable_inputs) noexcept;

    bool request_new_installation_name(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept;
    bool request_new_objc_constraint(tbd_with_options &all,   tbd_with_options &tbd, uint64_t *info) noexcept;
    bool request_new_parent_umbrella(tbd_with_options &all,   tbd_with_options &tbd, uint64_t *info) noexcept;
    bool request_new_platform(tbd_with_options &all,          tbd_with_options &tbd, uint64_t *info) noexcept;
    bool request_new_swift_version(tbd_with_options &all,     tbd_with_options &tbd, uint64_t *info) noexcept;

    bool request_if_should_ignore_flags(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept;
    bool request_if_should_ignore_uuids(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept;

    bool request_if_should_ignore_non_unique_uuids(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept;
    bool request_if_should_ignore_missing_uuids(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept;
}
