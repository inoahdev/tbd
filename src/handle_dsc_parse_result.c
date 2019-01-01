//
//  src/handle_dsc_result.h
//  tbd
//
//  Created by inoahdev on 12/31/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "handle_dsc_parse_result.h"

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

        case E_DYLD_SHARED_CACHE_PARSE_SEEK_FAIL: {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to seek to a location while parsing "
                        "dyld_shared_cache file (at path: %s)\n",
                        path);
            } else {
                fputs("Failed to seek to a location while parsing the "
                      "provided mach-o file\n",
                      stderr);
            }

            break;
        }

        case E_DYLD_SHARED_CACHE_PARSE_READ_FAIL: {
            if (print_paths) {
                fprintf(stderr,
                        "Failed to read data while parsing dyld_shared_cache "
                        "file (at path: %s)\n",
                        path);
            } else {
                fputs("Failed to read data while parsing dyld_shared_cache "
                      "file at the provided path\n",
                      stderr);
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

void
handle_dsc_image_parse_result(struct tbd_for_main *global,
                              struct tbd_for_main *tbd,
                              const char *path,
                              enum dsc_image_parse_result parse_result,
                              bool print_paths,
                              uint64_t *retained_info_in)
{
    
}