//
//  src/main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "copy.h"
#include "dir_recurse.h"
#include "macho_file.h"
#include "our_io.h"
#include "path.h"

#include "parse_or_list_fields.h"
#include "parse_dsc_for_main.h"
#include "parse_macho_for_main.h"

#include "request_user_input.h"
#include "tbd.h"
#include "tbd_for_main.h"
#include "tbd_write.h"
#include "unused.h"
#include "usage.h"
#include "util.h"

struct recurse_callback_info {
    struct tbd_for_main *tbd;
    struct tbd_for_main *orig;

    FILE *combine_file;
    uint64_t files_parsed;

    struct retained_user_info *retained;
    struct string_buffer *export_trie_sb;
};

static bool
recurse_directory_callback(const char *__notnull const dir_path,
                           const uint64_t dir_path_length,
                           const int fd,
                           struct dirent *const dirent,
                           const uint64_t name_length,
                           void *__notnull const callback_info)
{
    struct recurse_callback_info *const recurse_info =
        (struct recurse_callback_info *)callback_info;

    struct tbd_for_main *const orig = recurse_info->orig;
    struct tbd_for_main *const tbd = recurse_info->tbd;

    struct retained_user_info *const retained = recurse_info->retained;
    struct magic_buffer magic_buffer = {};

    const char *const name = dirent->d_name;
    const bool should_combine = tbd->options.combine_tbds;

    if (tbd->filetypes.macho) {
        struct parse_macho_for_main_args args = {
            .fd = fd,
            .magic_buffer = &magic_buffer,
            .retained = retained,

            .tbd = tbd,
            .orig = orig,

            .dir_path = dir_path,
            .dir_path_length = dir_path_length,

            .name = name,
            .name_length = name_length,

            .dont_handle_non_macho_error = true,
            .print_paths = true,

            .export_trie_sb = recurse_info->export_trie_sb
        };

        if (should_combine) {
            args.combine_file = recurse_info->combine_file;
        }

        const enum parse_macho_for_main_result parse_as_macho_result =
            parse_macho_file_for_main_while_recursing(&args);

        switch (parse_as_macho_result) {
            case E_PARSE_MACHO_FOR_MAIN_OK: {
                if (should_combine) {
                    recurse_info->combine_file = args.combine_file;
                }

                recurse_info->files_parsed += 1;
                close(fd);

                return true;
            }

            case E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO:
                break;

            case E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR:
                close(fd);
                return true;
        }
    }

    if (tbd->filetypes.dyld_shared_cache) {
        struct parse_dsc_for_main_args args = {
            .fd = fd,

            .magic_buffer = &magic_buffer,
            .retained = retained,

            .tbd = tbd,
            .orig = orig,

            .dsc_dir_path = dir_path,
            .dsc_dir_path_length = dir_path_length,

            .dsc_name = name,
            .dsc_name_length = name_length,

            .dont_handle_non_dsc_error = true,
            .print_paths = true,

            .export_trie_sb = recurse_info->export_trie_sb
        };

        if (should_combine) {
            args.combine_file = recurse_info->combine_file;
        }

        const enum parse_dsc_for_main_result parse_as_dsc_result =
            parse_dsc_for_main_while_recursing(&args);

        switch (parse_as_dsc_result) {
            case E_PARSE_DSC_FOR_MAIN_OK:
                if (should_combine) {
                    recurse_info->combine_file = args.combine_file;
                }

                recurse_info->files_parsed += 1;
                close(fd);

                return true;

            case E_PARSE_DSC_FOR_MAIN_NOT_A_SHARED_CACHE:
                break;

            case E_PARSE_DSC_FOR_MAIN_OTHER_ERROR:
                close(fd);
                return true;

            /*
             * This error shouldn't be returned while recursing.
             */

            case E_PARSE_DSC_FOR_MAIN_CLOSE_COMBINE_FILE_FAIL:
                break;
        }
    }

    close(fd);
    return true;
}

static bool
recurse_directory_fail_callback(const char *const dir_path,
                                __unused const uint64_t dir_path_length,
                                enum dir_recurse_fail_result result,
                                struct dirent *const dirent,
                                void *__unused const callback_info)
{
    switch (result) {
        case E_DIR_RECURSE_FAILED_TO_ALLOC_PATH:
            fputs("Failed to allocate memory for a path-string\n", stderr);
            break;

        case E_DIR_RECURSE_FAILED_TO_OPEN_FILE:
            fprintf(stderr,
                    "Failed to open file (at path %s/%s), error: %s\n",
                    dir_path,
                    dirent->d_name,
                    strerror(errno));

            break;

        case E_DIR_RECURSE_FAILED_TO_OPEN_SUBDIR:
            fprintf(stderr,
                    "Failed to open sub-directory at path: %s, error: %s\n",
                    dir_path,
                    strerror(errno));

            break;

        case E_DIR_RECURSE_FAILED_TO_READ_ENTRY:
            fprintf(stderr,
                    "Failed to read directory-entry while recursing directory "
                    "at path: %s, error: %s\n",
                    dir_path,
                    strerror(errno));

            break;
    }

    return true;
}

static void destroy_tbds_array(struct array *const tbds) {
    struct tbd_for_main *tbd = tbds->data;
    const struct tbd_for_main *const end = tbds->data_end;

    for (; tbd != end; tbd++) {
        tbd_for_main_destroy(tbd);
    }

    array_destroy(tbds);
}

static void setup_tbd_for_main(struct tbd_for_main *__notnull const tbd) {
    tbd->info.version = TBD_VERSION_V2;
    tbd->filetypes.macho = true;
    tbd->filetypes.dyld_shared_cache = true;
}

static void
print_not_supported_error(const struct tbd_for_main *__notnull const tbd,
                          const char *__notnull const option,
                          const enum tbd_version version)
{
    fprintf(stderr,
            "%s is not supported for .tbd version %s.\n",
            option,
            tbd_version_to_string(version));

    if (tbd->flags.provided_tbd_version) {
        fputs("Please either provide a different .tbd version or remove the "
              "option from the argument-list\n",
              stderr);
    } else {
        fputs("Please either manually provide a different version (with option "
              "-v/--version), or remove the option from the argument-list\n",
              stderr);
    }
}

static int
check_flags(const struct tbd_for_main *__notnull const tbd,
            const enum tbd_version version,
            const int result)
{
    int return_value = result;
    if (tbd->flags.provided_flags) {
        print_not_supported_error(tbd, "--replace-flags", version);
        return_value = 1;
    }

    if (tbd->flags.provided_ignore_flags) {
        print_not_supported_error(tbd, "--ignore-flags", version);
        return_value = 1;
    }

    return return_value;
}

static int
check_objc_constraint(const struct tbd_for_main *__notnull const tbd,
                      const enum tbd_version version,
                      const int result)
{
    int return_value = result;
    if (tbd->flags.provided_objc_constraint) {
        print_not_supported_error(tbd, "--replace-objc-constraint", version);
        return_value = 1;
    }

    if (tbd->flags.provided_ignore_objc_constraint) {
        print_not_supported_error(tbd, "--ignore-objc-constraint", version);
        return_value = 1;
    }

    return return_value;
}

static int
check_targets(const struct tbd_for_main *__notnull const tbd,
              const enum tbd_version version,
              const int result)
{
    if (!tbd->flags.provided_targets) {
        return result;
    }

    print_not_supported_error(tbd, "--replace-targets", version);
    return 1;
}

static bool
verify_tbd_for_main(struct tbd_for_main *__notnull const tbd,
                    const char *__notnull const path)
{
    const enum tbd_version version = tbd->info.version;
    int result = 0;

    switch (version) {
        case TBD_VERSION_NONE:
            fprintf(stderr,
                    "INTERNAL: Private tbd structure (for path: %s) was not "
                    "properly configured\n",
                    path);

            result = 1;
            break;

        case TBD_VERSION_V1:
            result = check_flags(tbd, version, result);
            result = check_targets(tbd, version, result);

            if (tbd->parse_options.ignore_objc_ehtype_syms) {
                if (tbd->flags.provided_tbd_version) {
                    fprintf(stderr,
                            "Note: ObjC ehtype-symbols are not parsed for "
                            "tbd-version %s, for path: %s\n",
                            tbd_version_to_string(tbd->info.version),
                            path);
                } else {
                    fprintf(stderr,
                            "Note: ObjC ehtype-symbols are not parsed for "
                            "default tbd-version %s, for path: %s\n",
                            tbd_version_to_string(tbd->info.version),
                            path);
                }
            }

            if (tbd->parse_options.ignore_undefineds) {
                if (tbd->flags.provided_tbd_version) {
                    fprintf(stderr,
                            "Note: Undefined-symbols are not parsed for "
                            "tbd-version %s, for path: %s\n",
                            tbd_version_to_string(tbd->info.version),
                            path);
                } else {
                    fprintf(stderr,
                            "Note: Undefined-symbols are not parsed for "
                            "default tbd-version %s, for path: %s\n",
                            tbd_version_to_string(tbd->info.version),
                            path);
                }
            }

            break;

        case TBD_VERSION_V2:
            if (tbd->parse_options.ignore_objc_ehtype_syms) {
                if (tbd->flags.provided_tbd_version) {
                    fprintf(stderr,
                            "Note: ObjC ehtype-symbols are not parsed for "
                            "tbd-version %s, for path: %s\n",
                            tbd_version_to_string(tbd->info.version),
                            path);
                } else {
                    fprintf(stderr,
                            "Note: ObjC ehtype-symbols are not parsed for "
                            "default tbd-version %s, for path: %s\n",
                            tbd_version_to_string(tbd->info.version),
                            path);
                }
            }

            // fall through
        case TBD_VERSION_V3:
            result = check_targets(tbd, version, result);
            break;

        case TBD_VERSION_V4:
            result = check_objc_constraint(tbd, version, result);
            break;
    }

    if (tbd->flags.provided_ignore_current_version) {
        if (tbd->flags.provided_current_version) {
            fprintf(stderr,
                    "Please choose either --ignore-current-version or "
                    "--replace-current-version for file from: %s\n",
                    path);

            result = 1;
        }
    }

    if (tbd->flags.provided_ignore_compat_version) {
        if (tbd->flags.provided_compat_version) {
            fprintf(stderr,
                    "Please choose either --ignore-compat-version or "
                    "--replace-compat-version for path: %s\n",
                    path);

            result = 1;
        }
    }

    if (tbd->flags.provided_ignore_flags) {
        if (tbd->flags.provided_flags) {
            fprintf(stderr,
                    "Please choose either --ignore-flags or --replace-flags "
                    "for path: %s\n",
                    path);

            result = 1;
        }
    }

    if (tbd->flags.provided_ignore_objc_constraint) {
        if (tbd->flags.provided_objc_constraint) {
            fprintf(stderr,
                    "Please choose either --ignore-objc-constraint or "
                    "--replace-objc-constraint for file from: %s\n",
                    path);

            result = 1;
        }
    }

    if (tbd->flags.provided_ignore_swift_version) {
        if (tbd->flags.provided_swift_version) {
            fprintf(stderr,
                    "Please choose either --ignore-swift-version or "
                    "--replace-swift-version for path: %s\n",
                    path);

            result = 1;
        }
    }

    if (tbd->macho_options.use_symbol_table) {
        if (tbd->macho_options.use_export_trie) {
            fprintf(stderr,
                    "Please provide either --use-export-trie or "
                    "--use-symbol-table for path: %s\n",
                    path);

            result = 1;
        }
    }

    /*
     * Check for any conflicts in the provided options.
     */

    if (strcmp(path, "stdin") != 0) {
        /*
         * We may have been provided with a path relative to the
         * current-directory.
         */

        uint64_t full_path_length = strlen(path);
        char *full_path =
            path_get_absolute_path(path,
                                   full_path_length,
                                   &full_path_length);

        if (full_path == NULL) {
            fputs("Failed to allocate memory\n", stderr);
            exit(1);
        }

        /*
         * Ensure that the file/directory actually exists on the filesystem.
         */

        struct stat info = {};
        if (stat(full_path, &info) != 0) {
            /*
             * ENOTDIR will be returned if a directory in the path hierarchy
             * isn't a directory at all, which still means that no
             * file/directory exists at the provided path.
             */

            if (errno == ENOENT || errno == ENOTDIR) {
                fprintf(stderr,
                        "No file or directory exists at path: %s\n",
                        full_path);
            } else {
                fprintf(stderr,
                        "Failed to retrieve information on file or directory "
                        "at path, error: %s\n",
                        full_path);
            }

            if (full_path != path) {
                free(full_path);
            }

            result = 1;
        } else if (S_ISREG(info.st_mode)) {
            if (tbd->options.recurse_directories) {
                fprintf(stderr, "Cannot recurse file at path: %s\n", path);
                if (full_path != path) {
                    free(full_path);
                }

                result = 1;
            }
        } else if (S_ISDIR(info.st_mode)) {
            if (!tbd->options.recurse_directories) {
                fputs("Please provide option '-r' if you want to recurse the "
                      "provided directory\n",
                      stderr);

                if (full_path != path) {
                    free(full_path);
                }

                result = 1;
            }
        } else {
            fprintf(stderr, "Unsupported object at path: %s\n", full_path);
            if (full_path != path) {
                free(full_path);
            }

            result = 1;
        }

        tbd->parse_path = full_path;
        tbd->parse_path_length = full_path_length;
    } else {
        if (tbd->options.recurse_directories) {
            fputs("Recursing the input file is not supported.\nPlease provide "
                  "a path to a directory for recursing\n",
                  stderr);

            result = 1;
        }
    }

    if (tbd->dsc_image_filters.item_count != 0) {
        if (!tbd->filetypes.dyld_shared_cache) {
            fprintf(stderr,
                    "dsc image-filters have been provided for path (%s) "
                    "that will not be parsed as a dyld_shared_cache file.\n"
                    "Please provide option --dsc to parse the file as a "
                    "dyld_shared_cache file",
                    path);

            result = 1;
        }
    }

    if (tbd->dsc_image_numbers.item_count != 0) {
        if (!tbd->filetypes.dyld_shared_cache) {
            fprintf(stderr,
                    "--filter-image-number has been provided for path (%s) "
                    "that will not be parsed as a dyld_shared_cache file.\n"
                    "Please provide option --dsc to parse the file as a "
                    "dyld_shared_cache file",
                    path);

            result = 1;
        }
    }

    return result;
}

int main(const int argc, char *const argv[]) {
    if (argc < 2) {
        print_usage();
        return 0;
    }

    /*
     * Store an index into the tbds array indicating the "current" tbd for when
     * we parsing output arguments.
     */

    struct array tbds = {};
    uint64_t current_tbd_index = 0;

    bool has_stdout = false;
    bool will_parse_export_trie = false;

    for (int index = 1; index != argc; index++) {
        /*
         * Every argument parsed in this loop should be an option. Any extra
         * arguments, like a path-string, should be parsed by the option it
         * belongs to.
         */

        const char *const argument = argv[index];
        const char argument_front = argument[0];

        if (argument_front != '-') {
            fprintf(stderr,
                    "Unrecognized argument (at index %d): %s\n",
                    index,
                    argument);

            destroy_tbds_array(&tbds);
            return 1;
        }

        const char *option = argument + 1;
        const char option_front = option[0];

        if (option_front == '-') {
            option += 1;
        }

        if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
            if (index != 1 || argc != 2) {
                fprintf(stderr, "%s needs to be run by itself\n", argument);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_usage();
            return 0;
        } else if (strcmp(option, "o") == 0 || strcmp(option, "output") == 0) {
            index += 1;
            if (index == argc) {
                fputs("Please provide either a path to a write-file or "
                      "\"stdout\" to print to stdout (terminal)\n",
                      stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            struct tbd_for_main *const tbd =
                array_get_item_at_index_unsafe(&tbds,
                                               sizeof(struct tbd_for_main),
                                               current_tbd_index);

            if (tbd == NULL) {
                /*
                 * Subtract one from the index to get the index of the '-o' arg.
                 */

                fprintf(stderr,
                        "No corresponding tbd exists for output arguments at "
                        "index: %d\n",
                        index - 1);

                destroy_tbds_array(&tbds);
                return 1;
            }

            bool found_path = false;
            for (; index != argc; index++) {
                /*
                 * Here, we can either receive an option or a path-string, so
                 * the same validation as above cannot be carried out here.
                 */

                const char *const in_arg = argv[index];
                const char in_arg_front = in_arg[0];

                if (in_arg_front == '-') {
                    const char *in_opt = in_arg + 1;
                    const char in_opt_front = in_opt[0];

                    if (in_opt_front == '-') {
                        in_opt += 1;
                    }

                    if (strcmp(in_opt, "preserve-subdirs") == 0) {
                        tbd->options.preserve_directory_subdirs = true;
                    } else if (strcmp(in_opt, "no-overwrite") == 0) {
                        tbd->options.no_overwrite = true;
                    } else if (strcmp(in_opt, "replace-path-extension") == 0) {
                        tbd->options.replace_path_extension = true;
                    } else if (strcmp(in_opt, "combine-tbds") == 0) {
                        tbd->options.combine_tbds = true;
                    } else {
                        fprintf(stderr, "Unrecognized option: %s\n", in_arg);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }

                    continue;
                }

                /*
                 * We only allow printing to stdout for single-files, and
                 * not when recursing directories.
                 */

                const char *const path = in_arg;
                if (strcmp(path, "stdout") == 0) {
                    if (tbd->options.recurse_directories) {
                        fputs("Writing to stdout (terminal) while recursing "
                              "a directory is not supported.\nPlease provide "
                              "a directory to write all created files to\n",
                              stderr);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    if (has_stdout) {
                        fputs("Printing more than one file to stdout is not "
                              "allowed\n",
                              stderr);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    found_path = true;
                    has_stdout = true;

                    continue;
                }

                /*
                 * Ensure options for recursing directories are not being
                 * provided for other contexts and circumstances.
                 *
                 * We allow dyld_shared_cache files to slip through as they can
                 * be exported to a directory due to the fact that they store
                 * multiple mach-o images.
                 */

                const struct tbd_for_main_options options = tbd->options;
                if (!options.recurse_directories &&
                    !tbd->filetypes.dyld_shared_cache)
                {
                    if (options.preserve_directory_subdirs) {
                        fputs("Option --preserve-subdirs can only be provided "
                              "for either recursing directoriess, or parsing "
                              "dyld_shared_cache files\n",
                              stderr);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    if (options.replace_path_extension) {
                        fputs("Option --replace-path-extension can only be "
                              "provided for recursing directories.\nYou can "
                              "change the extension of your write-file when "
                              "not recursing by changing the extension in its "
                              "path-string\n",
                              stderr);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    if (options.combine_tbds) {
                        fputs("Option --combine-tbds can only be provided "
                              "recursing directories and parsing "
                              "dyld_shared_cache files\n",
                              stderr);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }
                }

                /*
                 * We may have been provided with a path relative to the
                 * current-directory.
                 */

                uint64_t full_path_length = strlen(path);
                char *full_path =
                    path_get_absolute_path(path,
                                           full_path_length,
                                           &full_path_length);

                if (full_path == NULL) {
                    fputs("Failed to allocate memory\n", stderr);
                    destroy_tbds_array(&tbds);

                    return 1;
                }

                /*
                 * Verify that object at our write-path (if existing) is a
                 * directory either when recursing, or when parsing a
                 * dyld_shared_cache file, and a regular file when parsing a
                 * single file.
                 */

                struct stat info = {};
                if (stat(full_path, &info) == 0) {
                    if (S_ISREG(info.st_mode)) {
                        if (options.recurse_directories &&
                            !tbd->options.combine_tbds)
                        {
                            fputs("Writing to a regular file while recursing a "
                                  "directory is not supported.\nTo combine all "
                                  ".tbds into a single file, please provide "
                                  "the --combine option.\nOtherwise, please "
                                  "provide a directory to write all found "
                                  "files to\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

                            destroy_tbds_array(&tbds);
                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        if (!options.recurse_directories &&
                            !tbd->filetypes.dyld_shared_cache)
                        {
                            fputs("Writing to a directory while parsing a "
                                  "single mach-o file is not supported.\n"
                                  "Please provide a path to a file to write "
                                  "the provided mach-o file's .tbd file\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

                            destroy_tbds_array(&tbds);
                            return 1;
                        }

                        if (options.combine_tbds) {
                            fputs("We cannot combine all tbds to a single file "
                                  "and write to a directory.\nPlease provide a "
                                  "path to a file to write the created .tbd(s)"
                                  "\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

                            destroy_tbds_array(&tbds);
                            return 1;
                        }
                    }
                }

                /*
                 * Copy the path (if still from argv) to allow open_r to create
                 * the file, as modifying full_path's memory is necessary.
                 */

                if (full_path == path) {
                    full_path = alloc_and_copy(full_path, full_path_length);
                    if (full_path == NULL) {
                        fputs("Failed to allocate memory\n", stderr);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }
                }

                tbd->write_path = full_path;
                tbd->write_path_length = full_path_length;
                found_path = true;

                break;
            }

            if (!found_path) {
                fputs("Please provide either a path to a write-file or "
                      "\"stdout\" to print to stdout (terminal)\n",
                      stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            current_tbd_index += 1;
        } else if (strcmp(option, "p") == 0 || strcmp(option, "path") == 0) {
            index += 1;
            if (index == argc) {
                fputs("Please provide either a path to a mach-o file, a path "
                      "to a dyld_shared_cache file, or \"stdin\" to parse from "
                      "terminal input\n",
                      stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            struct tbd_for_main tbd = {};
            setup_tbd_for_main(&tbd);

            bool found_path = false;
            for (; index != argc; index++) {
                const char *const inner_arg = argv[index];
                const char inner_arg_front = inner_arg[0];

                if (inner_arg_front == '-') {
                    const char *inner_opt = inner_arg + 1;
                    const char inner_opt_front = inner_opt[0];

                    if (inner_opt_front == '-') {
                        inner_opt += 1;
                    }

                    const bool ret =
                        tbd_for_main_parse_option(&index,
                                                  &tbd,
                                                  argc,
                                                  argv,
                                                  inner_opt);

                    if (ret) {
                        continue;
                    }

                    fprintf(stderr, "Unrecognized option: %s\n", inner_arg);
                    destroy_tbds_array(&tbds);

                    return 1;
                }

                if (verify_tbd_for_main(&tbd, inner_arg)) {
                    destroy_tbds_array(&tbds);
                    return 1;
                }

                /*
                 * Copy the path, if still from argv, to allow open_r to create
                 * the file.
                 */

                if (tbd.parse_path == inner_arg) {
                    /*
                     * Prevent any ending slashes from being copied to make
                     * directory recursing easier.
                     *
                     * We only need to do this when full_path was not
                     * created with the current-directory, as our path
                     * functions don't append ending slashes.
                     */

                    tbd.parse_path_length =
                        remove_end_slashes(tbd.parse_path,
                                           tbd.parse_path_length);

                    tbd.parse_path =
                        alloc_and_copy(tbd.parse_path, tbd.parse_path_length);

                    if (tbd.parse_path == NULL) {
                        fputs("Failed to allocate memory\n", stderr);
                        exit(1);
                    }
                }

                found_path = true;
                break;
            }

            if (!found_path) {
                fputs("Please provide either a path to a mach-o file or "
                      "\"stdin\" to parse from terminal input\n",
                      stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            if (!tbd.parse_options.ignore_exports) {
                if (!tbd.macho_options.use_symbol_table) {
                    will_parse_export_trie = true;
                }
            }

            const enum array_result add_tbd_result =
                array_add_item(&tbds, sizeof(tbd), &tbd, NULL);

            if (add_tbd_result != E_ARRAY_OK) {
                fputs("Internal failure: Failed to add path-info to array\n",
                      stderr);

                destroy_tbds_array(&tbds);
                free(tbd.parse_path);

                return 1;
            }
        } else if (strcmp(option, "list-architectures") == 0) {
            if (index != 1 || argc > 3) {
                fputs("--list-architectures needs to be run either by itself, "
                      "or with a single path to a mach-o file whose archs will "
                      "be printed",
                      stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            /*
             * Two modes exist for option --list-architectures:
             *     - Listing the archs in the arch-info table.
             *     - Listing the archs in a provided mach-o file.
             */

            if (argc == 3) {
                const char *const path = argv[2];

                char *const full_path = path_get_absolute_path(path, 0, NULL);
                const int fd = our_open(full_path, O_RDONLY, 0);

                if (fd < 0) {
                    fprintf(stderr,
                            "Failed to open file at path: %s, error: %s\n",
                            full_path,
                            strerror(errno));

                    if (full_path != path) {
                        free(full_path);
                    }

                    return 1;
                }

                if (full_path != path) {
                    free(full_path);
                }

                macho_file_print_archs(fd);
            } else {
                print_arch_info_list();
            }

            return 0;
        } else if (strcmp(option, "list-dsc-images") == 0) {
            if (index != 1 || argc > 4) {
                fputs("--list-dsc-images needs to be run with a single path to "
                      "a dyld_shared_cache file whose images will be printed.\n"
                      "An additional option (--ordered) can be provided to "
                      "sort the image-paths before printing them\n",
                      stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            const char *const path = argv[2];

            char *const full_path = path_get_absolute_path(path, 0, NULL);
            const int fd = our_open(full_path, O_RDONLY, 0);

            if (fd < 0) {
                fprintf(stderr,
                        "Failed to open file at path: %s, error: %s\n",
                        full_path,
                        strerror(errno));

                if (full_path != path) {
                    free(full_path);
                }

                return 1;
            }

            if (full_path != path) {
                free(full_path);
            }

            /*
             * Two modes exist for option --list-dsc-images:
             *     - Listing the images by index.
             *     - Listing the images after sorting them.
             */

            if (argc == 4) {
                const char *const arg = argv[3];
                if (strcmp(arg, "--ordered") != 0) {
                    fprintf(stderr, "Unrecognized argument: %s\n", arg);
                    return 1;
                }

                print_list_of_dsc_images_ordered(fd);
            } else {
                print_list_of_dsc_images(fd);
            }

            return 0;
        } else if (strcmp(option, "list-objc-constraints") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-objc-constraints need to be run alone\n", stderr);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_objc_constraint_list();
            return 0;
        } else if (strcmp(option, "list-platforms") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-platforms need to be run alone\n", stderr);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_platform_list();
            return 0;
        } else if (strcmp(option, "list-tbd-flags") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-tbd-flags need to be run alone\n", stderr);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_tbd_flags_list();
            return 0;
        } else if (strcmp(option, "list-tbd-versions") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-tbd-versions need to be run alone\n", stderr);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_tbd_version_list();
            return 0;
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (index != 1 || argc != 2) {
                fprintf(stderr,
                        "Option %s needs to be run by itself\n",
                        argument);

                destroy_tbds_array(&tbds);
                return 1;
            }

            print_usage();
            return 0;
        } else {
            fprintf(stderr, "Unrecognized argument: %s\n", argument);
            return 1;
        }
    }

    if (tbds.item_count == 0) {
        fputs("Please provide paths to either files to parse or directories to "
              "recurse\n",
              stderr);

        destroy_tbds_array(&tbds);
        return 1;
    }

    struct string_buffer export_trie_sb = {};
    if (will_parse_export_trie) {
        const enum string_buffer_result reserve_sb_result =
            sb_reserve_space(&export_trie_sb, 512);

        if (reserve_sb_result != E_STRING_BUFFER_OK) {
            fputs("Failed to allocate memory\n", stderr);
            return 1;
        }
    }

    /*
     * If only a single file has been provided, we do not need to print the
     * path-strings of the file we're parsing.
     */

    const bool should_print_paths = (tbds.item_count != 1);
    struct retained_user_info retained = {};

    struct tbd_for_main *tbd = tbds.data;
    const struct tbd_for_main *const end = tbds.data_end;

    for (; tbd != end; tbd++) {
        /*
         * To allow user-input to modify tbd-info for single files, we create a
         * copy of tbd to separate the initial info from the user-input info.
         */

        struct tbd_for_main copy = *tbd;
        const struct tbd_for_main_options options = tbd->options;

        if (options.recurse_directories) {
            /*
             * We have to check write_path here, as its possible the
             * output-command was not provided, leaving the write_path NULL.
             */

            if (tbd->write_path == NULL) {
                fputs("Writing to stdout (the terminal) while recursing a "
                      "directory is not supported.\nPlease provide a directory "
                      "to write all created files to\n",
                      stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            struct recurse_callback_info recurse_info = {
                .tbd = &copy,
                .orig = tbd,
                .retained = &retained,
                .export_trie_sb = &export_trie_sb
            };

            enum dir_recurse_result recurse_dir_result = E_DIR_RECURSE_OK;
            if (options.recurse_subdirectories) {
                recurse_dir_result =
                    dir_recurse_with_subdirs(tbd->parse_path,
                                             tbd->parse_path_length,
                                             O_RDONLY,
                                             &recurse_info,
                                             recurse_directory_callback,
                                             recurse_directory_fail_callback);
            } else {
                recurse_dir_result =
                    dir_recurse(tbd->parse_path,
                                tbd->parse_path_length,
                                O_RDONLY,
                                &recurse_info,
                                recurse_directory_callback,
                                recurse_directory_fail_callback);
            }

            if (recurse_dir_result != E_DIR_RECURSE_OK) {
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to recurse directory (at path %s), "
                            "error: %s\n",
                            tbd->parse_path,
                            strerror(errno));
                } else {
                    fprintf(stderr,
                            "Failed to recurse directory at the provided path, "
                            "error: %s\n",
                            strerror(errno));
                }
            }

            if (recurse_info.files_parsed == 0) {
                if (should_print_paths) {
                    fprintf(stderr,
                            "No new .tbd files were created while parsing "
                            "directory (at path %s)\n",
                            tbd->parse_path);
                } else {
                    fputs("No new .tbd files were created while parsing "
                          "directory at the provided path\n",
                          stderr);
                }
            }

            if (tbd->options.combine_tbds) {
                if (tbd_write_footer(recurse_info.combine_file)) {
                    if (should_print_paths) {
                        fprintf(stderr,
                                "Failed to write footer for combined .tbd file "
                                "for files from directory (at path %s)\n",
                                tbd->parse_path);
                    } else {
                        fputs("Failed to write footer for combined .tbd file "
                              "for files from directory at the provided path\n",
                              stderr);
                    }

                    return 1;
                }

                fclose(recurse_info.combine_file);
            }

            /*
             * Since user-input can modify tbd (orig) to set info for all
             * created .tbd files, there may be some shared (and allocated) info
             * between both copy and `tbd`.
             *
             * To handle this situation, we destroy copy, because copy will
             * likely have more allocated information than `tbd`.
             *
             * tbd on the other hand is cleared with memset(), and not
             * destroyed, to avoid a double-free.
             */

            tbd_for_main_destroy(&copy);
            memset(tbd, 0, sizeof(*tbd));
        } else {
            char *const parse_path = tbd->parse_path;
            const int fd = our_open(parse_path, O_RDONLY, 0);

            if (fd < 0) {
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to open file (at path %s), error: %s\n",
                            tbd->parse_path,
                            strerror(errno));
                } else {
                    fprintf(stderr,
                            "Failed to open the file at the provided path, "
                            "error: %s\n",
                            strerror(errno));
                }

                continue;
            }

            /*
             * We need to store a buffer to read magic.
             */

            struct magic_buffer magic_buffer = {};
            if (tbd->filetypes.macho) {
                struct parse_macho_for_main_args args = {
                    .fd = fd,
                    .magic_buffer = &magic_buffer,
                    .retained = &retained,

                    .tbd = &copy,
                    .orig = tbd,

                    .dir_path = parse_path,
                    .dir_path_length = tbd->parse_path_length,

                    .dont_handle_non_macho_error = false,
                    .print_paths = should_print_paths,

                    .export_trie_sb = &export_trie_sb,
                    .options.verify_write_path = true
                };

                /*
                 * We're only supposed to print the non-macho error if no other
                 * filetypes are enabled.
                 */

                if (tbd->filetypes.dyld_shared_cache) {
                    args.dont_handle_non_macho_error = true;
                }

                const enum parse_macho_for_main_result parse_result =
                    parse_macho_file_for_main(args);

                if (parse_result != E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO) {
                    continue;
                }
            }

            if (tbd->filetypes.dyld_shared_cache) {
                struct parse_dsc_for_main_args args = {
                    .fd = fd,
                    .magic_buffer = &magic_buffer,
                    .retained = &retained,

                    .tbd = &copy,
                    .orig = tbd,

                    .dsc_dir_path = parse_path,
                    .dsc_dir_path_length = tbd->parse_path_length,

                    .dont_handle_non_dsc_error = false,
                    .print_paths = should_print_paths,

                    .export_trie_sb = &export_trie_sb,
                    .options.verify_write_path = true
                };

                const enum parse_dsc_for_main_result parse_result =
                    parse_dsc_for_main(args);

                if (parse_result != E_PARSE_DSC_FOR_MAIN_NOT_A_SHARED_CACHE) {
                    continue;
                }
            }

            if (!tbd->filetypes.user_provided) {
                if (should_print_paths) {
                    fputs("File (at path %s) is not among any of the provided "
                          "filetypes\n",
                          stderr);
                } else {
                    fputs("File at the provided path is not among any of the "
                          "provided filetypes\n",
                          stderr);
                }
            } else {
                if (should_print_paths) {
                    fputs("File (at path %s) is not among any of the supported "
                          "filetypes\n",
                          stderr);
                } else {
                    fputs("File at the provided path is not among any of the "
                          "supported filetypes\n",
                          stderr);
                }
            }

            tbd_for_main_destroy(tbd);
        }
    }

    /*
     * Since we called tbd_for_main_destroy() on all tbds in the for loop above,
     * we can avoid calling destroy_tbds_array() in favor of just calling
     * array_destroy().
     */

    sb_destroy(&export_trie_sb);
    array_destroy(&tbds);

    return 0;
}
