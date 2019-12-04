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
#include "our_io.h"
#include "parse_or_list_fields.h"
#include "request_user_input.h"

static uint64_t
request_choice(const char *__notnull const prompt,
               const char *__notnull const choices[const],
               const bool indent)
{
    do {
        if (indent) {
            fputs("\t\t", stdout);
        }

        fprintf(stdout, "%s (%s", prompt, choices[0]);

        const char *const *iter = choices + 1;
        const char *choice = *iter;

        for (; choice != NULL; choice = *(++iter)) {
            fprintf(stdout, ", %s", choice);
        }

        fputs("): ", stdout);
        fflush(stdout);

        char *input = NULL;

        size_t input_size = 0;
        ssize_t input_length = our_getline(&input, &input_size, stdin);

        fputc('\r', stdout);

        /*
         * We only have a newline-delimiter in our input-buffer if our length is
         * one.
         */

        if (input_length == 1) {
            free(input);
            continue;
        }

        if (input_length < 0) {
            fprintf(stderr,
                    "\nInternal error: getline() failed while trying to "
                    "receive user input, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        uint64_t index = 0;

        iter = choices;
        choice = *iter;
        input[input_length - 1] = '\0';

        for (; choice != NULL; choice = *(++iter), ++index) {
            if (strcmp(choice, input) == 0) {
                free(input);
                return index;
            }
        }

        free(input);
    } while (true);
}

static char *
request_input(const char *__notnull const prompt,
              const bool indent,
              uint32_t *const length_out)
{
    char *input = NULL;

    size_t input_size = 0;
    ssize_t input_length = 0;

    do {
        if (indent) {
            fputs("\t\t", stdout);
        }

        fprintf(stdout, "%s: ", prompt);
        fflush(stdout);

        input_length = our_getline(&input, &input_size, stdin);

        /*
         * We have an empty user-input if we only have a new-line character.
         */

        if (input_length > 1) {
            break;
        }

        if (input_length < 0) {
            fprintf(stderr,
                    "\nInternal error: getline() failed while trying to "
                    "receive user input, error: %s\n",
                    strerror(errno));

            exit(1);
        }

        free(input);
        input = NULL;
    } while (true);

    fputc('\r', stdout);

    /*
     * getline() returns to us a newline delimiter in our input-buffer.
     */

    uint32_t real_input_length = (uint32_t)input_length - 1;
    if (length_out != NULL) {
        *length_out = real_input_length;
    }

    /*
     * We need to remove the delimiter getline() returns back to us.
     */

    input[real_input_length] = '\0';
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
request_current_version(struct tbd_for_main *__notnull const global,
                        struct tbd_for_main *__notnull const tbd,
                        uint64_t *__notnull const info_in,
                        const bool indent,
                        FILE *__notnull file,
                        const char *__notnull const prompt,
                        ...)
{
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_CURRENT_VERS))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_CURRENT_VERSION) {
        const uint32_t global_current_version =
            global->info.fields.current_version;

        if (global_current_version != 0) {
            tbd->info.fields.current_version = global_current_version;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_CURRENT_VERSION;

            return true;
        }
    }

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Replace current-version?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_CURRENT_VERS;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    int64_t current_version = 0;

    do {
        char *const input =
            request_input("Replacement current-version", indent, NULL);

        current_version = parse_packed_version(input);
        if (current_version != -1) {
            free(input);
            break;
        }

        fprintf(stderr, "%s is not a valid current-version\n", input);
        free(input);
    } while (true);

    tbd->info.fields.current_version = (uint32_t)current_version;
    tbd->parse_options |= O_TBD_PARSE_IGNORE_CURRENT_VERSION;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.fields.current_version = (uint32_t)current_version;
        global->parse_options |= O_TBD_PARSE_IGNORE_CURRENT_VERSION;
    }

    return true;
}

bool
request_compat_version(struct tbd_for_main *__notnull const global,
                       struct tbd_for_main *__notnull const tbd,
                       uint64_t *__notnull const info_in,
                       const bool indent,
                       FILE *__notnull const file,
                       const char *__notnull const prompt,
                       ...)
{
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_COMPAT_VERS))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION) {
        const uint32_t global_compat_version =
            global->info.fields.compatibility_version;

        if (global_compat_version != 0) {
            tbd->info.fields.compatibility_version = global_compat_version;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;

            return true;
        }
    }

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Replace compatiblity-version?",
                       default_choices,
                       indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        *info_in |= F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_COMPAT_VERS;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    int64_t compat_version = 0;

    do {
        char *const input =
            request_input("Replacement compatibility-version", indent, NULL);

        compat_version = parse_packed_version(input);
        if (compat_version != -1) {
            free(input);
            break;
        }

        fprintf(stderr, "%s is not a valid compatibility-version\n", input);
        free(input);
    } while (true);

    tbd->info.fields.compatibility_version = (uint32_t)compat_version;
    tbd->parse_options |= O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.fields.compatibility_version = (uint32_t)compat_version;
        global->parse_options |= O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;
    }

    return true;
}

bool
request_install_name(struct tbd_for_main *__notnull const global,
                     struct tbd_for_main *__notnull const tbd,
                     uint64_t *__notnull const info_in,
                     const bool indent,
                     FILE *__notnull const file,
                     const char *__notnull const prompt,
                     ...)
{
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_INSTALL_NAME))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_INSTALL_NAME) {
        const char *const global_install_name =
            global->info.fields.install_name;

        if (global_install_name != NULL) {
            tbd->info.fields.install_name = global_install_name;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_INSTALL_NAME;

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

    uint32_t install_name_length = 0;
    const char *const install_name =
        request_input("Replacement install-name", indent, &install_name_length);

    tbd->info.fields.install_name = install_name;
    tbd->info.fields.install_name_length = install_name_length;

    tbd->info.flags |= F_TBD_CREATE_INFO_INSTALL_NAME_WAS_ALLOCATED;
    tbd->parse_options |= O_TBD_PARSE_IGNORE_INSTALL_NAME;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.fields.install_name = install_name;
        global->info.fields.install_name_length = install_name_length;
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
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_OBJC_CONSTRAINT))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT) {
        const enum tbd_objc_constraint global_objc_constraint =
            global->info.fields.objc_constraint;

        if (global_objc_constraint != TBD_OBJC_CONSTRAINT_NO_VALUE) {
            tbd->info.fields.objc_constraint = global_objc_constraint;
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

    enum tbd_objc_constraint objc_constraint = TBD_OBJC_CONSTRAINT_NO_VALUE;

    do {
        char *const input =
            request_input("Replacement objc-constraint? (Enter "
                          "--list-objc-constraint to list all objc-constraints",
                          indent,
                          NULL);

        if (strcmp(input, "--list-objc_constraint") == 0) {
            print_objc_constraint_list();
            free(input);

            continue;
        }

        objc_constraint = parse_objc_constraint(input);
        if (objc_constraint == TBD_OBJC_CONSTRAINT_NO_VALUE) {
            fprintf(stderr, "Unrecognized objc-constraint type: %s\n", input);
            free(input);

            continue;
        }

        tbd->info.fields.objc_constraint = objc_constraint;
        free(input);

        break;
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.fields.objc_constraint = objc_constraint;
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
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PARENT_UMBRELLA))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_PARENT_UMBRELLA) {
        const char *const global_parent_umbrella =
            global->info.fields.parent_umbrella;

        if (global_parent_umbrella != NULL) {
            tbd->info.fields.parent_umbrella = global_parent_umbrella;
            tbd->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;

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

    uint32_t parent_umbrella_length = 0;
    const char *const parent_umbrella =
        request_input("Replacement parent-umbrella",
                      indent,
                      &parent_umbrella_length);

    tbd->info.fields.parent_umbrella = parent_umbrella;
    tbd->info.fields.parent_umbrella_length = parent_umbrella_length;

    tbd->info.flags |= F_TBD_CREATE_INFO_PARENT_UMBRELLA_WAS_ALLOCATED;
    tbd->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.fields.parent_umbrella = parent_umbrella;
        global->info.fields.parent_umbrella_length = parent_umbrella_length;
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
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_PLATFORM))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_PLATFORM) {
        const enum tbd_platform global_platform = global->info.fields.platform;
        if (global_platform != TBD_PLATFORM_NONE) {
            tbd->info.fields.platform = global_platform;
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

    enum tbd_platform platform = TBD_PLATFORM_NONE;

    do {
        char *const input =
            request_input("Replacement platform? (Enter --list-platform to "
                          "list all platforms)",
                          indent,
                          NULL);

        if (strcmp(input, "--list-platform") == 0) {
            print_platform_list();
            free(input);

            continue;
        }

        platform = parse_platform(input);
        if (platform == TBD_PLATFORM_NONE) {
            fprintf(stderr, "Unrecognized platform: %s\n", input);
            free(input);

            continue;
        }

        tbd->info.fields.platform = platform;
        free(input);

        break;
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.fields.platform = platform;
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
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_REPLACE_SWIFT_VERSION))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (global->parse_options & O_TBD_PARSE_IGNORE_SWIFT_VERSION) {
        const uint32_t global_swift_version = global->info.fields.swift_version;
        if (global_swift_version != 0) {
            tbd->info.fields.swift_version = global_swift_version;
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

    uint32_t swift_version = 0;

    do {
        char *const input =
            request_input("Replacement swift-version", indent, NULL);

        swift_version = parse_swift_version(input);
        if (swift_version == 0) {
            fprintf(stderr, "A swift-version of %s is invalid\n", input);
            free(input);

            continue;
        }

        tbd->info.fields.swift_version = swift_version;
        free(input);

        break;
    } while (true);

    tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        global->info.fields.swift_version = swift_version;
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
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_FLAGS))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

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
    if ((tbd->flags & F_TBD_FOR_MAIN_NO_REQUESTS) ||
        (*info_in & F_RETAINED_USER_INPUT_INFO_NEVER_IGNORE_NON_UNIQUE_UUIDS))
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

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
