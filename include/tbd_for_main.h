//
//  include/tbd_for_main.h
//  tbd
//
//  Created by inoahdev on 11/30/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef TBD_FOR_MAIN_H
#define TBD_FOR_MAIN_H

#include <stdint.h>

#include "notnull.h"
#include "tbd.h"

enum tbd_for_main_dsc_image_flags {
    F_TBD_FOR_MAIN_DSC_IMAGE_CURRENTLY_PARSING = 1ull << 0,
};

enum tbd_for_main_dsc_image_filter_type {
    TBD_FOR_MAIN_DSC_IMAGE_FILTER_TYPE_FILE,
    TBD_FOR_MAIN_DSC_IMAGE_FILTER_TYPE_DIRECTORY,
    TBD_FOR_MAIN_DSC_IMAGE_FILTER_TYPE_PATH
};

enum tbd_for_main_dsc_image_filter_parse_status {
    /*
     * Not Found signifies no images that passed the filter were found.
     *
     * Happening signifies that an image passing the filter was found, and is
     * currently being parsed.
     *
     * Found signifies at least one image that passed the filter was found, but
     * not successfully parsed.
     *
     * Ok signifies that at least one image passing the filter was found and
     * was successfully parsed.
     */

    TBD_FOR_MAIN_DSC_IMAGE_FILTER_PARSE_NOT_FOUND,
    TBD_FOR_MAIN_DSC_IMAGE_FILTER_PARSE_HAPPENING,
    TBD_FOR_MAIN_DSC_IMAGE_FILTER_PARSE_FOUND,
    TBD_FOR_MAIN_DSC_IMAGE_FILTER_PARSE_OK,
};

struct tbd_for_main_dsc_image_filter {
    /*
     * tmp_ptr is used to store a pointer to the path-component needed when
     * creating the write-path.
     */

    const char *string;
    const char *tmp_ptr;

    uint64_t length;

    enum tbd_for_main_dsc_image_filter_type type;
    enum tbd_for_main_dsc_image_filter_parse_status status;
};

enum tbd_for_main_flags {
    F_TBD_FOR_MAIN_RECURSE_DIRECTORIES    = 1ull << 0,
    F_TBD_FOR_MAIN_RECURSE_SUBDIRECTORIES = 1ull << 1,

    F_TBD_FOR_MAIN_REPLACE_PATH_EXTENSION     = 1ull << 2,
    F_TBD_FOR_MAIN_PRESERVE_DIRECTORY_SUBDIRS = 1ull << 3,

    F_TBD_FOR_MAIN_REMOVE_ARCHS = 1ull << 4,
    F_TBD_FOR_MAIN_NO_OVERWRITE = 1ull << 5,
    F_TBD_FOR_MAIN_COMBINE_TBDS = 1ull << 6,

    F_TBD_FOR_MAIN_NO_REQUESTS     = 1ull << 7,
    F_TBD_FOR_MAIN_IGNORE_WARNINGS = 1ull << 8,

    /*
     * dyld_shared_cache extractions can be stored in either a file.
     * (Depending on the configuration)
     */

    F_TBD_FOR_MAIN_DSC_WRITE_PATH_IS_FILE = 1ull << 9,

    F_TBD_FOR_MAIN_PROVIDED_CURRENT_VERSION = 1ull << 10,
    F_TBD_FOR_MAIN_PROVIDED_COMPAT_VERSION  = 1ull << 11,

    F_TBD_FOR_MAIN_PROVIDED_TBD_VERSION = 1ull << 12
};

enum tbd_for_main_filetype {
    TBD_FOR_MAIN_FILETYPE_MACHO = 1ull << 0,
    TBD_FOR_MAIN_FILETYPE_DSC   = 1ull << 1
};

struct tbd_for_main {
    struct tbd_create_info info;

    char *parse_path;
    char *write_path;

    /*
     * Store the paths' length here to avoid multiple strlen() calls on the
     * paths in main's dir_recurse callback.
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

    struct array dsc_image_filters;
    struct array dsc_image_numbers;

    uint64_t dsc_filter_paths_count;
    uint64_t flags;
};

bool
tbd_for_main_has_filetype(const struct tbd_for_main *__notnull tbd,
                          enum tbd_for_main_filetype filetype);

bool
tbd_for_main_parse_option(int *__notnull const index_in,
                          struct tbd_for_main *__notnull tbd,
                          int argc,
                          const char *__notnull const *__notnull argv,
                          const char *__notnull option);

char *__notnull
tbd_for_main_create_write_path(const struct tbd_for_main *__notnull tbd,
                               const char *__notnull file_name,
                               uint64_t file_name_length,
                               const char *__notnull extension,
                               uint64_t extension_length,
                               uint64_t *length_out);

char *__notnull
tbd_for_main_create_write_path_for_recursing(
    const struct tbd_for_main *__notnull tbd,
    const char *__notnull folder_path,
    uint64_t folder_path_length,
    const char *__notnull file_name,
    uint64_t file_name_length,
    const char *__notnull extension,
    uint64_t extension_length,
    uint64_t *length_out);

char *__notnull
tbd_for_main_create_dsc_folder_path(const struct tbd_for_main *__notnull tbd,
                                    const char *__notnull folder_path,
                                    uint64_t folder_path_length,
                                    const char *__notnull file_path,
                                    uint64_t file_path_length,
                                    const char *__notnull extension,
                                    uint64_t extension_length,
                                    uint64_t *length_out);

char *__notnull
tbd_for_main_create_dsc_image_write_path(
    const struct tbd_for_main *__notnull tbd,
    const char *__notnull write_path,
    uint64_t write_path_length,
    const char *__notnull image_path,
    uint64_t image_path_length,
    const char *__notnull extension,
    uint64_t extension_length,
    uint64_t *length_out);

void
tbd_for_main_apply_missing_from(struct tbd_for_main *__notnull dst,
                                const struct tbd_for_main *__notnull src);

enum tbd_for_main_open_write_file_result {
    E_TBD_FOR_MAIN_OPEN_WRITE_FILE_OK,
    E_TBD_FOR_MAIN_OPEN_WRITE_FILE_FAILED,
    E_TBD_FOR_MAIN_OPEN_WRITE_FILE_PATH_ALREADY_EXISTS,
};

enum tbd_for_main_open_write_file_result
tbd_for_main_open_write_file_for_path(const struct tbd_for_main *__notnull tbd,
                                      char *__notnull path,
                                      uint64_t path_length,
                                      FILE **__notnull file_out,
                                      char **__notnull terminator_out);

void
tbd_for_main_write_to_file(const struct tbd_for_main *__notnull tbd,
                           char *__notnull write_path,
                           uint64_t write_path_length,
                           char *terminator,
                           FILE *__notnull file,
                           bool print_paths);

void
tbd_for_main_write_to_stdout(const struct tbd_for_main *__notnull tbd,
                             const char *__notnull input_path,
                             bool print_paths);

void
tbd_for_main_write_to_stdout_for_dsc_image(
    const struct tbd_for_main *__notnull tbd,
    const char *__notnull dsc_path,
    const char *__notnull image_path,
    bool print_paths);

void tbd_for_main_destroy(struct tbd_for_main *__notnull tbd);

#endif /* TBD_FOR_MAIN_H */
