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

#include "dir_recurse.h"
#include "parse_or_list_fields.h"

#include "parse_dsc_for_main.h"
#include "parse_macho_for_main.h"

#include "macho_file.h"
#include "path.h"

#include "recursive.h"
#include "usage.h"

struct recurse_callback_info {
    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    uint64_t retained_info;
    bool print_paths;
};

static bool
recurse_directory_callback(const char *const parse_path,
                           struct dirent *const dirent,
                           void *const callback_info)
{
    struct recurse_callback_info *const recurse_info =
        (struct recurse_callback_info *)callback_info;

    struct tbd_for_main *const tbd = recurse_info->tbd;

    const uint64_t options = tbd->options;
    const int fd = open(parse_path, O_RDONLY);

    if (fd < 0) {
        if (!(options & O_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            fprintf(stderr,
                    "Warning: Failed to open file (at path %s), error: %s\n",
                    parse_path,
                    strerror(errno));
        }

        return true;
    }

    struct stat sbuf = {};
    if (fstat(fd, &sbuf) < 0) {
        if (!(options & O_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            fprintf(stderr,
                    "Warning: Failed to get info on file (at path %s), "
                    "error: %s\n",
                    parse_path,
                    strerror(errno));
        }

        return true;
    }

    struct tbd_for_main *const global = recurse_info->global;
    uint64_t *const retained = &recurse_info->retained_info;

    /*
     * By default we always allow mach-o, but if the filetype is instead
     * dyld_shared_cache, we only recurse for dyld_shared_cache.
     */

    const uint64_t parse_path_length = strlen(parse_path);
    if (tbd->filetype != TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE) {
        const bool parse_as_macho_result =
            parse_macho_file(global,
                             tbd,
                             parse_path,
                             parse_path_length,
                             fd,
                             sbuf.st_size,
                             true,
                             retained);

        if (parse_as_macho_result) {
            close(fd);
            return true;
        }
    }

    if (options & O_TBD_FOR_MAIN_RECURSE_INCLUDE_DSC) {
        const bool parse_as_dsc_result =
            parse_shared_cache(global,
                               tbd,
                               parse_path,
                               parse_path_length,
                               fd,
                               sbuf.st_size,
                               true);

        if (parse_as_dsc_result) {
            close(fd);
            return true;
        }
    }

    close(fd);
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
                    const char *option = inner_arg + 1;
                    const char option_front = option[0];
                    
                    if (option_front == '-') {
                        option += 1;
                    }

                    if (strcmp(option, "preserve-subdirs") == 0) {
                        tbd->options |=
                            O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS;
                    } else if (strcmp(option, "no-overwrite") == 0) {
                        tbd->options |= O_TBD_FOR_MAIN_NO_OVERWRITE;
                    } else {
                        if (strcmp(option, "replace-path-extension") == 0) {
                            tbd->options |=
                                O_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION;
                        } else {
                            fprintf(stderr,
                                    "Unrecognized option: %s\n",
                                    inner_arg);

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

                if (strcmp(inner_arg, "stdout") == 0) {
                    const bool preserve_subdirs =
                        tbd->options &
                        O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS;

                    if (preserve_subdirs) {
                        fputs("Preserving sub-directories while writing to "
                              "stdout is not possible. Please provide an "
                              "output-directory to write files and directories "
                              "to\n",
                              stdout);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    if (has_stdout) {
                        fputs("Cannot print more than one file to stdout\n",
                              stderr);
                        
                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    has_stdout = true;
                    continue;
                }

                /*
                 * Ensure options for recursing directories are not being
                 * provided for other contexts and circumstances.
                 * 
                 * We allow dyld_shared_cache files to slip through as they will 
                 * always be exported to a directory.
                 */

                const uint64_t options = tbd->options;
                if (!(options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) &&
                    tbd->filetype != TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE)
                {
                    if (options & O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS) {
                        fputs("Option --preserve-subdirs can only be "
                              "provided recursing directoriess\n",
                              stderr);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    if (options & O_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION) {
                        fputs("Option --replace-path-extension can only be "
                              "provided for recursing directories. You can "
                              "change the extension of your output-file when "
                              "not recursing by changing the extension in its "
                              "path-string\n",
                              stderr);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }
                }

                /*
                 * We may have been provided with a path relative to the
                 * current-directory.
                 */

                const char *const path = inner_arg;
                char *full_path = path_get_absolute_path_if_necessary(path);

                if (path == NULL) {
                    fputs("Failed to allocate memory\n", stderr);
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
                        if (options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                            fputs("Writing to a regular file while recursing a "
                                  "directory is not supported, Please provide "
                                  "a directory to write all found files to\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

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

                        if (!(options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) &&
                            !is_dsc)
                        {
                            fputs("Writing to a directory while parsing a "
                                  "single file is not supported, Please "
                                  "provide a directory to write all found "
                                  "files to\n",
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
                 * the file (and directory hierarchy if needed).
                 */

                if (full_path == path) {
                    full_path = strdup(full_path);
                    if (full_path == NULL) {
                        fputs("Failed to allocate memory\n", stderr);
                        destroy_tbds_array(&tbds);

                        return 1;
                    }
                }

                tbd->write_path = full_path;
                tbd->write_path_length = strlen(full_path);

                found_path = true;
                break;
            }

            if (!found_path) {
                fputs("Please provide either a path to an output-file or "
                      "\"stdout\" to print to stdout (terminal)\n",
                      stderr);

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

                destroy_tbds_array(&tbds);
                return 1;
            }

            struct tbd_for_main tbd = {};
            bool found_path = false;

            for (; index < argc; index++) {
                const char *const inner_arg = argv[index];
                const char inner_arg_front = inner_arg[0];

                if (inner_arg_front == '-') {
                    const char *option = inner_arg + 1;
                    const char option_front = option[0];

                    if (option_front == '-') {
                        option += 1;
                    }

                    if (strcmp(option, "r") == 0 ||
                        strcmp(option, "recurse") == 0)
                    {
                        tbd.options |= O_TBD_FOR_MAIN_RECURSE_DIRECTORIES;

                        /*
                         * -r/--recurse may have an extra argument specifying
                         * whether or not to recurse sub-directories (By
                         * default, we don't).
                         */

                        const uint64_t specification_index = index + 1;
                        if (specification_index < argc) {
                            const char *const specification =
                                argv[specification_index];

                            if (strcmp(specification, "all") == 0) {
                                index += 1;
                                tbd.options |=
                                    O_TBD_FOR_MAIN_RECURSE_SUBDIRECTORIES;
                            } else if (strcmp(specification, "once") == 0) {
                                index += 1;
                            }
                        }
                    } else if (strcmp(option, "dsc") == 0) {
                        tbd.filetype = TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE;
                    } else if (strcmp(option, "include-dsc") == 0) {
                        tbd.options |= O_TBD_FOR_MAIN_RECURSE_INCLUDE_DSC;
                    } else if (strcmp(option, "skip-image-dirs") == 0) {
                        tbd.options |= O_TBD_FOR_MAIN_RECURSE_SKIP_IMAGE_DIRS;
                    } else {
                        const bool ret =
                            tbd_for_main_parse_option(&tbd,
                                                      argc,
                                                      argv,
                                                      option,
                                                      &index);
                            
                        if (ret) {
                            continue;
                        }
                        
                        fprintf(stderr, "Unrecognized option: %s\n", inner_arg);
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

                    char *full_path = path_get_absolute_path_if_necessary(path);
                    if (full_path == NULL) {
                        fputs("Failed to allocate memory\n", stderr);
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

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    if (S_ISREG(info.st_mode)) {
                        if (tbd.options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                            fprintf(stderr,
                                    "Recursing file (at path %s) is not "
                                    "supported, Please provide a path to a "
                                    "directory if recursing is needed\n",
                                    full_path);

                            if (full_path != path) {
                                free(full_path);
                            }

                            destroy_tbds_array(&tbds);
                            return 1;
                        }

                        if (tbd.options & O_TBD_FOR_MAIN_RECURSE_INCLUDE_DSC) {
                            fputs("Option (--include-dsc) is used to indicate "
                                  "that dyld_shared_cache files should also be "
                                  "parsed while recursing, in addition to "
                                  "mach-o files. Please use option --dsc "
                                  "instead to indicate you want to parse a "
                                  "dyld_shared_cache file\n", stderr);
                            
                            if (full_path != path) {
                                free(full_path);
                            }

                            destroy_tbds_array(&tbds);
                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        const uint64_t recurse_directories =
                            tbd.options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES;

                        if (!recurse_directories) {
                            fputs("Unable to open a directory as a "
                                  "mach-o file, Please provide option '-r' to "
                                  "indicate recursing\n",
                                  stderr);

                            if (full_path != path) {
                                free(full_path);
                            }

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

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    /*
                     * Prevent any ending slashes from being copied to make
                     * directory recursing easier.
                     */

                    const char *const last_slashes =
                        path_find_ending_row_of_slashes(full_path);

                    uint64_t full_path_length = 0;
                    if (last_slashes == NULL) {
                        full_path_length = strlen(full_path);
                    } else {
                        full_path_length = last_slashes - full_path;
                    }

                    /*
                     * Copy the path (if not from argv) to allow open_r to
                     * create the file (and directory hierarchy if needed).
                     */

                    if (full_path == path) {
                        full_path = strndup(full_path, full_path_length);
                        if (full_path == NULL) {
                            fputs("Failed to allocate memory\n", stderr);
                            destroy_tbds_array(&tbds);

                            return 1;
                        }
                    }

                    tbd.parse_path = full_path;
                    tbd.parse_path_length = full_path_length;
                } else {
                    if (tbd.options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                        fputs("Recursing the input file is not supported, "
                              "Please provide a path to a directory if "
                              "recursing is needed\n",
                              stderr);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }
                }

                /*
                 * Check for any conflicts in the provided options.
                 */

                if (tbd.parse_options & O_TBD_PARSE_IGNORE_FLAGS) {
                    if (tbd.options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
                        fprintf(stderr,
                                "Both modifying tbd-flags and removing the "
                                "field entirely for path (%s) is not "
                                "not supported. Please choose only a single "
                                "option\n",
                                path);

                        destroy_tbds_array(&tbds);
                        return 1;
                    }

                    if (tbd.flags_re != 0) {
                        fprintf(stderr,
                                "Both modifying tbd-flags and removing the "
                                "field entirely for file(s) at path (%s) is "
                                "not supported. Please choose only a single "
                                "option\n",
                                path);

                        destroy_tbds_array(&tbds);
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

                destroy_tbds_array(&tbds);
                return 1;
            }

            const enum array_result add_tbd_result =
                array_add_item(&tbds, sizeof(tbd), &tbd, NULL);

            if (add_tbd_result != E_ARRAY_OK) {
                fputs("Internal failure: Failed to add info to array\n",
                      stderr);

                free(tbd.parse_path);
                destroy_tbds_array(&tbds);

                return 1;
            }
        } else if (strcmp(option, "list-architectures") == 0) {
            if (index != 1 || argc > 3) {
                fputs("--list-architectures needs to be run by itself, or with "
                      "a single path to a mach-o file whose archs should be "
                      "printed",
                      stderr);

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
                    path_get_absolute_path_if_necessary(argument);

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
        } else if (strcmp(option, "list-recurse") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-recurse need to be run alone\n", stderr);

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

                destroy_tbds_array(&tbds);
                return 1;
            }

            print_tbd_flags_list();
            return 0;
        } else if (strcmp(option, "list-tbd-versions") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-tbd-flags need to be run alone\n", stderr);

                destroy_tbds_array(&tbds);
                return 1;
            }

            print_tbd_version_list();
            return 0;
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--usage needs to be run by itself\n", stderr);

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
            destroy_tbds_array(&tbds);

            return 1;
        }
    }

    if (array_is_empty(&tbds)) {
        fputs("Please provide paths to either files to parse or directories to "
              "recurse\n",
              stderr);

        destroy_tbds_array(&tbds);
        return 1;
    }

    /*
     * If only a single file has been provided, we do not need to print the
     * path-strings of the file we're parsing.
     */

    bool should_print_paths =
        array_get_item_count(&tbds, sizeof(struct tbd_for_main)) != 1;

    uint64_t retained_info = 0;

    struct tbd_for_main *tbd = tbds.data;
    const struct tbd_for_main *const end = tbds.data_end;

    for (; tbd != end; tbd++) {
        tbd_for_main_apply_from(tbd, &global);

        const uint64_t options = tbd->options;
        if (options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
            /*
             * We have to check here, as its possible output-commands were not
             * created for parse commands.
             */

            if (tbd->write_path == NULL) {
                fputs("Writing to stdout (terminal) while recursing "
                      "a directory is not supported, Please provide "
                      "a directory to write all found files to\n",
                      stderr);

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
                            options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES,
                            &recurse_info,
                            recurse_directory_callback);

            if (recurse_dir_result != E_DIR_RECURSE_OK) {
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to recurse directory (at path %s)\n",
                            tbd->parse_path);
                } else {
                    fputs("Failed to recurse the provided directory\n", stderr);
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
            
            struct stat sbuf = {};
            if (fstat(fd, &sbuf) < 0) {
                if (!(options & O_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
                    if (should_print_paths) {
                        fprintf(stderr,
                                "Failed to get info on file (at path %s), "
                                "error: %s\n",
                                parse_path,
                                strerror(errno));
                    } else {
                        fprintf(stderr,
                                "Failed to get info on file (at path %s), "
                                "error: %s\n",
                                parse_path,
                                strerror(errno));
                    }
                }

                continue;
            }

            switch (tbd->filetype) {
                case TBD_FOR_MAIN_FILETYPE_MACHO:
                    parse_macho_file(&global,
                                     tbd,
                                     parse_path,
                                     tbd->parse_path_length,
                                     fd,
                                     sbuf.st_size,
                                     true,
                                     &retained_info);

                    break;

                case TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE:
                    parse_shared_cache(&global,
                                       tbd,
                                       parse_path,
                                       tbd->parse_path_length,
                                       fd,
                                       sbuf.st_size,
                                       false);

                    break;
            }

            close(fd);
        }
    }

    destroy_tbds_array(&tbds);
    return 0;
}
