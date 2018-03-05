//
//  src/main_utils/parse_list_argument.h
//  tbd
//
//  Created by inoahdev on 1/26/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <cerrno>

#include "../mach-o/utils/tbd.h"

#include "../misc/current_directory.h"
#include "../misc/recurse.h"

#include "parse_list_argument.h"

#include "tbd_print_field_information.h"
#include "tbd_with_options.h"

namespace main_utils {
    bool parse_list_argument(const char *argument, int &index, int argc, const char *argv[]) noexcept {
        auto option = &argument[1];
        if (option[0] == '-') {
            option++;
        }

        if (strcmp(option, "list-architectures") == 0) {
            if (index != 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself or with a path to a mach-o file\n", argument);
                exit(1);
            }

            // Two modes exist for --list-architectures, either list out the
            // architecture-info table, or list the architectures of a single
            // provided path to a mach-o file

            if (index == argc - 1) {
                auto architecture_info = macho::get_architecture_info_table();

                fputs(architecture_info->name, stdout);
                architecture_info++;

                while (architecture_info->name != nullptr) {
                    fprintf(stdout, ", %s", architecture_info->name);
                    architecture_info++;
                }
            } else {
                index++;

                // Don't allow other arguments
                // to --list-architectures

                if (index + 2 <= argc) {
                    fprintf(stderr, "Unrecognized argument: %s\n", argv[index + 1]);
                    exit(1);
                }

                auto file = macho::file();
                auto path = misc::path_with_current_directory(argv[index]);

                struct stat sbuf;
                if (stat(path.c_str(), &sbuf) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object at provided path, failing with error: %s\n", strerror(errno));
                    exit(1);
                }

                if (!S_ISREG(sbuf.st_mode)) {
                    fputs("Object at provided path is not a regular file\n", stderr);
                    exit(1);
                }

                switch (file.open(path.c_str())) {
                    case macho::file::open_result::ok:
                        break;

                    case macho::file::open_result::not_a_macho:
                        fputs("File at provided path is not a mach-o\n", stderr);
                        exit(1);

                    case macho::file::open_result::invalid_macho:
                        fputs("File at provided path is an invalid mach-o\n", stderr);
                        exit(1);

                    case macho::file::open_result::failed_to_open_stream:
                        fprintf(stderr, "Failed to open stream for file at provided path, failing with error: %s\n", strerror(errno));
                        exit(1);

                    case macho::file::open_result::stream_seek_error:
                    case macho::file::open_result::stream_read_error:
                        fputs("Encountered an error while parsing file at provided path\n", stderr);
                        exit(1);

                    case macho::file::open_result::zero_containers:
                        fputs("Mach-o file at provided path has zero architectures\n", stderr);
                        exit(1);

                    case macho::file::open_result::too_many_containers:
                        fputs("Mach-o file at provided path has too many architectures for its file-size\n", stderr);
                        exit(1);

                    case macho::file::open_result::overlapping_containers:
                        fputs("Mach-o file at provided path has overlapping architectures\n", stderr);
                        exit(1);

                    case macho::file::open_result::invalid_container:
                        fputs("Mach-o file at provided path has an invalid architecture\n", stderr);
                        exit(1);
                }

                // Store architecture names in a vector before printing
                // so we can handle any errors encountered first

                auto cndex = 0;
                auto names = std::vector<const char *>();

                for (const auto &container : file.containers) {
                    const auto container_subtype = macho::subtype_from_cputype(macho::cputype(container.header.cputype), container.header.cpusubtype);
                    if (container_subtype == macho::subtype::none) {
                        fprintf(stderr, "Unrecognized cpu-subtype for architecture at index %d\n", cndex);
                        exit(1);
                    }

                    const auto architecture_info = macho::architecture_info_from_cputype(macho::cputype(container.header.cputype), container_subtype);
                    if (!architecture_info) {
                        fprintf(stderr, "Unrecognized cputype information for architecture at index %d\n", cndex);
                        exit(1);
                    }

                    cndex++;
                }

                fputs(names.front(), stdout);
                for (auto iter = names.cbegin() + 1; iter != names.cend(); iter++) {
                    fprintf(stdout, ", %s", *iter);
                }
            }

            fputc('\n', stdout);
        } else if (strcmp(option, "list-macho-dynamic-libraries") == 0) {
            if (index != 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself or with a path to a mach-o file\n", argument);
                exit(1);
            }

            // Two different modes exist for --list-macho-dynamic-libraries;
            // either recurse current-directory with default options, or
            // recurse provided director(ies) with provided options

            index++;
            if (index != argc) {
                auto paths = std::vector<std::pair<struct main_utils::tbd_with_options::options, std::string>>();
                struct main_utils::tbd_with_options::options options;

                options.recurse_directories_at_path = true;

                for (; index != argc; index++) {
                    const auto &argument = argv[index];
                    const auto &argument_front = argument[0];

                    if (argument_front == '-') {
                        auto option = &argument[1];
                        if (option[0] == '\0') {
                            fputs("Please provide a valid option\n", stderr);
                            exit(1);
                        }

                        if (option[0] == '-') {
                            option++;
                        }

                        if (strcmp(option, "dont-print-warnings") == 0) {
                            options.ignore_warnings = true;
                        } else if (strcmp(option, "r") == 0) {
                            if (index + 1 != argc) {
                                if (strcmp(argv[index + 1], "once") == 0) {
                                    index++;
                                } else if (strcmp(argv[index + 1], "all") == 0) {
                                    options.recurse_subdirectories_at_path = true;
                                    index++;
                                }
                            }
                        } else {
                            fprintf(stderr, "Unrecognized argument: %s\n", argument);
                            exit(1);
                        }

                        continue;
                    }

                    auto path = misc::path_with_current_directory(argv[index]);

                    struct stat sbuf;
                    if (stat(path.c_str(), &sbuf) != 0) {
                        if (index == argc - 1) {
                            fprintf(stderr, "Failed to retrieve information on file at provided path, failing with error: %s\n", strerror(errno));
                        } else {
                            fprintf(stderr, "Failed to retrieve information on file (at path %s), failing with error: %s\n", path.c_str(), strerror(errno));
                        }

                        exit(1);
                    }

                    if (!S_ISDIR(sbuf.st_mode)) {
                        if (index == argc - 1) {
                            fputs("Object at provided path is not a directory\n", stderr);
                        } else {
                            fprintf(stderr, "Object (at path %s) is not a directory\n", path.c_str());
                        }

                        exit(1);
                    }

                    paths.emplace_back(options, std::move(path));
                    options.clear();
                }

                if (paths.empty()) {
                    fputs("No directories have been provided to recurse in\n", stderr);
                    exit(1);
                }

                const auto &back = paths.back();
                for (const auto &pair : paths) {
                    const auto &options = pair.first;
                    const auto &path = pair.second;

                    auto recurse_options = misc::recurse::options();
                    if (!options.ignore_warnings) {
                        recurse_options.print_warnings = true;
                    }

                    if (options.recurse_subdirectories_at_path) {
                        recurse_options.recurse_subdirectories = true;
                    }

                    auto filetypes = misc::recurse::filetypes();
                    filetypes.dynamic_library = true;

                    const auto recursion_result = misc::recurse::macho_files(path.c_str(), filetypes, recurse_options, [](const macho::file &file, const std::string &path) {
                        fprintf(stdout, "%s\n", path.c_str());
                        return true;
                    });

                    switch (recursion_result) {
                        case misc::recurse::operation_result::ok:
                            break;

                        case misc::recurse::operation_result::failed_to_open_directory:
                            if (paths.size() != 1) {
                                fprintf(stderr, "Failed to open directory (at path %s), failing with error: %s\n", path.c_str(), strerror(errno));
                            } else {
                                fprintf(stderr, "Failed to open directory at provided path, failing with error: %s\n", strerror(errno));
                            }

                            break;

                        case misc::recurse::operation_result::found_no_matching_files:
                            if (options.recurse_subdirectories_at_path) {
                                if (paths.size() != 1) {
                                    fprintf(stderr, "Found no mach-o dynamic library files while recursing through directory and its sub-directories at path: %s\n", path.c_str());
                                } else {
                                    fputs("Found no mach-o dynamic library files while recursing through directory and its sub-directories at provided path\n", stderr);
                                }
                            } else {
                                if (paths.size() != 1) {
                                    fprintf(stderr, "Found no mach-o dynamic library files while recursing through directory at path: %s\n", path.c_str());
                                } else {
                                    fputs("Found no mach-o dynamic library files while recursing through directory at provided path\n", stderr);
                                }
                            }

                            break;

                        default:
                            break;
                    }

                    // Print a newline between each pair
                    if (pair != back) {
                        fputc('\n', stdout);
                    }
                }
            } else {
                auto path = misc::retrieve_current_directory();
                auto recurse_options = misc::recurse::options();

                recurse_options.print_warnings = true;
                recurse_options.recurse_subdirectories = true;

                auto filetypes = misc::recurse::filetypes();
                filetypes.dynamic_library = true;

                const auto recursion_result = misc::recurse::macho_files(path, filetypes, recurse_options, [](const macho::file &file, const std::string &path) {
                    fprintf(stdout, "%s\n", path.c_str());
                    return true;
                });

                switch (recursion_result) {
                    case misc::recurse::operation_result::ok:
                        break;

                    case misc::recurse::operation_result::failed_to_open_directory:
                        fprintf(stderr, "Failed to open directory at current-directory, failing with error: %s\n", strerror(errno));
                        break;

                    case misc::recurse::operation_result::found_no_matching_files:
                        fputs("Found no mach-o dynamic library files while recursing through directory and its sub-directories at provided path\n", stderr);
                        break;

                    default:
                        break;
                }
            }
        } else if (strcmp(option, "list-objc-constraint") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                exit(1);
            }

            fputs("none\n", stderr);
            fputs("retain_release\n", stdout);
            fputs("retain_release_or_gc\n", stdout);
            fputs("retain_release_for_simulator\n", stdout);
            fputs("gc\n", stdout);
        } else if (strcmp(option, "list-platform") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                exit(1);
            }

            main_utils::print_tbd_platforms();
        } else if (strcmp(option, "list-recurse") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                exit(1);
            }

            fputs("once, Recurse through all of a directory's files (default)\n", stdout);
            fputs("all,  Recurse through all of a directory's files and sub-directories\n", stdout);
        } else if (strcmp(option, "list-tbd-flags") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                exit(1);
            }

            fputs("flat_namespace\n", stdout);
            fputs("not_app_extension_safe\n", stdout);
        } else if (strcmp(option, "list-tbd-versions") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                exit(1);
            }

            main_utils::print_tbd_versions();
        } else {
            return false;
        }

        return true;
    }
}
