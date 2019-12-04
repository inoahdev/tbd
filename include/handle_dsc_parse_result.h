//
//  include/handle_dsc_parse_result.h
//  tbd
//
//  Created by inoahdev on 12/31/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef HANDLE_DSC_PARSE_RESULT_H
#define HANDLE_DSC_PARSE_RESULT_H

#include "dsc_image.h"
#include "dyld_shared_cache.h"

#include "macho_file.h"
#include "notnull.h"
#include "tbd_for_main.h"

void
handle_dsc_file_parse_result(const char *path,
                             enum dyld_shared_cache_parse_result parse_result,
                             bool print_paths);
void
handle_dsc_file_parse_result_while_recursing(
    const char *__notnull dir_path,
    const char *__notnull name,
    enum dyld_shared_cache_parse_result parse_result);

struct handle_dsc_image_parse_error_cb_info {
    uint64_t *retained_info_in;

    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    const char *dsc_dir_path;
    const char *dsc_name;
    const char *image_path;

    bool print_paths;
    bool did_print_messages_header;
};

void
print_dsc_image_parse_error_message_header(bool print_paths,
                                           const char *__notnull dsc_dir_path,
                                           const char *dsc_name);

bool
handle_dsc_image_parse_error_callback(struct tbd_create_info *__notnull info_in,
                                      enum macho_file_parse_error error,
                                      void *callback_info);

struct handle_dsc_image_parse_result_args {
    uint64_t *retained_info_in;

    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    const char *dsc_dir_path;
    const char *dsc_name;
    const char *image_path;

    enum dsc_image_parse_result parse_result;
};

/*
 * full_path should be stored in args.dsc_dir_path.
 */

bool
handle_dsc_image_parse_result_while_recursing(
	struct handle_dsc_image_parse_result_args args);

void
print_dsc_image_parse_error(const char *__notnull image_path,
                            enum dsc_image_parse_result parse_error,
                            bool indent);

#endif /* HANDLE_DSC_PARSE_RESULT_H */

