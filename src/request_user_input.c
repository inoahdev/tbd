//
//  src/request_user_input.c
//  tbd
//
//  Created by inoahdev on 13/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <sys/types.h>

#include <ctype.h>
#include <errno.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "notnull.h"
#include "parse_or_list_fields.h"
#include "request_user_input.h"

static uint64_t
request_choice(const char *__notnull const prompt,
               const char *__notnull const inputs[const],
               const bool indent)
{
    do {
        if (indent) {
            fputs("\t\t", stdout);
        }

        fprintf(stdout, "%s (%s", prompt, inputs[0]);

        const char *const *iter = inputs + 1;
        const char *acceptable_input = *iter;

        for (; acceptable_input != NULL; acceptable_input = *(++iter)) {
            fprintf(stdout, ", %s", acceptable_input);
        }

        fputs("): ", stdout);
        fflush(stdout);

        char *input = NULL;

        size_t input_size = 0;
        ssize_t input_length = getline(&input, &input_size, stdin);

        if (input_length < 0) {
            free(input);
            fprintf(stderr,
                    "\nInternal error: getline() failed while trying to "
                    "receive user input, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        uint64_t index = 0;
        const uint64_t check_length = (uint64_t)(input_length - 1);

        iter = inputs;
        acceptable_input = *iter;

        for (; acceptable_input != NULL; acceptable_input = *(++iter)) {
            if (strncmp(acceptable_input, input, check_length) != 0) {
                index++;
                continue;
            }

            if (acceptable_input[check_length] != '\0') {
                index++;
                continue;
            }

            free(input);
            return index;
        }

        free(input);
    } while (true);
}

static char *
request_input(const char *__notnull const prompt, const bool indent) {
    if (indent) {
        fputs("\t\t", stdout);
    }

    fprintf(stdout, "%s: ", prompt);
    fflush(stdout);

    char *input = NULL;

    size_t input_size = 0;
    ssize_t input_length = getline(&input, &input_size, stdin);

    if (input_length < 0) {
        free(input);
        fprintf(stderr,
                "\nInternal error: getline() failed while trying to "
                "receive user input, error: %s\n",
                strerror(errno));

        exit(1);
    }

    /*
     * We remove the delimiter getline() returns back to us.
     */

    input[input_length - 1] = '\0';
    return input;
}

static const char *const default_choices[] =
    { "for all", "yes", "no", "never", NULL };

enum default_choice_index {
    DEFAULT_CHOICE_INDEX_FOR_ALL,
    DEFAULT_CHOICE_INDEX_YES,
    DEFAULT_CHOICE_INDEX_NO,
    DEFAULT_CHOICE_INDEX_NEVER
};

bool
request_install_name(struct tbd_for_main *__notnull const global,
                     struct tbd_for_main *__notnull const tbd,
                     uint64_t *__notnull const info_in,
                     const bool indent,
                     FILE *__notnull const file,
                     const char *__notnull const prompt,
                     ...)
{
    if (tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_INSTALL_NAME) {
        return false;
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

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Replace install-name?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_INSTALL_NAME;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    tbd->info.install_name = request_input("Replacement install-name?", indent);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_INSTALL_NAME;
    tbd->info.flags |= F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.install_name = tbd->info.install_name;
        global->parse_options |= O_TBD_PARSE_IGNORE_INSTALL_NAME;
    }

    return true;
}

bool
request_objc_constraint(struct tbd_for_main *__notnull const global,
                        struct tbd_for_main *__notnull const tbd,
                        uint64_t *__notnull const info_in,
                        const bool indent,
                        FILE *__notnull const file,
                        const char *__notnull const prompt,
                        ...)
{
    if (tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    const uint64_t info = *info_in;
    if (info & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_OBJC_CONSTRAINT) {
        return false;
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

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Replace objc-constraint?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_OBJC_CONSTRAINT;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    do {
        char *const input =
            request_input("Replacement objc-constraint? (Enter "
                          "--list-objc-constraint to list all "
                          "objc-constraints",
                          indent);

        if (strcmp(input, "--list-objc_constraint") == 0) {
            print_objc_constraint_list();
            free(input);

            continue;
        }

        const enum tbd_objc_constraint objc_constraint =
            parse_objc_constraint(input);

        if (objc_constraint == 0) {
            fprintf(stderr, "Unrecognized objc-constraint type: %s\n", input);
            free(input);

            continue;
        }

        tbd->info.objc_constraint = objc_constraint;
        free(input);

        break;
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.objc_constraint = tbd->info.objc_constraint;
        global->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
    }

    return true;
}

bool
request_parent_umbrella(struct tbd_for_main *__notnull const global,
                        struct tbd_for_main *__notnull const tbd,
                        uint64_t *__notnull const info_in,
                        const bool indent,
                        FILE *__notnull const file,
                        const char *__notnull const prompt,
                        ...)
{
    if (tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    const uint64_t info = *info_in;
    if (info & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PARENT_UMBRELLA) {
        return false;
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

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Replace parent-umbrella?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PARENT_UMBRELLA;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    tbd->info.parent_umbrella =
        request_input("Replacement parent-umbrella?", indent);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;
    tbd->info.flags |= F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.parent_umbrella = tbd->info.parent_umbrella;
        global->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;
    }

    return true;
}

bool
request_platform(struct tbd_for_main *__notnull const global,
                 struct tbd_for_main *__notnull const tbd,
                 uint64_t *__notnull const info_in,
                 const bool indent,
                 FILE *__notnull const file,
                 const char *__notnull const prompt,
                 ...)
{
    if (tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PLATFORM) {
        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_PLATFORM) {
        const enum tbd_platform global_platform = global->info.platform;
        if (global->info.platform != 0) {
            tbd->info.platform = global_platform;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
        }

        return true;
    }

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Replace platform?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PLATFORM;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    do {
        char *const input =
            request_input("Replacement platform? "
                          "(Enter --list-platform to list all platforms)",
                          indent);

        if (strcmp(input, "--list-platform") == 0) {
            print_platform_list();
            free(input);

            continue;
        }

        const enum tbd_platform platform = parse_platform(input);
        if (platform == 0) {
            fprintf(stderr, "Unrecognized platform: %s\n", input);
            free(input);

            continue;
        }

        tbd->info.platform = platform;
        free(input);

        break;
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.platform = tbd->info.platform;
        global->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    }

    return true;
}

bool
request_swift_version(struct tbd_for_main *__notnull const global,
                      struct tbd_for_main *__notnull const tbd,
                      uint64_t *__notnull const info_in,
                      const bool indent,
                      FILE *__notnull const file,
                      const char *__notnull const prompt,
                      ...)
{
    if (tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_SWIFT_VERSION) {
        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION) {
        const uint32_t global_swift_version = global->info.swift_version;
        if (global_swift_version != 0) {
            tbd->info.swift_version = global_swift_version;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
        }

        return true;
    }

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Replace swift-version?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_SWIFT_VERSION;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    do {
        char *const input = request_input("Replacement swift-version?", indent);
        const uint32_t swift_version = parse_swift_version(input);

        if (swift_version == 0) {
            fprintf(stderr, "A swift-version of %s is invalid\n", input);
            free(input);

            continue;
        }

        free(input);
        break;
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.swift_version = tbd->info.swift_version;
        global->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
    }

    return true;
}

bool
request_if_should_ignore_flags(struct tbd_for_main *__notnull const global,
                               struct tbd_for_main *__notnull const tbd,
                               uint64_t *__notnull const info_in,
                               const bool indent,
                               FILE *__notnull const file,
                               const char *__notnull const prompt,
                               ...)
{
    if (tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    if (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_FLAGS) {
        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_FLAGS) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_FLAGS;
        return true;
    }

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Ignore flags?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_FLAGS;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    tbd->parse_options |= O_TBD_PARSE_IGNORE_FLAGS;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->parse_options |= O_TBD_PARSE_IGNORE_FLAGS;
    }

    return true;
}

bool
request_if_should_ignore_non_unique_uuids(
    struct tbd_for_main *__notnull const global,
    struct tbd_for_main *__notnull const tbd,
    uint64_t *__notnull const info_in,
    const bool indent,
    FILE *__notnull const file,
    const char *__notnull const prompt,
    ...)
{
    if (tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) {
        return false;
    }

    const uint64_t info = *info_in;
    if (info & F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_NON_UNIQUE_UUIDS) {
        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS;
        return true;
    }

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Ignore non-unique uuids?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_NON_UNIQUE_UUIDS;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    tbd->parse_options |= O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->parse_options |= O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS;
    }

    return true;
}
