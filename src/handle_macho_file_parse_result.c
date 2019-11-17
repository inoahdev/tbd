//
//  src/handle_macho_file_parse_result.c
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <errno.h>
#include <string.h>

#include "handle_macho_file_parse_result.h"
#include "request_user_input.h"

bool
handle_macho_file_for_main_error_callback(
    struct tbd_create_info *__unused __notnull const info_in,
    const enum macho_file_parse_error error,
    void *const callback_info)
{
    const struct handle_macho_file_parse_error_cb_info *const cb_info =
        (const struct handle_macho_file_parse_error_cb_info *)callback_info;

    bool request_result = false;
    switch (error) {
        case ERR_MACHO_FILE_PARSE_CURRENT_VERSION_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has architectures with "
                        "multiple current-versions that conflict with one "
                        "another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures with "
                        "multiple current-versions that conflict with one "
                        "another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "multiple current-versions that conflict with one "
                      "another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_COMPAT_VERSION_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has architectures with "
                        "multiple compatibility-versions that conflict with "
                        "one another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures with "
                        "multiple compatibility-versions that conflict with "
                        "one another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "multiple compatibility-versions that conflict with one "
                      "another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_FILETYPE_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has architectures with "
                        "multiple mach-o filetypes that conflict with one "
                        "another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures with "
                        "multiple mach-o filetypes that conflict with one "
                        "another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "multiple mach-o filetypes that conflict with one "
                      "another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_FLAGS_CONFLICT:
            if (cb_info->is_recursing) {
                request_result =
                    request_if_should_ignore_flags(cb_info->global,
                                                   cb_info->tbd,
                                                   cb_info->retained_info_in,
                                                   false,
                                                   stderr,
                                                   "Mach-o file (at "
                                                   "path %s/%s) has "
                                                   "architectures with "
                                                   "multiple, differing, set "
                                                   "of flags that conflict "
                                                   "with one another\n",
                                                   cb_info->dir_path,
                                                   cb_info->name);

            } else if (cb_info->print_paths) {
                request_result =
                    request_if_should_ignore_flags(cb_info->global,
                                                   cb_info->tbd,
                                                   cb_info->retained_info_in,
                                                   false,
                                                   stderr,
                                                   "Mach-o file (at path %s) "
                                                   "has architectures with "
                                                   "multiple, differing, set "
                                                   "of flags that conflict "
                                                   "with one another\n",
                                                   cb_info->dir_path);
            } else {
                request_result =
                    request_if_should_ignore_flags(cb_info->global,
                                                   cb_info->tbd,
                                                   cb_info->retained_info_in,
                                                   false,
                                                   stderr,
                                                   "The provided mach-o file "
                                                   "has architectures with "
                                                   "multiple, differing set of "
                                                   "flags that conflict with "
                                                   "another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INSTALL_NAME_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has architectures with "
                        "multiple install-names that conflict with one "
                        "another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures with "
                        "multiple install-names that conflict with one "
                        "another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "multiple install-names that conflict with one "
                      "another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_INVALID_PLATFORM:
            if (cb_info->is_recursing) {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s/%s), or one of "
                                     "its architectures, has an invalid "
                                     "platform\n",
                                     cb_info->dir_path,
                                     cb_info->name);

            } else if (cb_info->print_paths) {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s), or one of its "
                                     "architectures, has an invalid platform\n",
                                     cb_info->dir_path);
            } else {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "The provided mach-o file, or one of its "
                                     "architectures, has an invalid "
                                     "platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA:
            if (cb_info->is_recursing) {
                request_result =
                    request_parent_umbrella(cb_info->global,
                                            cb_info->tbd,
                                            cb_info->retained_info_in,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s/%s), or "
                                            "one of its architectures, has an "
                                            "invalid parent-umbrella\n",
                                            cb_info->dir_path,
                                            cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_parent_umbrella(cb_info->global,
                                            cb_info->tbd,
                                            cb_info->retained_info_in,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s), or one "
                                            "of its architectures, has an "
                                            "invalid parent-umbrella\n",
                                            cb_info->dir_path);
            } else {
                request_result =
                    request_parent_umbrella(cb_info->global,
                                            cb_info->tbd,
                                            cb_info->retained_info_in,
                                            false,
                                            stderr,
                                            "The provided mach-o file, or one "
                                            "of its architectures, has an "
                                            "invalid parent-umbrella\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INVALID_UUID:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an invalid uuid\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid uuid\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid uuid\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_OBJC_CONSTRAINT_CONFLICT:
            if (cb_info->is_recursing) {
                request_result =
                    request_objc_constraint(cb_info->global,
                                            cb_info->tbd,
                                            cb_info->retained_info_in,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s/%s) has "
                                            "architectures with multiple "
                                            "objc-constraints that conflict "
                                            "with one another\n",
                                            cb_info->dir_path,
                                            cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_objc_constraint(cb_info->global,
                                            cb_info->tbd,
                                            cb_info->retained_info_in,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s) has "
                                            "architectures with multiple "
                                            "objc-constraints that conflict "
                                            "with one another\n",
                                            cb_info->dir_path);
            } else {
                request_result =
                    request_objc_constraint(cb_info->global,
                                            cb_info->tbd,
                                            cb_info->retained_info_in,
                                            false,
                                            stderr,
                                            "The provided mach-o file has "
                                            "architectures with multiple "
                                            "objc-constraints that conflict "
                                            "with one another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_PARENT_UMBRELLA_CONFLICT:
            if (cb_info->print_paths) {
                request_result =
                    request_parent_umbrella(cb_info->global,
                                            cb_info->tbd,
                                            cb_info->retained_info_in,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s) has "
                                            "architectures with multiple "
                                            "parent-umbrellas that conflict "
                                            "with one another\n",
                                            cb_info->dir_path);
            } else {
                request_result =
                    request_parent_umbrella(cb_info->global,
                                            cb_info->tbd,
                                            cb_info->retained_info_in,
                                            false,
                                            stderr,
                                            "The provided mach-o file has "
                                            "architectures with multiple "
                                            "parent-umbrellas that conflict "
                                            "with one another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT:
            if (cb_info->is_recursing) {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s/%s) has "
                                     "architectures with multiple platforms "
                                     "that conflict with one another\n",
                                     cb_info->dir_path,
                                     cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s) has "
                                     "architectures with multiple platforms "
                                     "that conflict with one another\n",
                                     cb_info->dir_path);
            } else {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "The provided mach-o file has "
                                     "architectures with multiple platforms "
                                     "that conflict with one another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_SWIFT_VERSION_CONFLICT:
            if (cb_info->is_recursing) {
                request_result =
                    request_swift_version(cb_info->global,
                                          cb_info->tbd,
                                          cb_info->retained_info_in,
                                          false,
                                          stderr,
                                          "Mach-o file (at path %s/%s) has "
                                          "architectures with multiple "
                                          "swift-versions that conflict with "
                                          "one another\n",
                                          cb_info->dir_path,
                                          cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_swift_version(cb_info->global,
                                          cb_info->tbd,
                                          cb_info->retained_info_in,
                                          false,
                                          stderr,
                                          "Mach-o file (at path %s) has "
                                          "architectures with multiple "
                                          "swift-versions that conflict with "
                                          "one another\n",
                                          cb_info->dir_path);
            } else {
                request_result =
                    request_swift_version(cb_info->global,
                                          cb_info->tbd,
                                          cb_info->retained_info_in,
                                          false,
                                          stderr,
                                          "The provided mach-o file has "
                                          "architectures with multiple "
                                          "swift-versions that conflict with "
                                          "one another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_UUID_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has multiple "
                        "architectures with the same uuid\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has multiple architectures "
                        "with the same uuid\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has multiple architectures "
                      "with the same uuid\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_WRONG_FILETYPE:
            /*
             * We simply ignore any mach-o non-dynamic-library files while
             * recursing.
             */

            if (cb_info->is_recursing) {
                return false;
            }

            if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has the wrong mach-o filetype\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has the wrong mach-o filetype\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_INVALID_INSTALL_NAME:
            if (cb_info->is_recursing) {
                request_result =
                    request_install_name(cb_info->global,
                                         cb_info->tbd,
                                         cb_info->retained_info_in,
                                         false,
                                         stderr,
                                         "Mach-o file (at path %s/%s), or one "
                                         "of its architectures, has an invalid "
                                         "install-name\n",
                                         cb_info->dir_path,
                                         cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_install_name(cb_info->global,
                                         cb_info->tbd,
                                         cb_info->retained_info_in,
                                         false,
                                         stderr,
                                         "Mach-o file (at path %s), or one of "
                                         "its architectures, has an invalid "
                                         "install-name\n",
                                         cb_info->dir_path);
            } else {
                request_result =
                    request_install_name(cb_info->global,
                                         cb_info->tbd,
                                         cb_info->retained_info_in,
                                         false,
                                         stderr,
                                         "The provided mach-o file, or one of "
                                         "its architectures, has an invalid "
                                         "install-name\n");
            }

            if (!request_result) {
               return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_NOT_A_DYNAMIC_LIBRARY:
            /*
             * We simply ignore any mach-o non-dynamic-library files while
             * recursing.
             */

            if (cb_info->is_recursing) {
                return false;
            }

            if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) is not a dynamic-library\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file is not a dynamic library\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_NO_PLATFORM:
            if (cb_info->is_recursing) {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s/%s), does not "
                                     "have a platform\n",
                                     cb_info->dir_path,
                                     cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s), does not have "
                                     "a platform\n",
                                     cb_info->dir_path);
            } else {
                request_result =
                    request_platform(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     false,
                                     stderr,
                                     "The provided mach-o file does not have a "
                                     "platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_NO_UUID:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, does not have a uuid\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures does not have a uuid\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "does not have a uuid\n ",
                      stderr);
            }

            return false;
    }

    return true;
}

bool
handle_macho_file_parse_result(
    const struct handle_macho_file_parse_result_args args)
{
    switch (args.parse_result) {
        case E_MACHO_FILE_PARSE_OK:
            break;

        case E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK:
            return false;

        case E_MACHO_FILE_PARSE_NOT_A_MACHO:
            if (args.print_paths) {
                fprintf(stderr,
                        "File (at path %s) is not a valid mach-o file\n",
                        args.dir_path);
            } else {
                fputs("File at provided path is not a valid mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_SEEK_FAIL:
        case E_MACHO_FILE_PARSE_READ_FAIL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to read data while parsing mach-o file (at "
                        "path: %s)\n",
                        args.dir_path);
            } else {
                fputs("Failed to read data while parsing mach-o file at the "
                      "provided path\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_FSTAT_FAIL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to get information on mach-o file "
                        "(at path: %s)\n",
                        args.dir_path);
            } else {
                fputs("Failed to get information on mach-o file at the "
                      "provided path\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_SIZE_TOO_SMALL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, is too small to be a valid mach-o\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "is too small to be a valid mach-o\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_RANGE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid range\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid range\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an unsupported cpu-type\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an unsupported cpu-type\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_ARCHITECTURES:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has no architectures\n",
                        args.dir_path);
            } else {
                fputs("Mach-o file at the provided path has no architectures\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has too many architectures "
                        "to fit inside a mach-o file\n",
                        args.dir_path);
            } else {
                fputs("Mach-o file at the provided path has too many "
                      "architectures to fit inside a mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has an invalid "
                        "architecture\n",
                        args.dir_path);
            } else {
                fputs("Mach-o file at the provided path has an invalid "
                      "architecture\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has overlapping "
                        "architectures\n",
                        args.dir_path);
            } else {
                fputs("Mach-o file at the provided path has overlapping "
                      "architectures\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has multiple architectures "
                        "for the same cpu-type\n",
                        args.dir_path);
            } else {
                fputs("Mach-o file at the provided path has multiple "
                      "architectures for the same cpu-type\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has no valid architectures "
                        "that can be parsed\n",
                        args.dir_path);
            } else {
                fputs("Mach-o file at the provided path has no valid "
                      "architectures that can be parsed\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_ALLOC_FAIL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to allocate data while parsing mach-o file (at "
                        "path %s)\n",
                        args.dir_path);
            } else {
                fputs("Failed to allocate data while parsing the provided "
                      "mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_ARRAY_FAIL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to add items to an array while parsing mach-o "
                        "file (at path %s)\n",
                        args.dir_path);
            } else {
                fputs("Failed to add items to an array while parsing the "
                      "provided mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has no load-commands.\n"
                        "Because of this, no information was retrieved\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has no load-commands.\nBecause of this, no information "
                      "was retrieved\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has too many load-commands for its "
                        "size\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has too many load-commands for its "
                      "size\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has too small an area to store a "
                        "load-command\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has too small to store a load-command\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid load-command\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid load-command\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has a segment with too many "
                        "sections for its size\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has a segment with too many sections "
                      "for its size\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_SECTION:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has a segment with an invalid "
                        "section\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has a segment with an invalid "
                      "section\n",
                       stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_CLIENT:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid client-string\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid client-string\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_REEXPORT:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid re-export\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid re-export\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_STRING_TABLE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid string-table\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid string-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid symbol-table\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid symbol-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures with "
                        "conflicting cpu-types\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "conflicting cpu-types\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_EXPORTS:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has no exported clients, re-exports, "
                        "or symbols to be written out\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file has no exported clients, "
                      "re-exports, or symbols to be written out\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, doesn't have a symbol-table\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "doesn't have a symbol-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE:
            fputs("The provided mach-o file has an invalid exports-trie\n",
                  stderr);

            return false;

        case E_MACHO_FILE_PARSE_CREATE_SYMBOLS_FAIL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to create list of symbols while parsing mach-o "
                        "file at path: %s\n",
                        args.dir_path);
            } else {
                fputs("Failed to create list of symbols while parsing the "
                      "provided mach-o file\n",
                      stderr);
            }

            return false;
    }

    return true;
}

bool
handle_macho_file_parse_result_while_recursing(
    const struct handle_macho_file_parse_result_args args)
{
    switch (args.parse_result) {
        case E_MACHO_FILE_PARSE_OK:
            break;

        case E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK:
            return false;

        case E_MACHO_FILE_PARSE_NOT_A_MACHO:
            fprintf(stderr,
                    "File (at path %s/%s) is not a valid mach-o file\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_SEEK_FAIL:
        case E_MACHO_FILE_PARSE_READ_FAIL:
            fprintf(stderr,
                    "Failed to read data while parsing mach-o file (at "
                    "path: %s/%s)\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_FSTAT_FAIL:
            fprintf(stderr,
                    "Failed to get information on mach-o file "
                    "(at path: %s/%s)\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_SIZE_TOO_SMALL:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, is too small to be a valid "
                    "mach-o file\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_RANGE:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has an invalid range\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has an unsupported cpu-type\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_NO_ARCHITECTURES:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has no architectures\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has too many "
                    "architectures to fit inside a mach-o file\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has an invalid "
                    "architecture\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has overlapping "
                    "architectures\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has multiple "
                    "architectures for the same cpu-type\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has no valid "
                    "architectures that can be parsed\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_ALLOC_FAIL:
            fprintf(stderr,
                    "Failed to allocate data while parsing mach-o file (at "
                    "path %s/%s)\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_ARRAY_FAIL:
            fprintf(stderr,
                    "Failed to add items to an array while parsing mach-o "
                    "file (at path %s/%s)\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has no load-commands. "
                    "Subsequently, no information was extracted\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has too many load-commands for its "
                    "size\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has too small an area to store all "
                    "of its load-commands\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has an invalid load-command\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has a segment with too many "
                    "sections for its size\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_SECTION:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has a segment with an invalid "
                    "section\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_CLIENT:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has an invalid client-string\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_REEXPORT:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has an invalid re-export\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_STRING_TABLE:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has an invalid string-table\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s), or one of its "
                    "architectures, has an invalid symbol-table\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has architectures with "
                    "conflicting cpu-types\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_NO_EXPORTS: {
            if (args.tbd->flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                return false;
            }

            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has no exported clients, "
                    "re-exports, or symbols to be written out\n",
                    args.dir_path,
                    args.name);

            return false;
        }

        case E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has no symbol-table\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE:
            fprintf(stderr,
                    "Mach-o file (at path %s/%s) has an invalid exports-trie\n",
                    args.dir_path,
                    args.name);

            return false;

        case E_MACHO_FILE_PARSE_CREATE_SYMBOLS_FAIL:
            fprintf(stderr,
                    "Failed to create symbols-array while parsing mach-o file "
                    "at path: %s/%s\n",
                    args.dir_path,
                    args.name);

            return false;
    }

    return true;
}
