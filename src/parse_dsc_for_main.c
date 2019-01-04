//
//  src/parse_dsc_for_main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "handle_dsc_parse_result.h"
#include "parse_dsc_for_main.h"

#include "macho_file.h"
#include "path.h"
#include "recursive.h"

struct dsc_iterate_images_callback_info {
    struct dyld_shared_cache_info *dsc_info;
    const char *dsc_path;

    struct tbd_for_main *global;
    struct tbd_for_main *tbd;

    struct array images;
    char *write_path;

    uint64_t write_path_length; 
    uint64_t *retained_info;

    bool print_paths;
};

enum dyld_cache_image_info_pad {
    E_DYLD_CACHE_IMAGE_INFO_PAD_ALREADY_EXTRACTED = 1 << 0
};

static bool
path_has_image_entry(const struct array *const images,
                     const char *const path,
                     const uint64_t options)
{
    struct tbd_for_main_image *images_entry = images->data;
    const struct tbd_for_main_image *const images_end = images->data_end;

    for (; images_entry != images_end; images_entry++) {
        const char *const name = images_entry->name;
        const uint64_t name_length = images_entry->name_length;

        bool is_hierarchy = false;
        if (path_has_component(path, name, name_length, &is_hierarchy)) {
            if (is_hierarchy) {
                if (options & O_TBD_FOR_MAIN_RECURSE_SKIP_IMAGE_DIRS) {
                    return false;
                }
            }

            return true;
        }
    }

    return false;
}

static void
clear_create_info(struct tbd_create_info *const info_in,
                  const struct tbd_create_info *const orig)
{
    tbd_create_info_destroy(info_in);
    *info_in = *orig;
}

static bool
dsc_iterate_images_callback(struct dyld_cache_image_info *const image,
                            const char *const image_path,
                            const void *const item)
{
    if (image->pad & E_DYLD_CACHE_IMAGE_INFO_PAD_ALREADY_EXTRACTED) {
        return true;
    }

    struct dsc_iterate_images_callback_info *const callback_info =
        (struct dsc_iterate_images_callback_info *)item;

    struct tbd_for_main *const tbd = callback_info->tbd;

    /*
     * Skip any dyld_shared_cache images if we haven't been prompted to accept
     * them. We extract all the images in the dyld_shared_cache if none specific
     * have been provided.
     */

    const struct array *const images = &tbd->images;
    if (!array_is_empty(images)) {
        if (!path_has_image_entry(images, image_path, tbd->options)) {
            return true;
        }
    }

    struct tbd_create_info *const create_info = &callback_info->tbd->info;
    const struct tbd_create_info original_info = *create_info;

    const uint64_t macho_options =
        O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS | tbd->macho_options;

    const enum dsc_image_parse_result parse_image_result =
        dsc_image_parse(create_info,
                        callback_info->dsc_info,
                        image,
                        macho_options,
                        tbd->dsc_options,
                        0);

    const bool should_continue =
        handle_dsc_image_parse_result(callback_info->global,
                                      callback_info->tbd,
                                      callback_info->dsc_path,
                                      image_path,
                                      parse_image_result,
                                      callback_info->print_paths,
                                      callback_info->retained_info); 

    if (!should_continue) {
        clear_create_info(create_info, &original_info);
        return true;
    }

    char *const write_path =
        tbd_for_main_create_write_path(tbd,
                                       callback_info->write_path,
                                       callback_info->write_path_length,
                                       image_path,
                                       strlen(image_path),
                                       "tbd",
                                       3,
                                       false); 

    if (write_path == NULL) {
        fputs("Failed to allocate memory\n", stderr);
        exit(1);
    }

    tbd_for_main_write_to_path(tbd, image_path, write_path, true);
    clear_create_info(create_info, &original_info);
    
    free(write_path);
    image->pad |= E_DYLD_CACHE_IMAGE_INFO_PAD_ALREADY_EXTRACTED;
    
    return true;
}

bool 
parse_shared_cache(struct tbd_for_main *const global,
                   struct tbd_for_main *const tbd,
                   const char *const path,
                   const uint64_t path_length,
                   const int fd,
                   const uint64_t size,
                   const bool is_recursing,
                   const bool print_paths,
                   uint64_t *const retained_info_in)
{
    const uint64_t dsc_options =
        O_DYLD_SHARED_CACHE_PARSE_ZERO_IMAGE_PADS | tbd->dsc_options;

    struct dyld_shared_cache_info dsc_info = {};
    const enum dyld_shared_cache_parse_result parse_dsc_file_result =
        dyld_shared_cache_parse_from_file(&dsc_info,
                                          fd,
                                          size,
                                          dsc_options);

    if (parse_dsc_file_result == E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE) {
        return false;
    }

    if (parse_dsc_file_result != E_DYLD_SHARED_CACHE_PARSE_OK) {
        handle_dsc_file_parse_result(path, parse_dsc_file_result, print_paths);
        return true;
    }

    char *write_path = tbd->write_path;
    uint64_t write_path_length = tbd->write_path_length;

    /*
     * dyld_shared_cache tbds are always stored in a separate directory when
     * recursing.
     *
     * When recursing, the name of the directory is comprised of the file-name
     * of the dyld_shared_cache, followed by the extension '.tbds'.
     */

    if (is_recursing) {
        write_path =
            tbd_for_main_create_write_path(tbd,
                                           write_path,
                                           write_path_length,
                                           path,
                                           path_length,
                                           "tbds",
                                           4,
                                           true); 

        write_path_length = strlen(write_path);
    }

    /*
     * Only create the write-path directory at the last-moment to avoid
     * unnecessary mkdir() calls for a shared-cache that may turn up empty.
     */

    struct dsc_iterate_images_callback_info callback_info = {
        .dsc_info = &dsc_info,
        .dsc_path = path,
        .global = global,
        .tbd = tbd,
        .write_path = write_path,
        .write_path_length = write_path_length,
        .print_paths = print_paths,
        .retained_info = retained_info_in
    };

    const enum dyld_shared_cache_parse_result iterate_images_result =
        dyld_shared_cache_iterate_images_with_callback(
            &dsc_info,
            fd,
            0,
            &callback_info,
            dsc_iterate_images_callback);

    if (is_recursing) {
        free(write_path);
    }    

    if (iterate_images_result != E_DYLD_SHARED_CACHE_PARSE_OK) {
        handle_dsc_file_parse_result(path, iterate_images_result, print_paths);
    }

    return true;
}
