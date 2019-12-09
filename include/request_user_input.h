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

struct retained_user_info {
    bool never_replace_current_version : 1;
    bool never_replace_compat_version : 1;
    bool never_replace_install_name : 1;
    bool never_replace_objc_constraint : 1;
    bool never_replace_parent_umbrella : 1;
    bool never_replace_platform : 1;
    bool never_replace_swift_version : 1;

    bool never_replace_missing_uuids : 1;
    bool never_replace_non_unique_uuids : 1;

    bool never_ignore_flags : 1;
    bool never_ignore_non_unique_uuids : 1;
};

__printflike(6, 7)
bool
request_current_version(struct tbd_for_main *__notnull global,
                        struct tbd_for_main *__notnull tbd,
                        struct retained_user_info *__notnull retained,
                        bool indent,
                        FILE *__notnull prompt_file,
                        const char *__notnull prompt,
                        ...);

__printflike(6, 7)
bool
request_compat_version(struct tbd_for_main *__notnull global,
                       struct tbd_for_main *__notnull tbd,
                       struct retained_user_info *__notnull retained,
                       bool indent,
                       FILE *__notnull prompt_file,
                       const char *__notnull prompt,
                       ...);

__printflike(6, 7)
bool
request_install_name(struct tbd_for_main *__notnull global,
                     struct tbd_for_main *__notnull tbd,
                     struct retained_user_info *__notnull retained,
                     bool indent,
                     FILE *__notnull prompt_file,
                     const char *__notnull prompt,
                     ...);

__printflike(6, 7)
bool
request_objc_constraint(struct tbd_for_main *__notnull global,
                        struct tbd_for_main *__notnull tbd,
                        struct retained_user_info *__notnull retained,
                        bool indent,
                        FILE *__notnull prompt_file,
                        const char *__notnull prompt,
                        ...);

__printflike(6, 7)
bool
request_parent_umbrella(struct tbd_for_main *__notnull global,
                        struct tbd_for_main *__notnull tbd,
                        struct retained_user_info *__notnull retained,
                        bool indent,
                        FILE *__notnull prompt_file,
                        const char *__notnull prompt,
                        ...);

__printflike(6, 7)
bool
request_platform(struct tbd_for_main *__notnull global,
                 struct tbd_for_main *__notnull tbd,
                 struct retained_user_info *__notnull retained,
                 bool indent,
                 FILE *__notnull prompt_file,
                 const char *__notnull prompt,
                 ...);

__printflike(6, 7)
bool
request_swift_version(struct tbd_for_main *__notnull global,
                      struct tbd_for_main *__notnull tbd,
                      struct retained_user_info *__notnull retained,
                      bool indent,
                      FILE *__notnull prompt_file,
                      const char *__notnull prompt,
                      ...);

__printflike(6, 7)
bool
request_if_should_ignore_flags(struct tbd_for_main *__notnull global,
                               struct tbd_for_main *__notnull tbd,
                               struct retained_user_info *__notnull retained,
                               bool indent,
                               FILE *__notnull prompt_file,
                               const char *__notnull prompt,
                               ...);

__printflike(6, 7)
bool
request_if_should_ignore_non_unique_uuids(
    struct tbd_for_main *__notnull global,
    struct tbd_for_main *__notnull tbd,
    struct retained_user_info *__notnull retained,
    bool indent,
    FILE *__notnull prompt_file,
    const char *__notnull prompt,
    ...);

#endif /* REQUEST_USER_INPUT_H */
