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
    bool parse_all_images;
};

enum dyld_cache_image_info_pad {
    E_DYLD_CACHE_IMAGE_INFO_PAD_ALREADY_EXTRACTED = 1 << 0
};

static bool
path_has_image_entry(const char *const path,
                     const char *const filter,
                     const uint64_t filter_length,
                     const uint64_t options)
{
    bool is_hierarchy = false;
    if (path_has_component(path, filter, filter_length, &is_hierarchy)) {
        if (is_hierarchy) {
            if (options & O_TBD_FOR_MAIN_RECURSE_SKIP_IMAGE_DIRS) {
                return false;
            }
        }

        return true;
    }

    return false;
}

static bool
should_parse_image(const struct array *const filters,
                   const char *const path,
                   const uint64_t options)
{
    struct tbd_for_main_dsc_image_filter *filters_entry = filters->data;
    const struct tbd_for_main_dsc_image_filter *const filters_end =
        filters->data_end;

    for (; filters_entry != filters_end; filters_entry++) {
        const char *const filter = filters_entry->filter;
        const uint64_t length = filters_entry->length;

        if (path_has_image_entry(path, filter, length, options)) {
            return true;
        }
    }

    /*
     * By default, if no filters or indexes are provided, all images are parsed.
     *
     * However, if filters or indexes have been provided, only those images that
     * pass the filter, or are of the index, are parsed.
     */

    return false;
}

static void
clear_create_info(struct tbd_create_info *const info_in,
                  const struct tbd_create_info *const orig)
{
    tbd_create_info_destroy(info_in);
    *info_in = *orig;
}

static void
actually_parse_image(
    struct tbd_for_main *const tbd,
    struct dyld_cache_image_info *const image,
    const char *const image_path,
    const struct dsc_iterate_images_callback_info *const callback_info)
{
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
        return;
    }

    char *const write_path =
        tbd_for_main_create_write_path(tbd,
                                       callback_info->write_path,
                                       callback_info->write_path_length,
                                       image_path,
                                       strlen(image_path),
                                       "tbd",
                                       3,
                                       false, 
                                       NULL);

    if (write_path == NULL) {
        fputs("Failed to allocate memory\n", stderr);
        exit(1);
    }

    tbd_for_main_write_to_path(tbd, image_path, write_path, true);

    clear_create_info(create_info, &original_info);
    free(write_path);

    image->pad |= E_DYLD_CACHE_IMAGE_INFO_PAD_ALREADY_EXTRACTED;
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

    const struct array *const filters = &tbd->dsc_image_filters;
    const uint64_t options = tbd->options;

    if (!callback_info->parse_all_images) {
        if (!should_parse_image(filters, image_path, options)) {
            return true;
        }
    }

    actually_parse_image(tbd, image, image_path, callback_info);
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
                                           true,
                                           &write_path_length); 
    }

    struct dsc_iterate_images_callback_info callback_info = {
        .dsc_info = &dsc_info,
        .dsc_path = path,
        .global = global,
        .tbd = tbd,
        .write_path = write_path,
        .write_path_length = write_path_length,
        .retained_info = retained_info_in,
        .print_paths = print_paths,
        .parse_all_images = true
    };

    /*
     * If indexes have been provided, directly call the iterate_images callback
     * instead of waiting around for the indexes to match up.
     */

    const struct array *const filters = &tbd->dsc_image_filters;
    const struct array *const numbers = &tbd->dsc_image_numbers;

    if (!array_is_empty(numbers)) {
        const uint32_t *numbers_iter = numbers->data; 
        const uint32_t *const numbers_end = numbers->data_end;

        for (; numbers_iter != numbers_end; numbers_iter++) {
            const uint32_t number = *numbers_iter;
            if (number > dsc_info.images_count) {
                if (print_paths) {
                    fprintf(stderr,
                            "An image-number of %d goes beyond the "
                            "images-count of %d the dyld_shared_cache "
                            "(at path %s) has\n",
                            number,
                            dsc_info.images_count,
                            path);
                } else {
                    fprintf(stderr,
                            "An image-number of %d goes beyond the "
                            "images-count of %d the dyld_shared_cache "
                            "at the provided path has\n",
                            number,
                            dsc_info.images_count);
                }

                return false; 
            }

            const uint32_t index = number - 1;
            struct dyld_cache_image_info *const image = &dsc_info.images[index];

            const uint32_t path_file_offset = image->pathFileOffset;
            const char *const path =
                (const char *)(dsc_info.map + path_file_offset);

            actually_parse_image(tbd, image, path, &callback_info);
        } 

        /*
         * If there are no filters, we should simply return after handling the
         * indexes.
         *
         * Note: Since there were indexes, we do not parse all images as we
         * do usually by default.
         */

        if (array_is_empty(filters)) {
            if (is_recursing) {
                free(write_path);
            }

            return true;
        }

        callback_info.parse_all_images = false;
    } else {
        /*
         * By default, if no filters or indexes are provided, we parse all
         * images, otherwise, all images have to be explicitly allowed to be
         & parsed.
         */

        if (!array_is_empty(filters)) {
            callback_info.parse_all_images = false;
        }
    }

    /*
     * Only create the write-path directory at the last-moment to avoid
     * unnecessary mkdir() calls for a shared-cache that may turn up empty.
     */

    const enum dyld_shared_cache_parse_result iterate_images_result =
        dyld_shared_cache_iterate_images_with_callback(
            &dsc_info,
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

struct dsc_list_images_callback {
    uint64_t current_image_index;
};

static bool
dsc_list_images_callback(struct dyld_cache_image_info *const image,
                         const char *const image_path,
                         const void *const item)
{
    struct dsc_list_images_callback *const callback_info =
        (struct dsc_list_images_callback *)item;

    const uint64_t current_image_index = callback_info->current_image_index;

    fprintf(stdout, "\t%ld. %s\n", current_image_index + 1, image_path);
    callback_info->current_image_index = current_image_index + 1;

    return true;
}

void
print_list_of_dsc_images(const int fd,
                         const uint64_t start,
                         const uint64_t end)
{
    const uint64_t size = end - start;

    struct dyld_shared_cache_info dsc_info = {};
    const enum dyld_shared_cache_parse_result parse_dsc_file_result =
        dyld_shared_cache_parse_from_file(&dsc_info, fd, size, 0);

    if (parse_dsc_file_result != E_DYLD_SHARED_CACHE_PARSE_OK) {
        handle_dsc_file_parse_result(NULL, parse_dsc_file_result, false);
        exit(1);
    }

    fprintf(stdout, "There are %d images\n", dsc_info.images_count);

    struct dsc_list_images_callback callback_info = {};
    const enum dyld_shared_cache_parse_result iterate_images_result =
        dyld_shared_cache_iterate_images_with_callback(
            &dsc_info,
            &callback_info,
            dsc_list_images_callback);

    if (iterate_images_result != E_DYLD_SHARED_CACHE_PARSE_OK) {
        handle_dsc_file_parse_result(NULL, parse_dsc_file_result, false);
        exit(1);
    }
}
