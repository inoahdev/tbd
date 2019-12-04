//
//  src/tbd_for_main.c
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

#include "copy.h"
#include "macho_file.h"
#include "parse_or_list_fields.h"

#include "path.h"
#include "recursive.h"
#include "tbd.h"
#include "tbd_for_main.h"

static void
add_image_filter(int *__notnull const index_in,
                 struct tbd_for_main *__notnull const tbd,
                 const int argc,
                 const char *const *__notnull const argv,
                 const bool is_directory)
{
    const int index = *index_in;
    if (index == argc) {
        fputs("Please provide a name of an image (a simple path-component) to "
              "filter out images to be parsed\n",
              stderr);

        exit(1);
    }

    const char *const string = argv[index + 1];
    struct tbd_for_main_dsc_image_filter filter = {
        .string = string,
        .length = strlen(string)
    };

    if (is_directory) {
        filter.type = TBD_FOR_MAIN_DSC_IMAGE_FILTER_TYPE_DIRECTORY;
    }

    struct array *const filters = &tbd->dsc_image_filters;
    const enum array_result add_filter_result =
        array_add_item(filters, sizeof(filter), &filter, NULL);

    if (add_filter_result != E_ARRAY_OK) {
        fprintf(stderr,
                "Experienced an array failure trying to add image-filter %s\n",
                string);

        exit(1);
    }

    *index_in = index + 1;
}

static void
add_image_number(int *__notnull const index_in,
                 struct tbd_for_main *__notnull const tbd,
                 const int argc,
                 const char *const *__notnull const argv)
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
     * Limit the number to only 32-bits as that's the range allowed by the
     * dyld_cache_header structure.
     */

    if (number > UINT32_MAX) {
        fprintf(stderr,
                "An image number of \"%s\" is too large to be valid\n",
                number_string);

        exit(1);
    }

    const uint32_t number_32 = (uint32_t)number;
    const enum array_result add_number_result =
        array_add_item(&tbd->dsc_image_numbers,
                       sizeof(number_32),
                       &number_32,
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
add_image_path(int *__notnull const index_in,
               struct tbd_for_main *__notnull const tbd,
               const int argc,
               const char *const *__notnull const argv)
{
    const int index = *index_in;
    if (index == argc) {
        fputs("Please provide the path for an image to be parsed out\n",
              stderr);

        exit(1);
    }

    const char *const string = argv[index + 1];
    const struct tbd_for_main_dsc_image_filter path = {
        .string = string,
        .length = strlen(string),
        .type = TBD_FOR_MAIN_DSC_IMAGE_FILTER_TYPE_PATH
    };

    struct array *const paths = &tbd->dsc_image_filters;
    const enum array_result add_path_result =
        array_add_item(paths, sizeof(path), &path, NULL);

    if (add_path_result != E_ARRAY_OK) {
        fprintf(stderr,
                "Experienced an array failure trying to add image-path %s\n",
                string);

        exit(1);
    }

    tbd->dsc_filter_paths_count += 1;
    *index_in = index + 1;
}

bool
tbd_for_main_parse_option(int *const __notnull index_in,
                          struct tbd_for_main *const tbd,
                          const int argc,
                          const char *const *const argv,
                          const char *const option)
{
    int index = *index_in;
    if (strcmp(option, "allow-private-objc-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIV_OBJC_CLASS_SYMS;
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIV_OBJC_EHTYPE_SYMS;
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIV_OBJC_IVAR_SYMS;
    } else if (strcmp(option, "allow-private-objc-class-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIV_OBJC_CLASS_SYMS;
    } else if (strcmp(option, "allow-private-objc-ehtype-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIV_OBJC_EHTYPE_SYMS;
    } else if (strcmp(option, "allow-private-objc-ivar-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIV_OBJC_IVAR_SYMS;
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
    } else if (strcmp(option, "ignore-requests") == 0) {
        tbd->flags |= F_TBD_FOR_MAIN_NO_REQUESTS;
    } else if (strcmp(option, "ignore-swift-version") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
    } else if (strcmp(option, "ignore-undefineds") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_UNDEFINEDS;
    } else if (strcmp(option, "ignore-warnings") == 0) {
        tbd->flags |= F_TBD_FOR_MAIN_IGNORE_WARNINGS;
    } else if (strcmp(option, "ignore-wrong-filetype") == 0) {
        tbd->macho_options |= O_MACHO_FILE_PARSE_IGNORE_WRONG_FILETYPE;
    } else if (strcmp(option, "filter-image-directory") == 0) {
        add_image_filter(&index, tbd, argc, argv, true);
    } else if (strcmp(option, "filter-image-filename") == 0) {
        add_image_filter(&index, tbd, argc, argv, false);
    } else if (strcmp(option, "filter-image-number") == 0) {
        add_image_number(&index, tbd, argc, argv);
    } else if (strcmp(option, "image-path") == 0) {
        add_image_path(&index, tbd, argc, argv);
    } else if (strcmp(option, "dsc") == 0) {
        tbd->filetypes |= TBD_FOR_MAIN_FILETYPE_DSC;
        tbd->filetypes_count++;
    } else if (strcmp(option, "macho") == 0) {
        tbd->filetypes |= TBD_FOR_MAIN_FILETYPE_MACHO;
        tbd->filetypes_count++;
    } else if (strcmp(option, "replace-archs") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a list of architectures to replace the one "
                  "found in the provided input file(s)\n",
                  stderr);

            exit(1);
        }

        if (tbd->info.fields.archs != 0) {
            fputs("Note: Option --replace-archs has been provided multiple "
                  "times.\nOlder option's list of architectures will be "
                  "overriden\n",
                  stderr);
        }

        tbd->info.fields.archs =
            parse_architectures_list(index,
                                     argc,
                                     argv,
                                     &tbd->info.fields.archs_count,
                                     &index);

        tbd->parse_options |= O_TBD_PARSE_IGNORE_ARCHS_AND_UUIDS;
        tbd->write_options |= O_TBD_CREATE_IGNORE_UUIDS;
    } else if (strcmp(option, "replace-current-version") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a current-version to replace the one found "
                  "in the provided input file(s)\n",
                  stderr);

            exit(1);
        }

        if (tbd->flags & F_TBD_FOR_MAIN_PROVIDED_CURRENT_VERSION) {
            fputs("Note: Option --replace-current-version has been provided "
                  "multiple times.\nOlder option's current-version will be "
                  "overriden\n",
                  stderr);
        }

        const char *const arg = argv[index];
        const int64_t packed_version = parse_packed_version(arg);

        if (packed_version == -1) {
            fprintf(stderr, "%s is not a valid current-version\n", arg);
            exit(1);
        }

        tbd->info.fields.current_version = (uint32_t)packed_version;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_CURRENT_VERSION;
        tbd->flags |= F_TBD_FOR_MAIN_PROVIDED_CURRENT_VERSION;
    } else if (strcmp(option, "replace-compat-version") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a compatibility-version to replace the one "
                  "found in the provided input file(s)\n", stderr);

            exit(1);
        }

        if (tbd->flags & F_TBD_FOR_MAIN_PROVIDED_COMPAT_VERSION) {
            fputs("Note: Option --replace-compat-version has been provided "
                  "multiple times.\nOlder option's compatibility-version will "
                  "be overriden\n",
                  stderr);
        }

        const char *const arg = argv[index];
        const int64_t packed_version = parse_packed_version(arg);

        if (packed_version == -1) {
            fprintf(stderr, "%s is not a valid compatibility-version\n", arg);
            exit(1);
        }

        tbd->info.fields.compatibility_version = (uint32_t)packed_version;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;
        tbd->flags |= F_TBD_FOR_MAIN_PROVIDED_COMPAT_VERSION;
    } else if (strcmp(option, "replace-flags") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a list of flags to replace ones found in the "
                  "provided input file(s)\n",
                  stderr);

            exit(1);
        }

        if (tbd->info.fields.flags != 0) {
            fputs("Note: Option --replace-flags has been provided multiple "
                  "times.\nOlder option's list of flags will be overriden\n",
                  stderr);
        }

        tbd->info.fields.flags = parse_flags_list(index, argc, argv, &index);
        tbd->parse_options |= O_TBD_PARSE_IGNORE_FLAGS;
    } else if (strcmp(option, "replace-objc-constraint") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide an objc-constraint to replace the one found "
                  "in the provided input file(s)\n",
                  stderr);

            exit(1);
        }

        if (tbd->info.fields.objc_constraint != TBD_OBJC_CONSTRAINT_NO_VALUE) {
            fputs("Note: Option --replace-objc-constraint has been provided "
                  "multiple times.\nOlder option's objc-constraint will be "
                  "overriden\n",
                  stderr);
        }

        const char *const argument = argv[index];
        const enum tbd_objc_constraint objc_constraint =
            parse_objc_constraint(argument);

        if (objc_constraint == TBD_OBJC_CONSTRAINT_NO_VALUE) {
            fprintf(stderr,
                    "Unrecognized objc-constraint: %s.\nRun "
                    "--list-objc-constraint to see a list of valid "
                    "objc-constraints\n",
                    argument);

            exit(1);
        }

        tbd->info.fields.objc_constraint = objc_constraint;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
    } else if (strcmp(option, "replace-platform") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a platform to replace the ones found in "
                  "provided input file(s)\n",
                  stderr);

            exit(1);
        }

        if (tbd->info.fields.platform != TBD_PLATFORM_NONE) {
            fputs("Note: Option --replace-platform has been provided multiple "
                  "times.\nOlder option's platform will be overriden\n",
                  stderr);
        }

        const char *const argument = argv[index];
        const enum tbd_platform platform = parse_platform(argument);

        if (platform == TBD_PLATFORM_NONE) {
            fprintf(stderr,
                    "Unrecognized platform: %s.\nRun --list-platform to see a "
                    "list of valid platforms\n",
                    argument);

            exit(1);
        }

        tbd->info.fields.platform = platform;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    } else if (strcmp(option, "replace-swift-version") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a swift-version to replace the ones found in "
                  "provided input file(s)\n",
                  stderr);

            exit(1);
        }

        if (tbd->info.fields.swift_version != 0) {
            fputs("Note: Option --replace-swift-version has been provided "
                  "multiple times.\nOlder option's swift-version will be "
                  "overriden\n",
                  stderr);
        }

        const char *const argument = argv[index];
        const uint32_t swift_version = parse_swift_version(argument);

        if (swift_version == 0) {
            fprintf(stderr, "A swift-version of %s is invalid\n", argument);
            exit(1);
        }

        tbd->info.fields.swift_version = swift_version;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
    } else if (strcmp(option, "skip-invalid-archs") == 0) {
        tbd->macho_options |= O_MACHO_FILE_PARSE_SKIP_INVALID_ARCHITECTURES;
    } else if (strcmp(option, "use-symbol-table") == 0) {
        tbd->macho_options |= O_MACHO_FILE_PARSE_USE_SYMBOL_TABLE;
    } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a .tbd version\nTo get a list of .tbd "
                  "versions, run options --list-tbd-versions\n",
                  stderr);

            exit(1);
        }

        if (tbd->flags & F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION) {
            fprintf(stderr,
                    "Note: Option %s has been provided multiple times.\nOlder "
                    "option's .tbd version will be overriden\n",
                    argv[index - 1]);
        }

        const char *const argument = argv[index];
        const enum tbd_version version = parse_tbd_version(argument);

        if (version == TBD_VERSION_NONE) {
            fprintf(stderr,
                    "Unrecognized .tbd version: %s.\nRun --list-tbd-versions "
                    "to see a list of valid tbd-versions\n",
                    argument);

            exit(1);
        }

        tbd->info.version = version;
        tbd->flags |= F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION;
    } else if (strcmp(option, "v1") == 0) {
        if (tbd->flags & F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION) {
            fputs("Note: Option -v has been provided multiple times.\nOlder "
                  "option's .tbd version will be overriden\n",
                  stderr);
        }

        tbd->info.version = TBD_VERSION_V1;
        tbd->flags |= F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION;
    } else if (strcmp(option, "v2") == 0) {
        if (tbd->flags & F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION) {
            fputs("Note: Option -v has been provided multiple times.\nOlder "
                  "option's .tbd version will be overriden\n",
                  stderr);
        }

        tbd->info.version = TBD_VERSION_V2;
        tbd->flags |= F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION;
    } else if (strcmp(option, "v3") == 0) {
        if (tbd->flags & F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION) {
            fputs("Note: Option -v has been provided multiple times.\nOlder "
                  "option's .tbd version will be overriden\n",
                  stderr);
        }

        tbd->info.version = TBD_VERSION_V3;
        tbd->flags |= F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION;
    } else {
        return false;
    }

    *index_in = index;
    return true;
}

bool
tbd_for_main_has_filetype(const struct tbd_for_main *const tbd,
                          const enum tbd_for_main_filetype filetype)
{
    const uint64_t filetypes = tbd->filetypes;

    /*
     * We support all filetypes if no filetypes were provided by the user.
     */

    if (filetypes == 0) {
        return true;
    }

    return (filetypes & filetype);
}

char *
tbd_for_main_create_write_path(const struct tbd_for_main *const tbd,
                               const char *const file_name,
                               const uint64_t file_name_length,
                               const char *const extension,
                               const uint64_t extension_length,
                               uint64_t *const length_out)
{
    char *const write_path =
        path_append_comp_and_ext(tbd->write_path,
                                 tbd->write_path_length,
                                 file_name,
                                 file_name_length,
                                 extension,
                                 extension_length,
                                 length_out);

    if (write_path == NULL) {
        fputs("Failed to allocate memory\n", stderr);
        exit(1);
    }

    return write_path;
}

char *
tbd_for_main_create_write_path_for_recursing(
    const struct tbd_for_main *__notnull const tbd,
    const char *__notnull const folder_path,
    const uint64_t folder_path_length,
    const char *__notnull const file_name,
    const uint64_t file_name_length,
    const char *__notnull const extension,
    const uint64_t extension_length,
    uint64_t *const length_out)
{
    char *write_path = NULL;
    if (tbd->flags & F_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS) {
        /*
         * The subdirectories are simply the directories following the
         * user-provided recurse-directory.
         *
         * If file_path is related to tbd->parse_path, then we need to get the
         * sub-directories of file_path that are not in tbd->parse_path but are
         * in the hierarchy of file_path.
         */

        const uint64_t parse_path_length = tbd->parse_path_length;
        const uint64_t subdirs_length = folder_path_length - parse_path_length;

        const char *subdirs_iter = folder_path + parse_path_length;
        uint64_t new_file_name_length = file_name_length;

        if (tbd->flags & F_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION) {
            new_file_name_length =
                path_remove_extension(file_name, file_name_length);
        }

        write_path =
            path_append_two_comp_and_ext(tbd->write_path,
                                         tbd->write_path_length,
                                         subdirs_iter,
                                         subdirs_length,
                                         file_name,
                                         new_file_name_length,
                                         extension,
                                         extension_length,
                                         length_out);
    } else {
        write_path =
            path_append_comp_and_ext(tbd->write_path,
                                     tbd->write_path_length,
                                     file_name,
                                     file_name_length,
                                     extension,
                                     extension_length,
                                     length_out);
    }

    if (write_path == NULL) {
        fputs("Failed to allocate memory\n", stderr);
        exit(1);
    }

    return write_path;
}

char *
tbd_for_main_create_dsc_image_write_path(
    const struct tbd_for_main *__notnull const tbd,
    const char *__notnull const write_path,
    const uint64_t write_path_length,
    const char *__notnull const image_path,
    const uint64_t image_path_length,
    const char *__notnull const extension,
    const uint64_t extension_length,
    uint64_t *const length_out)
{
    uint64_t new_image_path_length = image_path_length;
    if (tbd->flags & F_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION) {
        new_image_path_length =
            path_remove_extension(image_path, image_path_length);
    }

    char *const image_write_path =
        path_append_comp_and_ext(write_path,
                                 write_path_length,
                                 image_path,
                                 new_image_path_length,
                                 extension,
                                 extension_length,
                                 length_out);

    if (image_write_path == NULL) {
        fputs("Failed to allocate memory\n", stderr);
        exit(1);
    }

    return image_write_path;
}

char *
tbd_for_main_create_dsc_folder_path(
    const struct tbd_for_main *__notnull const tbd,
    const char *__notnull const folder_path,
    const uint64_t folder_path_length,
    const char *__notnull const file_name,
    const uint64_t file_name_length,
    const char *__notnull const extension,
    const uint64_t extension_length,
    uint64_t *const length_out)
{
    char *write_path = NULL;
    if (tbd->flags & F_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS) {
        /*
         * The subdirectories are simply the directories following the
         * user-provided recurse-directory.
         *
         * Since the folder-path is a a sub-directory of tbd->parse_path, we
         * need to simply add the parse_path's length to get our sub-directories
         * to recreate in our write-path.
         */

        const uint64_t parse_path_length = tbd->parse_path_length;

        const char *subdirs_iter = folder_path + parse_path_length;
        const uint64_t subdirs_length = folder_path_length - parse_path_length;

        write_path =
            path_append_two_comp_and_ext(tbd->write_path,
                                         tbd->write_path_length,
                                         subdirs_iter,
                                         subdirs_length,
                                         file_name,
                                         file_name_length,
                                         extension,
                                         extension_length,
                                         length_out);
    } else {
        write_path =
            path_append_comp_and_ext(tbd->write_path,
                                     tbd->write_path_length,
                                     file_name,
                                     file_name_length,
                                     extension,
                                     extension_length,
                                     length_out);
    }

    if (write_path == NULL) {
        fputs("Failed to allocate memory\n", stderr);
        exit(1);
    }

    return write_path;
}

enum tbd_for_main_open_write_file_result
tbd_for_main_open_write_file_for_path(
    const struct tbd_for_main *__notnull const tbd,
    char *__notnull const path,
    const uint64_t path_length,
    FILE **__notnull const file_out,
    char **__notnull const terminator_out)
{
    char *terminator = NULL;
    const uint64_t tbd_flags = tbd->flags;

    const int flags = (tbd_flags & F_TBD_FOR_MAIN_NO_OVERWRITE) ? O_EXCL : 0;
    const int write_fd =
        open_r(path,
               path_length,
               O_WRONLY | O_TRUNC | flags,
               DEFFILEMODE,
               0755,
               &terminator);

    if (write_fd < 0) {
        /*
         * Although getting the file descriptor failed, its likely open_r still
         * created the directory hierarchy, and if so the terminator shouldn't
         * be NULL.
         */

        if (terminator != NULL) {
            /*
             * Ignore the return value as we cannot be sure if the remove failed
             * as the directories we created (that are pointed to by terminator)
             * may now be populated with other files.
             */

            remove_file_r(path, path_length, terminator);
        }

        if (errno == EEXIST) {
            return E_TBD_FOR_MAIN_OPEN_WRITE_FILE_PATH_ALREADY_EXISTS;
        }

        return E_TBD_FOR_MAIN_OPEN_WRITE_FILE_FAILED;
    }

    FILE *const file = fdopen(write_fd, "w");
    if (file == NULL) {
        return E_TBD_FOR_MAIN_OPEN_WRITE_FILE_FAILED;
    }

    *file_out = file;
    *terminator_out = terminator;

    return E_TBD_FOR_MAIN_OPEN_WRITE_FILE_OK;
}

void
tbd_for_main_write_to_file(const struct tbd_for_main *__notnull const tbd,
                           char *__notnull const write_path,
                           const uint64_t write_path_length,
                           char *const terminator,
                           FILE *__notnull const file,
                           const bool print_paths)
{
    const struct tbd_create_info *const create_info = &tbd->info;
    const enum tbd_create_result create_tbd_result =
        tbd_create_with_info(create_info, file, tbd->write_options);

    if (create_tbd_result != E_TBD_CREATE_OK) {
        if (!(tbd->flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to write to write-file (at path %s)\n",
                        write_path);
            } else {
                fputs("Failed to write to provided write-file\n", stderr);
            }
        }

        if (terminator != NULL) {
            remove_file_r(write_path, write_path_length, terminator);
        }
    }
}

void
tbd_for_main_write_to_stdout(const struct tbd_for_main *__notnull const tbd,
                             const char *__notnull const input_path,
                             const bool print_paths)
{
    const struct tbd_create_info *const create_info = &tbd->info;
    const enum tbd_create_result create_tbd_result =
        tbd_create_with_info(create_info, stdout, tbd->write_options);

    if (create_tbd_result != E_TBD_CREATE_OK) {
        if (!(tbd->flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
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

void
tbd_for_main_write_to_stdout_for_dsc_image(
    const struct tbd_for_main *__notnull const tbd,
    const char *__notnull const dsc_path,
    const char *__notnull const image_path,
    const bool print_paths)
{
    const struct tbd_create_info *const create_info = &tbd->info;
    const enum tbd_create_result create_tbd_result =
        tbd_create_with_info(create_info, stdout, tbd->write_options);

    if (create_tbd_result != E_TBD_CREATE_OK) {
        if (!(tbd->flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to write to stdout (the terminal), for image "
                        "(at path %s) in dyld shared-cache (at path: %s), "
                        "error: %s\n",
                        dsc_path,
                        image_path,
                        strerror(errno));
            } else {
                fprintf(stderr,
                        "Failed to write to stdout (the terminal) for "
                        "image (at path %s) in the provided dyld_shared_cache, "
                        "error: %s\n",
                        image_path,
                        strerror(errno));
            }
        }
    }
}

static int
dsc_image_filter_comparator(const void *__notnull const array_item,
                            const void *__notnull const item)
{
    const struct tbd_for_main_dsc_image_filter *const array_filter =
        (const struct tbd_for_main_dsc_image_filter *)array_item;

    const struct tbd_for_main_dsc_image_filter *const filter =
        (const struct tbd_for_main_dsc_image_filter *)item;

    const enum tbd_for_main_dsc_image_filter_type type = filter->type;
    const enum tbd_for_main_dsc_image_filter_type array_type =
        array_filter->type;

    if (array_type > type) {
        return 1;
    } else if (array_type < type) {
        return -1;
    }

    const uint64_t array_filter_length = array_filter->length;
    const uint64_t filter_length = filter->length;

    const char *const array_string = array_filter->string;
    const char *const string = filter->string;

    if (array_filter_length > filter_length) {
        return memcmp(array_string, string, filter_length + 1);
    } else if (array_filter_length < filter_length) {
        return memcmp(array_string, string, array_filter_length + 1);
    }

    return memcmp(array_string, string, filter_length);
}

static int
dsc_image_number_comparator(const void *__notnull const array_item,
                            const void *__notnull const item)
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

static uint64_t count_paths(const struct array *__notnull const filters) {
    uint64_t count = 0;

    const struct tbd_for_main_dsc_image_filter *filter = filters->data;
    const struct tbd_for_main_dsc_image_filter *const end = filters->data_end;

    for (; filter != end; filter++) {
        if (filter->type == TBD_FOR_MAIN_DSC_IMAGE_FILTER_TYPE_PATH) {
            count++;
        }
    }

    return count;
}

void
tbd_for_main_apply_missing_from(struct tbd_for_main *__notnull const dst,
                                const struct tbd_for_main *__notnull const src)
{
    if (dst->info.fields.archs_count == 0) {
        dst->info.fields.archs = src->info.fields.archs;
        dst->info.fields.archs_count = src->info.fields.archs_count;
    }

    if (!(dst->flags & F_TBD_FOR_MAIN_PROVIDED_CURRENT_VERSION)) {
        dst->info.fields.current_version = src->info.fields.current_version;
    }

    if (!(dst->flags & F_TBD_FOR_MAIN_PROVIDED_COMPAT_VERSION)) {
        const uint32_t comp_version = src->info.fields.compatibility_version;
        dst->info.fields.compatibility_version = comp_version;
    }

    if (dst->info.fields.flags == 0) {
        dst->info.fields.flags = src->info.fields.flags;
    }

    if (dst->info.fields.install_name == NULL) {
        dst->info.fields.install_name = src->info.fields.install_name;
    }

    if (dst->info.fields.parent_umbrella == NULL) {
        dst->info.fields.parent_umbrella = src->info.fields.parent_umbrella;
    }

    if (dst->info.fields.platform == TBD_PLATFORM_NONE) {
        dst->info.fields.platform = src->info.fields.platform;
    }

    if (dst->info.fields.objc_constraint == TBD_OBJC_CONSTRAINT_NO_VALUE) {
        dst->info.fields.objc_constraint = src->info.fields.objc_constraint;
    }

    if (dst->info.fields.swift_version == 0) {
        dst->info.fields.swift_version = src->info.fields.swift_version;
    }

    if (dst->info.version == TBD_VERSION_NONE) {
        dst->info.version = src->info.version;
    }

    if (tbd_for_main_has_filetype(dst, TBD_FOR_MAIN_FILETYPE_DSC)) {
        const struct array *const src_filters = &src->dsc_image_filters;
        const enum array_result add_filters_result =
            array_add_and_unique_items_from_array(
                &dst->dsc_image_filters,
                sizeof(struct tbd_for_main_dsc_image_filter),
                src_filters,
                dsc_image_filter_comparator);

        if (add_filters_result != E_ARRAY_OK) {
            fputs("Experienced an array failure when trying to add dsc "
                  "image-filters\n",
                  stderr);

            exit(1);
        }

        dst->dsc_filter_paths_count = count_paths(&dst->dsc_image_filters);

        const struct array *const src_numbers = &src->dsc_image_numbers;
        const enum array_result add_numbers_result =
            array_add_and_unique_items_from_array(&dst->dsc_image_numbers,
                                                  sizeof(uint32_t),
                                                  src_numbers,
                                                  dsc_image_number_comparator);

        if (add_numbers_result != E_ARRAY_OK) {
            fputs("Experienced an array failure when trying to add dsc "
                  "image-numbers\n",
                  stderr);

            exit(1);
        }
    }

    dst->macho_options |= src->macho_options;
    dst->dsc_options |= src->dsc_options;

    dst->parse_options |= src->parse_options;
    dst->write_options |= src->write_options;

    dst->flags |= src->flags;
    dst->filetypes |= src->filetypes;
    dst->info.flags |= src->info.flags;
}

void tbd_for_main_destroy(struct tbd_for_main *__notnull const tbd) {
    tbd_create_info_destroy(&tbd->info);

    array_destroy(&tbd->dsc_image_filters);
    array_destroy(&tbd->dsc_image_numbers);

    free(tbd->parse_path);
    free(tbd->write_path);

    tbd->parse_path = NULL;
    tbd->write_path = NULL;

    tbd->macho_options = 0;
    tbd->dsc_options = 0;

    tbd->write_options = 0;
    tbd->parse_options = 0;

    tbd->filetypes = 0;
    tbd->flags = 0;
}
