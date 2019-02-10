//
//  src/tbd_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 - 2019 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

#include "macho_file.h"
#include "parse_or_list_fields.h"

#include "path.h"
#include "recursive.h"
#include "tbd_for_main.h"

static void
add_image_filter(struct tbd_for_main *const tbd,
                 const int argc,
                 const char *const *const argv,
                 int *const index_in)
{
    const int index = *index_in;
    if (index == argc) {
        fputs("Please provide a name of an image (a simple "
              "path-component) to filter out images to be parsed\n",
              stderr);

        exit(1);
    }

    const char *const string = argv[index + 1];
    const struct tbd_for_main_dsc_image_filter filter = {
        .string = string,
        .length = strlen(string)
    };

    const enum array_result add_filter_result =
        array_add_item(&tbd->dsc_image_filters, sizeof(filter), &filter, NULL);

    if (add_filter_result != E_ARRAY_OK) {
        fprintf(stderr,
                "Experienced an array failure trying to add image-filter %s\n",
                string); 

        exit(1);
    }

    *index_in = index + 1;
}

static void
add_image_number(struct tbd_for_main *const tbd,
                 const int argc,
                 const char *const *const argv,
                 int *const index_in)
{
    const int index = *index_in;
    if (index == argc) {
        fputs("Please provide a name of an image (a simple path-component) to "
              "filter out images to be parsed\n",
              stderr);

        exit(1);
    }

    const char *const number_string = argv[index + 1];
    const uint64_t number = strtoul(number_string, NULL, 10);

    if (number == 0) {
        fprintf(stderr,
                "An image-number of \"%s\" is invalid\n",
                number_string);

        exit(1);
    } 

    /*
     * Limit the number to only 32-bit as that's the range allowed by the
     * dyld_cache_header structure.
     */

    if (number > UINT32_MAX) {
        fprintf(stderr,
                "An image number of \"%s\" is too large to be valid\n",
                number_string);

        exit(1);
    }

    const enum array_result add_number_result =
        array_add_item(&tbd->dsc_image_numbers,
                       sizeof(number),
                       &number,
                       NULL);

    if (add_number_result != E_ARRAY_OK) {
        fprintf(stderr,
                "Experienced an array failure trying to add image-number %s\n",
                number_string); 

        exit(1);
    }

    *index_in = index + 1;
}

static void
add_image_path(struct tbd_for_main *const tbd,
               const int argc,
               const char *const *const argv,
               int *const index_in)
{
    const int index = *index_in;
    if (index == argc) {
        fputs("Please provide the path for an image to be parsed out\n",
              stderr);

        exit(1);
    }

    const char *const string = argv[index + 1];
    const struct tbd_for_main_dsc_image_path path = {
        .string = string,
    };

    const enum array_result add_path_result =
        array_add_item(&tbd->dsc_image_paths, sizeof(path), &path, NULL);

    if (add_path_result != E_ARRAY_OK) {
        fprintf(stderr,
                "Experienced an array failure trying to add image-path %s\n",
                string); 

        exit(1);
    }

    *index_in = index + 1;
}

bool
tbd_for_main_parse_option(struct tbd_for_main *const tbd,
                          const int argc,
                          const char *const *const argv,
                          const char *const option,
                          int *const index_in)
{
    int index = *index_in;
    if (strcmp(option, "add-archs") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            fputs("Adding and replacing architectures is not supported, Please "
                  "choose only a single option\n",
                  stderr);

            exit(1);
        }

        index += 1;

        tbd->info.archs = parse_architectures_list(argc, argv, &index);
        tbd->options |= O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS;
    } else if (strcmp(option, "add-flags") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            if (tbd->flags_re != 0) {
                fputs("Replacing and adding flags is not supported, Please "
                      "choose only a single option\n",
                      stderr);

                exit(1);
            }
        }

        index += 1;

        tbd->info.flags = parse_flags_list(argc, argv, &index);
        tbd->options |= O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS;
    } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_NORMAL_SYMBOLS;
    } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_WEAK_DEF_SYMBOLS;
    } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS;
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS;
    } else if (strcmp(option, "allow-private-objc-class-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS;
    } else if (strcmp(option, "allow-private-objc-ivar-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS;
    } else if (strcmp(option, "ignore-clients") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_CLIENTS;
    } else if (strcmp(option, "ignore-compatibility-version") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;
    } else if (strcmp(option, "ignore-current-version") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_CURRENT_VERSION;
    } else if (strcmp(option, "ignore-missing-exports") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_MISSING_EXPORTS;
    } else if (strcmp(option, "ignore-missing-uuids") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_MISSING_UUIDS;
    } else if (strcmp(option, "ignore-non-unique-uuids") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS;
    } else if (strcmp(option, "ignore-objc-constraint") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
    } else if (strcmp(option, "ignore-parent-umbrella") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;
    } else if (strcmp(option, "ignore-reexports") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_REEXPORTS;
    } else if (strcmp(option, "ignore-swift-version") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
    } else if (strcmp(option, "ignore-requests") == 0) {
        tbd->options |= O_TBD_FOR_MAIN_NO_REQUESTS;
    } else if (strcmp(option, "ignore-warnings") == 0) {
        tbd->options |= O_TBD_FOR_MAIN_IGNORE_WARNINGS;
    } else if (strcmp(option, "image-filter-name") == 0) {
        add_image_filter(tbd, argc, argv, &index);
    } else if (strcmp(option, "image-filter-number") == 0) {
        add_image_number(tbd, argc, argv, &index);
    } else if (strcmp(option, "image-path") == 0) {
        add_image_path(tbd, argc, argv, &index);
    } else if (strcmp(option, "remove-archs") == 0) {
        if (!(tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS)) {
            if (tbd->archs_re != 0) {
                fputs("Replacing and removing architectures is not supported, "
                      "Please choose only a single option\n",
                      stderr);

                exit(1);
            }
        }
        
        index += 1;

        tbd->archs_re = parse_architectures_list(argc, argv, &index);
        tbd->options |= O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS;
    } else if (strcmp(option, "remove-flags") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            if (tbd->info.flags != 0) {
                fputs("Replacing and removing flags is not supported, Please "
                      "choose only a single option\n",
                      stderr);

                exit(1);
            }
        }

        index += 1;
        tbd->flags_re = parse_flags_list(argc, argv, &index);
    } else if (strcmp(option, "replace-archs") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            fputs("Adding/removing and replacing architectures is not "
                  "supported, Please choose only a single option\n",
                  stderr);

            exit(1);
        }
        
        index += 1;
        tbd->archs_re = parse_architectures_list(argc, argv, &index);
    } else if (strcmp(option, "replace-flags") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            fputs("Adding/removing and replacing flags is not supported, "
                  "Please choose only a single option\n",
                  stderr);

            exit(1);
        }

        index += 1;
        tbd->flags_re = parse_flags_list(argc, argv, &index);
    } else if (strcmp(option, "replace-objc-constraint") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide an objc-constraint value\n", stderr);
            exit(1);
        }

        const char *const argument = argv[index];
        const enum tbd_objc_constraint objc_constraint =
            parse_objc_constraint(argument);

        if (objc_constraint == 0) {
            fprintf(stderr,
                    "Unrecognized objc-constraint: %s. Run "
                    "--list-objc-constraint to see a list of valid "
                    "objc-constraints\n",
                    argument);

            exit(1);
        }

        tbd->info.objc_constraint = objc_constraint;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    } else if (strcmp(option, "replace-platform") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide an objc-constraint value\n", stderr);
            exit(1);
        }

        const char *const argument = argv[index];
        const enum tbd_platform platform = parse_platform(argument);

        if (platform == 0) {
            fprintf(stderr,
                    "Unrecognized platform: %s. Run --list-platform to see a "
                    "list of valid platforms\n",
                    argument);

            exit(1);
        }

        tbd->info.platform = platform;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    } else if (strcmp(option, "replace-swift-version") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a swift-version\n", stderr);
            exit(1);
        }

        tbd->info.swift_version = parse_swift_version(argv[index]);
        tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
    } else if (strcmp(option, "skip-image-dirs") == 0) {
        tbd->options |= O_TBD_FOR_MAIN_RECURSE_SKIP_IMAGE_DIRS;
    } else if (strcmp(option, "skip-invalid-archs") == 0) {
        tbd->macho_options |=
            O_MACHO_FILE_PARSE_SKIP_INVALID_ARCHITECTURES; 
    } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a tbd-version\n", stderr);
            exit(1);
        }

        const char *const argument = argv[index];
        const enum tbd_version version = parse_tbd_version(argument);

        if (version == 0) {
            fprintf(stderr,
                    "Unrecognized tbd-version: %s. Run --list-tbd-versions to "
                    "see a list of valid tbd-versions\n",
                    argument);

            exit(1);
        }

        tbd->info.version = version;
    } else {
        return false;
    }

    *index_in = index;
    return true;
}

char *
tbd_for_main_create_write_path(const struct tbd_for_main *const tbd,
                               const char *const folder_path,
                               const uint64_t folder_path_length,
                               const char *const file_path,
                               const uint64_t file_path_length,
                               const char *const extension,
                               const uint64_t extension_length,
                               const bool file_path_is_in_tbd,
                               uint64_t *const length_out)
{
    char *write_path = NULL;
    if (tbd->options & O_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS) {
        /*
         * The subdirectories are simply the directories following the
         * user-provided recurse-directory.
         * 
         * If file_path is related to tbd->parse_path, then we need to get the
         * sub-directories of file_path that are not in tbd->parse_path but are
         * in the hierarchy of file_path.
         */

        const char *subdirs_iter = file_path;
        uint64_t subdirs_length = file_path_length;

        if (file_path_is_in_tbd) {
            const uint64_t parse_path_length = tbd->parse_path_length;

            subdirs_iter += parse_path_length;
            subdirs_length -= parse_path_length;
        }

        if (tbd->options & O_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION) {
            const char *const original_extension =
                path_find_extension(subdirs_iter, subdirs_length);

            if (original_extension != NULL) {
                subdirs_length = (uint64_t)(original_extension - subdirs_iter);
            }
        }

        write_path =
            path_append_component_and_extension_with_len(folder_path,
                                                         folder_path_length,
                                                         subdirs_iter,
                                                         subdirs_length,
                                                         extension,
                                                         extension_length,
                                                         length_out);

        if (write_path == NULL) {
            fputs("Failed to allocate memory\n", stderr);
            exit(1);
        }
    } else {
        uint64_t file_name_length = 0;
        const char *const file_name =
            path_get_last_path_component(file_path,
                                         file_path_length,
                                         &file_name_length);

        if (file_name == NULL) {
            return strdup(folder_path);
        }

        write_path =
            path_append_component_and_extension_with_len(folder_path,
                                                         folder_path_length,
                                                         file_name,
                                                         file_name_length,
                                                         extension,
                                                         extension_length,
                                                         length_out);

        if (write_path == NULL) {
            fputs("Failed to allocate memory\n", stderr);
            exit(1);
        }
    }

    return write_path;
}

void
tbd_for_main_write_to_path(const struct tbd_for_main *const tbd,
                           const char *const input_path,
                           char *const write_path,
                           const uint64_t write_path_length,
                           const bool print_paths)
{
    char *terminator = NULL;
    const uint64_t options = tbd->options;

    const int flags = (options & O_TBD_FOR_MAIN_NO_OVERWRITE) ? O_EXCL : 0;
    const int write_fd =
        open_r(write_path,
               write_path_length,
               O_WRONLY | O_TRUNC | flags,
               DEFFILEMODE,
               0755,
               &terminator);
    
    if (write_fd < 0) {        
        if (!(options & O_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            /*
             * If the file already exists, we should just skip over to prevent
             * overwriting.
             * 
             * Note: EEXIST is only returned when O_EXCL was set, which is only
             * set for O_TBD_FOR_MAIN_NO_OVERWRITE, meaning no check here is
             * necessary.
             */

            if (errno == EEXIST) {
                fprintf(stderr,
                        "Warning: Skipping over file (at path %s) as a file at "
                        "its output-path (%s) already exists\n",
                        input_path,
                        write_path);
            } else {
                if (print_paths) {
                    fprintf(stderr,
                            "Failed to open output-file (for path: %s), "
                            "error: %s\n",
                            write_path,
                            strerror(errno));
                } else {
                    fprintf(stderr,
                            "Failed to open the provided output-file, "
                            "error: %s\n",
                            strerror(errno));
                }
            }
        }

        /*
         * Although getting the file descriptor failed, its likely open_r
         * still created the directory hierarchy (and if so the terminator
         * shouldn't be NULL).
         */

        if (terminator != NULL) {
            /*
             * Ignore the return value as we cannot be sure if the remove failed
             * as the directories we created (that are pointed to by terminator)
             * may now be populated with other files.
             */
            
            remove_partial_r(write_path, write_path_length, terminator);
        }

        return;
    }

    FILE *const write_file = fdopen(write_fd, "w");
    if (write_file == NULL) {
        if (!(options & O_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to open output-file (for path: %s) as FILE, "
                        "error: %s\n",
                        write_path,
                        strerror(errno));
            } else {
                fprintf(stderr,
                        "Failed to open the provided output-file as FILE, "
                        "error: %s\n",
                        strerror(errno));
            }
        }

        return;
    }
    
    const struct tbd_create_info *const create_info = &tbd->info;
    const enum tbd_create_result create_tbd_result =
        tbd_create_with_info(create_info, write_file, tbd->write_options);

    if (create_tbd_result != E_TBD_CREATE_OK) {
        if (!(options & O_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to write to output-file (for input-file at "
                        "path: %s, to output-file's path: %s)\n",
                        input_path,
                        write_path);
            } else {
                fputs("Failed to write to provided output-file\n", stderr);
            }
        }

        if (terminator != NULL) {
            /*
             * Ignore the return value as we cannot be sure if the remove failed
             * as the directories we created (that are pointed to by terminator)
             * may now be populated with other files.
             */
            
            remove_partial_r(write_path, write_path_length, terminator);
        }
    }

    fclose(write_file);
}

void
tbd_for_main_write_to_stdout(const struct tbd_for_main *const tbd,
                             const char *const input_path,
                             const bool print_paths)
{
    const struct tbd_create_info *const create_info = &tbd->info;
    const enum tbd_create_result create_tbd_result =
        tbd_create_with_info(create_info, stdout, tbd->write_options);

    if (create_tbd_result != E_TBD_CREATE_OK) {
        if (!(tbd->options & O_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to write to stdout (the terminal) (for "
                        "input-file at path: %s) error: %s\n",
                        input_path,
                        strerror(errno));
            } else {
                fputs("Failed to write to stdout (the terminal) for "
                      "the provided input-file, error: %s\n",
                      stderr);
            }
        }
    }
}

static int
tbd_for_main_dsc_image_filter_comparator(const void *const array_item,
                                         const void *const item)
{
    const struct tbd_for_main_dsc_image_filter *const array_filter =
        (const struct tbd_for_main_dsc_image_filter *)array_item;

    const struct tbd_for_main_dsc_image_filter *const filter =
        (const struct tbd_for_main_dsc_image_filter *)item;

    return strcmp(array_filter->string, filter->string);
}

static int
image_number_comparator(const void *const array_item,
                        const void *const item)
{
    const uint32_t array_number = *(const uint32_t *)array_item;
    const uint32_t number = *(const uint32_t *)item;

    if (array_number > number) {
        return 1;
    } else if (array_number < number) {
        return -1;
    }

    return 0;
}

static int
tbd_for_main_dsc_image_path_comparator(const void *const array_item,
                                       const void *const item)
{
    const struct tbd_for_main_dsc_image_path *const array_path =
        (const struct tbd_for_main_dsc_image_path *)array_item;

    const struct tbd_for_main_dsc_image_path *const path =
        (const struct tbd_for_main_dsc_image_path *)item;

    return strcmp(array_path->string, path->string);
}

void
tbd_for_main_apply_from(struct tbd_for_main *const dst,
                        const struct tbd_for_main *const src)
{
    if (dst->info.archs == 0) {
        /*
         * Only preset archs if we aren't later removing/replacing these archs.
         */

        if (dst->archs_re != 0) {
            dst->info.archs = src->info.archs;
        }
    }

    if (dst->info.current_version == 0) {
        dst->info.current_version = src->info.current_version;
    }

    if (dst->info.compatibility_version == 0) {
        dst->info.compatibility_version = src->info.compatibility_version;
    }

    if (dst->info.flags == 0) {
        /*
         * Only preset flags if we aren't later removing/replacing these flags.
         */
        
        if (dst->flags_re != 0) {
            dst->info.flags = src->info.flags;
        }
    }

    if (dst->info.install_name == NULL) {
        dst->info.install_name = src->info.install_name;
    }

    if (dst->info.parent_umbrella == NULL) {
        dst->info.parent_umbrella = src->info.parent_umbrella;
    }

    if (dst->info.platform == 0) {
        dst->info.platform = src->info.platform;
    }

    if (dst->info.objc_constraint == 0) {
        dst->info.objc_constraint = src->info.objc_constraint;
    }

    if (dst->info.swift_version == 0) {
        dst->info.swift_version = src->info.swift_version;
    }
        
    if (dst->info.version == 0) {
        dst->info.version = src->info.version;
    }

    if (dst->filetype == TBD_FOR_MAIN_FILETYPE_DYLD_SHARED_CACHE) {
        const struct array *const src_filters = &src->dsc_image_filters;
        if (!array_is_empty(src_filters)) {
            const enum array_result add_filters_result =
                array_add_and_unique_items_from_array(
                    &dst->dsc_image_filters,
                    sizeof(struct tbd_for_main_dsc_image_filter),
                    src_filters,
                    tbd_for_main_dsc_image_filter_comparator);

            if (add_filters_result != E_ARRAY_OK) {
                fputs("Experienced an array failure when trying to add dsc "
                      "image-filters\n",
                      stderr);

                exit(1);
            }
        }

        const struct array *const src_numbers = &src->dsc_image_numbers;
        if (!array_is_empty(src_numbers)) {
            const enum array_result add_numbers_result =
                array_add_and_unique_items_from_array(&dst->dsc_image_numbers,
                                                      sizeof(uint32_t),
                                                      src_numbers,
                                                      image_number_comparator);

            if (add_numbers_result != E_ARRAY_OK) {
                fputs("Experienced an array failure when trying to add dsc "
                      "image-numbers\n",
                      stderr);

                exit(1);
            }
        }

        const struct array *const src_paths = &src->dsc_image_paths;
        if (!array_is_empty(src_paths)) {
            const enum array_result add_paths_result =
                array_add_and_unique_items_from_array(
                    &dst->dsc_image_paths,
                    sizeof(struct tbd_for_main_dsc_image_path),
                    src_paths,
                    tbd_for_main_dsc_image_path_comparator);

            if (add_paths_result != E_ARRAY_OK) {
                fputs("Experienced an array failure when trying to add dsc "
                      "image-paths\n",
                      stderr);

                exit(1);
            }
        }

    }

    dst->macho_options |= src->macho_options;
    dst->dsc_options |= src->dsc_options;

    dst->parse_options |= src->parse_options;
    dst->write_options |= src->write_options;

    dst->options |= src->options;
}

void tbd_for_main_destroy(struct tbd_for_main *const tbd) {
    tbd_create_info_destroy(&tbd->info);

    array_destroy(&tbd->dsc_image_filters);
    array_destroy(&tbd->dsc_image_numbers);
    array_destroy(&tbd->dsc_image_paths);

    free(tbd->parse_path);
    free(tbd->write_path);

    tbd->parse_path = NULL;
    tbd->write_path = NULL;

    tbd->macho_options = 0;
    tbd->dsc_options = 0;

    tbd->write_options = 0;
    tbd->parse_options = 0;

    tbd->filetype = 0;
    tbd->options = 0;
}
