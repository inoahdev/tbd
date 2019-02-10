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
handle_macho_file_parse_result(struct tbd_for_main *const global,
                               struct tbd_for_main *const tbd,
                               const char *const path,
                               const enum macho_file_parse_result parse_result,
                               const bool print_paths,
                               uint64_t *const info_in)
{
    switch (parse_result) {
        case E_MACHO_FILE_PARSE_OK:
            break;

        case E_MACHO_FILE_PARSE_NOT_A_MACHO:
            return false;

        case E_MACHO_FILE_PARSE_SEEK_FAIL:
            if (print_paths) {
                fprintf(stderr,
                        "Failed to seek to a location while parsing mach-o "
                        "file (at path: %s)\n",
                        path);
            } else {
                fputs("Failed to seek to a location while parsing the provided "
                      "mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_READ_FAIL:
            if (print_paths) {
                fprintf(stderr,
                        "Failed to read data while parsing mach-o file (at "
                        "path: %s)\n",
                        path);
            } else {
                fputs("Failed to read data while parsing mach-o file at the "
                      "provided path\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_FSTAT_FAIL:
            if (print_paths) {
                fprintf(stderr,
                        "Failed to get information on mach-o file "
                        "(at path: %s)\n",
                        path);
            } else {
                fputs("Failed to get information on mach-o file at the "
                      "provided path\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_SIZE_TOO_SMALL:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, is too small to be a valid "
                        "mach-o\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "is too small to be a valid mach-o\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_RANGE:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid range\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid range\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_UNSUPPORTED_CPUTYPE:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an unsupported cpu-type\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has an unsupported cpu-type\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_ARCHITECTURES:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has no architectures\n",
                        path);
            } else {
                fputs("Mach-o file at the provided path has no architectures\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_ARCHITECTURES:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has too many architectures "
                        "to fit inside a mach-o file\n",
                        path);
            } else {
                fputs("Mach-o file at the provided path has too many "
                      "architectures to fit inside a mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_ARCHITECTURE:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has an invalid "
                        "architecture\n",
                        path);
            } else {
                fputs("Mach-o file at the provided path has an invalid "
                      "architecture\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_OVERLAPPING_ARCHITECTURES:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has overlapping "
                        "architectures\n",
                        path);
            } else {
                fputs("Mach-o file at the provided path has overlapping "
                      "architectures\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_MULTIPLE_ARCHS_FOR_CPUTYPE:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has multiple architectures "
                        "for the same cpu-type\n",
                        path);
            } else {
                fputs("Mach-o file at the provided path has multiple "
                      "architectures for the same cpu-type\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_VALID_ARCHITECTURES:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has no valid architectures "
                        "that can be parsed\n",
                        path);
            } else {
                fputs("Mach-o file at the provided path has no valid "
                      "architectures that can be parsed\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_ALLOC_FAIL:
            if (print_paths) {
                fprintf(stderr,
                        "Failed to allocate data while parsing mach-o file (at "
                        "path %s)\n",
                        path);
            } else {
                fputs("Failed to allocate data while parsing the provided "
                      "mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_ARRAY_FAIL:
            if (print_paths) {
                fprintf(stderr,
                        "Failed to add items to an array while parsing mach-o "
                        "file (at path %s)\n",
                        path);
            } else {
                fputs("Failed to add items to an array while parsing the "
                      "provided mach-o file\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_LOAD_COMMANDS:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has no load-commands. "
                        "Subsequently, no information was extracted\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has no load-commands. Subsequently, no information was "
                      "extracted\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_LOAD_COMMANDS:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has too many load-commands for its "
                        "size\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has too many load-commands for its "
                      "size\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has too small an area to store all "
                        "of its load-commands\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has too small an area to store all "
                      "of its load-commands\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_LOAD_COMMAND:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid load-command\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid load-command\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_TOO_MANY_SECTIONS:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has a segment with too many "
                        "sections for its size\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has a segment with too many sections "
                      "for its size\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_SECTION:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has a segment with an invalid "
                        "section\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has a segment with an invalid "
                      "section\n",
                       stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_CLIENT:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid client-string\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid client-string\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_INSTALL_NAME: {
            bool request_result = false;
            if (print_paths) {
                request_result =
                    request_install_name(global,
                                         tbd,
                                         info_in,
                                         stderr,
                                         "Mach-o file (at path %s), or one of "
                                         "its architectures, has an "
                                         "invalid install-name\n",
                                         path);
            } else {
                request_result =
                    request_install_name(global,
                                         tbd,
                                         info_in,
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
            if (print_paths) {
                request_result =
                    request_platform(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "Mach-o file (at path %s), or one of its "
                                     "architectures, has an invalid "
                                     "platform\n",
                                     path);
            } else {
                request_result =
                    request_platform(global,
                                     tbd,
                                     info_in,
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
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid re-export\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid re-export\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA: {
            bool request_result = false;
            if (print_paths) {
                request_result =
                    request_parent_umbrella(global,
                                            tbd,
                                            info_in,
                                            stderr,
                                            "Mach-o file (at path %s), or one "
                                            "of its architectures, has "
                                            "an invalid parent-umbrella\n",
                                            path);
            } else {
                request_result =
                    request_parent_umbrella(global,
                                            tbd,
                                            info_in,
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
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid string-table\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid string-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_SYMBOL_TABLE:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid symbol-table\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid symbol-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_INVALID_UUID:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has an invalid uuid\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its architectures, "
                      "has an invalid uuid\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_CONFLICTING_ARCH_INFO:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures with "
                        "conflicting cpu-types\n",
                        path);
            } else {
                fputs("The provided mach-o file has architectures with "
                      "conflicting cpu-types\n",
                      stderr);
                }

            return false;

        case E_MACHO_FILE_PARSE_CONFLICTING_FLAGS: {
            bool request_result = false;
            if (print_paths) {
                request_result =
                    request_if_should_ignore_flags(global,
                                                   tbd,
                                                   info_in,
                                                   stderr,
                                                   "Mach-o file (at path %s) "
                                                   "has architectures "
                                                   "with conflicting "
                                                   "information for its "
                                                   "tbd-flags\n",
                                                   path);
            } else {
                request_result =
                    request_if_should_ignore_flags(global,
                                                   tbd,
                                                   info_in,
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
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures with "
                        "conflicting information for its identification "
                        "(install-name, current-version, and/or"
                        "comatibility-version)"
                        "\n",
                        path);
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
            if (print_paths) {
                request_result =
                    request_objc_constraint(global,
                                            tbd,
                                            info_in,
                                            stderr,
                                            "Mach-o file (at path %s) has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "objc-constraint\n",
                                            path);
            } else {
                request_result =
                    request_objc_constraint(global,
                                            tbd,
                                            info_in,
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
            if (print_paths) {
                request_result =
                    request_parent_umbrella(global,
                                            tbd,
                                            info_in,
                                            stderr,
                                            "Mach-o file (at path %s) has "
                                            "architectures with conflicting "
                                            "information for its "
                                            "parent-umbrella\n",
                                            path);
            } else {
                request_result =
                    request_parent_umbrella(global,
                                            tbd,
                                            info_in,
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
            if (print_paths) {
                request_result =
                    request_platform(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "Mach-o file (at path %s) has "
                                     "architectures with conflicting "
                                     "information for its platform\n",
                                     path);
            } else {
                request_result =
                    request_platform(global,
                                     tbd,
                                     info_in,
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
            if (print_paths) {
                request_result =
                    request_swift_version(global,
                                          tbd,
                                          info_in,
                                          stderr,
                                          "Mach-o file (at path %s) has "
                                          "architectures with conflicting "
                                          "information for its swift-version\n",
                                          path);
            } else {
                request_result =
                    request_swift_version(global,
                                          tbd,
                                          info_in,
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
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s) has architectures sharing "
                        "the same uuid\n",
                        path);
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
            if (print_paths) {
                request_result =
                    request_platform(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "Mach-o file (at path %s), does not "
                                     "have a platform\n",
                                     path);
            } else {
                request_result =
                    request_platform(global,
                                     tbd,
                                     info_in,
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
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has no symbol-table\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has no symbol-table\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_UUID:
            if (print_paths) {
                fprintf(stderr,
                        "Mach-o file (at path %s), or one of its "
                        "architectures, has no uuid\n",
                        path);
            } else {
                fputs("The provided mach-o file, or one of its "
                      "architectures, has no uuid\n",
                      stderr);
            }

            return false;

        case E_MACHO_FILE_PARSE_NO_EXPORTS: {
            const uint64_t options = tbd->options;
            if (options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                if (options & O_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                    return false;
                }

                fprintf(stderr,
                        "Warning: Mach-o file (at path %s) has no "
                        "clients, re-exports, or symbols to be written "
                        "out\n",
                        path);
            } else {
                fputs("The provided mach-o file has no clients, "
                      "re-exports, or symbols to be written out\n",
                      stderr);

            }

            return false;
        }
    }

    /*
     * Handle the remove/replace fields
     */

    const uint64_t archs_re = tbd->archs_re;
    if (archs_re != 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            tbd->info.archs &= ~archs_re;
        } else {
            tbd->info.archs = archs_re;
        }
    }

    const uint32_t flags_re = tbd->flags_re;
    if (flags_re != 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            tbd->info.flags_field &= ~flags_re;
         } else {
            tbd->info.flags_field = flags_re;
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

    if (tbd->info.install_name == NULL) {
        bool request_result = false;
        if (print_paths) {
            request_result =
                request_install_name(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "Mach-o file (at path %s), does not have "
                                     "an install-name\n",
                                     path);
        } else {
            request_result =
                request_install_name(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "The provided mach-o file does not have "
                                     "an install-name\n");
        }

        if (!request_result) {
            return false;
        }
    }

    if (tbd->info.platform == 0) {
        bool request_result = false;
        if (print_paths) {
            request_result =
                request_platform(global,
                                 tbd,
                                 info_in,
                                 stderr,
                                 "Mach-o file (at path %s), does not have a "
                                 "platform\n",
                                 path);
        } else {
            request_result =
                request_platform(global,
                                 tbd,
                                 info_in,
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
