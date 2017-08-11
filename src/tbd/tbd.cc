//
//  src/tbd/tbd.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>

#include "group.h"
#include "tbd.h"

const char *tbd::platform_to_string(const enum tbd::platform &platform) noexcept {
    switch (platform) {
        case ios:
            return "ios";

        case macosx:
            return "macosx";

        case watchos:
            return "watchos";

        case tvos:
            return "tvos";

        default:
            return nullptr;
    }
}

enum tbd::platform tbd::string_to_platform(const char *platform) noexcept {
    if (strcmp(platform, "ios") == 0) {
        return platform::ios;
    } else if (strcmp(platform, "macosx") == 0) {
        return platform::macosx;
    } else if (strcmp(platform, "watchos") == 0) {
        return platform::watchos;
    } else if (strcmp(platform, "tvos") == 0) {
        return platform::tvos;
    }

    return (enum platform)-1;
}

enum tbd::version tbd::string_to_version(const char *version) noexcept {
    if (strcmp(version, "v1") == 0) {
        return tbd::version::v1;
    } else if (strcmp(version, "v2") == 0) {
        return tbd::version::v2;
    }

    return (enum tbd::version)-1;
}
