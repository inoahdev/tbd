//
//  src/handle_dsc_result.h
//  tbd
//
//  Created by inoahdev on 12/31/18.
//  Copyright © 2018 inoahdev. All rights reserved.
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
handle_dsc_image_parse_result(struct tbd_for_main *const global,
                              struct tbd_for_main *const tbd,
                              const char *const dsc_path,
                              const char *const image_path,
                              const enum dsc_image_parse_result parse_result,
                              const bool print_paths,
                              uint64_t *const info_in)
{
    do {
        bool should_break_out = false;
        switch (parse_result) {
            case E_DSC_IMAGE_PARSE_OK:
                should_break_out = true;
                break;

            case E_DSC_IMAGE_PARSE_NOT_A_MACHO:
                return false;

            case E_DSC_IMAGE_PARSE_SEEK_FAIL:
                if (print_paths) {
                    fprintf(stderr,
                            "Failed to seek to a location while parsing "
                            "dyld_shared_cache file (at path %s)\n",
                            dsc_path);
                } else {
                    fputs("Failed to seek to a location while parsing the "
                          "provided dyld_shared_cache file\n",
                          stderr);
                }

                return false;

            case E_DSC_IMAGE_PARSE_READ_FAIL:
                if (print_paths) {
                    fprintf(stderr,
                            "Failed to read data while parsing "
                            "dyld_shared_cache file (at path %s)\n",
                            dsc_path);
                } else {
                    fputs("Failed to read data while parsing dyld_shared_cache "
                          "file at the provided path\n",
                          stderr);
                }

                return false;

            case E_DSC_IMAGE_PARSE_NO_CORRESPONDING_MAPPING:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache (at path %s) has an image"
                            "(with path %s) that has no corresponding mapping\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has no corresponding mapping\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_FAT_NOT_SUPPORTED:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache (at path %s) has an image"
                            "(with path %s) that is a fat mach-o, which is not "
                            "supported\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that is a fat mach-o, which is not "
                            "supported\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_SIZE_TOO_SMALL:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache (at path %s) has an image "
                            "(with path %s) that is too small to be a valid "
                            "mach-o\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that is too small to be a valid "
                            "mach-o\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_ALLOC_FAIL:
                if (print_paths) {
                    fprintf(stderr,
                            "Failed to allocate data while parsing an image "
                            "(with path %s) of the dyld_shared_cache file "
                            "(at path %s)\n",
                            image_path,
                            dsc_path);
                } else {
                    fprintf(stderr,
                            "Failed to allocate data while parsing an image "
                            "(with path %s) of the provided dyld_shared_cache "
                            "file\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_ARRAY_FAIL:
                if (print_paths) {
                    fprintf(stderr,
                            "Failed to perform an array operation while "
                            "parsing an image (with path %s) of the "
                            "dyld_shared_cache file (at path %s)\n",
                            image_path,
                            dsc_path);
                } else {
                    fprintf(stderr,
                            "Failed to perform an array operation while "
                            "parsing an image (with path %s) of the provided "
                            "dyld_shared_cache file\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_NO_LOAD_COMMANDS:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has no load-commands. "
                            "Subsequently, no information was retrieved\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has no load-commands. "
                            "Subsequently, no information was retrieved\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_TOO_MANY_LOAD_COMMANDS:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) with too many load-commands for "
                            "its size\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) with too many load-commands for "
                            "its size\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_LOAD_COMMANDS_AREA_TOO_SMALL:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) with a load-commands area too "
                            "small to store all of its load-commands\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) with a load-commands area too "
                            "small to store all of its load-commands\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_INVALID_LOAD_COMMAND:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) with an invalid load-command\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) with an invalid load-command\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_TOO_MANY_SECTIONS:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) with a segment that has too many "
                            "sections for its size\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) with a segment that has too many "
                            "sections for its size\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_INVALID_SECTION:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) with a segment that has an invalid "
                            "section\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) with a segment that has an invalid "
                            "section\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_INVALID_CLIENT:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has an invalid "
                            "client-string\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has an invalid "
                            "client-string\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_INVALID_INSTALL_NAME:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has an invalid install-name\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has an invalid install-name\n",
                            image_path);
                }

                if (!request_install_name(global, tbd, info_in)) {
                    return false;
                }

                break;

            case E_DSC_IMAGE_PARSE_INVALID_PLATFORM:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has an invalid platform\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has an invalid platform\n",
                            image_path);
                }

                if (!request_platform(global, tbd, info_in)) {
                    return false;
                }

                break;

            case E_DSC_IMAGE_PARSE_INVALID_REEXPORT:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has an invalid re-export\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has an invalid re-export\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_INVALID_PARENT_UMBRELLA:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has an invalid "
                            "parent-umbrella\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has an invalid "
                            "parent-umbrella\n",
                            image_path);
                }

                if (!request_parent_umbrella(global, tbd, info_in)) {
                    return false;
                }

                break;

            case E_DSC_IMAGE_PARSE_INVALID_SYMBOL_TABLE:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has an invalid "
                            "symbol-table\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has an invalid "
                            "symbol-table\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_INVALID_UUID:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has an invalid uuid\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has an invalid uuid\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_NO_IDENTIFICATION:
                /*
                 * No identification means that the dsc-image (a mach-o) is not
                 * a library file, which we check for only here, at the last
                 * moment.
                 *
                 * No errors are printed, and this is simply inored.
                 */

                return false;

            case E_DSC_IMAGE_PARSE_NO_SYMBOL_TABLE:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has no symbol-table\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has no symbol-table\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_NO_UUID:
                if (print_paths) {
                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has no symbol-table\n",
                            dsc_path,
                            image_path);
                } else {
                    fprintf(stderr,
                            "The provided dyld_shared_cache file has an image "
                            "(with path %s) that has no symbol-table\n",
                            image_path);
                }

                return false;

            case E_DSC_IMAGE_PARSE_NO_EXPORTS: {
                const uint64_t options = tbd->options;
                if (options & O_TBD_FOR_MAIN_RECURSE_DIRECTORIES) {
                    if (options & O_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                        return false;
                    }

                    fprintf(stderr,
                            "dyld_shared_cache file (at path %s) has an image "
                            "(with path %s) that has no clients, re-exports, "
                            "or symbols to be written out\n",
                            dsc_path,
                            image_path);
                } else {
                    if (print_paths) {
                        fprintf(stderr,
                                "dyld_shared_cache file (at path %s) has an "
                                "image (with path %s) that has no clients, "
                                "re-exports, or symbols to be written out\n",
                                dsc_path,
                                image_path);
                    } else {
                        fprintf(stderr,
                                "The provided dyld_shared_cache file has an "
                                "image (with path %s) that has no clients, "
                                "re-exports, or symbols to be written out\n",
                                image_path);
                    }
                }

                return false;
            }
        }

        if (should_break_out) {
            break;
        }
    } while (true);

    /*
     * Handle the remove/replace fields
     */

    const uint64_t archs_re = tbd->archs_re;
    if (archs_re != 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            tbd->info.archs &= ~archs_re;
        } else {
            tbd->info.archs = archs_re; 
        }
    }

    const uint64_t flags_re = tbd->flags_re;
    if (flags_re != 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            tbd->info.flags &= ~flags_re;
         } else {
            tbd->info.flags = flags_re; 
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
        if (print_paths) {
            fprintf(stderr,
                    "dyld_shared_cache file (at path %s) has an image "
                    "(with path %s) that has an invalid install-name\n",
                    dsc_path,
                    image_path);
        } else {
            fprintf(stderr,
                    "The provided dyld_shared_cache file has an image "
                    "(with path %s) that has an invalid install-name\n",
                    image_path);
        }

        if (!request_install_name(global, tbd, info_in)) {
            return false;
        }
    }

    if (tbd->info.platform == 0) {
        if (print_paths) {
            fprintf(stderr,
                    "dyld_shared_cache file (at path %s) has an image "
                    "(with path %s) that has an invalid platform\n",
                    dsc_path,
                    image_path);
        } else {
            fprintf(stderr,
                    "The provided dyld_shared_cache file has an image "
                    "(with path %s) that has an invalid platform\n",
                    image_path);
        }

        if (!request_platform(global, tbd, info_in)) {
            return false;
        }
    }

    return true;
}
