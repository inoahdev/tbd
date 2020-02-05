//
//  src/handle_macho_file_parse_result.c
//  tbd
//
//  Created by inoahdev on 12/02/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#include <errno.h>
#include <string.h>

#include "handle_macho_file_parse_result.h"
#include "request_user_input.h"

bool
handle_macho_file_for_main_error_callback(
    struct tbd_create_info *__unused __notnull const info_in,
    const enum macho_file_parse_callback_type type,
    void *const callback_info)
{
    const struct handle_macho_file_parse_error_cb_info *const cb_info =
        (const struct handle_macho_file_parse_error_cb_info *)callback_info;

    bool request_result = false;
    switch (type) {
        case ERR_MACHO_FILE_PARSE_CURRENT_VERSION_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has archs with multiple "
                        "current-versions conflicting with one another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has archs with multiple "
                        "current-versions conflicting with one another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has archs with multiple "
                      "current-versions conflicting with one another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_COMPAT_VERSION_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has archs with multiple "
                        "compatibility-versions conflicting with one another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has archs with multiple "
                        "compatibility-versions conflicting with one another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has archs with multiple "
                      "compatibility-versions conflicting with one another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_EXPORT_TRIE_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has archs with multiple "
                        "export-tries conflicting with one another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has archs with multiple "
                        "export-tries conflicting with one another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has archs with multiple "
                      "export-tries conflicting with one another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_FILETYPE_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has archs with multiple "
                        "mach-o filetypes conflictingwith one another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has archs with multiple "
                        "mach-o filetypes conflicting with one another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has archs with multiple mach-o "
                      "filetypes conflicting with one another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_FLAGS_CONFLICT:
            if (cb_info->is_recursing) {
                request_result =
                    request_if_should_ignore_flags(cb_info->orig,
                                                   cb_info->tbd,
                                                   false,
                                                   stderr,
                                                   "Mach-o file (at "
                                                   "path %s/%s) has archs with "
                                                   "differing sets of flags "
                                                   "conflicting with one "
                                                   "another\n",
                                                   cb_info->dir_path,
                                                   cb_info->name);

            } else if (cb_info->print_paths) {
                request_result =
                    request_if_should_ignore_flags(cb_info->orig,
                                                   cb_info->tbd,
                                                   false,
                                                   stderr,
                                                   "Mach-o file (at path %s) "
                                                   "has archs with differing "
                                                   "sets of flags conflicting "
                                                   "with one another\n",
                                                   cb_info->dir_path);
            } else {
                request_result =
                    request_if_should_ignore_flags(cb_info->orig,
                                                   cb_info->tbd,
                                                   false,
                                                   stderr,
                                                   "The provided mach-o file "
                                                   "has archs with differing "
                                                   "sets of flags conflicting "
                                                   "with another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INSTALL_NAME_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has archs with multiple "
                        "install-names conflicting with one another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has archs with multiple "
                        "install-names conflicting with one another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has archs with multiple "
                      "install-names conflicting with one another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_INVALID_PLATFORM:
            if (cb_info->is_recursing) {
                request_result =
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s/%s), or one of "
                                     "its archs, has an invalid platform\n",
                                     cb_info->dir_path,
                                     cb_info->name);

            } else if (cb_info->print_paths) {
                request_result =
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s), or one of its "
                                     "archs, has an invalid platform\n",
                                     cb_info->dir_path);
            } else {
                request_result =
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
                                     false,
                                     stderr,
                                     "The provided mach-o file, or one of its "
                                     "archs, has an invalid platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA:
            if (cb_info->is_recursing) {
                request_result =
                    request_parent_umbrella(cb_info->orig,
                                            cb_info->tbd,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s/%s), or "
                                            "one of its archs, has an invalid "
                                            "parent-umbrella\n",
                                            cb_info->dir_path,
                                            cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_parent_umbrella(cb_info->orig,
                                            cb_info->tbd,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s), or one "
                                            "of its archs, has an invalid "
                                            "parent-umbrella\n",
                                            cb_info->dir_path);
            } else {
                request_result =
                    request_parent_umbrella(cb_info->orig,
                                            cb_info->tbd,
                                            false,
                                            stderr,
                                            "The provided mach-o file, or one "
                                            "of its archs, has an invalid "
                                            "parent-umbrella\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INVALID_UUID:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an invalid uuid\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-archs file (at path %s), or one of its archs, "
                        "has an invalid uuid\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "invalid uuid\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_OBJC_CONSTRAINT_CONFLICT:
            if (cb_info->is_recursing) {
                request_result =
                    request_objc_constraint(cb_info->orig,
                                            cb_info->tbd,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s/%s) has "
                                            "archs with multiple "
                                            "objc-constraints conflicting "
                                            "with one another\n",
                                            cb_info->dir_path,
                                            cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_objc_constraint(cb_info->orig,
                                            cb_info->tbd,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s) has "
                                            "archs with multiple "
                                            "objc-constraints conflicting with "
                                            "one another\n",
                                            cb_info->dir_path);
            } else {
                request_result =
                    request_objc_constraint(cb_info->orig,
                                            cb_info->tbd,
                                            false,
                                            stderr,
                                            "The provided mach-o file has "
                                            "archs multiple objc-constraints "
                                            "conflicting with one another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_PARENT_UMBRELLA_CONFLICT:
            if (cb_info->print_paths) {
                request_result =
                    request_parent_umbrella(cb_info->orig,
                                            cb_info->tbd,
                                            false,
                                            stderr,
                                            "Mach-o file (at path %s) has "
                                            "archs with multiple "
                                            "parent-umbrellas conflicting with "
                                            "one another\n",
                                            cb_info->dir_path);
            } else {
                request_result =
                    request_parent_umbrella(cb_info->orig,
                                            cb_info->tbd,
                                            false,
                                            stderr,
                                            "The provided mach-o file, has "
                                            "archs with multiple "
                                            "parent-umbrellas conflicting with "
                                            "one another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT:
            if (cb_info->is_recursing) {
                request_result =
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s/%s) has archs "
                                     "with multiple platforms conflicting with "
                                     "one another\n",
                                     cb_info->dir_path,
                                     cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s) has multiple "
                                     "platforms conflicting with one another\n",
                                     cb_info->dir_path);
            } else {
                request_result =
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
                                     false,
                                     stderr,
                                     "The provided mach-o file has archs with "
                                     "multiple platforms conflicting with one "
                                     "another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_SWIFT_VERSION_CONFLICT:
            if (cb_info->is_recursing) {
                request_result =
                    request_swift_version(cb_info->orig,
                                          cb_info->tbd,
                                          false,
                                          stderr,
                                          "Mach-o file (at path %s/%s) has "
                                          "archs with multiple swift-versions "
                                          "conflicting with one another\n",
                                          cb_info->dir_path,
                                          cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_swift_version(cb_info->orig,
                                          cb_info->tbd,
                                          false,
                                          stderr,
                                          "Mach-o file (at path %s) has archs "
                                          "with multiple swift-versions "
                                          "conflicting with one another\n",
                                          cb_info->dir_path);
            } else {
                request_result =
                    request_swift_version(cb_info->orig,
                                          cb_info->tbd,
                                          false,
                                          stderr,
                                          "The provided mach-o file has archs "
                                          "multiple swift-versions conflicting "
                                          "with one another\n");
            }

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_SYMBOL_TABLE_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has archs with multiple "
                        "symbol-tables conflicting with one another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has archs with multiple "
                        "symbol-tables conflicting with one another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has archs with multiple "
                      "symbol-tables conflicting with one another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_TARGET_PLATFORM_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has archs with multiple "
                        "platforms conflicting with one another\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has archs with multiple "
                        "platforms conflicting with one another\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has archs with multiple "
                      "platforms conflicting with one another\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_UUID_CONFLICT:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has multiple archs with "
                        "uuids that are not unique\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has multiple archs with "
                        "uuids that are not unique\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file has archs with multiple uuids "
                      "that are not unique\n",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_INVALID_INSTALL_NAME:
            if (cb_info->is_recursing) {
                request_result =
                    request_install_name(cb_info->orig,
                                         cb_info->tbd,
                                         false,
                                         stderr,
                                         "Mach-o file (at path %s/%s), or one "
                                         "of its archs, has an invalid "
                                         "install-name\n",
                                         cb_info->dir_path,
                                         cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_install_name(cb_info->orig,
                                         cb_info->tbd,
                                         false,
                                         stderr,
                                         "Mach-o file (at path %s), or one of "
                                         "its archs, has an invalid "
                                         "install-name\n",
                                         cb_info->dir_path);
            } else {
                request_result =
                    request_install_name(cb_info->orig,
                                         cb_info->tbd,
                                         false,
                                         stderr,
                                         "The provided mach-o file, or one of "
                                         "its archs, has an invalid "
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
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s/%s), does not "
                                     "have a platform\n",
                                     cb_info->dir_path,
                                     cb_info->name);
            } else if (cb_info->print_paths) {
                request_result =
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
                                     false,
                                     stderr,
                                     "Mach-o file (at path %s), does not have "
                                     "a platform\n",
                                     cb_info->dir_path);
            } else {
                request_result =
                    request_platform(cb_info->orig,
                                     cb_info->tbd,
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
                        "Mach-o file (at path %s/%s), or one of its archs, "
                        "does not have a uuid\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs does "
                        "not have a uuid\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, does not "
                      "have a uuid\n ",
                      stderr);
            }

            return false;

        case ERR_MACHO_FILE_PARSE_EXPECTED_SIM_PLATFORM:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, "
                        "has a simulator platform while not being a simulator "
                        "binary\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs has a "
                        "simulator platform while not being a simulator "
                        "binary\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has a "
                      "a simulator platform while not being a simulator "
                      "binary\n",
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
                        "Mach-o file (at path %s), or one of its archs, has "
                        "the wrong mach-o filetype\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has the "
                      "wrong mach-o filetype\n",
                      stderr);
            }

            return false;

        case WARN_MACHO_FILE_SYMBOL_TABLE_OUTOFSYNC:
            if (cb_info->is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has a symbol-table "
                        "out-of-sync with the export-trie\r\n",
                        cb_info->dir_path,
                        cb_info->name);
            } else if (cb_info->print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has a symbol-table "
                        "out-of-sync with the export-trie\r\n",
                        cb_info->dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has a "
                      "symbol-table out-of-sync with the export-trie",
                      stderr);
            }

            return false;
    }

    return true;
}

void
handle_macho_file_open_result(const enum macho_file_open_result result,
                              const char *__notnull const dir_path,
                              const char *const name,
                              const bool print_paths,
                              const bool is_recursing)
{
    switch (result) {
        case E_MACHO_FILE_OPEN_OK:
            break;

        case E_MACHO_FILE_OPEN_READ_FAIL:
            if (is_recursing) {
                fprintf(stderr,
                        "Failed to read data while parsing mach-o file (at "
                        "path: %s/%s)\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Failed to read data while parsing mach-o file (at "
                        "path: %s)\n",
                        dir_path);
            } else {
                fputs("Failed to read data while parsing mach-o file at the "
                      "provided path\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_OPEN_FSTAT_FAIL:
            if (is_recursing) {
                fprintf(stderr,
                        "Failed to get information on mach-o file (at "
                        "(at path: %s/%s)\n",
                        dir_path,
                        name);

            } else if (print_paths) {
                fprintf(stderr,
                        "Failed to get information on mach-o file (at "
                        "(at path: %s)\n",
                        dir_path);
            } else {
                fputs("Failed to get information on mach-o file at the "
                      "provided path\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_OPEN_NOT_A_MACHO:
            if (is_recursing) {
                fprintf(stderr,
                        "File (at path %s/%s) is not a valid mach-o file\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "File (at path %s) is not a valid mach-o file\n",
                        dir_path);
            } else {
                fputs("File at provided path is not a valid mach-o file\n",
                      stderr);
            }

            break;
    }
}

void
handle_macho_file_parse_result(const char *__notnull const dir_path,
                               const char *const name,
                               const enum macho_file_parse_result parse_result,
                               const bool print_paths,
                               const bool is_recursing,
                               const bool ignore_warnings)
{
    switch (parse_result) {
        case E_MACHO_FILE_PARSE_OK:
            break;

        case E_MACHO_FILE_PARSE_ERROR_PASSED_TO_CALLBACK:
            break;

        case E_MACHO_FILE_PARSE_SEEK_FAIL:
        case E_MACHO_FILE_PARSE_READ_FAIL:
            if (is_recursing) {
                fprintf(stderr,
                        "Failed to read data while parsing mach-o file (at "
                        "path: %s/%s)\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Failed to read data while parsing mach-o file (at "
                        "path: %s)\n",
                        dir_path);
            } else {
                fputs("Failed to read data while parsing mach-o file at the "
                      "provided path\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_SIZE_TOO_SMALL:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, is "
                        "too small to be a valid mach-o file\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, is too "
                        "small to be a valid mach-o\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, is too "
                      "small to be a valid mach-o\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_RANGE:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an invalid range\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has an "
                        "invalid range\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "invalid range\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an unsupported cpu-type\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has an "
                        "unsupported cpu-type\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "unsupported cpu-type\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_NO_ARCHITECTURES:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has no archs\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has no archs\n",
                        dir_path);
            } else {
                fputs("Mach-o file at the provided path has no archs\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has too many archs to fit "
                        "inside a mach-o file\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has too many archs\n",
                        dir_path);
            } else {
                fputs("Mach-o file at the provided path has too many archs\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has an invalid arch\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has an invalid arch\n",
                        dir_path);
            } else {
                fputs("Mach-o file at the provided path has an invalid arch\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has overlapping archs\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has overlapping archs\n",
                        dir_path);
            } else {
                fputs("Mach-o file at the provided path has overlapping "
                      "archs\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has multiple archs for "
                        "the same cpu-type\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has multiple archs for the "
                        "same cpu-type\n",
                        dir_path);
            } else {
                fputs("Mach-o file at the provided path has multiple archs for "
                      "the same cpu-type\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_PLATFORM:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has multiple archs for "
                        "the same platform\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has multiple archs for the "
                        "same platform\n",
                        dir_path);
            } else {
                fputs("Mach-o file at the provided path has multiple archs for "
                      "the same platform\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has no valid archs to "
                        "parse\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has no valid archs to "
                        "parse\n",
                        dir_path);
            } else {
                fputs("Mach-o file at the provided path has no valid archs to "
                      "parse\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_ALLOC_FAIL:
            if (is_recursing) {
                fprintf(stderr,
                        "Failed to allocate data while parsing mach-o file (at "
                        "path %s/%s)\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Failed to allocate data while parsing mach-o file (at "
                        "path %s)\n",
                        dir_path);
            } else {
                fputs("Failed to allocate data while parsing the provided "
                      "mach-o file\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_ARRAY_FAIL:
            if (is_recursing) {
                fprintf(stderr,
                        "Failed to add items to an array while parsing mach-o "
                        "file (at path %s/%s)\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Failed to add items to an array while parsing mach-o "
                        "file (at path %s)\n",
                        dir_path);
            } else {
                fputs("Failed to add items to an array while parsing the "
                      "provided mach-o file\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "no load-commands.\nSubsequently, no information was "
                        "extracted\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has no "
                        "load-commands.\nBecause of this, no information was "
                        "retrieved\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has no "
                      "load-commands.\nBecause of this, no information was "
                      "retrieved\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "too many load-commands for its size\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has "
                        "too many load-commands for its size\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has too "
                      "many load-commands for its size\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an area too small to store all of its load-commands\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has "
                        "an area too small to store a load-command\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "area too small to store a load-command\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an invalid load-command\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has an "
                        "invalid load-command\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "load-command\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "a segment with too many sections for its size\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has a "
                        "segment with too many sections for its size\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has a "
                      "segment with too many sections for its size\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_SECTION:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "a segment with an invalid section\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has a "
                        "has a segment with an invalid section\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has a "
                      "segment with an invalid section\n",
                       stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_CLIENT:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an invalid client-string\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has an "
                        "invalid client-string\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "invalid client-string\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_REEXPORT:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an invalid re-export\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has an "
                        "invalid re-export\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "invalid re-export\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_STRING_TABLE:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an invalid string-table\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has an "
                        "invalid string-table\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "invalid string-table\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its archs, has "
                        "an invalid symbol-table\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has an "
                        "invalid symbol-table\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, has an "
                      "invalid symbol-table\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has archs with "
                        "conflicting cpu-types\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has archs with conflicting "
                        "cpu-types\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file has archs with conflicting "
                      "cpu-types\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_NO_DATA:
            if (is_recursing) {
                if (ignore_warnings) {
                    break;
                }

                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has no exported clients, "
                        "re-exports, or symbols to be written out\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, has no "
                        "exported clients, re-exports, or symbols to be "
                        "written out\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file has no exported clients, "
                      "re-exports, or symbols to be written out\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has no symbol-table\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its archs, "
                        "doesn't have a symbol-table\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file, or one of its archs, doesn't "
                      "have a symbol-table\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE:
            if (is_recursing) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has an invalid "
                        "exports-trie\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has an invalid "
                        "exports-trie\n",
                        dir_path);
            } else {
                fputs("The provided mach-o file has an invalid exports-trie\n",
                    stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_CREATE_SYMBOL_LIST_FAIL:
            if (is_recursing) {
                fprintf(stderr,
                        "Failed to create symbols-list while parsing mach-o "
                        "file at path: %s/%s\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Failed to create symbols-list while parsing mach-o "
                        "file at path: %s\n",
                        dir_path);
            } else {
                fputs("Failed to create symbols-list while parsing the "
                      "provided mach-o file\n",
                      stderr);
            }

            break;

        case E_MACHO_FILE_PARSE_CREATE_TARGET_LIST_FAIL:
            if (is_recursing) {
                fprintf(stderr,
                        "Failed to create targets-list while parsing mach-o "
                        "file at path: %s/%s\n",
                        dir_path,
                        name);
            } else if (print_paths) {
                fprintf(stderr,
                        "Failed to create targets-list while parsing mach-o "
                        "file at path: %s\n",
                        dir_path);
            } else {
                fputs("Failed to create targets-list while parsing the "
                      "provided mach-o file\n",
                      stderr);
            }

            break;
    }
}
