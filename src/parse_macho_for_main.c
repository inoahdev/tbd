//
//  src/parse_macho_for_main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "handle_macho_file_parse_result.h"
#include "our_io.h"
#include "parse_macho_for_main.h"
#include "recursive.h"

static int
read_magic(void *__notnull const magic_in,
           uint64_t *__notnull const magic_in_size_in,
           const int fd)
{
    const uint64_t magic_in_size = *magic_in_size_in;
    if (magic_in_size >= sizeof(uint32_t)) {
        return 0;
    }

    const uint64_t read_size = sizeof(uint32_t) - magic_in_size;
    if (our_read(fd, magic_in + magic_in_size, read_size) < 0) {
        return 1;
    }

    *magic_in_size_in = sizeof(uint32_t);
    return 0;
}

static void verify_write_path(const struct tbd_for_main *__notnull const tbd) {
    const char *const write_path = tbd->write_path;
    if (write_path == NULL) {
        return;
    }

    struct stat sbuf = {};
    if (stat(write_path, &sbuf) < 0) {
        /*
         * The write-file doesn't have to exist.
         */

        if (errno != ENOENT) {
            fprintf(stderr,
                    "Failed to get information on object at the provided write-"
                    "path (%s), error: %s\n",
                    write_path,
                    strerror(errno));

            exit(1);
        }

        return;
    }

    if (!S_ISREG(sbuf.st_mode)) {
        fprintf(stderr,
                "Writing to a regular file while parsing a mach-o file (at "
                "path %s) is not supported",
                tbd->parse_path);

        exit(1);
    }
}

static FILE *
open_file_for_path(const struct parse_macho_for_main_args *__notnull const args,
                   char *__notnull const write_path,
                   const uint64_t write_path_length,
                   char **__notnull const terminator_out)
{
    FILE *file = NULL;

    const struct tbd_for_main *const tbd = args->tbd;
    const enum tbd_for_main_open_write_file_result open_file_result =
        tbd_for_main_open_write_file_for_path(tbd,
                                              write_path,
                                              write_path_length,
                                              &file,
                                              terminator_out);

    switch (open_file_result) {
        case E_TBD_FOR_MAIN_OPEN_WRITE_FILE_OK:
            break;

        case E_TBD_FOR_MAIN_OPEN_WRITE_FILE_FAILED:
            if (args->print_paths) {
                fprintf(stderr,
                        "Failed to open write-file (for path: %s), error: %s\n",
                        write_path,
                        strerror(errno));
            } else {
                fprintf(stderr,
                        "Failed to open the provided write-file, error: %s\n",
                        strerror(errno));
            }

            break;

        case E_TBD_FOR_MAIN_OPEN_WRITE_FILE_PATH_ALREADY_EXISTS:
            if (tbd->flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                break;
            }

            if (args->print_paths) {
                fprintf(stderr,
                        "File (at path %s) has an object at its write-path: %s"
                        "\n",
                        args->dir_path,
                        write_path);
            } else {
                fputs("File at the provided path has an object at its "
                      "write-path\n",
                      stderr);
            }

            break;
    }

    return file;
}

static FILE *
open_file_for_path_while_recursing(struct parse_macho_for_main_args *const args,
                                   char *__notnull const write_path,
                                   const uint64_t write_path_length,
                                   char **__notnull const terminator_out)
{
    FILE *file = args->combine_file;
    if (file != NULL) {
        return file;
    }

    const struct tbd_for_main *const tbd = args->tbd;
    const enum tbd_for_main_open_write_file_result open_file_result =
        tbd_for_main_open_write_file_for_path(tbd,
                                              write_path,
                                              write_path_length,
                                              &file,
                                              terminator_out);

    switch (open_file_result) {
        case E_TBD_FOR_MAIN_OPEN_WRITE_FILE_OK:
            break;

        case E_TBD_FOR_MAIN_OPEN_WRITE_FILE_FAILED:
            fprintf(stderr,
                    "Failed to open write-file (at path: %s), error: %s\n",
                    write_path,
                    strerror(errno));

            return NULL;

        case E_TBD_FOR_MAIN_OPEN_WRITE_FILE_PATH_ALREADY_EXISTS:
            if (tbd->flags & F_TBD_FOR_MAIN_IGNORE_WARNINGS) {
                return NULL;
            }

            fprintf(stderr,
                    "File at write-path (%s) already exists\n",
                    write_path);

            return NULL;
    }

    if (tbd->flags & F_TBD_FOR_MAIN_COMBINE_TBDS) {
        args->combine_file = file;
    }

    return file;
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
     * errors. Instead, we prefer to check manually for any field errors.
     */

    struct tbd_create_info *const info = &args.tbd->info;
    struct tbd_create_info *const g_info = &args.global->info;

    const struct handle_macho_file_parse_error_cb_info cb_info = {
        .retained_info_in = args.retained_info_in,

        .global = args.global,
        .tbd = args.tbd,

        .dir_path = args.dir_path,
        .name = args.name,

        .print_paths = args.print_paths,
        .is_recursing = false
    };

    struct string_buffer sb_buffer = {};
    struct macho_file_parse_extra_args extra = {
        .callback = handle_macho_file_for_main_error_callback,
        .callback_info = (void *)&cb_info,
        .export_trie_sb = &sb_buffer
    };

    const uint32_t magic = *(const uint32_t *)args.magic_in;
    const enum macho_file_parse_result parse_result =
        macho_file_parse_from_file(info,
                                   args.fd,
                                   magic,
                                   extra,
                                   args.tbd->parse_options,
                                   args.tbd->macho_options);

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
        tbd_create_info_clear_fields_and_create_from(info, g_info);
        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    if (args.options & O_PARSE_MACHO_FOR_MAIN_VERIFY_WRITE_PATH) {
        verify_write_path(args.tbd);
    }

    char *const write_path = args.tbd->write_path;
    const uint64_t write_path_length = args.tbd->write_path_length;

    FILE *file = NULL;
    char *terminator = NULL;

    if (write_path != NULL) {
        file = open_file_for_path(&args,
                                  write_path,
                                  write_path_length,
                                  &terminator);

        if (file == NULL) {
            tbd_create_info_clear_fields_and_create_from(info, g_info);
            return E_PARSE_MACHO_FOR_MAIN_OK;
        }

        tbd_for_main_write_to_file(args.tbd,
                                   write_path,
                                   write_path_length,
                                   terminator,
                                   file,
                                   args.print_paths);

        fclose(file);
    } else {
        tbd_for_main_write_to_stdout(args.tbd, args.dir_path, true);
    }

    tbd_create_info_clear_fields_and_create_from(info, g_info);
    return E_PARSE_MACHO_FOR_MAIN_OK;
}

enum parse_macho_for_main_result
parse_macho_file_for_main_while_recursing(
    struct parse_macho_for_main_args *__notnull const args_ptr)
{
    const struct parse_macho_for_main_args args = *args_ptr;
    if (read_magic(args.magic_in, args.magic_in_size_in, args.fd)) {
        if (errno == EOVERFLOW) {
            return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;
        }

        /*
         * Pass on the read-failure to
         * handle_macho_file_parse_result_while_recursing() as should be done
         * with every error faced in this function.
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
     * Handle any provided replacement options.
     */

    const uint32_t magic = *(const uint32_t *)args.magic_in;

    struct tbd_create_info *const info = &args.tbd->info;
    const struct tbd_create_info *const orig_info = args.orig;

    const struct handle_macho_file_parse_error_cb_info cb_info = {
        .retained_info_in = args.retained_info_in,

        .global = args.global,
        .tbd = args.tbd,

        .dir_path = args.dir_path,
        .name = args.name,

        .print_paths = args.print_paths,
        .is_recursing = true
    };

    struct macho_file_parse_extra_args extra = {
        .callback = handle_macho_file_for_main_error_callback,
        .callback_info = (void *)&cb_info,
        .export_trie_sb = args.export_trie_sb
    };

    const enum macho_file_parse_result parse_result =
        macho_file_parse_from_file(info,
                                   args.fd,
                                   magic,
                                   extra,
                                   args.tbd->parse_options,
                                   args.tbd->macho_options);

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
        tbd_create_info_clear_fields_and_create_from(info, orig_info);
        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    char *write_path = NULL;
    uint64_t write_path_length = 0;

    const bool should_combine = (args.tbd->flags & F_TBD_FOR_MAIN_COMBINE_TBDS);
    if (!should_combine) {
        write_path =
            tbd_for_main_create_write_path_for_recursing(args.tbd,
                                                         args.dir_path,
                                                         args.dir_path_length,
                                                         args.name,
                                                         args.name_length,
                                                         "tbd",
                                                         3,
                                                         &write_path_length);
    } else {
        write_path = args.tbd->write_path;
        write_path_length = args.tbd->write_path_length;

        args.tbd->write_options |= O_TBD_CREATE_IGNORE_FOOTER;
    }

    char *terminator = NULL;
    FILE *const file =
        open_file_for_path_while_recursing(args_ptr,
                                           write_path,
                                           write_path_length,
                                           &terminator);

    if (file == NULL) {
        if (!should_combine) {
            free(write_path);
        }

        tbd_create_info_clear_fields_and_create_from(info, orig_info);
        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    tbd_for_main_write_to_file(args.tbd,
                               write_path,
                               write_path_length,
                               terminator,
                               file,
                               args.print_paths);

    if (!should_combine) {
        fclose(file);
        free(write_path);
    }

    tbd_create_info_clear_fields_and_create_from(info, orig_info);
    return E_PARSE_MACHO_FOR_MAIN_OK;
}
