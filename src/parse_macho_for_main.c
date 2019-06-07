//
//  src/parse_macho_for_main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <sys/stat.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>

#include <unistd.h>

#include "handle_macho_file_parse_result.h"

#include "macho_file.h"
#include "parse_macho_for_main.h"

static void
clear_create_info(struct tbd_create_info *const info_in,
                  const struct tbd_create_info *const orig)
{
    tbd_create_info_destroy(info_in);
    *info_in = *orig;
}

static int
read_magic(void *const magic_in,
           uint64_t *const magic_in_size_in,
           const int fd)
{
    const uint64_t magic_in_size = *magic_in_size_in;
    if (magic_in_size >= sizeof(uint32_t)) {
        return 0;
    }

    const uint64_t read_size = sizeof(uint32_t) - magic_in_size;
    if (read(fd, magic_in + magic_in_size, read_size) < 0) {
        return 1;
    }

    *magic_in_size_in = sizeof(uint32_t);
    return 0;
}

static void
handle_write_result(const struct tbd_for_main *const tbd,
                    const char *const path,
                    const char *const write_path,
                    const enum tbd_for_main_write_to_path_result result,
                    const bool print_paths)
{
    switch (result) {
        case E_TBD_FOR_MAIN_WRITE_TO_PATH_OK:
            break;

        case E_TBD_FOR_MAIN_WRITE_TO_PATH_ALREADY_EXISTS:
            if (tbd->flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                return;
            }

            if (print_paths) {
                fprintf(stderr,
                        "Skipping over file (at path %s) as a file at its "
                        "write-path (%s) already exists\n",
                        path,
                        write_path);
            } else {
                fputs("Skipping over file at provided-path as a file at its "
                      "provided write-path already exists\n", stderr);
            }

            break;

        case E_TBD_FOR_MAIN_WRITE_TO_PATH_WRITE_FAIL:
            if (print_paths) {
                fprintf(stderr,
                        "Failed to write to output-file (at path %s)\n",
                        write_path);
            } else {
                fputs("Failed to write to provided output-file\n", stderr);
            }

            break;
    }
}

static void
handle_write_result_while_recursing(
    const struct tbd_for_main *const tbd,
    const char *const dir_path,
    const char *const name,
    const char *const write_path,
    const enum tbd_for_main_write_to_path_result result,
    const bool print_paths)
{
    switch (result) {
        case E_TBD_FOR_MAIN_WRITE_TO_PATH_OK:
            break;

        case E_TBD_FOR_MAIN_WRITE_TO_PATH_ALREADY_EXISTS:
            if (tbd->flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                return;
            }

            if (print_paths) {
                fprintf(stderr,
                        "Skipping over file (at path %s/%s) as a file at its "
                        "write-path (%s) already exists\n",
                        dir_path,
                        name,
                        write_path);
            } else {
                fputs("Skipping over file at provided-path as a file at its "
                      "provided write-path already exists\n", stderr);
            }

            break;

        case E_TBD_FOR_MAIN_WRITE_TO_PATH_WRITE_FAIL:
            if (print_paths) {
                fprintf(stderr,
                        "Failed to write to output-file (at path %s)\n",
                        write_path);
            } else {
                fputs("Failed to write to provided output-file\n", stderr);
            }

            break;
    }
}

static void verify_write_path(const struct tbd_for_main *const tbd) {
    const char *const write_path = tbd->write_path;
    if (write_path == NULL) {
        return;
    }

    struct stat sbuf = {};
    if (stat(write_path, &sbuf) < 0) {
        /*
         * Ignore any errors if the object doesn't even exist.
         */

        if (errno != ENOENT) {
            fprintf(stderr,
                    "Failed to get information on object at the provided "
                    "write-path (%s), error: %s\n",
                    write_path,
                    strerror(errno));

            exit(1);
        }

        return;
    }

    if (!S_ISREG(sbuf.st_mode)) {
        fprintf(stderr,
                "Writing to a regular file while parsing mach-o file (at path "
                "%s) is not supported",
                tbd->parse_path);

        exit(1);
    }
}

enum parse_macho_for_main_result
parse_macho_file_for_main(const struct parse_macho_for_main_args args) {
    if (read_magic(args.magic_in, args.magic_in_size_in, args.fd)) {
        if (errno == EOVERFLOW) {
            return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;
        }

        /*
         * Manually handle the read fail by passing on to
         * handle_macho_file_parse_result() as if we went to
         * macho_file_parse_from_file().
         */

        const struct handle_macho_file_parse_result_args handle_args = {
            .retained_info_in = args.retained_info_in,
            .global = args.global,
            .tbd = args.tbd,
            .dir_path = args.dir_path,
            .parse_result = E_MACHO_FILE_PARSE_READ_FAIL,
            .print_paths = args.print_paths
        };

        handle_macho_file_parse_result(handle_args);
        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    /*
     * Ignore invalid fields so a mach-o file is fully parsed regardless of
     * errors. Instead, we prefer to check manually for any field errors
     */

    const uint64_t macho_options =
        O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS | args.tbd->macho_options;

    struct tbd_create_info *const create_info = &args.tbd->info;
    struct tbd_create_info original_info = *create_info;

    const uint32_t magic = *(const uint32_t *)args.magic_in;
    const enum macho_file_parse_result parse_result =
        macho_file_parse_from_file(create_info,
                                   args.fd,
                                   magic,
                                   args.tbd->parse_options,
                                   macho_options);

    if (parse_result == E_MACHO_FILE_PARSE_NOT_A_MACHO) {
        if (!args.dont_handle_non_macho_error) {
            const struct handle_macho_file_parse_result_args handle_args = {
                .retained_info_in = args.retained_info_in,
                .global = args.global,
                .tbd = args.tbd,
                .dir_path = args.dir_path,
                .parse_result = parse_result,
                .print_paths = args.print_paths
            };

            handle_macho_file_parse_result(handle_args);
        }

        return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;
    }

    if (args.options & O_PARSE_MACHO_FOR_MAIN_VERIFY_WRITE_PATH) {
        verify_write_path(args.tbd);
    }

    const struct handle_macho_file_parse_result_args handle_args = {
        .retained_info_in = args.retained_info_in,
        .global = args.global,
        .tbd = args.tbd,
        .dir_path = args.dir_path,
        .parse_result = parse_result,
        .print_paths = args.print_paths
    };

    const bool should_continue = handle_macho_file_parse_result(handle_args);
    if (!should_continue) {
        clear_create_info(create_info, &original_info);
        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    char *write_path = args.tbd->write_path;
    if (write_path != NULL) {
        enum tbd_for_main_write_to_path_result ret =
            tbd_for_main_write_to_path(args.tbd,
                                       write_path,
                                       args.tbd->write_path_length,
                                       args.print_paths);

        if (ret != E_TBD_FOR_MAIN_WRITE_TO_PATH_OK) {
            handle_write_result(args.tbd,
                                args.dir_path,
                                write_path,
                                ret,
                                args.print_paths);
        }
    } else {
        tbd_for_main_write_to_stdout(args.tbd, args.dir_path, true);
    }

    clear_create_info(create_info, &original_info);
    return E_PARSE_MACHO_FOR_MAIN_OK;
}

enum parse_macho_for_main_result
parse_macho_file_for_main_while_recursing(
    const struct parse_macho_for_main_args args)
{
    if (read_magic(args.magic_in, args.magic_in_size_in, args.fd)) {
        if (errno == EOVERFLOW) {
            return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;
        }

        /*
         * Manually handle the read fail by passing on to
         * handle_macho_file_parse_result() as if we went to
         * macho_file_parse_from_file().
         */

        const struct handle_macho_file_parse_result_args handle_args = {
            .retained_info_in = args.retained_info_in,
            .global = args.global,
            .tbd = args.tbd,
            .dir_path = args.dir_path,
            .name = args.name,
            .parse_result = E_MACHO_FILE_PARSE_READ_FAIL,
            .print_paths = args.print_paths
        };

        handle_macho_file_parse_result_while_recursing(handle_args);
        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    /*
     * Handle the replacement options if provided.
     */

    const uint32_t magic = *(const uint32_t *)args.magic_in;

    const uint64_t parse_options = args.tbd->parse_options;
    const uint64_t macho_options =
        O_MACHO_FILE_PARSE_IGNORE_INVALID_FIELDS | args.tbd->macho_options;

    struct tbd_create_info *const create_info = &args.tbd->info;
    struct tbd_create_info original_info = *create_info;

    const enum macho_file_parse_result parse_result =
        macho_file_parse_from_file(create_info,
                                   args.fd,
                                   magic,
                                   parse_options,
                                   macho_options);

    if (parse_result == E_MACHO_FILE_PARSE_NOT_A_MACHO) {
        if (!args.dont_handle_non_macho_error) {
            const struct handle_macho_file_parse_result_args handle_args = {
                .retained_info_in = args.retained_info_in,
                .global = args.global,
                .tbd = args.tbd,
                .dir_path = args.dir_path,
                .name = args.name,
                .parse_result = parse_result,
                .print_paths = args.print_paths
            };

            handle_macho_file_parse_result_while_recursing(handle_args);
        }

        return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;
    }

    const struct handle_macho_file_parse_result_args handle_args = {
        .retained_info_in = args.retained_info_in,
        .global = args.global,
        .tbd = args.tbd,
        .dir_path = args.dir_path,
        .name = args.name,
        .parse_result = parse_result,
        .print_paths = args.print_paths
    };

    const bool should_continue =
        handle_macho_file_parse_result_while_recursing(handle_args);

    if (!should_continue) {
        clear_create_info(create_info, &original_info);
        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    uint64_t write_path_length = 0;
    char *const write_path =
        tbd_for_main_create_write_path_while_recursing(args.tbd,
                                                       args.dir_path,
                                                       args.dir_path_length,
                                                       args.name,
                                                       args.name_length,
                                                      "tbd",
                                                       3,
                                                       &write_path_length);

    if (write_path == NULL) {
        fputs("Failed to allocate memory\n", stderr);
        exit(1);
    }

    enum tbd_for_main_write_to_path_result write_to_path_result =
        tbd_for_main_write_to_path(args.tbd,
                                   write_path,
                                   write_path_length,
                                   true);

    free(write_path);

    if (write_to_path_result != E_TBD_FOR_MAIN_WRITE_TO_PATH_OK) {
        handle_write_result_while_recursing(args.tbd,
                                            args.dir_path,
                                            args.name,
                                            write_path,
                                            write_to_path_result,
                                            args.print_paths);
    }

    clear_create_info(create_info, &original_info);
    return E_PARSE_MACHO_FOR_MAIN_OK;
}
