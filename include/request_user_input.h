//
//  include/request_user_input.h
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef REQUEST_USER_INPUT_H
#define REQUEST_USER_INPUT_H

#include "notnull.h"
#include "tbd_for_main.h"

#ifndef __printflike
#define __printflike(fmtarg, firstvararg) \
    __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
#endif

enum retained_user_input_info_flags {
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_FLAGS           = 1ull << 0,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_CURRENT_VERS    = 1ull << 1,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_COMPAT_VERS     = 1ull << 2,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_INSTALL_NAME    = 1ull << 3,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_MISSING_UUIDS   = 1ull << 4,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_NON_UNIQUE_UUID = 1ull << 5,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_OBJC_CONSTRAINT = 1ull << 6,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PARENT_UMBRELLA = 1ull << 7,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PLATFORM        = 1ull << 8,
    F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_SWIFT_VERSION   = 1ull << 9,

    F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_FLAGS            = 1ull << 10,
    F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_NON_UNIQUE_UUIDS = 1ull << 11,
};

__printflike(6, 7)
bool
request_current_version(struct tbd_for_main *__notnull global,
                        struct tbd_for_main *__notnull tbd,
                        uint64_t *__notnull retained_info_in,
                        bool indent,
                        FILE *__notnull prompt_file,
                        const char *__notnull prompt,
                        ...);

__printflike(6, 7)
bool
request_compat_version(struct tbd_for_main *__notnull global,
                       struct tbd_for_main *__notnull tbd,
                       uint64_t *__notnull retained_info_in,
                       bool indent,
                       FILE *__notnull prompt_file,
                       const char *__notnull prompt,
                       ...);

__printflike(6, 7)
bool
request_install_name(struct tbd_for_main *__notnull global,
                     struct tbd_for_main *__notnull tbd,
                     uint64_t *__notnull retained_info_in,
                     bool indent,
                     FILE *__notnull prompt_file,
                     const char *__notnull prompt,
                     ...);

__printflike(6, 7)
bool
request_objc_constraint(struct tbd_for_main *__notnull global,
                        struct tbd_for_main *__notnull tbd,
                        uint64_t *__notnull retained_info_in,
                        bool indent,
                        FILE *__notnull prompt_file,
                        const char *__notnull prompt,
                        ...);

__printflike(6, 7)
bool
request_parent_umbrella(struct tbd_for_main *__notnull global,
                        struct tbd_for_main *__notnull tbd,
                        uint64_t *__notnull retained_info_in,
                        bool indent,
                        FILE *__notnull prompt_file,
                        const char *__notnull prompt,
                        ...);

__printflike(6, 7)
bool
request_platform(struct tbd_for_main *__notnull global,
                 struct tbd_for_main *__notnull tbd,
                 uint64_t *__notnull retained_info_in,
                 bool indent,
                 FILE *__notnull prompt_file,
                 const char *__notnull prompt,
                 ...);

__printflike(6, 7)
bool
request_swift_version(struct tbd_for_main *__notnull global,
                      struct tbd_for_main *__notnull tbd,
                      uint64_t *__notnull retained_info_in,
                      bool indent,
                      FILE *__notnull prompt_file,
                      const char *__notnull prompt,
                      ...);

__printflike(6, 7)
bool
request_if_should_ignore_flags(struct tbd_for_main *__notnull global,
                               struct tbd_for_main *__notnull tbd,
                               uint64_t *__notnull retained_info_in,
                               bool indent,
                               FILE *__notnull prompt_file,
                               const char *__notnull prompt,
                               ...);

__printflike(6, 7)
bool
request_if_should_ignore_non_unique_uuids(struct tbd_for_main *__notnull global,
                                          struct tbd_for_main *__notnull tbd,
                                          uint64_t *__notnull retained_info_in,
                                          bool indent,
                                          FILE *__notnull prompt_file,
                                          const char *__notnull prompt,
                                          ...);

#endif /* REQUEST_USER_INPUT_H */
