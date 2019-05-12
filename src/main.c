//
//  src/main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <stdint.h>
#include <stdlib.h>

#include <string.h>
#include <unistd.h>

#include "copy.h"
#include "dir_recurse.h"

#include "parse_or_list_fields.h"
#include "parse_dsc_for_main.h"
#include "parse_macho_for_main.h"

#include "macho_file.h"
#include "path.h"

#include "recursive.h"

#include "unused.h"
#include "usage.h"

struct recurse_callback_info {
    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    uint64_t files_parsed;
    uint64_t retained_info;

    bool print_paths;
};

static bool
recurse_directory_callback(const char *const dir_path,
                           const uint64_t dir_path_length,
                           const int fd,
                           struct dirent *__unused const dirent,
                           void *const callback_info)
{
    struct recurse_callback_info *const recurse_info =
        (struct recurse_callback_info *)callback_info;

    struct tbd_for_main *const tbd = recurse_info->tbd;
    struct tbd_for_main *const global = recurse_info->global;

    uint64_t *const retained = &recurse_info->retained_info;

    /*
     * Keep a buffer around for magic to use.
     */

    char magic[16] = {};
    uint64_t magic_size = 0;

    /*
     * By default we always allow mach-o, but if the filetype is instead
     * dyld_shared_cache, we only recurse for dyld_shared_cache.
     */

    const char *const name = dirent->d_name;
    const uint64_t name_length = strnlen(name, sizeof(dirent->d_name));

    if (tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_MACHO)) {
        const struct parse_macho_for_main_args args = {
            .fd = fd,
            .magic_in = &magic,

            .magic_in_size_in = &magic_size,
            .retained_info_in = retained,

            .global = global,
            .tbd = tbd,

            .dir_path = dir_path,
            .dir_path_length = dir_path_length,

            .name = name,
            .name_length = name_length,

            .ignore_non_macho_error = true,
            .print_paths = true
        };

        const enum parse_macho_for_main_result parse_as_macho_result =
            parse_macho_file_for_main_while_recursing(args);

        switch (parse_as_macho_result) {
            case E_PARSE_MACHO_FOR_MAIN_OK: {
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
        const struct parse_dsc_for_main_args args = {
            .fd = fd,
            .magic_in = &magic,

            .magic_in_size_in = &magic_size,
            .retained_info_in = retained,

            .global = global,
            .tbd = tbd,

            .dsc_dir_path = dir_path,
            .dsc_dir_path_length = dir_path_length,

            .dsc_name = dirent->d_name,
            .dsc_name_length = name_length,

            .ignore_non_cache_error = true,
            .print_paths = true
        };

        const enum parse_dsc_for_main_result parse_as_dsc_result =
            parse_dsc_for_main_while_recursing(args);

        switch (parse_as_dsc_result) {
            case E_PARSE_DSC_FOR_MAIN_OK:
                recurse_info->files_parsed += 1;
                close(fd);

                return true;

            case E_PARSE_DSC_FOR_MAIN_NOT_A_SHARED_CACHE:
                break;

            case E_PARSE_DSC_FOR_MAIN_OTHER_ERROR:
                close(fd);
                return true;
        }
    }

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
                    "at path: %s/%s, error: %s\n",
                    dir_path,
                    dirent->d_name,
                    strerror(errno));

            break;
    }

    return true;
}

static int validate_tbd_filetype(struct tbd_for_main *const tbd) {
    const struct array *const filters = &tbd->dsc_image_filters;
    const struct array *const paths = &tbd->dsc_image_paths;

    if (!array_is_empty(filters) || !array_is_empty(paths)) {
        if (!tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_DSC)) {
            fputs("dsc image-filters and/or dsc image-paths have been provided "
                  "for a file that is not a dyld_shared_cache\n",
                  stderr);

            exit(1);
        }
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
     * Global options are provided to set information of all files and all
     * directories that would ever be provided.
     *
     * Although this concept seem simple, some complexities remain.
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

    for (int index = 1; index < argc; index++) {
        /*
         * Every argument parsed here should be an option. Any extra arguments,
         * like a path-string, should be parsed by the option to which it
         * belongs.
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
                fputs("--help needs to be run by itself\n", stderr);
                return 1;
            }

            print_usage();
            return 0;
        } else if (strcmp(option, "o") == 0 || strcmp(option, "output") == 0) {
            index += 1;
            if (index == argc) {
                fputs("Please provide either a path to an output-file or "
                      "\"stdout\" to print to stdout (terminal)\n",
                      stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            struct tbd_for_main *const tbd =
                array_get_item_at_index(&tbds,
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
            for (; index < argc; index++) {
                /*
                 * Here, we can either receive an option or a path-string, so
                 * the same validation as above cannot be carried out here.
                 */

                const char *const inner_arg = argv[index];
                const char inner_arg_front = inner_arg[0];

                if (inner_arg_front == '-') {
                    const char *inner_opt = inner_arg + 1;
                    const char inner_opt_front = inner_opt[0];

                    if (inner_opt_front == '-') {
                        inner_opt += 1;
                    }

                    if (strcmp(inner_opt, "preserve-subdirs") == 0) {
                        tbd->flags |= F_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS;
                    } else if (strcmp(inner_opt, "no-overwrite") == 0) {
                        tbd->flags |= F_TBD_FOR_MAIN_NO_OVERWRITE;
                    } else {
                        if (strcmp(inner_opt, "replace-path-extension") == 0) {
                            tbd->flags |= F_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION;
                        } else {
                            fprintf(stderr,
                                    "Unrecognized option: %s\n",
                                    inner_arg);

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    }

                    continue;
                }

                /*
                 * We only allow printing to stdout for single-files, and
                 * not when recursing directories.
                 */

                const char *const path = inner_arg;
                if (strcmp(path, "stdout") == 0) {
                    if (tbd->flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                        fputs("Writing to stdout (terminal) while recursing "
                              "a directory is not supported, Please provide "
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

                const uint64_t options = tbd->flags;
                if (!(options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) &&
                    !tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_DSC))
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
                              "provided for recursing directories. You can "
                              "change the extension of your output-file when "
                              "not recursing by changing the extension in its "
                              "path-string\n",
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
                    path_get_absolute_path_if_necessary(path,
                                                        full_path_length,
                                                        &full_path_length);

                if (full_path == NULL) {
                    fputs("Failed to allocate memory\n", stderr);

                    tbd_for_main_destroy(&global);
                    destroy_tbds_array(&tbds);

                    return 1;
                }

                /*
                 * Verify that object at our output-path (if existing) is a
                 * directory when recursing, and a regular file when parsing a
                 * single file.
                 */

                struct stat info = {};
                if (stat(path, &info) == 0) {
                    if (S_ISREG(info.st_mode)) {
                        if (options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                            fputs("Writing to a regular file while recursing a "
                                  "directory is not supported, Please provide "
                                  "a directory to write all found files to\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        const bool has_dsc =
                            tbd_for_main_has_filetype(
                                tbd,
                                TBD_FOR_MAIN_FILETYPE_DSC);

                        if (!(options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) &&
                            !has_dsc)
                        {
                            fputs("Writing to a directory while parsing a "
                                  "single mach-o file is not supported, Please "
                                  "provide a path to a file to write the "
                                  "mach-o file\n",
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
                 * the file (and directory hierarchy if needed).
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
                fputs("Please provide either a path to an output-file or "
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
                fputs("Please provide either a path to a mach-o file or a "
                      "dyld_shared_cache file or \"stdin\" to parse from "
                      "terminal input\n",
                      stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            struct tbd_for_main tbd = {};
            bool found_path = false;

            for (; index < argc; index++) {
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

                        const int specification_index = index + 1;
                        if (specification_index < argc) {
                            const char *const specification =
                                argv[specification_index];

                            if (strcmp(specification, "all") == 0) {
                                index += 1;
                                tbd.flags |=
                                    F_TBD_FOR_MAIN_RECURSE_SUBDIRECTORIES;
                            } else if (strcmp(specification, "once") == 0) {
                                index += 1;
                            }
                        }
                    } else {
                        const bool ret =
                            tbd_for_main_parse_option(&tbd,
                                                      argc,
                                                      argv,
                                                      inner_opt,
                                                      &index);

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

                const char *const path = inner_arg;
                if (strcmp(path, "stdin") != 0) {
                    /*
                     * We may have been provided with a path relative to the
                     * current-directory.
                     */

                    uint64_t full_path_length = strlen(path);
                    char *full_path =
                        path_get_absolute_path_if_necessary(path,
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
                        if (errno == ENOENT) {
                            fprintf(stderr,
                                    "No file or directory exists at path: %s\n",
                                    full_path);
                        } else {
                            fprintf(stderr,
                                    "Failed to retrieve information on object "
                                    "at path: %s\n",
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
                            fprintf(stderr,
                                    "Recursing file (at path %s) is not "
                                    "supported, Please provide a path to a "
                                    "directory if recursing is needed\n",
                                    full_path);

                            if (full_path != path) {
                                free(full_path);
                            }

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        if (!(tbd.flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES)) {
                            fputs("Unable to open a directory as a mach-o file"
                                  ", Please provide option '-r' to indicate "
                                  "recursing\n",
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
                     * Copy the path (if not from argv) to allow open_r to
                     * create the file (and directory hierarchy if needed).
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

                        const char *const last_slashes =
                            path_find_ending_row_of_slashes(full_path,
                                                            full_path_length);

                        if (last_slashes != NULL) {
                            full_path_length =
                                (uint64_t)(last_slashes - full_path);
                        }

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
                        fputs("Recursing the input file is not supported, "
                              "Please provide a path to a directory if "
                              "recursing is needed\n",
                              stderr);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }
                }

                /*
                 * Check for any conflicts in the provided options.
                 */

                if (tbd.parse_options & O_TBD_PARSE_IGNORE_FLAGS) {
                    if (tbd.flags & F_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
                        fprintf(stderr,
                                "Both modifying tbd-flags and removing the "
                                "field entirely for path (%s) is not "
                                "not supported. Please choose only a single "
                                "option\n",
                                path);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        free(tbd.parse_path);
                        return 1;
                    }

                    if (tbd.flags_re != 0) {
                        fprintf(stderr,
                                "Both modifying tbd-flags and removing the "
                                "field entirely for file(s) at path (%s) is "
                                "not supported. Please choose only a single "
                                "option\n",
                                path);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        free(tbd.parse_path);
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

            const enum array_result add_tbd_result =
                array_add_item(&tbds, sizeof(tbd), &tbd, NULL);

            if (add_tbd_result != E_ARRAY_OK) {
                fputs("Internal failure: Failed to add info to array\n",
                      stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                free(tbd.parse_path);
                return 1;
            }
        } else if (strcmp(option, "list-architectures") == 0) {
            if (index != 1 || argc > 3) {
                fputs("--list-architectures needs to be run by itself, or with "
                      "a single path to a mach-o file whose archs will be "
                      "printed",
                      stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            /*
             * Two modes exist for option --list-architectures:
             *     - Listing the archs in the arch-info table
             *     - Listing the archs in a provided mach-o file.
             */

            if (argc == 3) {
                const char *const path = argv[2];
                char *const full_path =
                    path_get_absolute_path_if_necessary(path,
                                                        strlen(path),
                                                        NULL);

                const int fd = open(full_path, O_RDONLY);
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

                macho_file_print_archs(fd);
            } else {
                print_arch_info_list();
            }

            return 0;
        } else if (strcmp(option, "list-dsc-images") == 0) {
            if (index != 1 || argc != 3) {
                fputs("--list-dsc-images needs to be run with a single path to "
                      "a dyld_shared_cache file whose images will be printed\n",
                      stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            const char *const path = argv[2];
            char *const full_path =
                path_get_absolute_path_if_necessary(path, strlen(path), NULL);

            const int fd = open(full_path, O_RDONLY);
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

            print_list_of_dsc_images(fd);
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
                fputs("--list-tbd-flags need to be run alone\n", stderr);

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
            if (tbd_for_main_parse_option(&global, argc, argv, opt, &index)) {
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

    /*
     * If only a single file has been provided, we do not need to print the
     * path-strings of the file we're parsing.
     */

    uint64_t retained_info = 0;

    const bool should_print_paths = tbds.item_count != 1;
    const struct tbd_for_main *const end = tbds.data_end;

    struct tbd_for_main *tbd = tbds.data;
    for (; tbd != end; tbd++) {
        tbd_for_main_apply_from(tbd, &global);

        const uint64_t options = tbd->flags;
        if (options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
            /*
             * We have to check here, as its possible output-commands were not
             * provided for the parse commands.
             */

            if (tbd->write_path == NULL) {
                fputs("Writing to stdout (terminal) while recursing a "
                      "directory is not supported, Please provide a directory "
                      "to write all created files to\n",
                      stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            struct recurse_callback_info recurse_info = {
                .global = &global,
                .tbd = tbd,
                .retained_info = retained_info,
                .print_paths = true
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
                            "Failed to recurse directory (at path %s)\n",
                            tbd->parse_path);
                } else {
                    fputs("Failed to recurse the provided directory\n", stderr);
                }
            }

            if (recurse_info.files_parsed == 0) {
                if (should_print_paths) {
                    fprintf(stderr,
                            "No suitable files were found to create .tbd files "
                            "from while recursing directory (at path %s)\n",
                            tbd->parse_path);
                } else {
                    fputs("No suitable files were found to create .tbd files "
                          "from while recursing directory at the provided "
                          "path\n",
                          stderr);
                }
            }
        } else {
            char *const parse_path = tbd->parse_path;
            const int fd = open(parse_path, O_RDONLY);

            if (fd < 0) {
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to open file (at path %s), error: %s\n",
                            tbd->parse_path,
                            strerror(errno));
                } else {
                    fprintf(stderr,
                            "Failed to open the provided mach-o file, "
                            "error: %s\n",
                            strerror(errno));
                }

                continue;
            }

            /*
             * We need to store an external buffer to read magic.
             */

            char magic[16] = {};
            uint64_t magic_size = 0;
            bool matched_filetype = false;

            if (tbd_for_main_has_filetype(tbd, TBD_FOR_MAIN_FILETYPE_MACHO)) {
                struct parse_macho_for_main_args args = {
                    .fd = fd,
                    .magic_in = &magic,

                    .magic_in_size_in = &magic_size,
                    .retained_info_in = &retained_info,

                    .global = &global,
                    .tbd = tbd,

                    .dir_path = parse_path,
                    .dir_path_length = tbd->parse_path_length,

                    .ignore_non_macho_error = false,
                    .print_paths = should_print_paths,

                    .options = O_PARSE_MACHO_FOR_MAIN_VERIFY_WRITE_PATH
                };

                /*
                 * See if any other filetypes are enabled, in which case we
                 * don't print out the non-filetype error.
                 */

                if (tbd->filetypes_count != 1) {
                    args.ignore_non_macho_error = true;
                }

                const enum parse_macho_for_main_result parse_result =
                    parse_macho_file_for_main(args);

                if (parse_result != E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO) {
                    matched_filetype = true;
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

                    .dsc_dir_path = parse_path,
                    .dsc_dir_path_length = tbd->parse_path_length,

                    .ignore_non_cache_error = false,
                    .print_paths = should_print_paths,

                    /*
                     * Verify the write-path only at the last moment, as global
                     * configuration is now accounted for.
                     */

                    .options = O_PARSE_DSC_FOR_MAIN_VERIFY_WRITE_PATH
                };

                /*
                 * See if any other filetypes are enabled, in which case we
                 * don't print out the non-filetype error.
                 */

                if (tbd->filetypes_count != 1) {
                    args.ignore_non_cache_error = true;
                }

                const enum parse_dsc_for_main_result parse_result =
                    parse_dsc_for_main(args);

                if (parse_result != E_PARSE_DSC_FOR_MAIN_NOT_A_SHARED_CACHE) {
                    matched_filetype = true;
                }
            }

            if (!matched_filetype) {
                if (tbd->filetypes == 0) {
                    if (should_print_paths) {
                        fputs("File at provided path (%s) was not among any of "
                              "the supported filetypes\n",
                              stderr);
                    } else {
                        fputs("File at the provided path was not among any of "
                              "the supported filetypes\n",
                              stderr);
                    }
                } else if (tbd->filetypes_count != 1) {
                    if (should_print_paths) {
                        fputs("File at provided path (%s) was not among any of "
                              "the provided filetypes\n",
                              stderr);
                    } else {
                        fputs("File at the provided path was not among any of "
                              "the provided filetypes\n",
                              stderr);
                    }
                }
            }
        }
    }

    tbd_for_main_destroy(&global);
    destroy_tbds_array(&tbds);

    return 0;
}
