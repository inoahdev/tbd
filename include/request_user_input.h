//
//  include/request_user_input.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef REQUEST_USER_INPUT_H
#define REQUEST_USER_INPUT_H

#include "tbd_for_main.h"

enum retained_user_input_info_flags {
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_FLAGS           = 1 << 0,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_INSTALL_NAME    = 1 << 1,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_MISSING_UUIDS   = 1 << 2,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_NON_UNIQUE_UUID = 1 << 3,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_OBJC_CONSTRAINT = 1 << 4,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PARENT_UMBRELLA = 1 << 5,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PLATFORM        = 1 << 6,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_SWIFT_VERSION   = 1 << 7,

    F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_FLAGS            = 1 << 8,
    F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_NON_UNIQUE_UUIDS = 1 << 8,
};

bool
request_install_name(struct tbd_for_main *global,
                     struct tbd_for_main *tbd,
                     uint64_t *retained_info_in);

bool
request_objc_constraint(struct tbd_for_main *global,
                        struct tbd_for_main *tbd,
                        uint64_t *retained_info_in);

bool
request_parent_umbrella(struct tbd_for_main *global,
                        struct tbd_for_main *tbd,
                        uint64_t *retained_info_in);

bool
request_platform(struct tbd_for_main *global,
                 struct tbd_for_main *tbd,
                 uint64_t *retained_info_in);

bool
request_swift_version(struct tbd_for_main *global,
                      struct tbd_for_main *tbd,
                      uint64_t *retained_info_in);

bool
request_if_should_ignore_flags(struct tbd_for_main *global,
                               struct tbd_for_main *tbd,
                               uint64_t *retained_info_in);

bool
request_if_should_ignore_non_unique_uuids(struct tbd_for_main *global,
                                          struct tbd_for_main *tbd,
                                          uint64_t *retained_info_in);

#endif /* REQUEST_USER_INPUT_H */