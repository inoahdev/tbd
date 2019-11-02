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
handle_dsc_image_parse_error_callback(
    struct tbd_create_info *__unused __notnull const info_in,
    const enum macho_file_parse_error error,
    void *const callback_info)
{
    const struct handle_dsc_image_parse_error_cb_info *const cb_info =
        (const struct handle_dsc_image_parse_error_cb_info *)callback_info;

    bool request_result = true;
    switch (error) {
        case ERR_MACHO_FILE_PARSE_CURRENT_VERSION_CONFLICT:
            request_result =
                request_current_version(cb_info->global,
                                        cb_info->tbd,
                                        cb_info->retained_info_in,
                                        true,
                                        stderr,
                                        "\tImage (with path %s) has multiple "
                                        "current-versions that conflict with "
                                        "one another\r\n",
                                        cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_COMPAT_VERSION_CONFLICT:
            request_result =
                request_compat_version(cb_info->global,
                                       cb_info->tbd,
                                       cb_info->retained_info_in,
                                       true,
                                       stderr,
                                       "\tImage (with path %s) has multiple "
                                       "compatibility-versions that conflict "
                                       "with one another\r\n",
                                       cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_FLAGS_CONFLICT:
            request_result =
                request_if_should_ignore_flags(cb_info->global,
                                               cb_info->tbd,
                                               cb_info->retained_info_in,
                                               true,
                                               stderr,
                                               "\tImage (with path %s) has "
                                               "multiple, differing set of "
                                               "flags that conflict with one "
                                               "another\r\n",
                                               cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INSTALL_NAME_CONFLICT:
            request_result =
                request_install_name(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     true,
                                     stderr,
                                     "\tImage (with path %s) has multiple "
                                     "install-names that conflict with one "
                                     "another\r\n",
                                     cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_OBJC_CONSTRAINT_CONFLICT:
            request_result =
                request_objc_constraint(cb_info->global,
                                        cb_info->tbd,
                                        cb_info->retained_info_in,
                                        true,
                                        stderr,
                                        "\tImage (with path %s) has multiple "
                                        "objc-constraints that conflict with "
                                        "one another\r\n",
                                        cb_info->image_path);


            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_PARENT_UMBRELLA_CONFLICT:
            request_result =
                request_parent_umbrella(cb_info->global,
                                        cb_info->tbd,
                                        cb_info->retained_info_in,
                                        true,
                                        stderr,
                                        "\tImage (with path %s) has multiple "
                                        "parent-umbrellas that conflict with "
                                        "one another\r\n",
                                        cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_PLATFORM_CONFLICT:
            request_result =
                request_platform(cb_info->global,
                                 cb_info->tbd,
                                 cb_info->retained_info_in,
                                 true,
                                 stderr,
                                 "\tImage (with path %s) has multiple "
                                 "platforms that conflict with one another\r\n",
                                 cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_SWIFT_VERSION_CONFLICT:
            request_result =
                request_swift_version(cb_info->global,
                                      cb_info->tbd,
                                      cb_info->retained_info_in,
                                      true,
                                      stderr,
                                      "\tImage (with path %s) has multiple "
                                      "swift-versions that conflict with one "
                                      "another\r\n",
                                      cb_info->image_path);


            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_UUID_CONFLICT:
            fprintf(stderr,
                    "\tImage (with path %s) has multiple uuids that conflict "
                    "with one another\r\n",
                    cb_info->image_path);

            return false;

        case ERR_MACHO_FILE_PARSE_INVALID_INSTALL_NAME:
            request_result =
                request_install_name(cb_info->global,
                                     cb_info->tbd,
                                     cb_info->retained_info_in,
                                     true,
                                     stderr,
                                     "\tImage (with path %s) has an invalid "
                                     "install-name\r\n",
                                     cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INVALID_PARENT_UMBRELLA:
            request_result =
                request_parent_umbrella(cb_info->global,
                                        cb_info->tbd,
                                        cb_info->retained_info_in,
                                        true,
                                        stderr,
                                        "\tImage (with path %s) has an invalid "
                                        "parent-umbrella\r\n",
                                        cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INVALID_PLATFORM:
            request_result =
                request_platform(cb_info->global,
                                 cb_info->tbd,
                                 cb_info->retained_info_in,
                                 true,
                                 stderr,
                                 "\tImage (with path %s) has an invalid "
                                 "platform\r\n",
                                 cb_info->image_path);

            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_INVALID_UUID:
            fprintf(stderr,
                    "\tImage (with path %s) has an invalid uuid\r\n",
                    cb_info->image_path);

            return false;

        case ERR_MACHO_FILE_PARSE_NOT_A_DYNAMIC_LIBRARY:
            fprintf(stderr,
                    "\tImage (with path %s) has no identification:\r\n"
                    "\t(install-name, current-version, "
                    "compatibility-version\r\n",
                    cb_info->image_path);

            break;

        case ERR_MACHO_FILE_PARSE_NO_PLATFORM:
            request_result =
                request_platform(cb_info->global,
                                 cb_info->tbd,
                                 cb_info->retained_info_in,
                                 true,
                                 stderr,
                                 "\tImage (with path %s) doesn't have a "
                                 "platform\r\n",
                                 cb_info->image_path);


            if (!request_result) {
                return false;
            }

            break;

        case ERR_MACHO_FILE_PARSE_NO_UUID:
            fprintf(stderr,
                    "\tImage (with path %s) has no uuid\r\n",
                    cb_info->image_path);

            return false;
    }

    return true;
}

void
print_dsc_image_parse_error(const char *__notnull const image_path,
                            const enum dsc_image_parse_result parse_error)
{
    switch (parse_error) {
        case E_DSC_IMAGE_PARSE_OK:
        case E_DSC_IMAGE_PARSE_ERROR_PASSED_TO_CALLBACK:
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

        case E_DSC_IMAGE_PARSE_NO_SYMBOL_TABLE:
            fprintf(stderr,
                    "Image (with path %s) has no symbol-table\r\n",
                    image_path);

            break;

        case E_DSC_IMAGE_PARSE_NO_EXPORTS:
            fprintf(stderr,
                    "Image (with path %s) has no exported clients, re-exports, "
                    "or symbols to be written out\r\n",
                    image_path);

            break;
    }
}
