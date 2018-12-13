//
//  src/main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 inoahdev. All rights reserved.
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
#include "handle_macho_file_parse_result.h"

#include "parse_or_list_fields.h"
#include "path.h"

#include "recursive.h"
#include "usage.h"

struct recurse_callback_info {
    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    uint64_t retained_info;
    bool print_paths;
};

static void
clear_create_info(struct tbd_create_info *const info_in,
                  const struct tbd_create_info *const orig)
{
    tbd_create_info_destroy(info_in);
    *info_in = *orig;
}

static bool
recurse_directory_callback(const char *const parse_path,
                           struct dirent *const dirent,
                           void *const callback_info)
{
    const int fd = open(parse_path, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr,
                "Warning: Failed to open file (at path %s), error: %s\n",
                parse_path,
                strerror(errno));

        return true;
    }

    struct recurse_callback_info *const recurse_info =
        (struct recurse_callback_info *)callback_info;

    struct tbd_for_main *const tbd = recurse_info->tbd;
    struct tbd_create_info *const info = &tbd->info;

    /*
     * Store an original copy of the tbd, so we can restore to it later after
     * we're done here.
     */

    struct tbd_create_info original_info = *info;

    const uint64_t macho_options = O_MACHO_FILE_IGNORE_INVALID_FIELDS;
    const enum macho_file_parse_result parse_result =
        macho_file_parse_from_file(info, fd, tbd->parse_options, macho_options);

    const bool handle_ret =
        handle_macho_file_parse_result(recurse_info->global,
                                       tbd,
                                       parse_path,
                                       parse_result,
                                       recurse_info->global,
                                       &recurse_info->retained_info);

    if (!handle_ret) {
        clear_create_info(info, &original_info);
        close(fd);

        return true;
    }

    char *terminator = NULL;
    char *write_path = tbd->write_path;

    /*
     * Create the file-path string itself, and handle the different options
     * avaialble.
     */

    const uint64_t options = tbd->options;
    if (options & O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_HIERARCHY) {
        const char *const hierarchy_iter = parse_path + tbd->parse_path_length;
        uint64_t hierarchy_length = 0;

        if (options & O_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION) {
            const char *const hierarchy_extension =
                strrchr(hierarchy_iter, '.');

            if (hierarchy_extension != NULL) {
                hierarchy_length = hierarchy_extension - hierarchy_iter;
            }
        } else {
            hierarchy_length = strlen(hierarchy_iter);
        }

        write_path =
            path_append_component_and_extension_with_len(tbd->write_path,
                                                         tbd->write_path_length,
                                                         hierarchy_iter,
                                                         hierarchy_length,
                                                         "tbd",
                                                         3);

        if (write_path == NULL) {
            fputs("Failed to allocate memory\n", stderr);
            exit(1);
        }
    } else {
        /*
         * Since parse_path was created without any ending slashes, we can
         * find the slashes that precede the last component.
         */
        
        const char *const file_name =
            path_find_last_row_of_slashes(parse_path + tbd->parse_path_length);

        uint64_t file_name_length = 0;

        if (options & O_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION) {
            const char *const hierarchy_extension = strrchr(file_name, '.');
            if (hierarchy_extension != NULL) {
                file_name_length = hierarchy_extension - file_name;
            }
        } else {
            file_name_length = strlen(file_name);
        }

        write_path =
            path_append_component_and_extension_with_len(tbd->write_path,
                                                         tbd->write_path_length,
                                                         file_name,
                                                         file_name_length,
                                                         "tbd",
                                                         3);

        if (write_path == NULL) {
            fputs("Failed to allocate memory\n", stderr);
            exit(1);
        }
    }

    const int flags = (options & O_TBD_FOR_MAIN_NO_OVERWRITE) ? O_EXCL : 0;
    const int write_fd =
        open_r(write_path, O_WRONLY | flags, 0755, &terminator);
    
    if (write_fd < 0) {
        fprintf(stderr,
                "Warning: Failed to open output-file (for path: %s), "
                "error: %s\n",
                write_path,
                strerror(errno));

        clear_create_info(info, &original_info);
        
        free(write_path);
        close(fd);

        return true;
    }

    FILE *const write_file = fdopen(write_fd, "w");
    if (write_file == NULL) {
        fprintf(stderr,
                "Warning: Failed to open output-file (for path: %s) as FILE, "
                "error: %s\n",
                write_path,
                strerror(errno));

        clear_create_info(info, &original_info);
        free(write_path);

        close(write_fd);
        close(fd);

        return true;
    }
    
    const enum tbd_create_result create_tbd_result =
        tbd_create_with_info(info, write_file, tbd->write_options);

    if (create_tbd_result != E_TBD_CREATE_OK) {
        fprintf(stderr,
                "Warning: Failed to write to output-file "
                "(for input-file at path: %s, to output-file's path: %s),"
                "error: %s\n",
                parse_path,
                write_path,
                strerror(errno));

        if (terminator != NULL) {
            /*
             * Ignore the return value as we cannot be sure if the remove failed
             * as the directories we created (that are pointed to by terminator)
             * may now be populated with other files.
             */
            
            remove_parial_r(write_path, terminator);
        }
    }

    clear_create_info(info, &original_info);
    close(fd);

    fclose(write_file);
    free(write_path);

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

            return 1;
        }

        const char *option = argument + 1;
        const char option_front = option[0];

        if (option_front == '-') {
            option += 1;
        }

        if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
            if (index != 1 || index != argc - 1) {
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

                    if (strcmp(option, "preserve-hierarchy") == 0) {
                        tbd->options |=
                            O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_HIERARCHY;
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
                    const bool preserve_hierarchy =
                        tbd->options &
                        O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_HIERARCHY;

                    if (preserve_hierarchy) {
                        fputs("Writing to stdout (terminal) while recursing "
                              "a directory is not supported, Please provide "
                              "a directory to write all found files to\n",
                              stderr);

                        return 1;
                    }

                    if (has_stdout) {
                        fputs("Cannot print more than one file to stdout\n",
                              stderr);
                        
                        return 1;
                    }

                    has_stdout = true;
                    continue;
                }

                /*
                 * Ensure options for recursing directories are not being
                 * provided for other contexts and circumstances.
                 */

                const uint64_t options = tbd->options;
                if (!(options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES)) {
                    if (options & O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_HIERARCHY) {
                        fputs("Option --preserve-directories can only be "
                              "provided recursing directoriess\n",
                              stderr);

                        return 1;
                    }

                    if (options & O_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION) {
                        fputs("Option --replace-path-extension can only be "
                              "provided for recursing directories. You can "
                              "change the extension of your output-file when "
                              "not recursing by changing the extension in its "
                              "path-string\n",
                              stderr);

                        return 1;
                    }
                }

                /*
                 * Have the provided path be resolved with the current-directory
                 * if necessary.
                 */

                const char *const path =
                    path_get_absolute_path_if_necessary(inner_arg);
                
                if (path == NULL) {
                    fputs("Failed to allocate memory\n", stderr);
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

                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        if (!(options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES)) {
                            fputs("Writing to a directory while parsing a "
                                  "single file is not supported, Please "
                                  "provide a directory to write all found "
                                  "files to\n",
                                  stderr);

                            return 1;
                        }
                    }
                }

                /*
                 * Copy the path to allow fopen_r to create the file (and
                 * directory hierarchy if needed).
                 */

                char *const path_copy = strdup(path);
                if (path_copy == NULL) {
                    fputs("Failed to allocate memory\n", stderr);
                    return 1;
                }

                tbd->write_path = path_copy;
                tbd->write_path_length = strlen(path_copy);

                found_path = true;
            }

            if (!found_path) {
                fputs("Please provide either a path to an output-file or "
                      "\"stdout\" to print to stdout (terminal)\n",
                      stderr);

                return 1;
            }

            current_tbd_index += 1;
        } else if (strcmp(option, "p") == 0 || strcmp(option, "path") == 0) {
            index += 1;
            if (index == argc) {
                fputs("Please provide either a path to a mach-o file or "
                      "\"stdin\" to parse from terminal input\n",
                      stderr);

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
                        return 1;
                    }

                    continue;
                }

                /*
                 * We may have been provided with a path relative to the
                 * current-directory, signified by path[0], in which case we 
                 * must make the path absolute.
                 */

                const char *path = inner_arg;
                if (strcmp(path, "stdin") != 0) {
                    path = path_get_absolute_path_if_necessary(path);
                    if (path == NULL) {
                        fputs("Failed to allocate memory\n", stderr);
                        return 1;
                    }

                    /*
                    * Ensure that the file/directory actually exists on the
                    * filesystem.
                    */

                    struct stat info = {};
                    if (stat(path, &info) != 0) {
                        if (errno == ENOENT) {
                            fprintf(stderr,
                                    "No file or directory exists at path: %s\n",
                                    path);
                        } else {
                            fprintf(stderr,
                                    "Failed to retrieve information on object "
                                    "at path: %s\n",
                                    path);
                        }

                        return 1;
                    }

                    if (S_ISREG(info.st_mode)) {
                        if (tbd.options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                            fprintf(stderr,
                                    "Recursing file (at path %s) is not "
                                    "supported, Please provide a path to a "
                                    "directory if recursing is needed\n",
                                    path);

                            return 1;
                        }
                    } else if (S_ISDIR(info.st_mode)) {
                        const uint64_t recurse_directories =
                            tbd.options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES;

                        if (!recurse_directories) {
                            fputs("Unable to open a directory as a mach-o file, "
                                "Please provide option '-r' to indicate "
                                "recursing\n", stderr);

                            return 1;
                        }
                    } else {
                        fprintf(stderr,
                                "Unsupported object at path: %s\n",
                                path);

                        return 1;
                    }

                    /*
                     * Prevent any ending slashes from being copied to make
                     * directory recursing easier.
                     */

                    const char *const last_slashes =
                        path_find_ending_row_of_slashes(path);

                    uint64_t path_length = 0;
                    if (last_slashes == NULL) {
                        path_length = strlen(path);
                    } else {
                        path_length = last_slashes - path;
                    }

                    /*
                     * Copy the path to allow fopen_r to create the file (and
                     * directory hierarchy if needed).
                     */

                    char *const path_copy = strndup(path, path_length);
                    if (path_copy == NULL) {
                        fputs("Failed to allocate memory\n", stderr);
                        return 1;
                    }

                    tbd.parse_path = path_copy;
                    tbd.parse_path_length = path_length;
                } else {
                    if (tbd.options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                        fputs("Recursing the input file is not supported, "
                              "Please provide a path to a directory if "
                              "recursing is needed\n",
                              stderr);

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

                        return 1;
                    }

                    if (tbd.flags_re != 0) {
                        fprintf(stderr,
                                "Both modifying tbd-flags and removing the "
                                "field entirely for file(s) at path (%s) is "
                                "not supported. Please choose only a single "
                                "option\n",
                                path);

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

                return 1;
            }

            const enum array_result add_tbd_result =
                array_add_item(&tbds, sizeof(tbd), &tbd, NULL);

            if (add_tbd_result != E_ARRAY_OK) {
                fputs("Internal failure: Failed to add info to array\n",
                      stderr);

                return 1;
            }
        } else if (strcmp(option, "list-architectures") == 0) {
            if (index != 1) {
                fputs("--list-architectures needs to be run by itself\n",
                      stderr);

                exit(1);
            }

           /*
            * Two modes exist for option --list-architectures:
            *     - Listing the archs in the arch-info table
            *     - Listing the archs in a provided mach-o file.
            */

            const int path_index = index + 1;
            if (path_index < argc) {
                const char *const argument = argv[path_index];
                const char *const path =
                    path_get_absolute_path_if_necessary(argument);

                const int fd = open(path, O_RDONLY);
                if (fd < 0) {
                    fprintf(stderr,
                            "Failed to open file at path: %s, error: %s\n",
                            path,
                            strerror(errno));

                    exit(1);
                }

                macho_file_print_archs(fd);
            } else {
                print_arch_info_list();
            }
        } else if (strcmp(option, "list-objc-constraints") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-objc-constraints need to be run alone\n", stderr);
                exit(1);
            }

            print_objc_constraint_list();
        } else if (strcmp(option, "list-platforms") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-platforms need to be run alone\n", stderr);
                exit(1);
            }

            print_platform_list();
        } else if (strcmp(option, "list-recurse") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-recurse need to be run alone\n", stderr);
                exit(1);
            }

            fputs("once, Recurse through all of a directory's files (default)\n"
                  "all,  Recurse through all of a directory's files and "
                  "sub-directories\n",
                  stdout);
        } else if (strcmp(option, "list-tbd-flags") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-tbd-flags need to be run alone\n", stderr);
                exit(1);
            }

            print_tbd_flags_list();
        } else if (strcmp(option, "list-tbd-versions") == 0) {
            if (index != 1 || argc != 2) {
                fputs("--list-tbd-flags need to be run alone\n", stderr);
                exit(1);
            }

            print_tbd_version_list();
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (index != 1 || index != argc - 1) {
                fputs("--usage needs to be run by itself\n", stderr);
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
            return 1;
        }
    }

    if (array_is_empty(&tbds)) {
        fputs("Please provide paths to either files to parse or directories to "
              "recurse\n",
              stderr);

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
                    fputs("Failed to recurse provided directory\n", stderr);
                }
            }
        } else {
            char *const parse_path = tbd->parse_path;
            const int fd = open(parse_path, O_RDONLY);

            if (fd < 0) {
                if (should_print_paths) {
                    fprintf(stderr,
                            "Warning: Failed to open file (at path %s), "
                            "error: %s\n",
                            tbd->parse_path,
                            strerror(errno));
                } else {
                    fprintf(stderr,
                            "Warning: Failed to open the provided mach0o file, "
                            "error: %s\n",
                            strerror(errno));
                }

                return true;
            }
            
            struct tbd_create_info *const info = &tbd->info;

            const uint64_t macho_options = O_MACHO_FILE_IGNORE_INVALID_FIELDS;
            const enum macho_file_parse_result parse_result =
                macho_file_parse_from_file(info,
                                           fd,
                                           tbd->parse_options,
                                           macho_options);

            const bool handle_ret =
                handle_macho_file_parse_result(&global,
                                               tbd,
                                               parse_path,
                                               parse_result,
                                               should_print_paths,
                                               &retained_info);

            if (!handle_ret) {
                close(fd);
                return true;
            }

            char *terminator = NULL;
            char *const write_path = tbd->write_path;

            FILE *write_file = stdout;
            if (write_path != NULL) {
                const int write_fd =
                    open_r(write_path, O_WRONLY, 0755, &terminator);

                if (write_fd < 0) {
                    if (should_print_paths) {
                        fprintf(stderr,
                                "Warning: Failed to open output-file "
                                "(for path: %s), error: %s\n",
                                write_path,
                                strerror(errno));
                    } else {
                        fprintf(stderr,
                                "Warning: Failed to open the provided "
                                "output-file, error: %s\n",
                                strerror(errno));
                    }

                    close(fd);
                    return true;
                }

                write_file = fdopen(write_fd, "w");
                if (write_file == NULL) {
                    if (should_print_paths) {
                        fprintf(stderr,
                                "Warning: Failed to open output-file "
                                "(for path: %s) as FILE, error: %s\n",
                                write_path,
                                strerror(errno));
                    } else {
                        fprintf(stderr,
                                "Warning: Failed to open the provided "
                                "output-file as FILE, error: %s\n",
                                strerror(errno));
                    }
                    
                    close(fd);
                    return true;
                }
            }

            const enum tbd_create_result create_tbd_result =
                tbd_create_with_info(info, write_file, tbd->write_options);

            fclose(write_file);

            if (create_tbd_result != E_TBD_CREATE_OK) {
                if (should_print_paths) {
                    fprintf(stderr,
                            "Warning: Failed to write to output-file "
                            "(for file at path: %s, at path: %s), error: %s\n",
                            parse_path,
                            write_path,
                            strerror(errno));
                } else {
                    fprintf(stderr,
                            "Warning: Failed to write to the provided "
                            "output-file, error: %s\n",
                            strerror(errno));
                }

                if (terminator != NULL) {
                    /*
                     * Ignore the return value as we cannot be sure if the 
                     * emove failed as the directories we created (that are
                     * pointed to by terminator) may now be populated with
                     * multiple files.
                     */
                    
                    remove_parial_r(write_path, terminator);
                }
            }

            close(fd);
        }
    }

    destroy_tbds_array(&tbds);
    return 0;
}