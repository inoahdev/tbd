//
//  src/main_utils/tbd_parse_fields.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "tbd_parse_fields.h"

namespace main_utils {
    void parse_flags(struct macho::utils::tbd::flags *flags, int &index, int argc, const char *argv[]) {
        auto jndex = index;

        for (; jndex < argc; jndex++) {
            const auto argument = argv[jndex];
            if (strcmp(argument, "flat_namespace") == 0) {
                if (flags != nullptr) {
                    flags->set_has_flat_namespace();
                }
            } else if (strcmp(argument, "not_app_extension_safe") == 0) {
                if (flags != nullptr) {
                    flags->set_not_app_extension_safe();
                }
            } else {
                // Make sure the user provided at-least one flag

                if (jndex == index) {
                    fprintf(stderr, "Unrecognized tbd-flag: %s\n", argument);
                    exit(1);
                }

                break;
            }
        }

        index = jndex;
    }

    enum macho::utils::tbd::objc_constraint parse_objc_constraint_from_argument(int &index, int argc, const char *argv[]) {
        index++;
        if (index == argc) {
            fputs("Please provide an objc-constraint to replace one found\n", stderr);
            exit(1);
        }

        const auto objc_constraint = macho::utils::tbd::objc_constraint_from_string(argv[index]);
        if (objc_constraint == macho::utils::tbd::objc_constraint::none && strcmp(argv[index], "none") != 0) {
            fprintf(stderr, "Unrecognized objc-constraint value: %s\n", argv[index]);
            exit(1);
        }

        return objc_constraint;
    }

    enum macho::utils::tbd::platform parse_platform_from_argument(int &index, int argc, const char *argv[]) {
        ++index;
        if (index == argc) {
            fputs("Please provide a tbd-platform. Run --list-platforms to see a list of valid platforms\n", stderr);
            exit(1);
        }

        const auto argument = argv[index];
        const auto platform = macho::utils::tbd::platform_from_string(argument);

        if (platform == macho::utils::tbd::platform::none) {
            fprintf(stderr, "Unrecognized tbd-platform: %s\n", argument);
            exit(1);
        }

        return platform;
    }

    enum macho::utils::tbd::version parse_tbd_version(int &index, int argc, const char *argv[]) {
        ++index;
        if (index == argc) {
            fputs("Please provide a tbd-version. Run --list-versions to see a list of tbd-versions\n", stderr);
        }

        const auto argument = argv[index];
        if (strcmp(argument, "v1") == 0) {
            return macho::utils::tbd::version::v1;
        } else if (strcmp(argument, "v2") == 0) {
            return macho::utils::tbd::version::v2;
        }

        fprintf(stderr, "Unrecognized tbd-version: %s\n", argument);
        exit(1);
    }
}
