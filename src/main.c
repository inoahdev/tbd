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
recurse_directory_callback(const char *const parse_path,
                           const uint64_t parse_path_length,
                           struct dirent *__unused const dirent,
                           void *const callback_info)
{
    struct recurse_callback_info *const recurse_info =
        (struct recurse_callback_info *)callback_info;

    struct tbd_for_main *const tbd = recurse_info->tbd;

    const uint64_t flags = tbd->flags;
    const int fd = open(parse_path, O_RDONLY);

    if (fd < 0) {
        if (!(flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            fprintf(stderr,
                    "Warning: Failed to open file (at path %s), error: %s\n",
                    parse_path,
                    strerror(errno));
        }

        return true;
    }

    struct tbd_for_main *const global = recurse_info->global;
    uint64_t *const retained = &recurse_info->retained_info;

    /*
     * Keep a buffer for magic around to use.
     */

    char magic[16] = {};
    uint64_t magic_size = 0;

    /*
     * By default we always allow mach-o, but if the filetype is instead
     * dyld_shared_cache, we only recurse for dyld_shared_cache.
     */

    if (tbd->filetype != TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE) {
        const enum parse_macho_file_result parse_as_macho_result =
            parse_macho_file(&magic,
                             &magic_size,
                             retained,
                             global,
                             tbd,
                             parse_path,
                             parse_path_length,
                             fd,
                             true,
                             true);

        switch (parse_as_macho_result) {
            case E_PARSE_MACHO_FILE_OK: {
                recurse_info->files_parsed += 1;

                close(fd);
                return true;
            }

            case E_PARSE_MACHO_FILE_NOT_A_MACHO:
                break;

            case E_PARSE_MACHO_FILE_OTHER_ERROR:
                close(fd);
                return true;
        }

        if (!(flags & F_TBD_FOR_MAIN_RECURSE_INCLUDE_DSC)) {
            close(fd);
            return true;
        }
    } else if (!(flags & F_TBD_FOR_MAIN_RECURSE_INCLUDE_DSC)) {
        close(fd);
        return true;
    }

    const enum parse_shared_cache_result parse_as_dsc_result =
        parse_shared_cache(&magic,
                           &magic_size,
                           retained,
                           global,
                           tbd,
                           parse_path,
                           parse_path_length,
                           fd,
                           true,
                           true,
                           true);

    switch (parse_as_dsc_result) {
        case E_PARSE_SHARED_CACHE_OK:
            recurse_info->files_parsed += 1;

            close(fd);
            return true;

        case E_PARSE_SHARED_CACHE_NOT_A_SHARED_CACHE:
            break;

        case E_PARSE_SHARED_CACHE_OTHER_ERROR:
            close(fd);
            return true;
    }

    close(fd);
    return true;
}

static bool
recurse_directory_fail_callback(const char *const path,
                                __unused const uint64_t path_length,
                                enum dir_recurse_fail_result result,
                                struct dirent *__unused const dirent,
                                void *__unused const callback_info)
{
    switch (result) {
        case E_DIR_RECURSE_FAILED_TO_ALLOCATE_PATH:
            fputs("Failed to allocate memory for path-string\n", stderr);
            break;

        case E_DIR_RECURSE_FAILED_TO_OPEN_SUBDIR:
            fprintf(stderr,
                    "Failed to open sub-directory at path: %s, error: %s\n",
                    path,
                    strerror(errno));

            break;

        case E_DIR_RECURSE_FAILED_TO_READ_ENTRY:
            fprintf(stderr,
                    "Failed to read directory-entry while recursing directory "
                    "at path: %s, error: %s\n",
                    path,
                    strerror(errno));

            break;
    }

    return true;
}

static void verify_dsc_write_path(struct tbd_for_main *const tbd) {
    const char *const write_path = tbd->write_path;
    if (write_path == NULL) {
        /*
         * If we have exactly zero filters and zero numbers, and exactly one
         * path, we can write to stdout (which is what NULL write_path
         * represents).
         *
         * The reason why no filters, no numbers, and no paths is not allowed to
         * write to stdout is because no filters, no numbers, and no paths means
         * all images are parsed.
         */

        const struct array *const filters = &tbd->dsc_image_filters;
        const struct array *const numbers = &tbd->dsc_image_numbers;
        const struct array *const paths = &tbd->dsc_image_paths;

        if (array_is_empty(filters)) {
            if (array_is_empty(numbers)) {
                if (paths->item_count == 1) {
                    return;
                }
            }
        }

        fprintf(stderr,
                "Please provide a directory to write .tbd files created from "
                "images of the dyld_shared_cache file at the provided "
                "path: %s\n",
                tbd->parse_path);

        exit(1);
    }

    struct stat sbuf = {};
    if (stat(write_path, &sbuf) < 0) {
        /*
         * Ignore any errors if the object doesn't even exist.
         */

        if (errno != ENOENT) {
            fprintf(stderr,
                    "Failed to get information on object at the provided "
                    "write-path (%s), error: %s\n",
                    write_path,
                    strerror(errno));

            exit(1);
        }

        return;
    }

    if (S_ISREG(sbuf.st_mode)) {
        /*
         * We allow writing to regular files only with the following conditions:
         *     (1) No filters have been provided. This is because we can't tell
         *         before iterating how many images will pass the filter.
         *
         *     (2) Either only one image-number, or only one image-path has been
         *         provided.
         */

        const struct array *const filters = &tbd->dsc_image_filters;
        if (array_is_empty(filters)) {
            const struct array *const numbers = &tbd->dsc_image_numbers;
            const struct array *const paths = &tbd->dsc_image_paths;

            const uint64_t numbers_count = numbers->item_count;
            const uint64_t paths_count = paths->item_count;

            if (numbers_count == 1 && paths_count == 0) {
                tbd->flags |= F_TBD_FOR_MAIN_DSC_WRITE_PATH_IS_FILE;
                return;
            }

            if (numbers_count == 0 && paths_count == 1) {
                tbd->flags |= F_TBD_FOR_MAIN_DSC_WRITE_PATH_IS_FILE;
                return;
            }
        }

        fputs("Writing to a regular file while parsing multiple images from a "
              "dyld_shared_cache file is not supported, Please provide a "
              "directory to write all tbds to\n",
              stderr);

        exit(1);
    }
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
                    tbd->filetype != TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE)
                {
                    if (options & F_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS) {
                        fputs("Option --preserve-subdirs can only be provided "
                              "for either recursing directoriess, or parsing "
                              "parsing dyld_shared_cache files\n",
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
                        /*
                         * dyld_shared_cache files are always written to a
                         * directory.
                         */

                        const bool is_dsc =
                            tbd->filetype ==
                            TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE;

                        if (!(options & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES) &&
                            !is_dsc)
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
                fputs("Please provide either a path to a mach-o file or "
                      "\"stdin\" to parse from terminal input\n",
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
                    } else if (strcmp(inner_opt, "dsc") == 0) {
                        tbd.filetype = TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE;
                    } else if (strcmp(inner_opt, "include-dsc") == 0) {
                        tbd.flags |= F_TBD_FOR_MAIN_RECURSE_INCLUDE_DSC;
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

                        if (tbd.flags & F_TBD_FOR_MAIN_RECURSE_INCLUDE_DSC) {
                            fputs("Option (--include-dsc) is used to indicate "
                                  "that dyld_shared_cache files should also be "
                                  "parsed while recursing, in addition to "
                                  "mach-o files. Please use option --dsc "
                                  "instead to indicate you want to parse a "
                                  "dyld_shared_cache file\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

                            tbd_for_main_destroy(&global);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        const uint64_t recurse_directories =
                            tbd.flags & F_TBD_FOR_MAIN_RECURSE_DIRECTORIES;

                        if (!recurse_directories) {
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

                if (tbd.filetype != TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE) {
                    const struct array *const filters = &tbd.dsc_image_filters;
                    if (!array_is_empty(filters)) {
                        fprintf(stderr,
                                "Option --image-filter-name, provided for path "
                                "(%s), is only for parsing dyld_shared_cache "
                                "files\n",
                                path);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        free(tbd.parse_path);
                        return 1;
                    }

                    const struct array *const numbers = &tbd.dsc_image_numbers;
                    if (!array_is_empty(numbers)) {
                        fprintf(stderr,
                                "Option --image-filter-number, provided for "
                                "path (%s), is only for parsing "
                                "dyld_shared_cache files\n",
                                path);

                        tbd_for_main_destroy(&global);
                        destroy_tbds_array(&tbds);

                        free(tbd.parse_path);
                        return 1;
                    }
                }

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

                free(tbd.parse_path);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

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
        } else if (strcmp(option, "list-recurse") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-recurse need to be run alone\n", stderr);

                tbd_for_main_destroy(&global);
                destroy_tbds_array(&tbds);

                return 1;
            }

            fputs("once, Recurse through all of a directory's files (default)\n"
                  "all,  Recurse through all of a directory's files and "
                  "sub-directories\n",
                  stdout);

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
                fputs("--usage needs to be run by itself\n", stderr);

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

            const enum dir_recurse_result recurse_dir_result =
                dir_recurse(tbd->parse_path,
                            tbd->parse_path_length,
                            options & F_TBD_FOR_MAIN_RECURSE_SUBDIRECTORIES,
                            &recurse_info,
                            recurse_directory_callback,
                            recurse_directory_fail_callback);

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

            switch (tbd->filetype) {
                case TBD_FOR_MAIN_FILETYPE_MACHO:
                    parse_macho_file(&magic,
                                     &magic_size,
                                     &retained_info,
                                     &global,
                                     tbd,
                                     parse_path,
                                     tbd->parse_path_length,
                                     fd,
                                     false,
                                     should_print_paths);

                    break;

                case TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE: {
                    /*
                     * Verify the write-path only at the last moment, as global
                     * configuration is now accounted for.
                     */

                    verify_dsc_write_path(tbd);
                    parse_shared_cache(&magic,
                                       &magic_size,
                                       &retained_info,
                                       &global,
                                       tbd,
                                       parse_path,
                                       tbd->parse_path_length,
                                       fd,
                                       false,
                                       false,
                                       should_print_paths);

                    break;
                }
            }

            close(fd);
        }
    }

    extern int total_comparator_calls;

    printf("\n\nTOTAL_NUMERATOR_CALLS: %d\n\n", total_comparator_calls);

    tbd_for_main_destroy(&global);
    destroy_tbds_array(&tbds);

    return 0;
}
