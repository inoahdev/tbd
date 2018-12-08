//
//  src/main.c
//  tbd
//
//  Created by inoahdev on 11/18/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <errno.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>

#include <unistd.h>

#include "macho_file.h"
#include "usage.h"

void print_exports(const struct array *const exports) {
    const struct tbd_export_info *info = exports->data;
    const struct tbd_export_info *const end = exports->data_end;

    for (; info != end; info++) {
        const uint64_t archs = info->archs;
        const char *const string = info->string;

        const enum tbd_export_type type = info->type;
        const char *type_string = NULL;

        switch (type) {
            case TBD_EXPORT_TYPE_CLIENT:
                type_string = "client";
                break;

            case TBD_EXPORT_TYPE_REEXPORT:
                type_string = "re-export";
                break;

            case TBD_EXPORT_TYPE_NORMAL_SYMBOL:
                type_string = "normal-symbol";
                break;

            case TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL:
                type_string = "objc-class";
                break;

            case TBD_EXPORT_TYPE_OBJC_IVAR_SYMBOL:
                type_string = "objc-ivar";
                break;

            case TBD_EXPORT_TYPE_WEAK_DEF_SYMBOL:
                type_string = "weak-def";
                break;
        }

        fprintf(stdout,
                "arch: %ld, type: %s, string: %s\n",
                archs,
                type_string,
                string);
    }
}

struct info_with_options {
    uint64_t options;
    struct tbd_create_info info;
};

int test_main(const int argc, const char *const argv[]) {
    if (argc < 2) {
        print_usage();
        return 0;
    }

    const char *const file_path = argv[1];
    const int fd = open(file_path, O_RDONLY);

    if (fd < 0) {
        fprintf(stderr,
                "Failed to open provided file (at path %s), error: %s\n",
                file_path,
                strerror(errno));

        return 1;
    }
    
    struct tbd_create_info info = {};
    const enum macho_file_parse_result parse_file_result =
        macho_file_parse_from_file(&info, fd, 0, 0);

    if (parse_file_result != E_MACHO_FILE_PARSE_OK) {
        fprintf(stderr,
                "Failed to parse mach-o file, error=%d\n",
                (int)parse_file_result);

        return 1;
    }

    FILE *const out = fopen("/home/administrator/Desktop/UIKit.tbd", "w");
    if (out == NULL) {
        fprintf(stderr,
                "Failed to open output file, error: %s\n",
                strerror(errno));

        return 1;
    }

    info.version = TBD_VERSION_V2;
    const enum tbd_create_result create_tbd_result =
        tbd_create_with_info(&info, out, 0);

    if (create_tbd_result != E_TBD_CREATE_OK) {
        fflush(stdout);
        fprintf(stderr,
                "\n\nFailed to create tbd, error=%d\n",
                (int)create_tbd_result);

        return 1;
    }

    return 0;
}
