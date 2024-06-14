//
//  src/parse_macho_for_main.c
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 - 2020 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "handle_macho_file_parse_result.h"
#include "macho_file.h"
#include "our_io.h"
#include "parse_macho_for_main.h"
#include "recursive.h"
#include "tbd.h"
#include "tbd_for_main.h"

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
            if (tbd->options.ignore_warnings) {
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
            if (tbd->options.ignore_warnings) {
                return NULL;
            }

            fprintf(stderr,
                    "File at write-path (%s) already exists\n",
                    write_path);

            return NULL;
    }

    if (tbd->options.combine_tbds) {
        args->combine_file = file;
    }

    return file;
}

enum parse_macho_for_main_result
parse_macho_file_for_main(const struct parse_macho_for_main_args args) {
    struct macho_file macho = {};
    struct range range = {};

    const enum macho_file_open_result open_macho_result =
        macho_file_open(&macho, args.magic_buffer, args.fd, range);

    switch (open_macho_result) {
        case E_MACHO_FILE_OPEN_OK:
            break;

        case E_MACHO_FILE_OPEN_READ_FAIL:
        case E_MACHO_FILE_OPEN_FSTAT_FAIL:
            handle_macho_file_open_result(open_macho_result,
                                          args.dir_path,
                                          args.name,
                                          args.print_paths,
                                          false);

            return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;

        case E_MACHO_FILE_OPEN_NOT_A_MACHO:
            if (args.dont_handle_non_macho_error) {
                return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;
            }

            handle_macho_file_open_result(open_macho_result,
                                          args.dir_path,
                                          args.name,
                                          args.print_paths,
                                          false);

            return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;

        default:
            break;
    }

    struct tbd_create_info *const info = &args.tbd->info;

    const struct tbd_create_info *const orig = &args.orig->info;
    const struct handle_macho_file_parse_error_cb_info cb_info = {
        .tbd = args.tbd,

        .dir_path = args.dir_path,
        .name = args.name,

        .print_paths = args.print_paths,
        .is_recursing = false
    };

    struct string_buffer sb_buffer = {};
    struct macho_file_parse_extra_args extra = {
        .callback = handle_macho_file_for_main_error_callback,
        .cb_info = (void *)&cb_info,
        .export_trie_sb = &sb_buffer
    };

    const enum macho_file_parse_result parse_macho_result =
        macho_file_parse_from_file(info,
                                   &macho,
                                   extra,
                                   args.tbd->parse_options,
                                   args.tbd->macho_options);

    if (parse_macho_result != E_MACHO_FILE_PARSE_OK) {
        tbd_create_info_clear_fields_and_create_from(info, orig);
        handle_macho_file_parse_result(args.dir_path,
                                       args.name,
                                       parse_macho_result,
                                       args.print_paths,
                                       false,
                                       args.tbd->options.ignore_warnings);

        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    tbd_for_main_handle_post_parse(args.tbd);

    if (args.options.verify_write_path) {
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
            tbd_create_info_clear_fields_and_create_from(info, orig);
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

    tbd_create_info_clear_fields_and_create_from(info, orig);
    return E_PARSE_MACHO_FOR_MAIN_OK;
}

enum parse_macho_for_main_result
parse_macho_file_for_main_while_recursing(
    struct parse_macho_for_main_args *__notnull const args)
{
    struct macho_file macho = {};
    struct range range = {};

    const enum macho_file_open_result open_macho_result =
        macho_file_open(&macho, args->magic_buffer, args->fd, range);

    switch (open_macho_result) {
        case E_MACHO_FILE_OPEN_OK:
            break;

        case E_MACHO_FILE_OPEN_READ_FAIL:
        case E_MACHO_FILE_OPEN_FSTAT_FAIL:
            handle_macho_file_open_result(open_macho_result,
                                          args->dir_path,
                                          args->name,
                                          args->print_paths,
                                          true);

            return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;

        case E_MACHO_FILE_OPEN_NOT_A_MACHO:
            if (args->dont_handle_non_macho_error) {
                return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;
            }

            handle_macho_file_open_result(open_macho_result,
                                          args->dir_path,
                                          args->name,
                                          args->print_paths,
                                          true);

            return E_PARSE_MACHO_FOR_MAIN_NOT_A_MACHO;

        default:
            break;
    }

    /*
     * Handle any provided replacement options.
     */

    struct tbd_for_main *const tbd = args->tbd;
    struct tbd_create_info *const info = &tbd->info;

    struct tbd_for_main *const orig = args->orig;
    struct tbd_create_info *const orig_info = &orig->info;

    const char *const dir_path = args->dir_path;
    const char *const name = args->name;
    const bool print_paths = args->print_paths;

    const struct handle_macho_file_parse_error_cb_info cb_info = {
        .orig = orig,
        .tbd = tbd,

        .dir_path = dir_path,
        .name = name,

        .print_paths = print_paths,
        .is_recursing = true
    };

    struct macho_file_parse_extra_args extra = {
        .callback = handle_macho_file_for_main_error_callback,
        .cb_info = (void *)&cb_info,
        .export_trie_sb = args->export_trie_sb
    };

    const enum macho_file_parse_result parse_macho_result =
        macho_file_parse_from_file(info,
                                   &macho,
                                   extra,
                                   tbd->parse_options,
                                   tbd->macho_options);

    if (parse_macho_result != E_MACHO_FILE_PARSE_OK) {
        tbd_create_info_clear_fields_and_create_from(info, orig_info);
        handle_macho_file_parse_result(dir_path,
                                       name,
                                       parse_macho_result,
                                       print_paths,
                                       true,
                                       tbd->options.ignore_warnings);

        return E_PARSE_MACHO_FOR_MAIN_OTHER_ERROR;
    }

    tbd_for_main_handle_post_parse(tbd);

    char *write_path = NULL;
    uint64_t write_path_length = 0;

    const bool should_combine = tbd->options.combine_tbds;
    if (!should_combine) {
        write_path =
            tbd_for_main_create_write_path_for_recursing(tbd,
                                                         dir_path,
                                                         args->dir_path_length,
                                                         name,
                                                         args->name_length,
                                                         "tbd",
                                                         3,
                                                         &write_path_length);
    } else {
        write_path = tbd->write_path;
        write_path_length = tbd->write_path_length;

        tbd->write_options.ignore_footer = true;
    }

    char *terminator = NULL;
    FILE *const file =
        open_file_for_path_while_recursing(args,
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

    tbd_for_main_write_to_file(tbd,
                               write_path,
                               write_path_length,
                               terminator,
                               file,
                               print_paths);

    if (!should_combine) {
        fclose(file);
        free(write_path);
    }

    tbd_create_info_clear_fields_and_create_from(info, orig_info);
    return E_PARSE_MACHO_FOR_MAIN_OK;
}
