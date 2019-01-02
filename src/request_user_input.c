//
//  src/request_user_input.c
//  tbd
//
//  Created by inoahdev on 13/02/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <ctype.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>

#include "parse_or_list_fields.h"
#include "request_user_input.h"

static void
request_input(const char *const prompt,
              const char *const inputs[const],
              char **const result_out)
{
    do {
        fputs(prompt, stdout);

        if (inputs != NULL) {
            fprintf(stdout, " (%s", inputs[0]);
            
            const char *const *iter = &inputs[1];
            const char *acceptable_input = *iter;

            for (; acceptable_input != NULL; acceptable_input = *(++iter)) {
                fprintf(stdout, ", %s", acceptable_input);
            }

            fputc(')', stdout);
        }

        fputs(": ", stdout);
        fflush(stdout);

        char *input = NULL;
        size_t input_size = 0;

        if (getline(&input, &input_size, stdin) < 0) {
            free(input);
            fprintf(stderr,
                    "\nInternal error: getline() failed while trying to "
                    "receive user input, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        if (inputs != NULL) {
            const char *const *iter = inputs;
            const char *acceptable_input = *iter;

            for (; acceptable_input != NULL; acceptable_input = *(++iter)) {
                if (strcmp(acceptable_input, input) == 0) {
                    continue;
                }

                *result_out = input;
                return;
            }
        }
    } while (true);
}

bool
request_install_name(struct tbd_for_main *const global,
                     struct tbd_for_main *const tbd,
                     uint64_t *const info_in)
{
    if (tbd->options & O_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (info_in != NULL) {
        if (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_INSTALL_NAME) {
            return false;
        }
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_INSTALL_NAME) {
        if (global->info.install_name != NULL) {
            const char *const global_install_name = global->info.install_name;
            if (global->info.install_name != NULL) {
                tbd->info.install_name = global_install_name;
                tbd->parse_options |= O_TBD_PARSE_IGNORE_INSTALL_NAME;
            }

            return true;
        }
    }

    char *should_replace = NULL;
    const char *const inputs[] = { "for all", "yes", "no", "never", NULL };

    request_input("Replace install-name?", inputs, &should_replace);

    if (strcmp(should_replace, "never\n") == 0) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_INSTALL_NAME;

        free(should_replace);
        return false;
    } else if (strcmp(should_replace, "no\n") == 0) {
        free(should_replace);
        return false;
    }

    request_input("Replacement install-name?",
                  NULL,
                  (char **)&tbd->info.install_name);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_INSTALL_NAME;
    tbd->info.flags |= F_TBD_CREATE_INFO_STRINGS_WERE_COPIED;

    if (strcmp(should_replace, "for all\n") == 0) {
        global->info.install_name = tbd->info.install_name;
        global->parse_options |= O_TBD_PARSE_IGNORE_INSTALL_NAME;
    }

    free(should_replace);
    return true;
}

bool
request_objc_constraint(struct tbd_for_main *const global,
                        struct tbd_for_main *const tbd,
                        uint64_t *const info_in)
{
    if (tbd->options & O_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (info_in != NULL) {
        const uint64_t info = *info_in;
        if (info & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_OBJC_CONSTRAINT) {
            return false;
        }
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT) {
        const enum tbd_objc_constraint global_objc_constraint =
            global->info.objc_constraint;

        if (global_objc_constraint != 0) {
            tbd->info.objc_constraint = global_objc_constraint;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
        }

        return true;
    }

    char *should_replace = NULL;
    const char *const inputs[] = { "for all", "yes", "no", "never", NULL };

    request_input("Replace objc-constraint?", inputs, &should_replace);

    if (strcmp(should_replace, "never\n") == 0) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_OBJC_CONSTRAINT;
        
        free(should_replace);
        return false;
    } else if (strcmp(should_replace, "no\n") == 0) {
        free(should_replace);
        return false;
    }

    do {
        char *input = NULL;
        request_input("Replacement objc-constraint? (Enter "
                      "--list-objc-constraint to list all objc-constraints\n",
                      NULL,
                      &input);

        if (strcmp(input, "--list-objc_constraint") == 0) {
            fputs("none\n"
                  "retain_release\n"
                  "retain_release_or_gc\n"
                  "retain_release_for_simulator\n"
                  "gc\n",
                  stdout);

            continue;
        }

        const enum tbd_objc_constraint objc_constraint =
            parse_objc_constraint(input);

        free(input);

        if (objc_constraint == 0) {
            fprintf(stderr, "Unrecognized objc-constraint type: %s\n", input);
            continue;
        }

        tbd->info.objc_constraint = objc_constraint;
        break;
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
    if (strcmp(should_replace, "for all\n") == 0) {
        global->info.objc_constraint = tbd->info.objc_constraint;
        global->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
    }

    free(should_replace);
    return true;
}

bool
request_parent_umbrella(struct tbd_for_main *const global,
                        struct tbd_for_main *const tbd,
                        uint64_t *const info_in)
{
    if (tbd->options & O_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (info_in != NULL) {
        const uint64_t info = *info_in;
        if (info & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PARENT_UMBRELLA) {
            return false;
        }
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_PARENT_UMBRELLA) {
        const char *const global_parent_umbrella =
            global->info.parent_umbrella;

        if (global_parent_umbrella != NULL) {
            if (global->info.parent_umbrella != NULL) {
                tbd->info.parent_umbrella = global_parent_umbrella;
                tbd->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;
            }

            return true;
        }
    }

    char *should_replace = NULL;
    const char *const inputs[] = { "for all", "yes", "no", "never", NULL };

    request_input("Replace parent-umbrella?", inputs, &should_replace);

    if (strcmp(should_replace, "never\n") == 0) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PARENT_UMBRELLA;

        free(should_replace);
        return false;
    } else if (strcmp(should_replace, "no\n") == 0) {
        free(should_replace);
        return false;
    }

    request_input("Replacement parent-umbrella?",
                  NULL,
                  (char **)&tbd->info.parent_umbrella);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;
    tbd->info.flags |= F_TBD_CREATE_INFO_STRINGS_WERE_COPIED;

    if (strcmp(should_replace, "for all\n") == 0) {
        global->info.parent_umbrella = tbd->info.parent_umbrella;
        global->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;
    }

    free(should_replace);
    return true;
}

bool
request_platform(struct tbd_for_main *const global,
                 struct tbd_for_main *const tbd,
                 uint64_t *const info_in)
{
    if (tbd->options & O_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (info_in != NULL) { 
        if (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PLATFORM) {
            return false;
        }
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_PLATFORM) {
        const enum tbd_platform global_platform = global->info.platform;
        if (global->info.platform != 0) {
            tbd->info.platform = global_platform;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
        }

        return true;
    }

    char *should_replace = NULL;
    const char *const inputs[] = { "for all", "yes", "no", "never", NULL };

    request_input("Replace platform?", inputs, &should_replace);

    if (strcmp(should_replace, "never\n") == 0) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PLATFORM;

        free(should_replace);
        return false;
    } else if (strcmp(should_replace, "no\n") == 0) {
        free(should_replace);
        return false;
    }

    do {
        char *input = NULL;
        request_input("Replacement platform? "
                      "(Enter --list-platform to list all platforms)",
                      NULL,
                      &input);

        if (strcmp(input, "--list-platform") == 0) {
            fputs("macosx\n"
                  "ios\n"
                  "watchos\n"
                  "tvos\n",
                  stdout);

            free(input);
            continue;
        }

        const enum tbd_platform platform = parse_platform(input);
        free(input);

        if (platform == 0) {
            fprintf(stderr, "Unrecognized platform: %s\n", input);
            free(input);

            continue;
        }

        tbd->info.platform = platform;
        break;
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    if (strcmp(should_replace, "for all\n") == 0) {
        global->info.platform = tbd->info.platform;
        global->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    }

    free(should_replace);
    return true;
}

static bool has_non_digits(const char *const string) {
    const char *iter = string;
    for (char ch = *string; ch != '\0'; ch = *(++iter)) {
        if (isdigit(ch) != 0) {
            continue;
        }

        return true;
    }

    return false;
}

bool
request_swift_version(struct tbd_for_main *const global,
                      struct tbd_for_main *const tbd,
                      uint64_t *const info_in)
{
    if (tbd->options & O_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (info_in != NULL) {
        if (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_SWIFT_VERSION) {
            return false;
        }
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION) {
        const uint32_t global_swift_version = global->info.swift_version;
        if (global_swift_version != 0) {
            tbd->info.swift_version = global_swift_version;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
        }

        return true;
    }

    char *should_replace = NULL;
    const char *const inputs[] = { "for all", "yes", "no", "never", NULL };

    request_input("Replace swift-version?", inputs, &should_replace);

    if (strcmp(should_replace, "never\n") == 0) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_SWIFT_VERSION;

        free(should_replace);
        return false;
    } else if (strcmp(should_replace, "no\n") == 0) {
        free(should_replace);
        return false;
    }

    do {
        char *input = NULL;
        request_input("Replacement swift-version?", NULL, &input);

        if (strcmp(input, "1.2") == 0) {
            tbd->info.swift_version = 2;
        } else {
            if (has_non_digits(input)) {
                fprintf(stderr, "%s is not a valid swift-version\n", input);
                free(input);

                continue;
            }
            
            uint32_t swift_version = strtoul(input, NULL, 10);
            free(input);

            if (swift_version > 1) {
                swift_version -= 1;
            }

            tbd->info.swift_version = swift_version;
            break;
        }
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;

    if (strcmp(should_replace, "for all\n") == 0) {
        global->info.swift_version = tbd->info.swift_version;
        global->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
    }

    free(should_replace);
    return true;
}

bool
request_if_should_ignore_flags(struct tbd_for_main *const global,
                               struct tbd_for_main *const tbd,
                               uint64_t *const info_in)
{
    if (tbd->options & O_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (info_in != NULL) {
        if (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_FLAGS) {
            return false;
        }
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_FLAGS) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_FLAGS;
        return true;
    }

    char *should_replace = NULL;
    const char *const inputs[] = { "for all", "yes", "no", "never", NULL };

    request_input("Ignore flags?", inputs, &should_replace);

    if (strcmp(should_replace, "never\n") == 0) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_FLAGS;
        
        free(should_replace);
        return false;
    } else if (strcmp(should_replace, "no\n") == 0) {
        free(should_replace);
        return false;
    }

    tbd->parse_options |= O_TBD_PARSE_IGNORE_FLAGS;
    if (strcmp(should_replace, "for all\n") == 0) {
        global->parse_options |= O_TBD_PARSE_IGNORE_FLAGS;
    }

    free(should_replace);
    return true;
}

bool
request_if_should_ignore_non_unique_uuids(struct tbd_for_main *const global,
                                          struct tbd_for_main *const tbd,
                                          uint64_t *const info_in)
{
    if (tbd->options & O_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (info_in != NULL) {
        const uint64_t info = *info_in;
        if (info & F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_NON_UNIQUE_UUIDS) {
            return false;
        }
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS;
        return true;
    }

    char *should_replace = NULL;
    const char *const inputs[] = { "for all", "yes", "no", "never", NULL };

    request_input("Ignore non-unique uuids?", inputs, &should_replace);

    if (strcmp(should_replace, "never\n") == 0) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_NON_UNIQUE_UUIDS;

        free(should_replace);
        return false;
    } else if (strcmp(should_replace, "no\n") == 0) {
        free(should_replace);
        return false;
    }

    tbd->parse_options |= O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS;
    if (strcmp(should_replace, "for all\n") == 0) {
        global->parse_options |= O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS;
    }

    free(should_replace);
    return true;
}
