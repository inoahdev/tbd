//
//  src/tbd_for_main.h
//  tbd
//
//  Created by inoahdev on 12/01/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <ctype.h>
#include <errno.h>

#include <stdlib.h>
#include <string.h>

#include "macho_file.h"
#include "parse_or_list_fields.h"
#include "tbd_for_main.h"

bool
tbd_for_main_parse_option(struct tbd_for_main *const tbd,
                          const int argc,
                          const char *const *const argv,
                          const char *const option,
                          int *const index_in)
{
    int index = *index_in;
    if (strcmp(option, "add-archs") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            fputs("Adding and replacing architectures is not supported, Please "
                  "choose only a single option\n",
                  stderr);

            exit(1);
        }

        index += 1;

        tbd->info.archs = parse_architectures_list(argc, argv, &index);
        tbd->options |= O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS;
    } else if (strcmp(option, "add-flags") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            if (tbd->flags_re != 0) {
                fputs("Replacing and adding flags is not supported, Please "
                      "choose only a single option\n",
                      stderr);

                exit(1);
            }
        }

        index += 1;

        tbd->info.flags = parse_flags_list(argc, argv, &index);
        tbd->options |= O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS;
    } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_NORMAL_SYMBOLS;
    } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_WEAK_DEF_SYMBOLS;
    } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS;
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS;
    } else if (strcmp(option, "allow-private-objc-class-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_OBJC_CLASS_SYMBOLS;
    } else if (strcmp(option, "allow-private-objc-ivar-symbols") == 0) {
        tbd->parse_options |= O_TBD_PARSE_ALLOW_PRIVATE_OBJC_IVAR_SYMBOLS;
    } else if (strcmp(option, "ignore-clients") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_CLIENTS;
    } else if (strcmp(option, "ignore-compatibility-version") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_COMPATIBILITY_VERSION;
    } else if (strcmp(option, "ignore-current-version") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_CURRENT_VERSION;
    } else if (strcmp(option, "ignore-missing-exports") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_MISSING_SYMBOL_TABLE;
        tbd->write_options |= O_TBD_CREATE_IGNORE_EXPORTS;
    } else if (strcmp(option, "ignore-missing-uuids") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_MISSING_UUIDS;
    } else if (strcmp(option, "ignore-non-unique-uuids") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_NON_UNIQUE_UUIDS;
    } else if (strcmp(option, "ignore-objc-constraint") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_OBJC_CONSTRAINT;
    } else if (strcmp(option, "ignore-parent-umbrella") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_PARENT_UMBRELLA;
    } else if (strcmp(option, "ignore-reexports") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_REEXPORTS;
    } else if (strcmp(option, "ignore-swift-version") == 0) {
        tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
    } else if (strcmp(option, "ignore-warnings") == 0) {
        tbd->options |= O_TBD_FOR_MAIN_IGNORE_WARNINGS;
    } else if (strcmp(option, "remove-archs") == 0) {
        if (!(tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS)) {
            if (tbd->archs_re != 0) {
                fputs("Replacing and removing architectures is not supported, "
                      "Please choose only a single option\n",
                      stderr);

                exit(1);
            }
        }
        
        index += 1;

        tbd->archs_re = parse_architectures_list(argc, argv, &index);
        tbd->options |= O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS;
    } else if (strcmp(option, "remove-flags") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            if (tbd->info.flags != 0) {
                fputs("Replacing and removing flags is not supported, Please "
                      "choose only a single option\n",
                      stderr);

                exit(1);
            }
        }

        index += 1;
        tbd->flags_re = parse_flags_list(argc, argv, &index);
    } else if (strcmp(option, "replace-archs") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_ARCHS) {
            fputs("Adding/removing and replacing architectures is not "
                  "supported, Please choose only a single option\n",
                  stderr);

            exit(1);
        }
        
        index += 1;
        tbd->archs_re = parse_architectures_list(argc, argv, &index);
    } else if (strcmp(option, "replace-flags") == 0) {
        if (tbd->options & O_TBD_FOR_MAIN_ADD_OR_REMOVE_FLAGS) {
            fputs("Adding/removing and replacing flags is not supported, "
                  "Please choose only a single option\n",
                  stderr);

            exit(1);
        }

        index += 1;
        tbd->flags_re = parse_flags_list(argc, argv, &index);
    } else if (strcmp(option, "replace-objc-constraint") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide an objc-constraint value\n", stderr);
            exit(1);
        }

        const char *const argument = argv[index];
        const enum tbd_objc_constraint objc_constraint =
            parse_objc_constraint(argument);

        if (objc_constraint == 0) {
            fprintf(stderr,
                    "Unrecognized objc-constraint: %s. Run "
                    "--list-objc-constraint to see a list of valid "
                    "objc-constraints\n",
                    argument);

            exit(1);
        }

        tbd->info.objc_constraint = objc_constraint;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    } else if (strcmp(option, "replace-platform") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide an objc-constraint value\n", stderr);
            exit(1);
        }

        const char *const argument = argv[index];
        const enum tbd_platform platform = parse_platform(argument);

        if (platform == 0) {
            fprintf(stderr,
                    "Unrecognized platform: %s. Run --list-platform to see a "
                    "list of valid platforms\n",
                    argument);

            exit(1);
        }

        tbd->info.platform = platform;
        tbd->parse_options |= O_TBD_PARSE_IGNORE_PLATFORM;
    } else if (strcmp(option, "replace-swift-version") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a swift-version\n", stderr);
            exit(1);
        }

        tbd->info.swift_version = parse_swift_version(argv[index]);
        tbd->parse_options |= O_TBD_PARSE_IGNORE_SWIFT_VERSION;
    } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
        index += 1;
        if (index == argc) {
            fputs("Please provide a tbd-version\n", stderr);
            exit(1);
        }

        const char *const argument = argv[index];
        const enum tbd_version version = parse_tbd_version(argument);

        if (version == 0) {
            fprintf(stderr,
                    "Unrecognized tbd-version: %s. Run --list-tbd-versions to "
                    "see a list of valid tbd-versions\n",
                    argument);

            exit(1);
        }

        tbd->info.version = version;
    } else {
        return false;
    }

    *index_in = index;
    return true;
}

void
tbd_for_main_apply_from(struct tbd_for_main *const dst,
                        const struct tbd_for_main *const src)
{
    if (dst->info.archs == 0) {
        /*
         * Only preset archs if we aren't later removing/replacing these archs.
         */

        if (dst->archs_re != 0) {
            dst->info.archs = src->info.archs;
        }
    }

    if (dst->info.current_version == 0) {
        dst->info.current_version = src->info.current_version;
    }

    if (dst->info.compatibility_version == 0) {
        dst->info.compatibility_version = src->info.compatibility_version;
    }

    if (dst->info.flags == 0) {
        /*
         * Only preset flags if we aren't later removing/replacing these flags.
         */
        
        if (dst->flags_re != 0) {
            dst->info.flags = src->info.flags;
        }
    }

    if (dst->info.install_name == NULL) {
        dst->info.install_name = src->info.install_name;
    }

    if (dst->info.parent_umbrella == NULL) {
        dst->info.parent_umbrella = src->info.parent_umbrella;
    }

    if (dst->info.platform == 0) {
        dst->info.platform = src->info.platform;
    }

    if (dst->info.objc_constraint == 0) {
        dst->info.objc_constraint = src->info.objc_constraint;
    }

    if (dst->info.swift_version == 0) {
        dst->info.swift_version = src->info.swift_version;
    }
        
    if (dst->info.version == 0) {
        dst->info.version = src->info.version;
    }

    dst->parse_options |= src->parse_options;
    dst->write_options |= src->write_options;
    dst->options |= src->options;
}

void tbd_for_main_destroy(struct tbd_for_main *const tbd) {
    tbd_create_info_destroy(&tbd->info);

    free(tbd->parse_path);
    free(tbd->write_path);

    tbd->parse_path = NULL;
    tbd->write_path = NULL;
}