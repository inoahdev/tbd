//
//  src/handle_dsc_parse_result.c
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

        case E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE: {
            if (print_paths) {
                fprintf(stderr,
                        "File (at path %s) is not a valid dyld_shared_cache "
                        "file\n",
                        path);
            } else {
                fputs("File at the provided path is not a valid "
                      "dyld_shared_cache file\n",
                      stderr);
            }

            break;
        }

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
                        "dyld_shared_cache file (at path %s) has "
                        "overlapping ranges\n",
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
                        "dyld_shared_cache file (at path %s) has "
                        "overlapping images\n",
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
                        "dyld_shared_cache file (at path %s) has "
                        "overlapping mappings\n",
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

void
handle_dsc_file_parse_result_while_recursing(
    const char *__notnull const dir_path,
    const char *__notnull const name,
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
                        "dyld_shared_cache file (at path %s/%s)\n",
                        dir_path,
                        name);
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
                        "file (at path %s/%s)\n",
                        dir_path,
                        name);
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
                        "(at path %s/%s)\n",
                        dir_path,
                        name);
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
                        "memory, error: %s/%s\n",
                        dir_path,
                        name,
                        strerror(errno));
            } else {
                fprintf(stderr,
                        "Failed to map the provided dyld_shared_cache file to "
                        "memory, error: %s\n",
                        strerror(errno));
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_NOT_A_CACHE: {
            if (print_paths) {
                fprintf(stderr,
                        "File (at path %s/%s) is not a valid dyld_shared_cache "
                        "file\n",
                        dir_path,
                        name);
            } else {
                fputs("File at the provided path is not a valid "
                      "dyld_shared_cache file\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_INVALID_IMAGES: {
            if (print_paths) {
                fprintf(stderr,
                        "dyld_shared_cache file (at path %s/%s) has invalid "
                        "images\n",
                        dir_path,
                        name);
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
                        "dyld_shared_cache file (at path %s/%s) has invalid "
                        "mappigns\n",
                        dir_path,
                        name);
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
                        "dyld_shared_cache file (at path %s/%s) has "
                        "overlapping ranges\n",
                        dir_path,
                        name);
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
                        "dyld_shared_cache file (at path %s/%s) has "
                        "overlapping images\n",
                        dir_path,
                        name);
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
                        "dyld_shared_cache file (at path %s/%s) has "
                        "overlapping mappings\n",
                        dir_path,
                        name);
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
handle_dsc_image_parse_result(
    const struct handle_dsc_image_parse_result_args args)
{
    switch (args.parse_result) {
        case E_DSC_IMAGE_PARSE_OK:
            break;

        case E_DSC_IMAGE_PARSE_INVALID_INSTALL_NAME: {
            const bool request_result =
                request_install_name(args.global,
                                     args.tbd,
                                     args.retained_info_in,
                                     true,
                                     stderr,
                                     "\tImage (with path %s) has an invalid "
                                     "install-name\r\n",
                                     args.image_path);

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_INVALID_PLATFORM: {
            const bool request_result =
                request_platform(args.global,
                                 args.tbd,
                                 args.retained_info_in,
                                 true,
                                 stderr,
                                 "\tImage (with path %s) has an invalid "
                                 "platform\r\n",
                                 args.image_path);

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_INVALID_PARENT_UMBRELLA: {
            const bool request_result =
                request_parent_umbrella(args.global,
                                        args.tbd,
                                        args.retained_info_in,
                                        true,
                                        stderr,
                                        "\tImage (with path %s) has an invalid "
                                        "parent-umbrella\r\n",
                                        args.image_path);

            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_NO_PLATFORM: {
            const bool request_result =
                request_platform(args.global,
                                 args.tbd,
                                 args.retained_info_in,
                                 true,
                                 stderr,
                                 "\tImage (with path %s) doesn't have a "
                                 "platform\r\n",
                                 args.image_path);


            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_CONFLICTING_IDENTIFICATION:
            fprintf(stderr,
                    "\tImage (with path %s) has conflicting information for "
                    "its identification: (install-name, current-version, "
                    "compatibility-version\r\n",
                    args.image_path);

            break;

        case E_DSC_IMAGE_PARSE_CONFLICTING_OBJC_CONSTRAINT: {
            const bool request_result =
                request_objc_constraint(args.global,
                                        args.tbd,
                                        args.retained_info_in,
                                        true,
                                        stderr,
                                        "\tImage (with path %s) has "
                                        "conflicting values for its "
                                        "objc-constraint\r\n",
                                        args.image_path);


            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_CONFLICTING_PARENT_UMBRELLA:{
            const bool request_result =
                request_parent_umbrella(args.global,
                                        args.tbd,
                                        args.retained_info_in,
                                        true,
                                        stderr,
                                        "\tImage (with path %s) has "
                                        "conflicting values for its "
                                        "parent-umbrella\r\n",
                                        args.image_path);


            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_CONFLICTING_PLATFORM: {
            const bool request_result =
                request_platform(args.global,
                                 args.tbd,
                                 args.retained_info_in,
                                 true,
                                 stderr,
                                 "\tImage (with path %s) has conflicting "
                                 "values for its platform\r\n",
                                 args.image_path);


            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_CONFLICTING_SWIFT_VERSION: {
            const bool request_result =
                request_swift_version(args.global,
                                      args.tbd,
                                      args.retained_info_in,
                                      true,
                                      stderr,
                                      "\tImage (with path %s) has conflicting "
                                      "values for its swift-version\r\n",
                                      args.image_path);


            if (!request_result) {
                return false;
            }

            break;
        }

        case E_DSC_IMAGE_PARSE_CONFLICTING_UUID:
            fprintf(stderr,
                    "Image (with path %s) has conflicting values for its "
                    "uuid\r\n",
                    args.image_path);

            return false;

        default:
            return false;
    }

    /*
     * Because the O_MACHO_FILE_IGNORE_INVALID_FIELDS option was passed in from
     * parse_dsc_from_main(), we may have some incomplete, yet mandatory
     * fields.
     */

    if (args.tbd->info.fields.install_name == NULL) {
        const bool request_result =
            request_install_name(args.global,
                                 args.tbd,
                                 args.retained_info_in,
                                 true,
                                 stderr,
                                 "\tImage (with path %s) doesn't have an "
                                 "install-name\r\n",
                                 args.image_path);

        if (!request_result) {
            return false;
        }
    }

    if (args.tbd->info.fields.platform == TBD_PLATFORM_NONE) {
        const bool request_result =
            request_platform(args.global,
                             args.tbd,
                             args.retained_info_in,
                             true,
                             stderr,
                             "\tImage (with path %s) doesn't have a "
                             "platform\r\n",
                             args.image_path);

        if (!request_result) {
            return false;
        }
    }

    return true;
}

void
print_dsc_image_parse_error(const char *__notnull const image_path,
                            const enum dsc_image_parse_result parse_error)
{
    switch (parse_error) {
        case E_DSC_IMAGE_PARSE_OK:
            break;

        case E_DSC_IMAGE_PARSE_NOT_A_MACHO:
            fprintf(stderr,
                    "Image (at path %s) is not a valid mach-o image\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_SEEK_FAIL:
        case E_DSC_IMAGE_PARSE_READ_FAIL:
            fprintf(stderr,
                    "Image (with path %s) could not be parsed due to a read "
                    "failure\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_MAPPING:
            fprintf(stderr,
                    "Image (with path %s) has no corresponding mapping\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED:
            fprintf(stderr,
                    "Image (with path %s) is an unsupported mach-o fat "
                    "image\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL:
            fprintf(stderr,
                    "Image (with path %s) is too small to be a valid mach-o "
                    "image\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_RANGE:
            fprintf(stderr,
                    "Image (with path %s) has an invalid range\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_ALLOC_FAIL:
            fprintf(stderr,
                    "Image (with path %s) could not be parsed due to a memory "
                    "allocation failure\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_ARRAY_FAIL:
            fprintf(stderr,
                    "Image (with path %s) could not be parsed due to an array "
                    "failure\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_LOAD_COMMANDS:
            fprintf(stderr,
                    "Image (with path %s) has no mach-o load-commands\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_TOO_MANY_LOAD_COMMANDS:
            fprintf(stderr,
                    "Image (with path %s) has too many mach-o load-commands "
                    "for its size\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL:
            fprintf(stderr,
                    "Image (with path %s) has a load-commands area too small "
                    "to store all of its mach-o load-commands\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_LOAD_COMMAND:
            fprintf(stderr,
                    "Image (with path %s) has an invalid load-command\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_TOO_MANY_SECTIONS:
            fprintf(stderr,
                    "Image (with path %s) has a mach-o segment with too many "
                    "sections for its size\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_SECTION:
            fprintf(stderr,
                    "Image (with path %s) has a mach-o segment with an invalid "
                    "section\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_CLIENT:
            fprintf(stderr,
                    "Image (with path %s) has an invalid client\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_REEXPORT:
            fprintf(stderr,
                    "Image (with path %s) has an invalid re-export\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_SYMBOL_TABLE:
            fprintf(stderr,
                    "Image (with path %s) has an invalid symbol-table\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_STRING_TABLE:
            fprintf(stderr,
                    "Image (with path %s) has an invalid string-table\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_INVALID_UUID:
            fprintf(stderr,
                    "Image (with path %s) has an invalid uuid\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_IDENTIFICATION:
            fprintf(stderr,
                    "Image (with path %s) has no identification mach-o "
                    "load-command\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_SYMBOL_TABLE:
            fprintf(stderr,
                    "Image (with path %s) has no symbol-table\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_UUID:
            fprintf(stderr,
                    "Image (with path %s) has no uuid\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_EXPORTS: {
            fprintf(stderr,
                    "Image (with path %s) has no exported clients, re-exports, "
                    "or symbols to be written out\r\n",
                    image_path);

            break;
        }

        case E_DSC_IMAGE_PARSE_CONFLICTING_OBJC_CONSTRAINT:
            fprintf(stderr,
                    "Image (with path %s) has conflicting values for its "
                    "objc-constraint\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_CONFLICTING_PARENT_UMBRELLA:
            fprintf(stderr,
                    "Image (with path %s) has conflicting values for its "
                    "parent-umbrella\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_CONFLICTING_PLATFORM:
            fprintf(stderr,
                    "Image (with path %s) has conflicting values for its "
                    "platform\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_CONFLICTING_SWIFT_VERSION:
            fprintf(stderr,
                    "Image (with path %s) has conflicting values for its "
                    "swift-version\r\n",
                    image_path);
            break;

        case E_DSC_IMAGE_PARSE_CONFLICTING_UUID:
            fprintf(stderr,
                    "Image (with path %s) has conflicting values for its "
                    "uuid\r\n",
                    image_path);

            break;

        default:
            break;
    }
}
