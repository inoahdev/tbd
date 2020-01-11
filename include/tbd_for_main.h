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

#include "dsc_image.h"
#include "macho_file.h"
#include "notnull.h"
#include "request_user_input.h"
#include "tbd.h"

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

struct tbd_for_main_flags {
    bool recurse_directories    : 1;
    bool recurse_subdirectories : 1;

    bool replace_path_extension     : 1;
    bool preserve_directory_subdirs : 1;

    bool no_overwrite : 1;
    bool combine_tbds : 1;

    bool no_requests     : 1;
    bool ignore_warnings : 1;

    /*
     * dyld_shared_cache extractions can be stored in a file.
     * (Depending on the configuration)
     */

    bool dsc_write_path_is_file : 1;

    bool provided_archs           : 1;
    bool provided_current_version : 1;
    bool provided_compat_version  : 1;
    bool provided_flags           : 1;
    bool provided_install_name    : 1;
    bool provided_objc_constraint : 1;
    bool provided_platform        : 1;
    bool provided_swift_version   : 1;
    bool provided_targets         : 1;

    bool provided_tbd_version : 1;

    bool provided_ignore_current_version : 1;
    bool provided_ignore_compat_version  : 1;

    bool provided_ignore_flags           : 1;
    bool provided_ignore_objc_constraint : 1;
    bool provided_ignore_swift_version   : 1;

    bool added_filetypes : 1;
};

struct tbd_for_main_filetypes {
    union {
        uint32_t value;
        struct {
            bool macho : 1;
            bool dyld_shared_cache : 1;
            bool user_provided : 1;
        };
    };
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

    struct tbd_for_main_filetypes filetypes;

    /*
     * We store an option-set for each of the filetypes, as we need differing
     * sets for different options.
     */

    struct macho_file_parse_options macho_options;
    struct dyld_shared_cache_parse_options dsc_options;

    struct tbd_parse_options parse_options;
    struct tbd_create_options write_options;

    struct array dsc_image_filters;
    struct array dsc_image_numbers;

    enum tbd_platform platform;
    uint64_t dsc_filter_paths_count;

    struct retained_user_info retained;
    struct tbd_for_main_flags flags;
};

bool
tbd_for_main_parse_option(int *__notnull const index_in,
                          struct tbd_for_main *__notnull tbd,
                          int argc,
                          char *__notnull const *__notnull argv,
                          const char *__notnull option);

void tbd_for_main_handle_post_parse(struct tbd_for_main *__notnull tbd);

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
