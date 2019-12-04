//
//  src/main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdio.h>
#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <stdint.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include "string_buffer.h"
#include "copy.h"
#include "dir_recurse.h"

#include "parse_or_list_fields.h"
#include "parse_dsc_for_main.h"
#include "parse_macho_for_main.h"

#include "macho_file.h"
#include "notnull.h"

#include "our_io.h"
#include "path.h"

#include "recursive.h"
#include "tbd.h"
#include "tbd_write.h"

#include "unused.h"
#include "usage.h"
#include "util.h"

struct recurse_callback_info {
    struct tbd_for_main *global;
    struct tbd_for_main *tbd;
    const struct tbd_create_info *orig;

    FILE *combine_file;

    uint64_t files_parsed;
    uint64_t retained_info;

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

    struct tbd_for_main *const tbd = recurse_info->tbd;
    struct tbd_for_main *const global = recurse_info->global;

    uint64_t *const retained = &recurse_info->retained_info;

    /*
     * We need to store a buffer for the parse_*_for_main_while_recursing()
     * APIs.
     */

    char magic[16] = {};
    uint64_t magic_size = 0;

    const char *const name = dirent->d_name;
    const bool should_combine = (tbd->flags & F_TBD_FOR_MAIN_COMBINE_TBDS);

    if (tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_MACHO)) {
        struct parse_macho_for_main_args args = {
            .fd = fd,
            .magic_in = &magic,

            .magic_in_size_in = &magic_size,
            .retained_info_in = retained,

            .global = global,
            .tbd = tbd,
            .orig = recurse_info->orig,

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

    if (tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_DSC)) {
        struct parse_dsc_for_main_args args = {
            .fd = fd,
            .magic_in = &magic,

            .magic_in_size_in = &magic_size,
            .retained_info_in = retained,

            .global = global,
            .tbd = tbd,
            .orig = recurse_info->orig,

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

static int validate_tbd_filetype(struct tbd_for_main *const tbd) {
    const struct array *const filters = &tbd->dsc_image_filters;
    if (filters->item_count == 0) {
        return 0;
    }

    if (!tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_DSC)) {
        fputs("dsc image-filters have been provided for a file that is not a "
              "dyld_shared_cache file\n",
              stderr);

        exit(1);
    }

    return 0;
}

static void destroy_tbds_array(struct array *const tbds) {
    struct tbd_for_main *tbd = tbds->data;
    const struct tbd_for_main *const end = tbds->data_end;

    for (; tbd != end; tbd++) {
        tbd_for_main_destroy(tbd);
    }

    array_destroy(tbds);
}

int main(const int argc, const char *const argv[]) {
    if (argc < 2) {
        print_usage();
        return 0;
    }

    /*
     * We distinguish between "local" and "global" options.
     *
     * Local options are provided to set information of only one file or
     * directory that is provided with one "--path" option.
     *
     * Global options are provided to set information of all files and all
     * directories that would ever be provided.
     *
     * Although this construction seems simple, some complexities remain.
     *
     * It would make no sense for a global option to override an explicity
     * stated local option, as there would have been no reason to have
     * explicitly provided the local option if a global option of a different
     * type was to be provided.
     *
     * To store our global information, we keep a tbd_with_options struct here
     * where information can be stored, and later set the information of "local"
     * tbd_with_options structures.
     */

    struct array tbds = {};
    struct tbd_for_main global = {
        .info.version = TBD_VERSION_V2
    };

    /*
     * Store an index into the tbds array indicating the "current" tbd for when
     * we parsing output arguments.
     */

    uint64_t current_tbd_index = 0;

    bool has_stdout = false;
    bool will_parse_exports = false;

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

            tbd_for_main_destroy(&global);
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

                tbd_for_main_destroy(&global);
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

                tbd_for_main_destroy(&global);
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
                        tbd->flags |= F_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS;
                    } else if (strcmp(in_opt, "no-overwrite") == 0) {
                        tbd->flags |= F_TBD_FOR_MAIN_NO_OVERWRITE;
                    } else if (strcmp(in_opt, "replace-path-extension") == 0) {
                        tbd->flags |= F_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION;
                    } else if (strcmp(in_opt, "combine-tbds") == 0) {
                        tbd->flags |= F_TBD_FOR_MAIN_COMBINE_TBDS;
                    } else {
                        fprintf(stderr, "Unrecognized option: %s\n", in_arg);

                        tbd_for_main_destroy(&global);
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
                    if (tbd->flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                        fputs("Writing to stdout (terminal) while recursing "
                              "a directory is not supported.\nPlease provide "
                              "a directory to write all created files to\n",
                              stderr);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }

                    if (has_stdout) {
                        fputs("Printing more than one file to stdout is not "
                              "allowed\n",
                              stderr);

                        tbd_for_main_destroy(&global);
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

                const bool has_dsc =
                    tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_DSC);

                const uint64_t options = tbd->flags;
                if (!(options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) &&
                    !has_dsc)
                {
                    if (options & F_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS) {
                        fputs("Option --preserve-subdirs can only be provided "
                              "for either recursing directoriess, or parsing "
                              "dyld_shared_cache files\n",
                              stderr);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }

                    if (options & F_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION) {
                        fputs("Option --replace-path-extension can only be "
                              "provided for recursing directories.\nYou can "
                              "change the extension of your write-file when "
                              "not recursing by changing the extension in its "
                              "path-string\n",
                              stderr);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }

                    if (options & F_TBD_FOR_MAIN_COMBINE_TBDS) {
                        fputs("Option --combine-tbds can only be provided "
                              "recursing directories and parsing "
                              "dyld_shared_cache files\n",
                              stderr);

                        tbd_for_main_destroy(&global);
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

                    tbd_for_main_destroy(&global);
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
                        if (options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES &&
                            !(tbd->flags & F_TBD_FOR_MAIN_COMBINE_TBDS))
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

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        if (!(options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) &&
                            !has_dsc)
                        {
                            fputs("Writing to a directory while parsing a "
                                  "single mach-o file is not supported.\n"
                                  "Please provide a path to a file to write "
                                  "the provided mach-o file's .tbd file\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }

                        if (options & F_TBD_FOR_MAIN_COMBINE_TBDS) {
                            fputs("We cannot combine all tbds to a single file "
                                  "and write to a directory.\nPlease provide a "
                                  "path to a file to write the created .tbd(s)"
                                  "\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

                            tbd_for_main_destroy(&global);
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

                        tbd_for_main_destroy(&global);
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

                tbd_for_main_destroy(&global);
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

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            struct tbd_for_main tbd = {};
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

                    if (strcmp(inner_opt, "r") == 0 ||
                        strcmp(inner_opt, "recurse") == 0)
                    {
                        tbd.flags |= F_TBD_FOR_MAIN_RECURSE_DIRECTORIES;

                        /*
                         * -r/--recurse may have an extra argument specifying
                         * whether or not to recurse sub-directories (By
                         * default, we don't).
                         */

                        const int spec_index = index + 1;
                        if (spec_index < argc) {
                            const char *const spec = argv[spec_index];
                            if (strcmp(spec, "all") == 0) {
                                index += 1;
                                tbd.flags |=
                                    F_TBD_FOR_MAIN_RECURSE_SUBDIRECTORIES;
                            } else if (strcmp(spec, "once") == 0) {
                                index += 1;
                            }
                        }
                    } else {
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

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }

                    continue;
                }

                /*
                 * Check for any conflicts in the provided options.
                 */

                const char *const path = inner_arg;
                if (tbd.parse_options & O_TBD_PARSE_IGNORE_FLAGS) {
                    if (tbd.info.fields.flags != 0) {
                        fprintf(stderr,
                                "Both modifying tbd-flags, and removing the "
                                "field entirely, for file(s) at path (%s), is "
                                "not supported.\nPlease select only one "
                                "option\n",
                                path);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }
                }

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

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }

                    /*
                     * Ensure that the file/directory actually exists on the
                     * filesystem.
                     */

                    struct stat info = {};
                    if (stat(full_path, &info) != 0) {
                        /*
                         * ENOTDIR will be returned if a directory in the
                         * path hierarchy isn't a directory at all, which still
                         * means that no file/directory exists at the provided
                         * path.
                         */

                        if (errno == ENOENT || errno == ENOTDIR) {
                            fprintf(stderr,
                                    "No file or directory exists at path: %s\n",
                                    full_path);
                        } else {
                            fprintf(stderr,
                                    "Failed to retrieve information on file or "
                                    "directory at path, error: %s\n",
                                    full_path);
                        }

                        if (full_path != path) {
                            free(full_path);
                        }

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }

                    if (S_ISREG(info.st_mode)) {
                        if (tbd.flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                            fputs("Only directories can be recursed\n", stderr);
                            if (full_path != path) {
                                free(full_path);
                            }

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        if (!(tbd.flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES)) {
                            fputs("Please provide option '-r' to if you want "
                                  "to recurse the provided directory\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    } else {
                        fprintf(stderr,
                                "Unsupported object at path: %s\n",
                                full_path);

                        if (full_path != path) {
                            free(full_path);
                        }

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }

                    /*
                     * Copy the path, if still from argv, to allow open_r to
                     * create the file.
                     */

                    if (full_path == path) {
                        /*
                         * Prevent any ending slashes from being copied to make
                         * directory recursing easier.
                         *
                         * We only need to do this when full_path was not
                         * created with the current-directory, as our path
                         * functions don't append ending slashes.
                         */

                        full_path_length =
                            remove_end_slashes(full_path, full_path_length);

                        full_path = alloc_and_copy(full_path, full_path_length);
                        if (full_path == NULL) {
                            fputs("Failed to allocate memory\n", stderr);

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    }

                    tbd.parse_path = full_path;
                    tbd.parse_path_length = full_path_length;
                } else {
                    if (tbd.flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                        fputs("Recursing the input file is not supported.\n"
                              "Please provide a path to a directory for "
                              "recursing\n",
                              stderr);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }
                }

                validate_tbd_filetype(&tbd);
                found_path = true;

                break;
            }

            if (!found_path) {
                fputs("Please provide either a path to a mach-o file or "
                      "\"stdin\" to parse from terminal input\n",
                      stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            if (!(tbd.parse_options & O_TBD_PARSE_IGNORE_EXPORTS)) {
                will_parse_exports = true;
            }

            const enum array_result add_tbd_result =
                array_add_item(&tbds, sizeof(tbd), &tbd, NULL);

            if (add_tbd_result != E_ARRAY_OK) {
                fputs("Internal failure: Failed to add path-info to array\n",
                      stderr);

                tbd_for_main_destroy(&global);
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

                tbd_for_main_destroy(&global);
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

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_objc_constraint_list();
            return 0;
        } else if (strcmp(option, "list-platforms") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-platforms need to be run alone\n", stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_platform_list();
            return 0;
        } else if (strcmp(option, "list-tbd-flags") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-tbd-flags need to be run alone\n", stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_tbd_flags_list();
            return 0;
        } else if (strcmp(option, "list-tbd-versions") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-tbd-versions need to be run alone\n", stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_tbd_version_list();
            return 0;
        } else if (strcmp(option, "no-overwrite") == 0) {
            global.flags |= F_TBD_FOR_MAIN_NO_OVERWRITE;
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (index != 1 || argc != 2) {
                fprintf(stderr,
                        "Option %s needs to be run by itself\n",
                        argument);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            print_usage();
            return 0;
        } else {
            const char *const opt = option;
            if (tbd_for_main_parse_option(&index, &global, argc, argv, opt)) {
                continue;
            }

            fprintf(stderr, "Unrecognized option: %s\n", argument);

            tbd_for_main_destroy(&global);
            destroy_tbds_array(&tbds);

            return 1;
        }
    }

    if (tbds.item_count == 0) {
        fputs("Please provide paths to either files to parse or directories to "
              "recurse\n",
              stderr);

        tbd_for_main_destroy(&global);
        destroy_tbds_array(&tbds);

        return 1;
    }

    struct string_buffer export_trie_sb = {};
    if (will_parse_exports) {
        const enum string_buffer_result reserve_sb_result =
            sb_reserve_space(&export_trie_sb, 32);

        if (reserve_sb_result != E_STRING_BUFFER_OK) {
            fputs("Failed to allocate memory\n", stderr);
            return 1;
        }
    }

    /*
     * If only a single file has been provided, we do not need to print the
     * path-strings of the file we're parsing.
     */

    uint64_t retained_info = 0;
    const bool should_print_paths = (tbds.item_count != 1);

    struct tbd_for_main *tbd = tbds.data;
    const struct tbd_for_main *const end = tbds.data_end;

    for (; tbd != end; tbd++) {
        tbd_for_main_apply_missing_from(tbd, &global);

        const uint64_t options = tbd->flags;
        const struct tbd_create_info orig = tbd->info;

        if (options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
            /*
             * We have to check write_path here, as its possible the
             * output-command was not provided, leaving the write_path NULL.
             */

            if (tbd->write_path == NULL) {
                fputs("Writing to stdout (the terminal) while recursing a "
                      "directory is not supported.\nPlease provide a directory "
                      "to write all created files to\n",
                      stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            struct recurse_callback_info recurse_info = {
                .global = &global,
                .tbd = tbd,
                .orig = &orig,
                .retained_info = retained_info,
                .export_trie_sb = &export_trie_sb
            };

            enum dir_recurse_result recurse_dir_result = E_DIR_RECURSE_OK;
            if (options & F_TBD_FOR_MAIN_RECURSE_SUBDIRECTORIES) {
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

            if (tbd->flags & F_TBD_FOR_MAIN_COMBINE_TBDS) {
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

            char magic[16] = {};
            uint64_t magic_size = 0;

            if (tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_MACHO)) {
                struct parse_macho_for_main_args args = {
                    .fd = fd,
                    .magic_in = &magic,

                    .magic_in_size_in = &magic_size,
                    .retained_info_in = &retained_info,

                    .global = &global,
                    .tbd = tbd,
                    .orig = &orig,

                    .dir_path = parse_path,
                    .dir_path_length = tbd->parse_path_length,

                    .dont_handle_non_macho_error = false,
                    .print_paths = should_print_paths,

                    /*
                     * Verify the write-path only at the last moment, as global
                     * configuration is now accounted for.
                     */

                    .export_trie_sb = &export_trie_sb,
                    .options = O_PARSE_MACHO_FOR_MAIN_VERIFY_WRITE_PATH
                };

                /*
                 * We're only supposed to print the non-macho error if no other
                 * filetypes are enabled.
                 */

                if (tbd->filetypes_count != 1) {
                    args.dont_handle_non_macho_error = true;
                }

                const enum parse_macho_for_main_result parse_result =
                    parse_macho_file_for_main(args);

                if (parse_result != E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO) {
                    continue;
                }
            }

            if (tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_DSC)) {
                struct parse_dsc_for_main_args args = {
                    .fd = fd,
                    .magic_in = &magic,

                    .magic_in_size_in = &magic_size,
                    .retained_info_in = &retained_info,

                    .global = &global,
                    .tbd = tbd,
                    .orig = &orig,

                    .dsc_dir_path = parse_path,
                    .dsc_dir_path_length = tbd->parse_path_length,

                    .dont_handle_non_dsc_error = false,
                    .print_paths = should_print_paths,

                    /*
                     * Verify the write-path only at the last moment, as global
                     * configuration is now accounted for.
                     */

                    .export_trie_sb = &export_trie_sb,
                    .options = O_PARSE_DSC_FOR_MAIN_VERIFY_WRITE_PATH
                };

                /*
                 * If other filetypes are enabled, we don't print out the
                 * non-filetype error.
                 */

                if (tbd->filetypes_count != 1) {
                    args.dont_handle_non_dsc_error = true;
                }

                const enum parse_dsc_for_main_result parse_result =
                    parse_dsc_for_main(args);

                if (parse_result != E_PARSE_DSC_FOR_MAIN_NOT_A_SHARED_CACHE) {
                    continue;
                }
            }

            if (tbd->filetypes == 0) {
                if (should_print_paths) {
                    fputs("File (at path %s) is not among any of the supported "
                          "filetypes\n",
                          stderr);
                } else {
                    fputs("File at the provided path is not among any of the "
                          "supported filetypes\n",
                          stderr);
                }
            } else if (tbd->filetypes_count != 1) {
                if (should_print_paths) {
                    fputs("File (at path %s) is not among any of the provided "
                          "filetypes\n",
                          stderr);
                } else {
                    fputs("File at the provided path is not among any of the "
                          "provided filetypes\n",
                          stderr);
                }
            }
        }
    }

    sb_destroy(&export_trie_sb);

    tbd_for_main_destroy(&global);
    destroy_tbds_array(&tbds);

    return 0;
}
