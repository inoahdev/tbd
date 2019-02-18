//
//  src/handle_dsc_result.h
//  tbd
//
//  Created by inoahdev on 12/31/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <errno.h>
#include <string.h>

#include "handle_dsc_parse_result.h"
#include "request_user_input.h"

void
handle_dsc_file_parse_result(
    const char *const path,
    const enum dyld_shared_cache_parse_result parse_result,
    const bool print_paths)
{
    switch (parse_result) {
        case E_DYLD_SHARED_CACHE_PARSE_OK:
            break;

        case E_DYLD_SHARED_CACHE_PARSE_ALLOC_FAIL: {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to allocate data while parsing "
                        "dyld_shared_cache file (at path %s)\n",
                        path);
            } else {
                fputs("Failed to allocate data while parsing the provided "
                      "dyld_shared_cache file\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_READ_FAIL: {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to read data while parsing dyld_shared_cache "
                        "file (at path %s)\n",
                        path);
            } else {
                fputs("Failed to read data while parsing the provided "
                      "dyld_shared_cache file\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_FSTAT_FAIL: {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to get information on dyld_shared_cache file "
                        "(at path %s)\n",
                        path);
            } else {
                fputs("Failed to get information on provided dyld_shared_cache "
                      "file\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_MMAP_FAIL: {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to map dyld_shared_cache file (at path %s) to "
                        "memory, error: %s\n",
                        path,
                        strerror(errno));
            } else {
                fprintf(stderr,
                        "Failed to map the provided dyld_shared_cache file to "
                        "memory, error: %s\n",
                        strerror(errno));
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE:
            break;

        case E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES: {
            if (print_paths) {
                fprintf(stderr,
                        "dyld_shared_cache file (at path %s) has invalid "
                        "images\n",
                        path);
            } else {
                fputs("dyld_shared_cache file at the provided path has invalid "
                      "images\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_INVALID_MAPPINGS: {
            if (print_paths) {
                fprintf(stderr,
                        "dyld_shared_cache file (at path %s) has invalid "
                        "mappigns\n",
                        path);
            } else {
                fputs("dyld_shared_cache file at the provided path has invalid "
                      "mappings\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_RANGES: {
            if (print_paths) {
                fprintf(stderr,
                        "dyld_shared_cache file (at path %s) has overlapping "
                        "ranges\n",
                        path);
            } else {
                fputs("dyld_shared_cache file at the provided path has "
                      "overlapping ranges\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_IMAGES: {
            if (print_paths) {
                fprintf(stderr,
                        "dyld_shared_cache file (at path %s) has overlapping "
                        "images\n",
                        path);
            } else {
                fputs("dyld_shared_cache file at the provided path has "
                      "overlapping images\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_OVERLAPPING_MAPPINGS: {
            if (print_paths) {
                fprintf(stderr,
                        "dyld_shared_cache file (at path %s) has overlapping "
                        "mappings\n",
                        path);
            } else {
                fputs("dyld_shared_cache file at the provided path has "
                      "overlapping mappings\n",
                      stderr);
            }

            break;
        }
    }
}

bool
handle_dsc_image_parse_result(uint64_t *const info_in,
                              struct tbd_for_main *const global,
                              struct tbd_for_main *const tbd,
                              const char *const dsc_path,
                              const char *const image_path,
                              const enum dsc_image_parse_result parse_result,
                              const bool print_paths)
{
    switch (parse_result) {
        case E_DSC_IMAGE_PARSE_OK:
            break;

        case E_DSC_IMAGE_PARSE_INVALID_INSTALL_NAME: {
            bool request_result = false;
            if (print_paths) {
                request_result =
                    request_install_name(global,
                                         tbd,
                                         info_in,
                                         stderr,
                                         "dyld_shared_cache file (at path %s) "
                                         "has an image (with path %s) that has "
                                         "an invalid install-name\n",
                                         dsc_path,
                                         image_path);
            } else {
                request_result =
                    request_install_name(global,
                                         tbd,
                                         info_in,
                                         stderr,
                                         "The provided dyld_shared_cache file "
                                         "has an image (with path %s) that has "
                                         "an invalid install-name\n",
                                         image_path);
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_INVALID_PLATFORM: {
            bool request_result = false;
            if (print_paths) {
                request_result =
                    request_install_name(global,
                                         tbd,
                                         info_in,
                                         stderr,
                                         "dyld_shared_cache file (at path %s) "
                                         "has an image (with path %s) that has "
                                         "an invalid platform\n",
                                         dsc_path,
                                         image_path);
            } else {
                request_result =
                    request_install_name(global,
                                         tbd,
                                         info_in,
                                         stderr,
                                         "The provided dyld_shared_cache file "
                                         "has an image (with path %s) that has "
                                         "an invalid platform\n",
                                         image_path);
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_INVALID_PARENT_UMBRELLA: {
            bool request_result = false;
            if (print_paths) {
                request_result =
                    request_parent_umbrella(global,
                                            tbd,
                                            info_in,
                                            stderr,
                                            "dyld_shared_cache file (at "
                                            "path %s) has an image (with "
                                            "path %s) that has an invalid "
                                            "parent-umbrella\n",
                                            dsc_path,
                                            image_path);
            } else {
                request_result =
                    request_parent_umbrella(global,
                                            tbd,
                                            info_in,
                                            stderr,
                                            "The provided dyld_shared_cache "
                                            "file has an image (with path %s) "
                                            "that has an invalid "
                                            "parent-umbrella\n",
                                            image_path);
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_NO_PLATFORM: {
            bool request_result = false;
            if (print_paths) {
                request_result =
                    request_platform(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "dyld_shared_cache file (at path %s) has "
                                     "an image (with path %s) that doesn't "
                                     "have a platform\n",
                                     dsc_path,
                                     image_path);
            } else {
                request_result =
                    request_platform(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "The provided dyld_shared_cache file has "
                                     "an image (with path %s) that doesn't "
                                     "have a platform\n",
                                     image_path);
            }

            if (!request_result) {
                return false;
            }

            break;
        }

        default:
            return false;
    }

    /*
     * Handle the remove/replace fields.
     */

    const uint64_t archs_re = tbd->archs_re;
    if (archs_re != 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            tbd->info.archs &= ~archs_re;
        } else {
            tbd->info.archs = archs_re;
        }
    }

    const uint32_t flags_re = tbd->flags_re;
    if (flags_re != 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            tbd->info.flags_field &= ~flags_re;
         } else {
            tbd->info.flags_field = flags_re;
        }
    }

    /*
     * If some of the fields are empty, but their respective error-codes weren't
     * returned (for when O_MACHO_PARSE_IGNORE_INVALID_FIELDS is provided),
     * We instead check here and request info from the user.
     *
     * Ignore objc-constraint, swift-version, and parent-umbrella as they aren't
     * mandatory fields and therefore aren't always provided.
     */

    if (tbd->info.install_name == NULL) {
        bool request_result = false;
        if (print_paths) {
            request_result =
                request_install_name(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "dyld_shared_cache file (at path %s) has "
                                     "an image (with path %s) that doesn't "
                                     "have an install-name\n",
                                     dsc_path,
                                     image_path);
        } else {
            request_result =
                request_install_name(global,
                                     tbd,
                                     info_in,
                                     stderr,
                                     "The provided dyld_shared_cache file has "
                                     "an image (with path %s) that doesn't "
                                     "have an install-name\n",
                                     image_path);
        }

        if (!request_result) {
            return false;
        }
    }

    if (tbd->info.platform == 0) {
        bool request_result = false;
        if (print_paths) {
            request_result =
                request_platform(global,
                                 tbd,
                                 info_in,
                                 stderr,
                                 "dyld_shared_cache file (at path %s) has an "
                                 "image (with path %s) that doesn't have a "
                                 "platform\n",
                                 dsc_path,
                                 image_path);
        } else {
            request_result =
                request_platform(global,
                                 tbd,
                                 info_in,
                                 stderr,
                                 "The provided dyld_shared_cache file has an "
                                 "image (with path %s) that doesn't have a "
                                 "platform\n",
                                 image_path);
        }

        if (!request_result) {
            return false;
        }
    }

    return true;
}

void
print_dsc_image_parse_error(struct tbd_for_main *const tbd,
                            const char *const image_path,
                            const enum dsc_image_parse_result parse_error)
{
    switch (parse_error) {
        case E_DSC_IMAGE_PARSE_OK:
            break;

        case E_DSC_IMAGE_PARSE_NOT_A_MACHO:
            fprintf(stderr,
                    "Image (at path %s) is not a valid mach-o image\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_SEEK_FAIL:
            fprintf(stderr,
                    "Image (with path %s) could not be parsed due to a seek "
                    "failure\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_READ_FAIL:
            fprintf(stderr,
                    "Image (with path %s) could not be parsed due to a read "
                    "failure\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_CORRESPONDING_MAPPING:
            fprintf(stderr,
                    "Image (with path %s) has no corresponding mapping\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED:
            fprintf(stderr,
                    "Image (with path %s) is an unsupported mach-o fat image\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL:
            fprintf(stderr,
                    "Image (with path %s) is too small to be a valid mach-o "
                    "image\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_RANGE:
            fprintf(stderr,
                    "Image (with path %s) has an invalid range\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_ALLOC_FAIL:
            fprintf(stderr,
                    "Image (with path %s) could not be parsed due to an "
                    "allocation failure\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_ARRAY_FAIL:
            fprintf(stderr,
                    "Image (with path %s) could not be parsed due to an array "
                    "failure\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_LOAD_COMMANDS:
            fprintf(stderr,
                    "Image (with path %s) has no mach-o load-commands\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_TOO_MANY_LOAD_COMMANDS:
            fprintf(stderr,
                    "Image (with path %s) has too many mach-o load-commands "
                    "for its size\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL:
            fprintf(stderr,
                    "Image (with path %s) has a load-commands area too small "
                    "to store all of its mach-o load-commands\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_LOAD_COMMAND:
            fprintf(stderr,
                    "Image (with path %s) has an invalid load-command\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_TOO_MANY_SECTIONS:
            fprintf(stderr,
                    "Image (with path %s) has a mach-o segment with too many "
                    "sections for its size\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_SECTION:
            fprintf(stderr,
                    "Image (with path %s) has a mach-o segment with an invalid "
                    "section\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_CLIENT:
            fprintf(stderr,
                    "Image (with path %s) has an invalid client\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_REEXPORT:
            fprintf(stderr,
                    "Image (with path %s) has an invalid re-export\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_SYMBOL_TABLE:
            fprintf(stderr,
                    "Image (with path %s) has an invalid symbol-table\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_STRING_TABLE:
            fprintf(stderr,
                    "Image (with path %s) has an invalid string-table\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_UUID:
            fprintf(stderr,
                    "Image (with path %s) has an invalid uuid\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_IDENTIFICATION:
            fprintf(stderr,
                    "Image (with path %s) has no identification mach-o "
                    "load-command\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_SYMBOL_TABLE:
            fprintf(stderr,
                    "Image (with path %s) has no symbol-table\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_UUID:
            fprintf(stderr,
                    "Image (with path %s) has no uuid\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_EXPORTS: {
            const uint64_t options = tbd->options;
            if (options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                if (!(options & O_TBD_FOR_MAIN_IGNORE_WARNINGS)) {
                    fprintf(stderr,
                            "Image (with path %s) has no clients, re-exports, "
                            "or symbols to be written out\n",
                            image_path);
                }
            } else {
                fprintf(stderr,
                        "Image (with path %s) has no clients, re-exports, "
                        "or symbols to be written out\n",
                        image_path);
            }

            break;
        }

        default:
            break;
    }
}