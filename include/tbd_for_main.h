//
//  src/tbd_for_main.h
//  tbd
//
//  Created by inoahdev on 11/30/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef TBD_FOR_MAIN_H
#define TBD_FOR_MAIN_H

#include <stdint.h>
#include "tbd.h"

enum tbd_for_main_dsc_image_flags {
    F_TBD_FOR_MAIN_DSC_IMAGE_FOUND_ONE         = 1 << 0,
    F_TBD_FOR_MAIN_DSC_IMAGE_CURRENTLY_PARSING = 1 << 1
};

enum tbd_for_main_dsc_image_filter_type {
    TBD_FOR_MAIN_DSC_IMAGE_FILTER_TYPE_FILE,
    TBD_FOR_MAIN_DSC_IMAGE_FILTER_TYPE_DIRECTORY
};

struct tbd_for_main_dsc_image_filter {
    /*
     * tmp_ptr is used to store a pointer to the path-component needed when
     * creating the right write-path.
     */

    const char *string;
    const char *tmp_ptr;

    enum tbd_for_main_dsc_image_filter_type type;

    uint64_t length;
    uint64_t flags;
};

struct tbd_for_main_dsc_image_path {
    const char *string;

    uint64_t length;
    uint64_t flags;
};

enum tbd_for_main_flags {
    F_TBD_FOR_MAIN_RECURSE_DIRECTORIES    = 1 << 0,
    F_TBD_FOR_MAIN_RECURSE_SUBDIRECTORIES = 1 << 1,

    F_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS = 1 << 2,
    F_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS = 1 << 3,

    F_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS = 1 << 5,

    F_TBD_FOR_MAIN_NO_OVERWRITE           = 1 << 6,
    F_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION = 1 << 7,

    F_TBD_FOR_MAIN_IGNORE_WARNINGS = 1 << 9,
    F_TBD_FOR_MAIN_NO_REQUESTS     = 1 << 10,

    /*
     * dyld_shared_cache extractions can be stored in either a file or a
     * directory. (Depending on the configuration)
     */

    F_TBD_FOR_MAIN_DSC_WRITE_PATH_IS_FILE = 1 << 11
};

enum tbd_for_main_filetype {
    TBD_FOR_MAIN_FILETYPE_MACHO = 1 << 0,
    TBD_FOR_MAIN_FILETYPE_DSC   = 1 << 1
};

struct tbd_for_main {
    struct tbd_create_info info;

    char *parse_path;
    char *write_path;

    /*
     * Store the paths' length here to avoid multiple strlen() calls on
     * the paths in main's dir_recurse callback.
     */

    uint64_t parse_path_length;
    uint64_t write_path_length;

    uint64_t filetypes;
    uint64_t filetypes_count;

    /*
     * We store an option-set for each of the filetypes, as we need differing
     * sets for different options.
     */

    uint64_t macho_options;
    uint64_t dsc_options;

    uint64_t parse_options;
    uint64_t write_options;

    uint64_t flags;

    /*
     * Archs and flags to either replace/remove ones found in a mach-o file.
     * Default is to replace.
     */

    uint64_t archs_re;
    uint32_t flags_re;

    struct array dsc_image_filters;
    struct array dsc_image_numbers;
    struct array dsc_image_paths;
};

bool
tbd_for_main_has_filetype(const struct tbd_for_main *tbd,
                          enum tbd_for_main_filetype filetype);

bool
tbd_for_main_parse_option(struct tbd_for_main *tbd,
                          int argc,
                          const char *const *argv,
                          const char *option,
                          int *index);

char *
tbd_for_main_create_write_path(const struct tbd_for_main *tbd,
                               const char *file_name,
                               uint64_t file_name_length,
                               const char *extension,
                               uint64_t extension_length,
                               uint64_t *length_out);

char *
tbd_for_main_create_write_path_while_recursing(const struct tbd_for_main *tbd,
                                               const char *folder_path,
                                               uint64_t folder_path_length,
                                               const char *file_name,
                                               uint64_t file_name_length,
                                               const char *extension,
                                               uint64_t extension_length,
                                               uint64_t *length_out);

char *
tbd_for_main_create_dsc_folder_path(const struct tbd_for_main *tbd,
                                    const char *folder_path,
                                    uint64_t folder_path_length,
                                    const char *file_path,
                                    uint64_t file_path_length,
                                    const char *extension,
                                    uint64_t extension_length,
                                    uint64_t *length_out);

char *
tbd_for_main_create_dsc_image_write_path(const struct tbd_for_main *tbd,
                                         const char *write_path,
                                         uint64_t write_path_length,
                                         const char *image_path,
                                         uint64_t image_path_length,
                                         const char *extension,
                                         uint64_t extension_length,
                                         uint64_t *length_out);

void
tbd_for_main_apply_from(struct tbd_for_main *dst,
                        const struct tbd_for_main *src);

enum tbd_for_main_write_to_path_result {
    E_TBD_FOR_MAIN_WRITE_TO_PATH_OK,

    E_TBD_FOR_MAIN_WRITE_TO_PATH_ALREADY_EXISTS,
    E_TBD_FOR_MAIN_WRITE_TO_PATH_WRITE_FAIL
};

enum tbd_for_main_write_to_path_result
tbd_for_main_write_to_path(const struct tbd_for_main *tbd,
                           char *write_path,
                           uint64_t write_path_length,
                           bool print_paths);

void
tbd_for_main_write_to_stdout(const struct tbd_for_main *tbd,
                             const char *input_path,
                             bool print_paths);

void
tbd_for_main_write_to_stdout_for_dsc_image(const struct tbd_for_main *tbd,
                                           const char *dsc_path,
                                           const char *image_path,
                                           bool print_paths);

void tbd_for_main_destroy(struct tbd_for_main *tbd);

#endif /* TBD_FOR_MAIN_H */
