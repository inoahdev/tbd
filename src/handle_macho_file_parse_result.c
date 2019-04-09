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
handle_macho_file_parse_result(
    const struct handle_macho_file_parse_result_args args)
{
    switch (args.parse_result) {
        case E_MACHO_FILE_PARSE_OK:
            break;

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
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to seek to a location while parsing mach-o "
                        "file (at path: %s)\n",
                        args.dir_path);
            } else {
                fputs("Failed to seek to a location while parsing the provided "
                      "mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_READ_FAIL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to read data while parsing mach-o file (at "
                        "args.dir_path: %s)\n",
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
                        "architectures, is too small to be a valid "
                        "mach-o file\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "is too small to be a valid mach-o file\n",
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
                fputs("The provided mach-o file, or one of its "
                      "architectures, has an unsupported cpu-type\n",
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
                        "Mach-o file (at path %s) has too many "
                        "architectures to fit inside a mach-o file\n",
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
                        "Mach-o file (at path %s) has multiple "
                        "architectures for the same cpu-type\n",
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
                        "Mach-o file (at path %s) has no valid "
                        "architectures that can be parsed\n",
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
                        "architectures, has no load-commands. "
                        "Subsequently, no information was extracted\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has no load-commands. Subsequently, no information was "
                      "extracted\n",
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
                        "architectures, has too small an area to store all "
                        "of its load-commands\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has too small an area to store all "
                      "of its load-commands\n",
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

        case E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_install_name(args.global,
                                         args.tbd,
                                         args.retained_info_in,
                                         stderr,
                                         "Mach-o file (at path %s), or one "
                                         "of its architectures, has an invalid "
                                         "install-name\n",
                                         args.dir_path);
            } else {
                request_result =
                    request_install_name(args.global,
                                         args.tbd,
                                         args.retained_info_in,
                                         stderr,
                                         "The provided mach-o file, or one of "
                                         "its architectures, has an "
                                         "invalid install-name\n");
            }

            if (!request_result) {
               return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_INVALID_PLATFORM: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "Mach-o file (at path %s), or one of "
                                     "its architectures, has an invalid "
                                     "platform\n",
                                     args.dir_path);
            } else {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "The provided mach-o file, or one of its "
                                     "architectures, has an invalid "
                                     "platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

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

        case E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_parent_umbrella(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "Mach-o file (at path %s), or "
                                            "one of its architectures, has an "
                                            "invalid parent-umbrella\n",
                                            args.dir_path);
            } else {
                request_result =
                    request_parent_umbrella(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "The provided mach-o file, or one "
                                            "of its architectures, has "
                                            "an invalid parent-umbrella\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

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

        case E_MACHO_FILE_PARSE_INVALID_UUID:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid uuid\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid uuid\n",
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

        case E_MACHO_FILE_PARSE_CONFLICTING_FLAGS: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_if_should_ignore_flags(args.global,
                                                   args.tbd,
                                                   args.retained_info_in,
                                                   stderr,
                                                   "Mach-o file "
                                                   "(at path %s) has "
                                                   "architectures with "
                                                   "conflicting information "
                                                   "for its tbd-flags\n",
                                                   args.dir_path);
            } else {
                request_result =
                    request_if_should_ignore_flags(args.global,
                                                   args.tbd,
                                                   args.retained_info_in,
                                                   stderr,
                                                   "The provided mach-o file "
                                                   "has architectures with "
                                                   "conflicting information "
                                                   "for its tbd-flags\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures with "
                        "conflicting information for its identification "
                        "(install-name, current-version, and/or"
                        "comatibility-version)"
                        "\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "conflicting information for its identification "
                      "(install-name, current-version, and/or"
                      "comatibility-version)\n",
                      stderr);
        }

        return false;

        case E_MACHO_FILE_PARSE_CONFLICTING_OBJC_CONSTRAINT: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_objc_constraint(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "Mach-o file (at path %s) has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "objc-constraint\n",
                                            args.dir_path);
            } else {
                request_result =
                    request_objc_constraint(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "The provided mach-o file has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "objc-constraint\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_parent_umbrella(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "Mach-o file (at path %s) has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "parent-umbrella\n",
                                            args.dir_path);
            } else {
                request_result =
                    request_parent_umbrella(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "The provided mach-o file has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "parent-umbrella\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "Mach-o file (at path %s) has "
                                     "architectures with conflicting "
                                     "information for its platform\n",
                                     args.dir_path);
            } else {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "The provided mach-o file has "
                                     "architectures with conflicting "
                                     "information for its platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_swift_version(args.global,
                                          args.tbd,
                                          args.retained_info_in,
                                          stderr,
                                          "Mach-o file (at path %s) has "
                                          "architectures with conflicting "
                                          "information for its swift-version\n",
                                          args.dir_path);
            } else {
                request_result =
                    request_swift_version(args.global,
                                          args.tbd,
                                          args.retained_info_in,
                                          stderr,
                                          "The provided mach-o file has "
                                          "architectures with conflicting "
                                          "information for its "
                                          "swift-version\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_UUID:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures sharing "
                        "the same uuid\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file has architectures sharing the "
                      "same uuid\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_IDENTIFICATION:
            /*
             * No identification means that the mach-o file is not a library
             * file, which we check for only here, at the last moment.
             *
             * No errors are printed, and this is simply inored
             */

            return false;

        case E_MACHO_FILE_PARSE_NO_PLATFORM: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "Mach-o file (at path %s), does not "
                                     "have a platform\n",
                                     args.dir_path);
            } else {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "The provided mach-o file does not "
                                     "have a platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has no symbol-table\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has no symbol-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_UUID:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has no uuid\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has no uuid\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_EXPORTS: {
            const uint64_t flags = args.tbd->flags;
            if (flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                if (flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                    return false;
                }

                fprintf(stderr,
                        "Mach-o file (at path %s) has no clients, "
                        "re-exports, or symbols to be written out\n",
                        args.dir_path);
            } else {
                fputs("The provided mach-o file has no clients, re-exports, or "
                      "symbols to be written out\n",
                      stderr);

            }

            return false;
        }
    }

    /*
     * Handle the remove/replace fields
     */

    const uint64_t archs_re = args.tbd->archs_re;
    if (archs_re != 0) {
        if (args.tbd->flags & F_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            args.tbd->info.archs &= ~archs_re;
        }
    }

    const uint32_t flags_re = args.tbd->flags_re;
    if (flags_re != 0) {
        if (args.tbd->flags & F_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            args.tbd->info.flags_field &= ~flags_re;
        }
    }

    /*
     * If some of the fields are empty (for when
     * O_MACHO_PARSE_IGNORE_INVALID_FIELDS is provided), request info from the
     * user.
     *
     * Ignore objc-constraint, swift-version, and parent-umbrella as they aren't
     * mandatory fields and aren't always provided.
     */

    if (args.tbd->info.install_name == NULL) {
        bool request_result = false;
        if (args.print_paths) {
            request_result =
                request_install_name(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "Mach-o file (at path %s), does not "
                                     "have an install-name\n",
                                     args.dir_path);
        } else {
            request_result =
                request_install_name(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "The provided mach-o file does not have "
                                     "an install-name\n");
        }

        if (!request_result) {
            return false;
        }
    }

    if (args.tbd->info.platform == 0) {
        bool request_result = false;
        if (args.print_paths) {
            request_result =
                request_platform(args.global,
                                 args.tbd,
                                 args.retained_info_in,
                                 stderr,
                                 "Mach-o file (at path %s), does not have a "
                                 "platform\n",
                                 args.dir_path);
        } else {
            request_result =
                request_platform(args.global,
                                 args.tbd,
                                 args.retained_info_in,
                                 stderr,
                                 "The provided mach-o file does not have a "
                                 "platform\n");
        }

        if (!request_result) {
            return false;
        }
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

        case E_MACHO_FILE_PARSE_NOT_A_MACHO:
            if (args.print_paths) {
                fprintf(stderr,
                        "File (at path %s/%s) is not a valid mach-o file\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("File at provided path is not a valid mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_SEEK_FAIL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to seek to a location while parsing mach-o "
                        "file (at path: %s/%s)\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("Failed to seek to a location while parsing the provided "
                      "mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_READ_FAIL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Failed to read data while parsing mach-o file (at "
                        "args.dir_path: %s/%s)\n",
                        args.dir_path,
                        args.name);
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
                        "(at path: %s/%s)\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("Failed to get information on mach-o file at the "
                      "provided path\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_SIZE_TOO_SMALL:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, is too small to be a valid "
                        "mach-o file\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "is too small to be a valid mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_RANGE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an invalid range\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid range\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an unsupported cpu-type\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has an unsupported cpu-type\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_ARCHITECTURES:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has no architectures\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("Mach-o file at the provided path has no architectures\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has too many "
                        "architectures to fit inside a mach-o file\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("Mach-o file at the provided path has too many "
                      "architectures to fit inside a mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has an invalid "
                        "architecture\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("Mach-o file at the provided path has an invalid "
                      "architecture\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has overlapping "
                        "architectures\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("Mach-o file at the provided path has overlapping "
                      "architectures\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has multiple "
                        "architectures for the same cpu-type\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("Mach-o file at the provided path has multiple "
                      "architectures for the same cpu-type\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has no valid "
                        "architectures that can be parsed\n",
                        args.dir_path,
                        args.name);
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
                        "path %s/%s)\n",
                        args.dir_path,
                        args.name);
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
                        "file (at path %s/%s)\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("Failed to add items to an array while parsing the "
                      "provided mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has no load-commands. "
                        "Subsequently, no information was extracted\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has no load-commands. Subsequently, no information was "
                      "extracted\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has too many load-commands for its "
                        "size\n",
                        args.dir_path,
                        args.name);
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
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has too small an area to store all "
                        "of its load-commands\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has too small an area to store all "
                      "of its load-commands\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an invalid load-command\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid load-command\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has a segment with too many "
                        "sections for its size\n",
                        args.dir_path,
                        args.name);
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
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has a segment with an invalid "
                        "section\n",
                        args.dir_path,
                        args.name);
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
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an invalid client-string\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid client-string\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_install_name(args.global,
                                         args.tbd,
                                         args.retained_info_in,
                                         stderr,
                                         "Mach-o file (at path %s/%s), or one "
                                         "of its architectures, has an invalid "
                                         "install-name\n",
                                         args.dir_path,
                                         args.name);
            } else {
                request_result =
                    request_install_name(args.global,
                                         args.tbd,
                                         args.retained_info_in,
                                         stderr,
                                         "The provided mach-o file, or one of "
                                         "its architectures, has an "
                                         "invalid install-name\n");
            }

            if (!request_result) {
               return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_INVALID_PLATFORM: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "Mach-o file (at path %s/%s), or one of "
                                     "its architectures, has an invalid "
                                     "platform\n",
                                     args.dir_path,
                                     args.name);
            } else {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "The provided mach-o file, or one of its "
                                     "architectures, has an invalid "
                                     "platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_INVALID_REEXPORT:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an invalid re-export\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid re-export\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_parent_umbrella(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "Mach-o file (at path %s/%s), or "
                                            "one of its architectures, has an "
                                            "invalid parent-umbrella\n",
                                            args.dir_path,
                                            args.name);
            } else {
                request_result =
                    request_parent_umbrella(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "The provided mach-o file, or one "
                                            "of its architectures, has "
                                            "an invalid parent-umbrella\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_INVALID_STRING_TABLE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an invalid string-table\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid string-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an invalid symbol-table\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid symbol-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_UUID:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has an invalid uuid\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid uuid\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has architectures with "
                        "conflicting cpu-types\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "conflicting cpu-types\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_CONFLICTING_FLAGS: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_if_should_ignore_flags(args.global,
                                                   args.tbd,
                                                   args.retained_info_in,
                                                   stderr,
                                                   "Mach-o file "
                                                   "(at path %s/%s) has "
                                                   "architectures with "
                                                   "conflicting information "
                                                   "for its tbd-flags\n",
                                                   args.dir_path,
                                                   args.name);
            } else {
                request_result =
                    request_if_should_ignore_flags(args.global,
                                                   args.tbd,
                                                   args.retained_info_in,
                                                   stderr,
                                                   "The provided mach-o file "
                                                   "has architectures with "
                                                   "conflicting information "
                                                   "for its tbd-flags\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_IDENTIFICATION:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has architectures with "
                        "conflicting information for its identification "
                        "(install-name, current-version, and/or"
                        "comatibility-version)"
                        "\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "conflicting information for its identification "
                      "(install-name, current-version, and/or"
                      "comatibility-version)\n",
                      stderr);
        }

        return false;

        case E_MACHO_FILE_PARSE_CONFLICTING_OBJC_CONSTRAINT: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_objc_constraint(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "Mach-o file (at path %s/%s) has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "objc-constraint\n",
                                            args.dir_path,
                                            args.name);
            } else {
                request_result =
                    request_objc_constraint(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "The provided mach-o file has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "objc-constraint\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_PARENT_UMBRELLA: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_parent_umbrella(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "Mach-o file (at path %s/%s) has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "parent-umbrella\n",
                                            args.dir_path,
                                            args.name);
            } else {
                request_result =
                    request_parent_umbrella(args.global,
                                            args.tbd,
                                            args.retained_info_in,
                                            stderr,
                                            "The provided mach-o file has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "parent-umbrella\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_PLATFORM: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "Mach-o file (at path %s/%s) has "
                                     "architectures with conflicting "
                                     "information for its platform\n",
                                     args.dir_path,
                                     args.name);
            } else {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "The provided mach-o file has "
                                     "architectures with conflicting "
                                     "information for its platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_SWIFT_VERSION: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_swift_version(args.global,
                                          args.tbd,
                                          args.retained_info_in,
                                          stderr,
                                          "Mach-o file (at path %s/%s) has "
                                          "architectures with conflicting "
                                          "information for its swift-version\n",
                                          args.dir_path,
                                          args.name);
            } else {
                request_result =
                    request_swift_version(args.global,
                                          args.tbd,
                                          args.retained_info_in,
                                          stderr,
                                          "The provided mach-o file has "
                                          "architectures with conflicting "
                                          "information for its "
                                          "swift-version\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_CONFLICTING_UUID:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has architectures sharing "
                        "the same uuid\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file has architectures sharing the "
                      "same uuid\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_IDENTIFICATION:
            /*
             * No identification means that the mach-o file is not a library
             * file, which we check for only here, at the last moment.
             *
             * No errors are printed, and this is simply inored
             */

            return false;

        case E_MACHO_FILE_PARSE_NO_PLATFORM: {
            bool request_result = false;
            if (args.print_paths) {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "Mach-o file (at path %s/%s), does not "
                                     "have a platform\n",
                                     args.dir_path,
                                     args.name);
            } else {
                request_result =
                    request_platform(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "The provided mach-o file does not "
                                     "have a platform\n");
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_MACHO_FILE_PARSE_NO_SYMBOL_TABLE:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has no symbol-table\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has no symbol-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_UUID:
            if (args.print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s/%s), or one of its "
                        "architectures, has no uuid\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has no uuid\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_EXPORTS: {
            const uint64_t flags = args.tbd->flags;
            if (flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                if (flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                    return false;
                }

                fprintf(stderr,
                        "Mach-o file (at path %s/%s) has no clients, "
                        "re-exports, or symbols to be written out\n",
                        args.dir_path,
                        args.name);
            } else {
                fputs("The provided mach-o file has no clients, re-exports, or "
                      "symbols to be written out\n",
                      stderr);

            }

            return false;
        }
    }

    /*
     * Handle the remove/replace fields
     */

    const uint64_t archs_re = args.tbd->archs_re;
    if (archs_re != 0) {
        if (args.tbd->flags & F_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            args.tbd->info.archs &= ~archs_re;
        }
    }

    const uint32_t flags_re = args.tbd->flags_re;
    if (flags_re != 0) {
        if (args.tbd->flags & F_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            args.tbd->info.flags_field &= ~flags_re;
        }
    }

    /*
     * If some of the fields are empty (for when
     * O_MACHO_PARSE_IGNORE_INVALID_FIELDS is provided), request info from the
     * user.
     *
     * Ignore objc-constraint, swift-version, and parent-umbrella as they aren't
     * mandatory fields and aren't always provided.
     */

    if (args.tbd->info.install_name == NULL) {
        bool request_result = false;
        if (args.print_paths) {
            request_result =
                request_install_name(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "Mach-o file (at path %s/%s), does not "
                                     "have an install-name\n",
                                     args.dir_path,
                                     args.name);
        } else {
            request_result =
                request_install_name(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     stderr,
                                     "The provided mach-o file does not have "
                                     "an install-name\n");
        }

        if (!request_result) {
            return false;
        }
    }

    if (args.tbd->info.platform == 0) {
        bool request_result = false;
        if (args.print_paths) {
            request_result =
                request_platform(args.global,
                                 args.tbd,
                                 args.retained_info_in,
                                 stderr,
                                 "Mach-o file (at path %s/%s), does not have a "
                                 "platform\n",
                                 args.dir_path,
                                 args.name);
        } else {
            request_result =
                request_platform(args.global,
                                 args.tbd,
                                 args.retained_info_in,
                                 stderr,
                                 "The provided mach-o file does not have a "
                                 "platform\n");
        }

        if (!request_result) {
            return false;
        }
    }

    return true;
}
