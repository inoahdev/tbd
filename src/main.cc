//
//  src/main.cc
//  tbd
//
//  Created by inoahdev on 4/16/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <iostream>
#include <cerrno>

#include <dirent.h>
#include <unistd.h>

#include "misc/recurse.h"
#include "tbd/tbd.h"

const char *retrieve_current_directory() {
    // Store current-directory as a static variable to avoid
    // calling getcwd more times than necessary.

    static auto current_directory = (const char *)nullptr;
    if (!current_directory) {
        // getcwd(nullptr, 0) will return a dynamically
        // allocated buffer just large enough to store
        // the current-directory.

        const auto current_directory_string = getcwd(nullptr, 0);
        if (!current_directory_string) {
            fprintf(stderr, "Failed to get current-working-directory, failing with error (%s)\n", strerror(errno));
            exit(1);
        }

        const auto current_directory_length = strlen(current_directory_string);
        const auto &current_directory_back = current_directory_string[current_directory_length - 1];

        if (current_directory_back != '/') {
            // As current_directory is a path to a directory,
            // the caller of this function expects to have a
            // path ending with a forward slash.

            auto new_current_directory_string = (char *)malloc(current_directory_length + 2);

            strncpy(new_current_directory_string, current_directory_string, current_directory_length);
            free(current_directory_string);

            new_current_directory_string[current_directory_length] = '/';
            new_current_directory_string[current_directory_length + 1] = '\0';

            current_directory = new_current_directory_string;
        } else {
            current_directory = current_directory_string;
        }
    }

    return current_directory;
}

void parse_architectures_list(uint64_t &architectures, int &index, int argc, const char *argv[]) {
    while (index < argc) {
        const auto &architecture_string = argv[index];
        const auto &architecture_string_front = architecture_string[0];

        // Quickly filter out an option or path instead of a (relatively)
        // expensive call to macho::architecture_info_from_name().

        if (architecture_string_front == '-' || architecture_string_front == '/') {
            // If the architectures vector is empty, the user did not provide any architectures
            // but did provided the architecture option, which requires at least one architecture
            // being provided.

            if (!architectures) {
                fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                exit(1);
            }

            break;
        }

        const auto architecture_info = macho::architecture_info_from_name(architecture_string);
        if (!architecture_info || !architecture_info->name) {
            // macho::architecture_info_from_name() returning nullptr can be the result of one
            // of two scenarios, The string stored in architecture being invalid,
            // such as being mispelled, or the string being the path object inevitebly
            // following the architecture argument.

            // If the architectures vector is empty, the user did not provide any architectures
            // but did provide the architecture option, which requires at least one architecture
            // being provided.

            if (!architectures) {
                fprintf(stderr, "Unrecognized architecture with name (%s)\n", architecture_string);
                exit(1);
            }

            break;
        }

        const auto architecture_info_table = macho::get_architecture_info_table();
        const auto architecture_info_table_index = ((uint64_t)architecture_info - (uint64_t)architecture_info_table) / sizeof(macho::architecture_info);

        architectures |= ((uint64_t)1 << architecture_info_table_index);
        index++;
    }

    // As the caller of this function is in a for loop,
    // the index is incremented once again once this function
    // returns. To make up for this, decrement index by 1.

    index--;
}

void recursively_create_directories_from_file_path_without_check(char *path, char *slash, bool create_last_as_directory) {
    do {
        // In order to avoid unnecessary (and expensive) allocations,
        // terminate the string at the location of the forward slash
        // and revert back after use.

        slash[0] = '\0';

        if (mkdir(path, 0755) != 0) {
            fprintf(stderr, "Failed to create directory (at path %s) with mode (0755), failing with error (%s)\n", path, strerror(errno));
            exit(1);
        }

        slash[0] = '/';
        slash = strchr(&slash[1], '/');
    } while (slash != nullptr);

    if (create_last_as_directory) {
        if (mkdir(path, 0755) != 0) {
            fprintf(stderr, "Failed to create directory (at path %s) with mode (0755), failing with error (%s)\n", path, strerror(errno));
            exit(1);
        }
    }
}

size_t recursively_create_directories_from_file_path(char *path, bool create_last_as_directory) {
    // If the path starts off with a forward slash, it will
    // result in the while loop running on a path that is
    // empty. To avoid this, start the search at the first
    // index.

    auto slash = strchr(&path[1], '/');
    auto return_value = (size_t)0;

    while (slash != nullptr) {
        // In order to avoid unnecessary (and expensive) allocations,
        // terminate the string at the location of the forward slash
        // and revert back after use.

        slash[0] = '\0';

        if (access(path, F_OK) != 0) {
            if (!return_value) {
                return_value = (uint64_t)slash - (uint64_t)path;
            }

            slash[0] = '/';

            // If a directory doesn't exist, it's assumed the sub-directories
            // won't exist either, so avoid additional calls to access()
            // by directly calling mkdir

            recursively_create_directories_from_file_path_without_check(path, slash, create_last_as_directory);
        } else {
            slash[0] = '/';
        }

        slash = strchr(&slash[1], '/');
    }

    if (!return_value) {
        return_value = strlen(path);

        if (create_last_as_directory) {
            if (access(path, F_OK) != 0) {
                if (mkdir(path, 0755) != 0) {
                    fprintf(stderr, "Failed to create directory (at path %s) with mode (0755), failing with error (%s)\n", path, strerror(errno));
                    exit(1);
                }
            }
        }
    }

    return return_value;
}

void recursively_remove_directories_from_file_path(char *path, size_t start_index = 0, size_t end_index = 0) {
    if (!end_index) {
        end_index = strlen(path);
    }

    if (start_index >= end_index) {
        return;
    }

    auto slash = strrchr(&path[start_index], '/');

    const auto start = &path[start_index];
    const auto end = &path[end_index];

    while (slash > end) {
        slash[0] = '\0';

        auto new_slash = strrchr(start, '/');

        slash[0] = '/';
        slash = new_slash;
    }

    auto old_end_begin = end[0];
    end[0] = '\0';

    if (access(path, F_OK) == 0) {
        if (remove(path) != 0) {
            fprintf(stderr, "Failed to remove object (at path %s), failing with error (%s)\n", path, strerror(errno));
            return;
        }
    } else {
        end[0] = old_end_begin;
        return;
    }

    end[0] = old_end_begin;

    while ((uint64_t)slash >= (uint64_t)start) {
        // In order to avoid unnecessary (and expensive) allocations,
        // terminate the string at the location of the forward slash
        // and revert back after use.

        slash[0] = '\0';

        if (remove(path) != 0) {
            fprintf(stderr, "Failed to remove object (at path %s), failing with error (%s)\n", path, strerror(errno));
            return;
        }

        auto new_slash = strrchr(start, '/');

        slash[0] = '/';
        slash = new_slash;
    }
}

void print_platforms() {
    auto platform_number = 1;
    auto platform = tbd::platform_to_string((enum tbd::platform)platform_number);

    fputs(platform, stdout);

    while (true) {
        platform_number++;
        platform = tbd::platform_to_string((enum tbd::platform)platform_number);

        if (!platform) {
            break;
        }

        fprintf(stdout, ", %s", platform);
    }

    fputc('\n', stdout);
}

enum creation_handling {
    creation_handling_print_paths = 1 << 0,
    creation_handling_ignore_no_provided_architectures = 1 << 1,
};

bool create_tbd_file(const char *macho_file_path, macho::file &file, const char *tbd_file_path, FILE *tbd_file, unsigned int options, tbd::platform platform, tbd::version version, uint64_t architectures, uint64_t architecture_overrides, unsigned int creation_handling_options) {
    auto result = tbd::create_from_macho_library(file, tbd_file, options, platform, version, architectures, architecture_overrides);
    if (result == tbd::creation_result::platform_not_found || result == tbd::creation_result::platform_not_supported || result == tbd::creation_result::multiple_platforms) {
        switch (result) {
            case tbd::creation_result::platform_not_found:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stdout, "Failed to find platform in mach-o library (at path %s). ", macho_file_path);
                } else {
                    fputs("Failed to find platform in provided mach-o library. ", stdout);
                }

                break;

            case tbd::creation_result::platform_not_supported:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stdout, "Platform in mach-o library (at path %s) is unsupported. ", macho_file_path);
                } else {
                    fputs("Platform in provided mach-o library is unsupported. ", stdout);
                }

                break;

            case tbd::creation_result::multiple_platforms:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stdout, "Multiple platforms found in mach-o library (at path %s). ", macho_file_path);
                } else {
                    fputs("Multiple platforms found in provided mach-o library. ", stdout);
                }

                break;

            default:
                break;
        }

        auto new_platform = tbd::platform::none;
        auto platform_string = std::string();

        do {
            fputs("Please provide a replacement platform (Input --list-platform to see a list of platforms): ", stdout);
            getline(std::cin, platform_string);

            if (platform_string == "--list-platform") {
                print_platforms();
            } else {
                new_platform = tbd::string_to_platform(platform_string.data());
            }
        } while (new_platform == tbd::platform::none);

        result = tbd::create_from_macho_library(file, tbd_file, options, platform, version, architectures, architecture_overrides);
    }

    switch (result) {
        case tbd::creation_result::ok:
            return true;

        case tbd::creation_result::invalid_subtype:
        case tbd::creation_result::invalid_cputype:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, is for an unrecognized machine\n", macho_file_path);
            } else {
                fputs("Provided mach-o file, or one of its architectures, is for an unrecognized machine\n", stderr);
            }

            break;

        case tbd::creation_result::invalid_load_command:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has an invalid load-command\n", macho_file_path);
            } else {
                fputs("Provided mach-o file, or one of its architectures, has an invalid load-command\n", stderr);
            }

            break;

        case tbd::creation_result::invalid_segment:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has an invalid segment\n", macho_file_path);
            } else {
                fputs("Provided mach-o file, or one of its architectures, has an invalid segment\n", stderr);
            }

            break;

        case tbd::creation_result::failed_to_iterate_load_commands:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Failed to iterate through mach-o file (at path %s), or one of its architectures's load-commands\n", macho_file_path);
            } else {
                fputs("Failed to iterate through provided mach-o file, or one of its architectures's load-commands\n", stderr);
            }

            break;

        case tbd::creation_result::failed_to_iterate_symbols:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Failed to iterate through mach-o file (at path %s), or one of its architectures's symbols\n", macho_file_path);
            } else {
                fputs("Failed to iterate through provided mach-o file, or one of its architectures's symbols\n", stderr);
            }

            break;

        case tbd::creation_result::contradictary_load_command_information:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has multiple load-commands of the same type with contradictory information\n", macho_file_path);
            } else {
                fputs("Provided mach-o file, or one of its architectures, has multiple load-commands of the same type with contradictory information\n", stderr);
            }

            break;

        case tbd::creation_result::uuid_is_not_unique:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "One of mach-o file (at path %s)'s architectures has a uuid that is not unique from other architectures\n", macho_file_path);
            } else {
                fputs("One of provided mach-o file's architectures has a uuid that is not unique from other architectures\n", stderr);
            }

            break;

        case tbd::creation_result::platform_not_found:
        case tbd::creation_result::platform_not_supported:
        case tbd::creation_result::multiple_platforms:
            break;

        case tbd::creation_result::not_a_library:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, is not a mach-o library\n", macho_file_path);
            } else {
                fputs("Provided mach-o file, or one of its architectures, is not a mach-o library\n", stderr);
            }

            break;

        case tbd::creation_result::has_no_uuid:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, does not have a uuid\n", macho_file_path);
            } else {
                fputs("Provided mach-o file, or one of its architectures, does not have a uuid\n", stderr);
            }

            break;

        case tbd::creation_result::contradictary_container_information:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s) has information in architectures contradicting the same information in other architectures\n", macho_file_path);
            } else {
                fputs("Provided mach-o file has information in architectures contradicting the same information in other architectures\n", stderr);
            }

            break;

        case tbd::creation_result::no_provided_architectures:
            if (creation_handling_options & creation_handling_ignore_no_provided_architectures) {
                return true;
            }

            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s) does not have architectures provided to output tbd from\n", macho_file_path);
            } else {
                fputs("Provided mach-o file does not have architectures provided to output tbd from\n", stderr);
            }

            break;

        case tbd::creation_result::no_symbols_or_reexports:
            if (creation_handling_options & creation_handling_print_paths) {
                fprintf(stderr, "Mach-o file (at path %s) does not have any symbols or reexports to be outputted\n", macho_file_path);
            } else {
                fputs("Provided mach-o file does not have any symbols or reexports to be outputted\n", stderr);
            }

            break;
    }

    return false;
}

void print_usage() {
    fputs("Usage: tbd [-p file-paths] [-o/-output output-paths-or-stdout]\n", stdout);
    fputs("Main options:\n", stdout);
    fputs("    -h, --help,     Print this message\n", stdout);
    fputs("    -o, --output,   Path(s) to output file(s) to write converted .tbd. If provided file(s) already exists, contents will be overriden. Can also provide \"stdout\" to print to stdout\n", stdout);
    fputs("    -p, --path,     Path(s) to mach-o file(s) to convert to a .tbd\n", stdout);
    fputs("    -u, --usage,    Print this message\n", stdout);

    fputc('\n', stdout);
    fputs("Path options:\n", stdout);
    fputs("Usage: tbd -p [-a/--arch architectures] [--archs architecture-overrides] [--platform ios/macosx/watchos/tvos] [-r/--recurse/ -r=once/all / --recurse=once/all] [-v/--version v1/v2] /path/to/macho/library\n", stdout);
    fputs("    -a, --arch,     Specify architecture(s) to output to tbd\n", stdout);
    fputs("        --archs,    Specify architecture(s) to use, instead of the ones in the provieded mach-o file(s)\n", stdout);
    fputs("        --platform, Specify platform for all mach-o library files provided\n", stdout);
    fputs("    -r, --recurse,  Specify directory to recurse and find mach-o library files in\n", stdout);
    fputs("    -v, --version,  Specify version of tbd to convert to (default is v2)\n", stdout);

    fputc('\n', stdout);
    fputs("Outputting options:\n", stdout);
    fputs("Usage: tbd -o [--maintain-directories] /path/to/output/file\n", stdout);
    fputs("        --maintain-directories, Maintain directories where mach-o library files were found in (subtracting the path provided)\n", stdout);

    fputc('\n', stdout);
    fputs("Global options:\n", stdout);
    fputs("    -a, --arch,     Specify architecture(s) to output to tbd (where architectures were not already specified)\n", stdout);
    fputs("        --archs,    Specify architecture(s) to override architectures found in file (where default architecture-overrides were not already provided)\n", stdout);
    fputs("        --platform, Specify platform for all mach-o library files provided (applying to all mach-o library files where platform was not provided)\n", stdout);
    fputs("    -v, --version,  Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tnd-version was not provided)\n", stdout);

    fputc('\n', stdout);
    fputs("Symbol options: (Both path and global options)\n", stdout);
    fputs("        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-normal-symbols, Allow all non-external symbols (of no type) (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars\n", stdout);
    fputs("        --allow-private-objc-classes,   Allow all non-external objc-classes\n", stdout);
    fputs("        --allow-private-objc-ivars,     Allow all non-external objc-ivars\n", stdout);

    fputc('\n', stdout);
    fputs("List options:\n", stdout);
    fputs("        --list-architectures,   List all valid architectures for tbd-files\n", stdout);
    fputs("        --list-macho-libraries, List all valid mach-o libraries in current-directory (or at provided path(s))\n", stdout);
    fputs("        --list-platform,        List all valid platforms\n", stdout);
    fputs("        --list-recurse,         List all valid recurse options for parsing directories\n", stdout);
    fputs("        --list-versions,        List all valid versions for tbd-files\n", stdout);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fputs("Please run -h or -u to see a list of options\n", stderr);
        return 1;
    }

    enum tool_options : uint32_t {
        recurse_directories = 1 << 6, // start at 1 << 6 to support tbd-symbol options
        recurse_subdirectories = 1 << 7,
        maintain_directories = 1 << 8,
    };

    typedef struct tbd_file {
        std::string path;
        std::string output_path;

        uint64_t architectures;
        uint64_t architecture_overrides;

        enum tbd::platform platform;
        enum tbd::version version;

        unsigned int options;
    } tbd_file;

    auto architectures = uint64_t();
    auto architecture_overrides = uint64_t();

    auto tbds = std::vector<tbd_file>();

    auto current_directory = std::string();
    auto output_paths_index = 0;

    auto options = 0;
    auto platform = tbd::platform::none;
    auto version = tbd::version::v2;

    // To parse the argument list, the for loop below parses
    // each option, and requires each option to parse its own
    // user input in the argument-list.

    for (auto i = 1; i < argc; i++) {
        const auto &argument = argv[i];
        const auto &argument_front = argument[0];

        if (argument_front != '-') {
            fprintf(stderr, "Unrecognized argument: %s\n", argument);
            return 1;
        }

        auto option = &argument[1];
        const auto &option_front = option[0];

        if (!option_front) {
            fputs("Please provide a valid option\n", stderr);
            return 1;
        }

        if (option_front == '-') {
            option++;
        }

        const auto is_first_argument = i == 1;
        const auto is_last_argument = i == argc - 1;

        if (strcmp(option, "a") == 0 || strcmp(option, "arch") == 0) {
            if (is_last_argument) {
                fputs("Please provide a list of architectures to output as tbd\n", stderr);
                return 1;
            }

            i++;
            parse_architectures_list(architectures, i, argc, argv);
        } else if (strcmp(option, "archs") == 0) {
            if (is_last_argument) {
                fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                return 1;
            }

            i++;
            parse_architectures_list(architecture_overrides, i, argc, argv);
        } else if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_usage();
            return 0;
        } else if (strcmp(option, "allow-all-private-symbols") == 0) {
            options |= tbd::symbol_options::allow_all_private_symbols;
        } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
            options |= tbd::symbol_options::allow_private_normal_symbols;
        } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
            options |= tbd::symbol_options::allow_private_weak_symbols;
        } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
            options |= tbd::symbol_options::allow_private_objc_symbols;
        } else if (strcmp(option, "allow-private-objc-classes") == 0) {
            options |= tbd::symbol_options::allow_private_objc_classes;
        } else if (strcmp(option, "allow-private-objc-ivars") == 0) {
            options |= tbd::symbol_options::allow_private_objc_ivars;
        } else if (strcmp(option, "list-architectures") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            auto architecture_info = macho::get_architecture_info_table();
            fputs(architecture_info->name, stdout);

            while (true) {
                architecture_info++;

                if (!architecture_info->name) {
                    break;
                }

                fprintf(stdout, ", %s", architecture_info->name);
            }

            fputc('\n', stdout);
            return 0;
        } else if (strcmp(option, "list-macho-libraries") == 0) {
            if (!is_first_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            if (!is_last_argument) {
                auto paths = std::vector<std::pair<std::string, unsigned int>>();
                auto options = 0;

                for (i++; i < argc; i++) {
                    const auto &argument = argv[i];
                    const auto &argument_front = argument[0];

                    if (argument_front == '-') {
                        auto option = &argument[1];
                        auto option_front = option[0];

                        if (!option_front) {
                            fputs("Please provide a valid option\n", stderr);
                            return 1;
                        }

                        if (option_front == '-') {
                            option++;
                        }

                        if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                            options |= recurse_directories | recurse_subdirectories;
                        } else if (strncmp(option, "r=", 2) == 0 || strncmp(option, "recurse=", 8) == 0) {
                            const auto recurse_type_string = strchr(option, '=') + 1;
                            const auto &recurse_type_string_front = recurse_type_string[0];

                            if (!recurse_type_string_front) {
                                fputs("Please provide a recurse type\n", stderr);
                                return 1;
                            }

                            options |= recurse_directories;

                            if (strcmp(recurse_type_string, "all") == 0) {
                                options |= recurse_subdirectories;
                            } else if (strcmp(recurse_type_string, "once") != 0) {
                                fprintf(stderr, "Unrecognized recurse-type (%s)\n", recurse_type_string);
                                return 1;
                            }
                        } else {
                            fprintf(stderr, "Unrecognized argument: %s\n", argument);
                            return 1;
                        }

                        continue;
                    }

                    if (argument_front != '/') {
                        auto path = std::string();
                        auto current_directory = retrieve_current_directory();

                        const auto argument_length = strlen(argument);
                        const auto current_directory_length = strlen(current_directory);
                        const auto path_length = argument_length + current_directory_length;

                        path.reserve(path_length);
                        path.append(current_directory);
                        path.append(argument);

                        paths.emplace_back(std::move(path), options);
                    } else {
                        paths.emplace_back(argument, options);
                    }

                    options = 0;
                }

                if (paths.empty()) {
                    fprintf(stderr, "Please provide a path for option (%s)\n", argument);
                    return 1;
                }

                const auto &paths_back = paths.back();
                for (const auto &pair : paths) {
                    const auto &path = pair.first;
                    const auto path_data = path.data();

                    struct stat sbuf;
                    if (stat(path_data, &sbuf) != 0) {
                        fprintf(stderr, "Failed to retrieve information on object (at path %s), failing with error (%s)\n", path_data, strerror(errno));
                        return 1;
                    }

                    const auto path_is_directory = S_ISDIR(sbuf.st_mode);
                    const auto &options = pair.second;

                    if (options & recurse_directories) {
                        if (!path_is_directory) {
                            fprintf(stderr, "Cannot recurse file (at path %s)\n", path_data);
                            return 1;
                        }

                        auto found_libraries = false;
                        recurse::macho_library_paths(path_data, options & recurse_subdirectories, [&](std::string &library_path) {
                            found_libraries = true;
                            fprintf(stdout, "%s\n", library_path.data());
                        });

                        if (!found_libraries) {
                            if (options & recurse_subdirectories) {
                                fprintf(stderr, "No mach-o library files were found while recursing through path (%s)\n", path_data);
                            } else {
                                fprintf(stderr, "No mach-o library files were found while recursing once through path (%s)\n", path_data);
                            }
                        }

                        // Print a newline between each pair for readibility
                        // purposes, But an extra new-line is not needed for the
                        // last pair

                        if (pair != paths_back) {
                            fputc('\n', stdout);
                        }
                    } else {
                        if (path_is_directory) {
                            fprintf(stderr, "Cannot open directory (at path %s) as a macho-file, use -r (or -r=) to recurse the directory\n", path_data);
                            return 1;
                        }

                        auto path_is_library_check_error = macho::file::check_error::ok;
                        const auto path_is_library = macho::file::is_valid_library(path, &path_is_library_check_error);

                        if (path_is_library_check_error == macho::file::check_error::failed_to_open_descriptor) {
                            // Instead of ignoring this failure, it is better to inform the user of the open
                            // failure to so they are aware of why a file may not have been parsed

                            fprintf(stderr, "Failed to open file (at path %s), failing with error (%s)\n", path.data(), strerror(errno));
                        } else {
                            // As the user provided only one path to a specific mach-o library file,
                            // --list-macho-libraries is expected to explicity print out whether or
                            // not the provided mach-o library file is valid.

                            if (path_is_library) {
                                fprintf(stdout, "Mach-o file (at path %s) is a library\n", path_data);
                            } else {
                                fprintf(stdout, "Mach-o file (at path %s) is not a library\n", path_data);
                            }
                        }
                    }
                }
            } else {
                // If a path was not provided, --list-macho-libraries is
                // expected to instead recurse the current-directory.

                const auto path = retrieve_current_directory();

                struct stat sbuf;
                if (stat(path, &sbuf) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object (at path %s), failing with error (%s)\n", path, strerror(errno));
                    return 1;
                }

                auto found_libraries = false;
                recurse::macho_library_paths(path, true, [&](std::string &library_path) {
                    found_libraries = true;
                    fprintf(stdout, "%s\n", library_path.data());
                });

                if (!found_libraries) {
                    fprintf(stderr, "No mach-o library files were found while recursing through path (%s)\n", path);
                }
            }

            return 0;
        } else if (strcmp(option, "list-platform") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_platforms();
            return 0;
        } else if (strcmp(option, "list-recurse") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            fputs("once, Recurse through all of a directory's files\n", stdout);
            fputs("all,  Recurse through all of a directory's files and sub-directories (default)\n", stdout);

            return 0;
        } else if (strcmp(option, "list-versions") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            fputs("v1\nv2 (default)\n", stdout);
            return 0;
        } else if (strcmp(option, "o") == 0 || strcmp(option, "output") == 0) {
            if (is_last_argument) {
                fputs("Please provide path(s) to output files\n", stderr);
                return 1;
            }

            auto should_maintain_directories = false;
            auto provided_output_path = false;

            // To parse options for the output command in the middle of an
            // argument list, while also keeping a similar argument and option
            // stype, the output option handles custom output options in between
            // the output option argument and the output path argument.

            for (i++; i < argc; i++) {
                const auto &argument = argv[i];
                const auto &argument_front = argument[0];

                if (argument_front == '-') {
                    auto option = &argument[1];
                    const auto &option_front = option[0];

                    if (!option_front) {
                        fputs("Please provide a valid option\n", stderr);
                        return 1;
                    }

                    if (option_front == '-') {
                        option++;
                    }

                    if (strcmp(option, "maintain-directories") == 0) {
                        should_maintain_directories = true;
                    } else {
                        fprintf(stderr, "Unrecognized option: %s\n", argument);
                        return 1;
                    }

                    continue;
                }

                auto path = std::string(argument);
                if (path != "stdout") {
                    const auto &path_front = path.front();
                    if (path_front != '/') {
                        // If the user-provided path-string does not begin with
                        // a forward slash, it is assumed that the path exists
                        // in the current-directory.

                        path.insert(0, retrieve_current_directory());
                    }
                }

                const auto tbds_size = tbds.size();
                if (output_paths_index >= tbds_size) {
                    fprintf(stderr, "No coresponding mach-o files for output-path (%s, at index %d)\n", path.data(), output_paths_index);
                    return 1;
                }

                auto &tbd = tbds.at(output_paths_index);
                auto &tbd_options = tbd.options;

                if (should_maintain_directories) {
                    tbd_options |= maintain_directories;
                }

                if (path == "stdout") {
                    if (tbd_options & recurse_directories) {
                        fprintf(stderr, "Can't output mach-o files found while recursing to stdout\n");
                        return 1;
                    }
                }

                struct stat sbuf;
                if (stat(path.data(), &sbuf) == 0) {
                    const auto path_is_directory = S_ISDIR(sbuf.st_mode);
                    if (path_is_directory) {
                        if (!(tbd_options & recurse_directories)) {
                            fprintf(stderr, "Cannot output tbd-file to directory (at path %s), please provide a path to a file to output to\n", path.data());
                            return 1;
                        }

                        const auto &path_back = path.back();
                        if (path_back != '/') {
                            path.append(1, '/');
                        }
                    } else {
                        const auto path_is_regular_file = S_ISREG(sbuf.st_mode);
                        if (path_is_regular_file) {
                            if (tbd_options & recurse_directories) {
                                fprintf(stderr, "Cannot output mach-o files found while recursing directory (at path %s) to file (at path %s). Please provide a directory to output tbd-files to\n", tbd.path.data(), path.data());
                                return 1;
                            }
                        }
                    }
                } else {
                    if (tbd_options & recurse_directories) {
                        const auto &path_back = path.back();
                        if (path_back != '/') {
                            path.append(1, '/');
                        }

                        if (access(path.data(), F_OK) != 0) {
                            // If an output-directory does not exist, it is expected
                            // to be created.

                            recursively_create_directories_from_file_path(path.data(), true);
                        }
                    }
                }

                tbd.output_path = path;
                provided_output_path = true;

                break;
            }

            // To support the current format of providing output options,
            // a single output option only supports a single output-path

            if (!provided_output_path) {
                fputs("Please provide path(s) to output files\n", stderr);
                return 1;
            }

            output_paths_index++;
        } else if (strcmp(option, "p") == 0 || strcmp(option, "path") == 0) {
            if (is_last_argument) {
                fputs("Please provide path(s) to mach-o files\n", stderr);
                return 1;
            }

            // To parse options for the path command in the middle of an
            // argument list, while also keeping a similar argument and option
            // stype, the path option handles custom output options in between
            // the path option argument and the mach-o library path argument.

            auto local_architectures = uint64_t();
            auto local_architecture_overrides = uint64_t();

            auto local_options = 0;
            auto local_platform = tbd::platform::none;
            auto local_tbd_version = (enum tbd::version)0;

            for (i++; i < argc; i++) {
                const auto &argument = argv[i];
                const auto &argument_front = argument[0];

                if (argument_front == '-') {
                    auto option = &argument[1];
                    const auto &option_front = option[0];

                    if (!option_front) {
                        fputs("Please provide a valid option\n", stderr);
                        return 1;
                    }

                    if (option_front == '-') {
                        option++;
                    }

                    const auto is_last_argument = i == argc - 1;
                    if (strcmp(option, "a") == 0 || strcmp(option, "arch") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                            return 1;
                        }

                        i++;
                        parse_architectures_list(local_architectures, i, argc, argv);
                    } else if (strcmp(option, "archs") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                            return 1;
                        }

                        i++;
                        parse_architectures_list(local_architecture_overrides, i, argc, argv);
                    } else if (strcmp(option, "allow-all-private-symbols") == 0) {
                        local_options |= tbd::symbol_options::allow_all_private_symbols;
                    } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
                        local_options |= tbd::symbol_options::allow_private_normal_symbols;
                    } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
                        local_options |= tbd::symbol_options::allow_private_weak_symbols;
                    } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
                        local_options |= tbd::symbol_options::allow_private_objc_symbols;
                    } else if (strcmp(option, "allow-private-objc-classes") == 0) {
                        local_options |= tbd::symbol_options::allow_private_objc_classes;
                    } else if (strcmp(option, "allow-private-objc-ivars") == 0) {
                        local_options |= tbd::symbol_options::allow_private_objc_ivars;
                    } else if (strcmp(option, "p") == 0) {
                        fprintf(stderr, "Please provide a path for option (%s)\n", argument);
                        return 1;
                    } else if (strcmp(option, "platform") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a platform-string. Run --list-platform to see a list of platforms\n", stderr);
                            return 1;
                        }

                        i++;

                        const auto &platform_string = argv[i];
                        local_platform = tbd::string_to_platform(platform_string);

                        if (local_platform == tbd::platform::none) {
                            fprintf(stderr, "Platform-string (%s) is invalid\n", platform_string);
                            return 1;
                        }
                    } else if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                        local_options |= recurse_directories | recurse_subdirectories;
                    } else if (strncmp(option, "r=", 2) == 0 || strncmp(option, "recurse=", 8) == 0) {
                        const auto recurse_type_string = strchr(option, '=') + 1;
                        const auto &recurse_type_string_front = recurse_type_string[0];

                        if (!recurse_type_string_front) {
                            fputs("Please provide a recurse type\n", stderr);
                            return 1;
                        }

                        local_options |= recurse_directories;

                        if (strcmp(recurse_type_string, "all") == 0) {
                            local_options |= recurse_subdirectories;
                        } else if (strcmp(recurse_type_string, "once") != 0) {
                            fprintf(stderr, "Unrecognized recurse-type (%s)\n", recurse_type_string);
                            return 1;
                        }
                    } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a tbd-version\n", stderr);
                            return 1;
                        }

                        i++;

                        const auto &version_string = argv[i];

                        local_tbd_version = tbd::string_to_version(version_string);
                        if (local_tbd_version == (enum tbd::version)0) {
                            fprintf(stderr, "(%s) is not a valid tbd-version\n", version_string);
                            return 1;
                        }
                    } else {
                        fprintf(stderr, "Unrecognized argument: %s\n", argument);
                        return 1;
                    }

                    continue;
                }

                auto path = std::string(argument);
                auto &path_front = path.front();

                // If the user-provided path-string does not begin with
                // a forward slash, it is assumed that the path exists
                // in the current-directory

                if (path_front != '/') {
                    path.insert(0, retrieve_current_directory());
                }

                struct stat sbuf;
                if (stat(path.data(), &sbuf) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object (at path %s), failing with error (%s)\n", path.data(), strerror(errno));
                    return 1;
                }

                auto tbd = tbd_file({});

                const auto path_is_directory = S_ISDIR(sbuf.st_mode);
                if (path_is_directory) {
                    if (!(local_options & recurse_directories)) {
                        fprintf(stderr, "Cannot open directory (at path %s) as a macho-file, use -r to recurse the directory\n", path.data());
                        return 1;
                    }

                    const auto &path_back = path.back();
                    if (path_back != '/') {
                        path.append(1, '/');
                    }
                } else {
                    const auto path_is_regular_file = S_ISREG(sbuf.st_mode);
                    if (path_is_regular_file) {
                        if (local_options & recurse_directories) {
                            fprintf(stderr, "Cannot recurse file (at path %s)\n", path.data());
                            return 1;
                        }

                        tbd.path = path;
                    } else {
                        fprintf(stderr, "Object (at path %s) is not a regular file\n", path.data());
                        return 1;
                    }
                }

                tbd.path = path;

                auto tbd_platform = local_platform;
                auto tbd_version = local_tbd_version;

                tbd.architectures = local_architectures;
                tbd.architecture_overrides = local_architecture_overrides;

                tbd.options = local_options;
                tbd.platform = tbd_platform;
                tbd.version = tbd_version;

                tbds.emplace_back(tbd);

                // Clear the local option fields to signal that a
                // path was provided

                local_architectures = 0;

                local_options = 0;
                local_platform = tbd::platform::none;
                local_tbd_version = (enum tbd::version)0;

                break;
            }

            // It is expected for --path to error out if the user has
            // not provided a path to a mach-o library path or to a
            // directory where some could be found

            if (local_architectures != 0 || local_architecture_overrides != 0 || local_platform != tbd::platform::none || local_options != 0 || local_tbd_version != (enum tbd::version)0) {
                fputs("Please provide a path to a directory to recurse through\n", stderr);
                return 1;
            }
        } else if (strcmp(option, "platform") == 0) {
            if (is_last_argument) {
                fputs("Please provide a platform-string. Run --list-platform to see a list of platforms\n", stderr);
                return 1;
            }

            i++;

            const auto &platform_string = argv[i];
            platform = tbd::string_to_platform(platform_string);

            if (platform == tbd::platform::none) {
                fprintf(stderr, "Platform-string (%s) is invalid\n", platform_string);
                return 1;
            }
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_usage();
            return 0;
        } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
            if (is_last_argument) {
                fputs("Please provide a tbd-version\n", stderr);
                return 1;
            }

            i++;

            const auto &version_string = argv[i];
            const auto &version_string_front = version_string[0];

            if (version_string_front == '-') {
                fputs("Please provide a tbd-version\n", stderr);
                return 1;
            }

            if (strcmp(version_string, "v1") == 0) {
                version = tbd::version::v1;
            } else if (strcmp(version_string, "v2") != 0) {
                fprintf(stderr, "tbd-version (%s) is invalid\n", version_string);
                return 1;
            }
        } else {
            fprintf(stderr, "Unrecognized argument: %s\n", argument);
            return 1;
        }
    }

    const auto tbds_size = tbds.size();
    if (!tbds_size) {
        fputs("No mach-o files have been provided\n", stderr);
        return 1;
    }

    auto should_print_paths = true;
    if (tbds.size() < 2) {
        const auto &tbd = tbds.front();
        const auto &tbd_options = tbd.options;

        if (!(tbd_options & recurse_directories)) {
            should_print_paths = false;
        }
    }

    for (auto &tbd : tbds) {
        auto &tbd_path = tbd.path;
        auto &tbd_output_path = tbd.output_path;

        auto &tbd_options = tbd.options;

        if (options & tbd::symbol_options::allow_all_private_symbols) {
            tbd_options |= tbd::symbol_options::allow_all_private_symbols;
        } else {
            if (options & tbd::symbol_options::allow_private_normal_symbols) {
                tbd_options |= tbd::symbol_options::allow_private_normal_symbols;
            }

            if (options & tbd::symbol_options::allow_private_weak_symbols) {
                tbd_options |= tbd::symbol_options::allow_private_weak_symbols;
            }

            if (options & tbd::symbol_options::allow_private_objc_symbols) {
                tbd_options |= tbd::symbol_options::allow_private_objc_symbols;
            } else {
                if (options & tbd::symbol_options::allow_private_objc_classes) {
                    tbd_options |= tbd::symbol_options::allow_private_objc_classes;
                }

                if (options & tbd::symbol_options::allow_private_objc_ivars) {
                    tbd_options |= tbd::symbol_options::allow_private_objc_ivars;
                }
            }
        }

        if (tbd_options & recurse_directories) {
            if (tbd_output_path.empty()) {
                fprintf(stderr, "Cannot output mach-o files found while recursing directory (at path %s) to stdout. Please provide a directory to output tbd-files to\n", tbd_path.data());
                exit(1);
            }

            const auto tbd_path_length = tbd_path.length();
            const auto tbd_output_path_length = tbd_output_path.length();

            auto outputted_any_macho_libraries = false;

            recurse::macho_libraries(tbd_path.data(), tbd_options & recurse_subdirectories, [&](std::string &library_path, macho::file &file) {
                auto output_path_front = std::string::npos;
                if (tbd_options & maintain_directories) {
                    output_path_front = tbd_path_length;
                } else {
                    output_path_front = library_path.find_last_of('/');
                }

                auto output_path = library_path.substr(output_path_front);

                auto output_path_length = output_path.length();
                auto output_path_new_length = output_path_length + tbd_output_path_length + 4;

                output_path.reserve(output_path_new_length);

                output_path.insert(0, tbd_output_path);
                output_path.append(".tbd");

                auto creation_start_location = recursively_create_directories_from_file_path(output_path.data(), false);
                auto output_file = fopen(output_path.data(), "w");

                if (!output_file) {
                    fprintf(stderr, "Failed to open file (at path %s) for writing, failing with error (%s)\n", output_path.data(), strerror(errno));
                    return;
                }

                auto result = create_tbd_file(library_path.data(), file, output_path.data(), output_file, tbd.options, platform != tbd::platform::none ? platform : tbd.platform, version != (enum tbd::version)0 ? version : tbd.version, !tbd.architectures ? architectures : tbd.architectures, !tbd.architecture_overrides ? architecture_overrides : tbd.architecture_overrides, creation_handling_print_paths);
                if (!result) {
                    auto output_path_directory_end_index = (uint64_t)strrchr(output_path.data(), '/') - (uint64_t)output_path.data();
                    recursively_remove_directories_from_file_path(output_path.data(), creation_start_location, output_path_directory_end_index);
                }

                fclose(output_file);
                outputted_any_macho_libraries = true;
            });

            if (!outputted_any_macho_libraries) {
                if (tbd_options & recurse_subdirectories) {
                    fprintf(stderr, "No mach-o files were found for outputting while recursing through directory (at path %s)\n", tbd_path.data());
                } else {
                    fprintf(stderr, "No mach-o files were found for outputting while recursing once through directory (at path %s)\n", tbd_path.data());
                }
            }
        } else {
            auto output_file = stdout;

            auto output_file_path = (const char *)nullptr;
            auto recursive_directory_creation_index = (size_t)0;

            if (!tbd_output_path.empty()) {
                recursive_directory_creation_index = recursively_create_directories_from_file_path(tbd_output_path.data(), false);

                output_file_path = tbd_output_path.data();
                output_file = fopen(tbd_output_path.data(), "w");

                if (!output_file) {
                    if (should_print_paths) {
                        fprintf(stderr, "Failed to open file (at path %s) for writing, failing with error (%s)\n", tbd_output_path.data(), strerror(errno));
                    } else {
                        fprintf(stderr, "Failed to open file at provided output-path for writing, failing with error (%s)\n", strerror(errno));
                    }

                    continue;
                }
            }

            auto library_file = macho::file();
            auto library_file_open_result = macho::file::open_from_library(&library_file, tbd_path.data());

            if (library_file_open_result == macho::file::open_result::failed_to_open_stream) {
                if (should_print_paths) {
                    fprintf(stderr, "Failed to open file (at path %s) for reading, failing with error (%s)\n", tbd_path.data(), strerror(errno));
                } else {
                    fprintf(stderr, "Failed to open file at provided path for reading, failing with error (%s)\n", strerror(errno));
                }

                continue;
            }

            const auto result = create_tbd_file(tbd_path.data(), library_file, output_file_path, output_file, tbd.options, platform != tbd::platform::none ? platform : tbd.platform, version != (enum tbd::version)0 ? version : tbd.version, !tbd.architectures ? architectures : tbd.architectures, !tbd.architecture_overrides ? architecture_overrides : tbd.architecture_overrides, creation_handling_print_paths);
            if (!result) {
                if (!tbd_output_path.empty()) {
                    auto output_path_directory_end_index = (uint64_t)strrchr(tbd_output_path.data(), '/') - (uint64_t)tbd_output_path.data();
                    recursively_remove_directories_from_file_path(tbd_output_path.data(), recursive_directory_creation_index, output_path_directory_end_index);
                }
            }

            if (output_file != stdout) {
                fclose(output_file);
            }
        }
    }
}
