//
//  src/request_user_input.c
//  tbd
//
//  Created by inoahdev on 13/02/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#include <errno.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "our_io.h"
#include "parse_or_list_fields.h"
#include "request_user_input.h"
#include "tbd_for_main.h"

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

        fputc('\r', stdout);

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
request_current_version(struct tbd_for_main *__notnull const orig,
                        struct tbd_for_main *__notnull const tbd,
                        const bool indent,
                        FILE *__notnull file,
                        const char *__notnull const prompt,
                        ...)
{
    if (tbd->options.no_requests ||
        tbd->retained.never_replace_current_version)
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (orig->parse_options.ignore_current_version) {
        const uint32_t orig_current_version = orig->info.fields.current_version;
        if (orig_current_version != 0) {
            tbd->info.fields.current_version = orig_current_version;
            tbd->parse_options.ignore_current_version = true;

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
        tbd->retained.never_replace_current_version = true;
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
    tbd->parse_options.ignore_current_version = true;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        orig->info.fields.current_version = (uint32_t)current_version;
        orig->parse_options.ignore_current_version = true;
    }

    return true;
}

bool
request_compat_version(struct tbd_for_main *__notnull const orig,
                       struct tbd_for_main *__notnull const tbd,
                       const bool indent,
                       FILE *__notnull const file,
                       const char *__notnull const prompt,
                       ...)
{
    if (tbd->options.no_requests ||
        tbd->retained.never_replace_compat_version)
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (orig->parse_options.ignore_compat_version) {
        const uint32_t orig_compat_version =
            orig->info.fields.compatibility_version;

        if (orig_compat_version != 0) {
            tbd->info.fields.compatibility_version = orig_compat_version;
            tbd->parse_options.ignore_compat_version = true;

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
        tbd->retained.never_replace_compat_version = true;
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
    tbd->parse_options.ignore_compat_version = true;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        orig->info.fields.compatibility_version = (uint32_t)compat_version;
        orig->parse_options.ignore_compat_version = true;
    }

    return true;
}

bool
request_install_name(struct tbd_for_main *__notnull const orig,
                     struct tbd_for_main *__notnull const tbd,
                     const bool indent,
                     FILE *__notnull const file,
                     const char *__notnull const prompt,
                     ...)
{
    if (tbd->options.no_requests ||
        tbd->retained.never_replace_install_name)
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (orig->parse_options.ignore_install_name) {
        const char *const orig_install_name = orig->info.fields.install_name;
        const uint64_t orig_install_name_length =
            orig->info.fields.install_name_length;

        if (orig_install_name != NULL && orig_install_name_length != 0) {
            tbd->info.fields.install_name = orig_install_name;
            tbd->info.fields.install_name_length = orig_install_name_length;
            tbd->parse_options.ignore_install_name = true;

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
        tbd->retained.never_replace_install_name = true;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    uint32_t install_name_length = 0;
    const char *const install_name =
        request_input("Replacement install-name", indent, &install_name_length);

    tbd->info.fields.install_name = install_name;
    tbd->info.fields.install_name_length = install_name_length;
    tbd->parse_options.ignore_install_name = true;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        orig->info.fields.install_name = install_name;
        orig->info.fields.install_name_length = install_name_length;
        orig->parse_options.ignore_install_name = true;
    } else {
        /*
         * Have install-name freed only if we're not giving it to orig.
         */

        tbd->info.flags.install_name_was_allocated = true;
    }

    return true;
}

bool
request_objc_constraint(struct tbd_for_main *__notnull const orig,
                        struct tbd_for_main *__notnull const tbd,
                        const bool indent,
                        FILE *__notnull const file,
                        const char *__notnull const prompt,
                        ...)
{
    if (tbd->options.no_requests ||
        tbd->retained.never_replace_objc_constraint)
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (orig->parse_options.ignore_objc_constraint) {
        const enum tbd_objc_constraint orig_objc_constraint =
            orig->info.fields.archs.objc_constraint;

        if (orig_objc_constraint != TBD_OBJC_CONSTRAINT_NO_VALUE) {
            tbd->info.fields.archs.objc_constraint = orig_objc_constraint;
            tbd->parse_options.ignore_objc_constraint = true;
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
        tbd->retained.never_replace_objc_constraint = true;
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

        tbd->info.fields.archs.objc_constraint = objc_constraint;
        free(input);

        break;
    } while (true);

    tbd->parse_options.ignore_objc_constraint = true;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        orig->info.fields.archs.objc_constraint = objc_constraint;
        orig->parse_options.ignore_objc_constraint = true;
    }

    return true;
}

bool
request_parent_umbrella(struct tbd_for_main *__notnull const orig,
                        struct tbd_for_main *__notnull const tbd,
                        const bool indent,
                        FILE *__notnull const file,
                        const char *__notnull const prompt,
                        ...)
{
    if (tbd->options.no_requests ||
        tbd->retained.never_replace_parent_umbrella)
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    struct tbd_parse_options tbd_options = {};
    if (orig->parse_options.ignore_parent_umbrellas) {
        const struct tbd_metadata_info *const orig_umbrella_info =
            tbd_ci_get_single_parent_umbrella(&orig->info);

        if (orig_umbrella_info != NULL) {
            const enum tbd_ci_add_parent_umbrella_result add_umbrella_result =
                tbd_ci_add_parent_umbrella(&tbd->info,
                                           orig_umbrella_info->string,
                                           orig_umbrella_info->length,
                                           0,
                                           tbd_options);

            if (add_umbrella_result != E_TBD_CI_ADD_PARENT_UMBRELLA_OK) {
                fputs("Failed to add missing parent-umbrella\n", stderr);
                exit(1);
            }

            tbd->parse_options.ignore_parent_umbrellas = true;
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
        tbd->retained.never_replace_parent_umbrella = true;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    uint32_t parent_umbrella_length = 0;
    const char *const parent_umbrella =
        request_input("Replacement parent-umbrella",
                      indent,
                      &parent_umbrella_length);

    const enum tbd_ci_add_parent_umbrella_result add_umbrella_result =
        tbd_ci_add_parent_umbrella(&tbd->info,
                                   parent_umbrella,
                                   parent_umbrella_length,
                                   0,
                                   tbd_options);

    if (add_umbrella_result != E_TBD_CI_ADD_PARENT_UMBRELLA_OK) {
        fputs("Failed to set provided parent-umbrella\n", stderr);
        exit(1);
    }

    tbd->parse_options.ignore_parent_umbrellas = true;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        const enum tbd_ci_add_parent_umbrella_result add_orig_umbrella_result =
            tbd_ci_add_parent_umbrella(&tbd->info,
                                       parent_umbrella,
                                       parent_umbrella_length,
                                       0,
                                       tbd_options);

        if (add_orig_umbrella_result != E_TBD_CI_ADD_PARENT_UMBRELLA_OK) {
            fputs("Failed to set provided parent-umbrella to all\n", stderr);
            exit(1);
        }

        orig->parse_options.ignore_parent_umbrellas = true;
    }

    return true;
}

bool
request_platform(struct tbd_for_main *__notnull const orig,
                 struct tbd_for_main *__notnull const tbd,
                 const bool indent,
                 FILE *__notnull const file,
                 const char *__notnull const prompt,
                 ...)
{
    if (tbd->options.no_requests || tbd->retained.never_replace_platform) {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (orig->parse_options.ignore_platform) {
        const enum tbd_platform orig_platform =
            tbd_ci_get_single_platform(&orig->info);

        if (orig_platform != TBD_PLATFORM_NONE) {
            tbd_ci_set_single_platform(&tbd->info, orig_platform);
            tbd->parse_options.ignore_platform = true;
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
        tbd->retained.never_replace_platform = true;
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

        tbd_ci_set_single_platform(&tbd->info, platform);
        free(input);

        break;
    } while (true);

    tbd->parse_options.ignore_platform = true;;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        tbd_ci_set_single_platform(&orig->info, platform);
        orig->parse_options.ignore_platform = true;
    }

    return true;
}

bool
request_swift_version(struct tbd_for_main *__notnull const orig,
                      struct tbd_for_main *__notnull const tbd,
                      const bool indent,
                      FILE *__notnull const file,
                      const char *__notnull const prompt,
                      ...)
{
    if (tbd->options.no_requests || tbd->retained.never_replace_swift_version) {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (orig->parse_options.ignore_swift_version) {
        const uint32_t orig_swift_version = orig->info.fields.swift_version;
        if (orig_swift_version != 0) {
            tbd->info.fields.swift_version = orig_swift_version;
            tbd->parse_options.ignore_swift_version = true;
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
        tbd->retained.never_replace_swift_version = true;
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

    tbd->parse_options.ignore_swift_version = true;

    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        orig->info.fields.swift_version = swift_version;
        orig->parse_options.ignore_swift_version = true;
    }

    return true;
}

bool
request_if_should_ignore_flags(struct tbd_for_main *__notnull const orig,
                               struct tbd_for_main *__notnull const tbd,
                               const bool indent,
                               FILE *__notnull const file,
                               const char *__notnull const prompt,
                               ...)
{
    if (tbd->options.no_requests || tbd->retained.never_ignore_flags) {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (orig->parse_options.ignore_flags) {
        tbd->parse_options.ignore_flags = true;
        return true;
    }

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Ignore flags?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        tbd->retained.never_ignore_flags = true;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    tbd->parse_options.ignore_flags = true;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        orig->parse_options.ignore_flags = true;
    }

    return true;
}

bool
request_if_should_ignore_non_unique_uuids(
    struct tbd_for_main *__notnull const orig,
    struct tbd_for_main *__notnull const tbd,
    const bool indent,
    FILE *__notnull const file,
    const char *__notnull const prompt,
    ...)
{
    if (tbd->options.no_requests ||
        tbd->retained.never_ignore_non_unique_uuids)
    {
        va_list args;
        va_start(args, prompt);

        vfprintf(file, prompt, args);
        va_end(args);

        return false;
    }

    if (orig->parse_options.ignore_non_unique_uuids) {
        tbd->parse_options.ignore_non_unique_uuids = true;
        return true;
    }

    va_list args;
    va_start(args, prompt);

    vfprintf(file, prompt, args);
    va_end(args);

    const uint64_t choice_index =
        request_choice("Ignore non-unique uuids?", default_choices, indent);

    if (choice_index == DEFAULT_CHOICE_INDEX_NEVER) {
        tbd->retained.never_ignore_non_unique_uuids = true;
        return false;
    } else if (choice_index == DEFAULT_CHOICE_INDEX_NO) {
        return false;
    }

    tbd->parse_options.ignore_non_unique_uuids = true;
    if (choice_index == DEFAULT_CHOICE_INDEX_FOR_ALL) {
        orig->parse_options.ignore_non_unique_uuids = true;
    }

    return true;
}
