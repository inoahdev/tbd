//
//  src/main.cc
//  tbd
//
//  Created by inoahdev on 4/16/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <sys/stat.h>
#include <iostream>

#include <unistd.h>

#include "misc/path_utilities.h"
#include "misc/recursively.h"
#include "misc/recurse.h"

#include "mach-o/utils/tbd/tbd.h"

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
            fprintf(stderr, "Failed to get current working-directory, failing with error: %s\n", strerror(errno));
            exit(1);
        }

        const auto current_directory_length = strlen(current_directory_string);
        const auto &current_directory_back = current_directory_string[current_directory_length - 1];

        if (current_directory_back != '/' && current_directory_back != '\\') {
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

        if (architecture_string_front == '-' || architecture_string_front == '/' || architecture_string_front == '\\') {
            // If the architectures vector is empty, the user did not provide any architectures
            // but did provided the architecture option, which requires at least one architecture
            // being provided.

            if (!architectures) {
                fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                exit(1);
            }

            break;
        }

        const auto architecture_info_table_index = macho::architecture_info_index_from_name(architecture_string);
        if (architecture_info_table_index == -1) {
            // macho::architecture_info_from_name() returning nullptr can be the result of one
            // of two scenarios, The string stored in architecture being invalid,
            // such as being misspelled, or the string being the path object inevitably
            // following the architecture argument.

            // If the architectures vector is empty, the user did not provide any architectures
            // but did provide the architecture option, which requires at least one architecture
            // being provided.

            if (!architectures) {
                fprintf(stderr, "Unrecognized architecture with name (%s)\n", macho::get_architecture_info_table()[architecture_info_table_index].name);
                exit(1);
            }

            break;
        }

        architectures |= (static_cast<uint64_t>(1) << architecture_info_table_index);
        index++;
    }

    // As the caller of this function is in a for loop,
    // the index is incremented once again once this function
    // returns. To make up for this, decrement index by 1.

    index--;
}

void print_platforms() {
    auto platform_number = 1;
    auto platform = macho::utils::tbd::platform_to_string((enum macho::utils::tbd::platform)platform_number);

    fputs(platform, stdout);

    while (true) {
        platform_number++;
        platform = macho::utils::tbd::platform_to_string((enum macho::utils::tbd::platform)platform_number);

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
    creation_handling_dont_print_warnings = 1 << 2
};

bool create_tbd_file(const char *macho_file_path, macho::file &file, const char *tbd_file_path, FILE *tbd_file, uint64_t options, macho::utils::tbd::flags flags, macho::utils::tbd::objc_constraint constraint, macho::utils::tbd::platform platform, macho::utils::tbd::version version, uint64_t architectures, uint64_t architecture_overrides, uint64_t creation_handling_options) {
    auto result = macho::utils::tbd::create_from_macho_library(file, tbd_file, options, flags, constraint, platform, version, architectures, architecture_overrides);
    if (result == macho::utils::tbd::creation_result::platform_not_found || result == macho::utils::tbd::creation_result::platform_not_supported || result == macho::utils::tbd::creation_result::unrecognized_platform ||  result == macho::utils::tbd::creation_result::multiple_platforms) {
        switch (result) {
            case macho::utils::tbd::creation_result::platform_not_found:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stdout, "Failed to find platform in mach-o library (at path %s). ", macho_file_path);
                } else {
                    fputs("Failed to find platform in provided mach-o library. ", stdout);
                }

                break;

            case macho::utils::tbd::creation_result::platform_not_supported:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stdout, "Platform in mach-o library (at path %s) is unsupported. ", macho_file_path);
                } else {
                    fputs("Platform in provided mach-o library is unsupported. ", stdout);
                }

                break;

            case macho::utils::tbd::creation_result::unrecognized_platform:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stdout, "Platform in mach-o library (at path %s) is unrecognized. ", macho_file_path);
                } else {
                    fputs("Platform in provided mach-o library is unrecognized. ", stdout);
                }

                break;

            case macho::utils::tbd::creation_result::multiple_platforms:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stdout, "Multiple platforms found in mach-o library (at path %s). ", macho_file_path);
                } else {
                    fputs("Multiple platforms found in provided mach-o library. ", stdout);
                }

                break;

            default:
                break;
        }

        auto new_platform = macho::utils::tbd::platform::none;
        auto platform_string = std::string();

        do {
            fputs("Please provide a replacement platform (Input --list-platform to see a list of platforms): ", stdout);
            getline(std::cin, platform_string);

            if (platform_string == "--list-platform") {
                print_platforms();
            } else {
                new_platform = macho::utils::tbd::platform_from_string(platform_string.data());
            }
        } while (new_platform == macho::utils::tbd::platform::none);

        result = macho::utils::tbd::create_from_macho_library(file, tbd_file, options, flags, constraint, platform, version, architectures, architecture_overrides);
    }

    if (!(creation_handling_options & creation_handling_dont_print_warnings)) {
        switch (result) {
            case macho::utils::tbd::creation_result::ok:
                return true;

            case macho::utils::tbd::creation_result::stream_seek_error:
            case macho::utils::tbd::creation_result::stream_read_error:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Failed to read file-stream while mach-o file (at path %s), failing with error (%s)\n", macho_file_path, strerror(ferror(file.stream)));
                } else {
                    fprintf(stderr, "Failed to read file-stream while parsing provided mach-o file, failing with error (%s)\n", strerror(ferror(file.stream)));
                }

                break;

            case macho::utils::tbd::creation_result::unsupported_filetype:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, is of an unsupported filetype\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, is of an unsupported filetype\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::inconsistent_flags:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has inconsistent information between its architectures\n", macho_file_path);
                } else {
                    fputs("Provided mch-o file has inconsistent information between its architectures\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::invalid_subtype:
            case macho::utils::tbd::creation_result::invalid_cputype:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, is for an unsupported machine\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, is for an unsupported machine\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::invalid_load_command:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has an invalid load-command\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, has an invalid load-command\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::invalid_segment:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has an invalid segment\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, has an invalid segment\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::invalid_sub_client:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has an invalid sub-client command\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, has an invalid sub-client command\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::invalid_sub_umbrella:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has an invalid sub-umbrella command\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, has an invalid sub-umbrella command\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::failed_to_iterate_load_commands:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Failed to iterate through mach-o file (at path %s), or one of its architecture's load-commands\n", macho_file_path);
                } else {
                    fputs("Failed to iterate through provided mach-o file, or one of its architecture's load-commands\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::failed_to_iterate_symbols:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Failed to iterate through mach-o file (at path %s), or one of its architecture's symbols\n", macho_file_path);
                } else {
                    fputs("Failed to iterate through provided mach-o file, or one of its architecture's symbols\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::contradictary_load_command_information:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has multiple load-commands of the same type with contradictory information\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, has multiple load-commands of the same type with contradictory information\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::empty_installation_name:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, has an empty installation-name\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, has an empty installation-name\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::uuid_is_not_unique:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "One of mach-o file (at path %s)'s architectures has a uuid that is not unique from other architectures\n", macho_file_path);
                } else {
                    fputs("One of provided mach-o file's architectures has a uuid that is not unique from other architectures\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::platform_not_found:
            case macho::utils::tbd::creation_result::platform_not_supported:
            case macho::utils::tbd::creation_result::unrecognized_platform:
            case macho::utils::tbd::creation_result::multiple_platforms:
                break;

            case macho::utils::tbd::creation_result::not_a_library:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, is not a mach-o library\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, is not a mach-o library\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::has_no_uuid:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s), or one of its architectures, does not have a uuid\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file, or one of its architectures, does not have a uuid\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::contradictary_container_information:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has information in architectures contradicting the same information in other architectures\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file has information in architectures contradicting the same information in other architectures\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::no_provided_architectures:
                if (!(creation_handling_options & creation_handling_ignore_no_provided_architectures)) {
                    if (creation_handling_options & creation_handling_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) does not have architectures provided to output tbd from\n", macho_file_path);
                    } else {
                        fputs("Provided mach-o file does not have architectures provided to output tbd from\n", stderr);
                    }
                }

                break;

            case macho::utils::tbd::creation_result::failed_to_allocate_memory:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Failed to allocate memory necessary for operating on mach-o file (at path %s)\n", macho_file_path);
                } else {
                    fputs("Failed to allocate memory necessary for operating on mach-o file at provided path\n", stderr);
                }

                break;

            case macho::utils::tbd::creation_result::no_symbols_or_reexports:
                if (creation_handling_options & creation_handling_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) does not have any symbols or reexports to be outputted\n", macho_file_path);
                } else {
                    fputs("Provided mach-o file does not have any symbols or reexports to be outputted\n", stderr);
                }

                break;
        }

        return false;
    }

    return true;
}

void print_usage() {
    fputs("Usage: tbd [-p file-paths] [-o/-output output-paths-or-stdout]\n", stdout);
    fputs("Main options:\n", stdout);
    fputs("    -h, --help,     Print this message\n", stdout);
    fputs("    -o, --output,   Path(s) to output file(s) to write converted .tbd. If provided file(s) already exists, contents will be overridden. Can also provide \"stdout\" to print to stdout\n", stdout);
    fputs("    -p, --path,     Path(s) to mach-o file(s) to convert to a .tbd. Can also provide \"stdin\" to use stdin\n", stdout);
    fputs("    -u, --usage,    Print this message\n", stdout);

    fputc('\n', stdout);
    fputs("Path options:\n", stdout);
    fputs("Usage: tbd -p [-a/--arch architectures] [--archs architecture-overrides] [--platform platform] [-r/--recurse/ -r=once/all / --recurse=once/all] [-v/--version v1/v2] /path/to/macho/library\n", stdout);
    fputs("    -a, --arch,     Specify architecture(s) to output to tbd\n", stdout);
    fputs("        --archs,    Specify architecture(s) to use, instead of the ones in the provided mach-o file(s)\n", stdout);
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
    fputs("    -v, --version,  Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tbd-version was not provided)\n", stdout);

    fputc('\n', stdout);
    fputs("Miscellaneous options:\n", stdout);
    fputs("        --dont-print-warnings,    Don't print any warnings (both path and global option)\n", stdout);
    fputs("        --replace-path-extension, Replace path-extension on provided mach-o file(s) when creating an output-file (Replace instead of appending .tbd) (both path and global option)\n", stdout);

    fputc('\n', stdout);
    fputs("Symbol options: (Both path and global options)\n", stdout);
    fputs("        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-normal-symbols, Allow all non-external symbols (of no type) (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars\n", stdout);
    fputs("        --allow-private-objc-classes,   Allow all non-external objc-classes\n", stdout);
    fputs("        --allow-private-objc-ivars,     Allow all non-external objc-ivars\n", stdout);

    fputc('\n', stdout);
    fputs("tbd field options: (Both path and global options)\n", stdout);
    fputs("        --flags,                  Specify flags to add onto ones found in provided mach-o file(s)\n", stdout);
    fputs("        --objc-constraint,        Specify objc-constraint to use instead of one(s) found in provided mach-o file(s)\n", stdout);
    fputs("        --remove-flags,           Remove flags field from outputted tbds\n", stdout);
    fputs("        --remove-objc-constraint, Remove objc-constraint field from outputted tbds\n", stdout);

    fputc('\n', stdout);
    fputs("List options:\n", stdout);
    fputs("        --list-architectures,    List all valid architectures for .tbd files. Also able to list architectures of a provided mach-o file\n", stdout);
    fputs("        --list-tbd-flags,        List all valid flags for .tbd files\n", stdout);
    fputs("        --list-macho-libraries,  List all valid mach-o libraries in current-directory (or at provided path(s))\n", stdout);
    fputs("        --list-objc-constraints, List all valid objc-constraint options for .tbd files\n", stdout);
    fputs("        --list-platform,         List all valid platforms\n", stdout);
    fputs("        --list-recurse,          List all valid recurse options for parsing directories\n", stdout);
    fputs("        --list-versions,         List all valid versions for .tbd files\n", stdout);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fputs("Please run -h or -u to see a list of options\n", stderr);
        return 1;
    }

    enum misc_options : uint64_t {
        recurse_directories    = 1 << 8, // Use second-byte to support native tbd options
        recurse_subdirectories = 1 << 9,
        maintain_directories   = 1 << 10,
        dont_print_warnings    = 1 << 11,
        replace_path_extension = 1 << 12
    };

    typedef struct tbd_file {
        std::string path;
        std::string output_path;

        uint64_t architectures;
        uint64_t architecture_overrides;

        enum macho::utils::tbd::flags flags;
        enum macho::utils::tbd::objc_constraint constraint;
        enum macho::utils::tbd::platform platform;
        enum macho::utils::tbd::version version;

        uint64_t options;
    } tbd_file;

    uint64_t architectures = 0;
    uint64_t architecture_overrides = 0;

    auto tbds = std::vector<tbd_file>();
    auto output_paths_index = 0;

    auto options = uint64_t();

    auto flags = macho::utils::tbd::flags::none;
    auto objc_constraint = macho::utils::tbd::objc_constraint::no_value;
    auto platform = macho::utils::tbd::platform::none;
    auto version = macho::utils::tbd::version::v2;

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
            options |= macho::utils::tbd::options::allow_all_private_symbols;
        } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
            options |= macho::utils::tbd::options::allow_private_normal_symbols;
        } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
            options |= macho::utils::tbd::options::allow_private_weak_symbols;
        } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
            options |= macho::utils::tbd::options::allow_private_objc_symbols;
        } else if (strcmp(option, "allow-private-objc-classes") == 0) {
            options |= macho::utils::tbd::options::allow_private_objc_classes;
        } else if (strcmp(option, "allow-private-objc-ivars") == 0) {
            options |= macho::utils::tbd::options::allow_private_objc_ivars;
        } else if (strcmp(option, "dont-print-warnings") == 0) {
            options |= dont_print_warnings;
        } else if (strcmp(option, "flags") == 0) {
            if (is_last_argument) {
                fputs("Please provide a list of flags to add onto ones found in provided mach-o file(s)\n", stderr);
                return 1;
            }

            auto j = ++i;
            for (; j < argc; j++) {
                const auto argument = argv[j];
                if (strcmp(argument, "flat_namespace") == 0) {
                    flags |= macho::utils::tbd::flags::flat_namespace;
                } else if (strcmp(argument, "not_app_extension_safe") == 0) {
                    flags |= macho::utils::tbd::flags::not_app_extension_safe;
                } else {
                    if (j == i) {
                        fputs("Please provide a list of flags to add onto ones found in provided mach-o file(s)\n", stderr);
                        return 1;
                    }

                    j--;
                    break;
                }
            }

            i = j;
        } else if (strcmp(option, "list-architectures") == 0) {
            if (!is_first_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            if (is_last_argument) {
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
            } else {
                i++;

                if (i + 2 <= argc) {
                    fprintf(stderr, "Unrecognized argument: %s\n", argv[i + 1]);
                    return 1;
                }

                auto path = std::string(argv[i]);
                if (const auto &path_front = path.front(); path_front != '/' && path_front != '\\') {
                    path.insert(0, retrieve_current_directory());
                }

                auto macho_file = macho::file();
                auto macho_file_open_result = macho_file.open(path);

                switch (macho_file_open_result) {
                    case macho::file::open_result::ok:
                        break;

                    case macho::file::open_result::failed_to_open_stream:
                        fprintf(stderr, "Failed to open file at provided path for reading, failing with error: %s\n", strerror(errno));
                        return 1;

                    case macho::file::open_result::failed_to_allocate_memory:
                        fputs("Failed to allocate memory necessary for operating on file at provided path\n", stderr);
                        return 1;

                    case macho::file::open_result::stream_seek_error:
                    case macho::file::open_result::stream_read_error:
                        fprintf(stderr, "Encountered an error while reading through file at provided path, likely not a valid mach-o. Reading failed with error: %s\n", strerror(ferror(macho_file.stream)));
                        return 1;

                    case macho::file::open_result::zero_architectures:
                        fputs("Fat mach-o file at provided path does not have any architectures\n", stderr);
                        return 1;

                    case macho::file::open_result::invalid_container:
                        fputs("Mach-o file at provided path is invalid\n", stderr);
                        return 1;

                    case macho::file::open_result::not_a_macho:
                        fputs("File at provided path is not a valid mach-o\n", stderr);
                        return 1;

                    case macho::file::open_result::not_a_library:
                    case macho::file::open_result::not_a_dynamic_library:
                        break;
                }

                auto architecture_names = std::vector<const char *>();
                architecture_names.reserve(macho_file.containers.size());

                for (const auto &container : macho_file.containers) {
                    const auto container_subtype = macho::subtype_from_cputype(container.header.cputype, container.header.cpusubtype);
                    const auto container_arch_info = macho::architecture_info_from_cputype(container.header.cputype, container_subtype);

                    if (!container_arch_info) {
                        fputs("Mach-o file at provided path has unknown architectures\n", stderr);
                        return 1;
                    }

                    architecture_names.emplace_back(container_arch_info->name);
                }

                fputs(architecture_names.front(), stdout);
                for (auto architecture_names_iter = architecture_names.begin() + 1; architecture_names_iter != architecture_names.end(); architecture_names_iter++) {
                    fprintf(stdout, ", %s", *architecture_names_iter);
                }

                fputc('\n', stdout);
            }

            return 0;
        } else if (strcmp(option, "list-tbd-flags") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            fputs("flat_namespace\nnot_app_extension_safe\n", stdout);
            return 0;
        } else if (strcmp(option, "list-macho-libraries") == 0) {
            if (!is_first_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            if (!is_last_argument) {
                auto paths = std::vector<std::pair<std::string, uint64_t>>();
                auto options = uint64_t();

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

                        if (strcmp(option, "dont-print-warnings") == 0) {
                            options |= dont_print_warnings;
                        } else if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
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
                                fprintf(stderr, "Unrecognized recurse-type: %s\n", recurse_type_string);
                                return 1;
                            }
                        } else {
                            fprintf(stderr, "Unrecognized argument: %s\n", argument);
                            return 1;
                        }

                        continue;
                    }

                    if (argument_front != '/' && argument_front != '\\') {
                        auto path = std::string();
                        auto current_directory = retrieve_current_directory();

                        const auto argument_length = strlen(argument);
                        const auto current_directory_length = strlen(current_directory);
                        const auto path_length = argument_length + current_directory_length;

                        path.reserve(path_length);
                        path.append(current_directory);
                        path.append(argument);

                        paths.emplace_back(std::move(path::clean(path)), options);
                    } else {
                        auto path = std::string(argument);
                        paths.emplace_back(path::clean(path), options);
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
                        fprintf(stderr, "Failed to retrieve information on object (at path %s), failing with error: %s\n", path_data, strerror(errno));
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
                        auto recurse_options = uint64_t();

                        if (!(options & dont_print_warnings)) {
                            recurse_options |= recurse::options::print_warnings;
                        }

                        if (options & recurse_subdirectories) {
                            recurse_options |= recurse::options::recurse_subdirectories;
                        }

                        auto recursion_result = recurse::macho_library_paths(path_data, recurse_options, [&](std::string &library_path) {
                            found_libraries = true;
                            fprintf(stdout, "%s\n", library_path.data());
                        });

                        switch (recursion_result) {
                            case recurse::operation_result::ok: {
                                if (!found_libraries) {
                                    if (options & recurse_subdirectories) {
                                        fprintf(stderr, "No mach-o library files were found while recursing through path (%s)\n", path_data);
                                    } else {
                                        fprintf(stderr, "No mach-o library files were found while recursing once through path (%s)\n", path_data);
                                    }
                                }

                                break;
                            }

                            case recurse::operation_result::failed_to_open_directory:
                                fprintf(stderr, "Warning: Failed to open directory (at path %s) for recursing, failing with error: %s\n", path_data, strerror(errno));
                                break;
                        }

                        // Print a newline between each pair for readability
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

                            fprintf(stderr, "Failed to open file (at path %s), failing with error: %s\n", path.data(), strerror(errno));
                        } else {
                            // As the user provided only one path to a specific mach-o library file,
                            // --list-macho-libraries is expected to explicitly print out whether or
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
                    fprintf(stderr, "Failed to retrieve information on current-directory (at path %s), failing with error: %s\n", path, strerror(errno));
                    return 1;
                }

                auto found_libraries = false;
                auto recursion_result = recurse::macho_library_paths(path, recurse::options::print_warnings | recurse::options::recurse_subdirectories, [&](std::string &library_path) {
                    found_libraries = true;
                    fprintf(stdout, "%s\n", library_path.data());
                });

                switch (recursion_result) {
                    case recurse::operation_result::ok:
                        if (!found_libraries) {
                            fprintf(stderr, "No mach-o library files were found while recursing through path (%s)\n", path);
                        }

                        break;

                    case recurse::operation_result::failed_to_open_directory:
                        fprintf(stderr, "Failed to open directory (at path %s) for recursing, failing with error: %s\n", path, strerror(errno));
                        return 1;
                }
            }

            return 0;
        } else if (strcmp(option, "list-objc-constraints") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            auto objc_constraint_integer = uint64_t(1);
            auto objc_constraint_string = macho::utils::tbd::objc_constraint_to_string(macho::utils::tbd::objc_constraint(objc_constraint_integer));

            while (objc_constraint_string != nullptr) {
                fprintf(stdout, "%s\n", objc_constraint_string);

                objc_constraint_integer++;
                objc_constraint_string = macho::utils::tbd::objc_constraint_to_string(macho::utils::tbd::objc_constraint(objc_constraint_integer));
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

            auto output_options = uint64_t();
            auto provided_output_path = false;

            // To parse options for the output command in the middle of an
            // argument list, while also keeping a similar argument and option
            // type, the output option handles custom output options in between
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
                        output_options |= maintain_directories;
                    } else {
                        fprintf(stderr, "Unrecognized option: %s\n", argument);
                        return 1;
                    }

                    continue;
                }

                auto path = std::string(argument);
                if (path != "stdout") {
                    path::clean(path);

                    if (const auto &path_front = path.front(); path_front != '/' && path_front != '\\') {
                        // If the user-provided path-string does not begin with
                        // a forward slash, it is assumed that the path exists
                        // in the current-directory.

                        path.insert(0, retrieve_current_directory());
                    }
                }

                const auto tbds_size = tbds.size();
                if (output_paths_index >= tbds_size) {
                    fprintf(stderr, "No corresponding mach-o files for output-path (%s, at index %d)\n", path.data(), output_paths_index);
                    return 1;
                }

                auto &tbd = tbds.at(output_paths_index);
                auto &tbd_options = tbd.options;

                if (output_options & maintain_directories) {
                    if (!(tbd_options & recurse_directories)) {
                        fprintf(stderr, "Option (--maintain-directories) for file (at path %s) can only be provided when recursing a directory\n", tbd.path.data());
                        return 1;
                    }

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
                            fprintf(stderr, "Cannot output a .tbd to directory (at path %s), please provide a path to a file to output to\n", path.data());
                            return 1;
                        }

                        if (const auto &path_back = path.back(); path_back != '/' && path_back != '\\') {
                            path.append(1, '/');
                        }
                    } else {
                        const auto path_is_regular_file = S_ISREG(sbuf.st_mode);
                        if (path_is_regular_file) {
                            if (tbd_options & recurse_directories) {
                                fprintf(stderr, "Cannot output mach-o files found while recursing directory (at path %s) to file (at path %s). Please provide a directory to output .tbd files to\n", tbd.path.data(), path.data());
                                return 1;
                            }
                        }
                    }
                } else {
                    if (tbd_options & recurse_directories) {
                        if (const auto &path_back = path.back(); path_back != '/' && path_back != '\\') {
                            path.append(1, '/');
                        }

                        // If an output-directory does not exist, it is expected
                        // to be created.

                        recursively_create_directories_from_file_path_creating_last_as_directory(path.data(), 0);
                    }
                }

                tbd.output_path = std::move(path);
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
        } else if (strcmp(option, "objc-constraint") == 0) {
            if (is_last_argument) {
                fputs("Please provide an objc-constraint\n", stderr);
                return 1;
            }

            i++;

            const auto objc_constraint_string = argv[i];
            const auto objc_constraint_inner = macho::utils::tbd::objc_constraint_from_string(objc_constraint_string);

            if (objc_constraint_inner == macho::utils::tbd::objc_constraint::no_value) {
                fprintf(stderr, "Unrecognized objc-constraint value provided (%s)\n", objc_constraint_string);
                return 1;
            }

            objc_constraint = objc_constraint_inner;
        } else if (strcmp(option, "p") == 0 || strcmp(option, "path") == 0) {
            if (is_last_argument) {
                fputs("Please provide path(s) to mach-o files\n", stderr);
                return 1;
            }

            // To parse options for the path command in the middle of an
            // argument list, while also keeping a similar argument and options
            // type, the path option handles custom output options in between
            // the path option argument and the mach-o library path argument.

            uint64_t local_architectures = 0;
            uint64_t local_architecture_overrides = 0;

            auto local_options = uint64_t();

            auto local_flags = macho::utils::tbd::flags::none;
            auto local_objc_constraint = macho::utils::tbd::objc_constraint::no_value;
            auto local_platform = macho::utils::tbd::platform::none;
            auto local_tbd_version = (enum macho::utils::tbd::version)0;

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
                        local_options |= macho::utils::tbd::options::allow_all_private_symbols;
                    } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
                        local_options |= macho::utils::tbd::options::allow_private_normal_symbols;
                    } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
                        local_options |= macho::utils::tbd::options::allow_private_weak_symbols;
                    } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
                        local_options |= macho::utils::tbd::options::allow_private_objc_symbols;
                    } else if (strcmp(option, "allow-private-objc-classes") == 0) {
                        local_options |= macho::utils::tbd::options::allow_private_objc_classes;
                    } else if (strcmp(option, "allow-private-objc-ivars") == 0) {
                        local_options |= macho::utils::tbd::options::allow_private_objc_ivars;
                    } else if (strcmp(option, "dont-print-warnings") == 0) {
                        local_options |= dont_print_warnings;
                    } else if (strcmp(option, "flags") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a list of flags to add onto ones found in provided mach-o file(s)\n", stderr);
                            return 1;
                        }

                        auto j = ++i;
                        for (; j < argc; j++) {
                            const auto argument = argv[j];
                            if (strcmp(argument, "flat_namespace") == 0) {
                                local_flags |= macho::utils::tbd::flags::flat_namespace;
                            } else if (strcmp(argument, "not_app_extension_safe") == 0) {
                                local_flags |= macho::utils::tbd::flags::not_app_extension_safe;
                            } else {
                                if (j == i) {
                                    fputs("Please provide a list of flags to add onto ones found in provided mach-o file(s)\n", stderr);
                                    return 1;
                                }

                                j--;
                                break;
                            }
                        }

                        i = j;
                    } else if (strcmp(option, "objc-constraint") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide an objc-constraint\n", stderr);
                            return 1;
                        }

                        i++;

                        const auto objc_constraint_string = argv[i];
                        const auto objc_constraint_inner = macho::utils::tbd::objc_constraint_from_string(objc_constraint_string);

                        if (objc_constraint_inner == macho::utils::tbd::objc_constraint::no_value) {
                            fprintf(stderr, "Unrecognized objc-constraint value provided (%s)\n", objc_constraint_string);
                            return 1;
                        }

                        local_objc_constraint = objc_constraint_inner;
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
                        local_platform = macho::utils::tbd::platform_from_string(platform_string);

                        if (local_platform == macho::utils::tbd::platform::none) {
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
                    } else if (strcmp(option, "remove-objc-constraint") == 0) {
                        options |= macho::utils::tbd::options::remove_objc_constraint;
                    } else if (strcmp(option, "replace-path-extension") == 0) {
                        local_options |= replace_path_extension;
                    } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a tbd-version\n", stderr);
                            return 1;
                        }

                        i++;

                        const auto &version_string = argv[i];

                        local_tbd_version = macho::utils::tbd::version_from_string(version_string);
                        if (local_tbd_version == (enum macho::utils::tbd::version)0) {
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

                // If the user-provided path-string does not begin with
                // a forward slash, it is assumed that the path exists
                // in the current-directory

                if (path == "stdin") {
                    if (local_options & recurse_directories) {
                        fputs("Cannot recurse stdin\n", stderr);
                        return 1;
                    }

                    if (local_options & replace_path_extension) {
                        fputs("Cannot replace path-extension of stdin\n", stderr);
                        return 1;
                    }
                } else {
                    path::clean(path);

                    if (const auto &path_front = path.front(); path_front != '/' && path_front != '\\') {
                        path.insert(0, retrieve_current_directory());
                    }

                    struct stat sbuf;
                    if (stat(path.data(), &sbuf) != 0) {
                        fprintf(stderr, "Failed to retrieve information on object (at path %s), failing with error: %s\n", path.data(), strerror(errno));
                        return 1;
                    }

                    const auto path_is_directory = S_ISDIR(sbuf.st_mode);
                    if (path_is_directory) {
                        if (!(local_options & recurse_directories)) {
                            fprintf(stderr, "Cannot open directory (at path %s) as a macho-file, use -r to recurse the directory\n", path.data());
                            return 1;
                        }

                        const auto &path_back = path.back();
                        if (path_back != '/' && path_back != '\\') {
                            path.append(1, '/');
                        }
                    } else {
                        const auto path_is_regular_file = S_ISREG(sbuf.st_mode);
                        if (path_is_regular_file) {
                            if (local_options & recurse_directories) {
                                fprintf(stderr, "Cannot recurse file (at path %s)\n", path.data());
                                return 1;
                            }

                            if (local_options & replace_path_extension) {
                                fputs("Replacing path-extension for output-files is meant only to be used while recursing\n", stderr);
                                return 1;
                            }
                        } else {
                            fprintf(stderr, "Object (at path %s) is not a regular file\n", path.data());
                            return 1;
                        }
                    }
                }

                auto tbd = tbd_file({});
                tbd.path = std::move(path);

                tbd.architectures = local_architectures;
                tbd.architecture_overrides = local_architecture_overrides;

                tbd.options = local_options;
                tbd.platform = local_platform;
                tbd.version = local_tbd_version;

                tbds.emplace_back(std::move(tbd));

                // Clear the local fields to signal that a
                // path was provided

                local_architectures = 0;
                local_architecture_overrides = 0;

                local_options = 0;
                local_platform = macho::utils::tbd::platform::none;
                local_tbd_version = (enum macho::utils::tbd::version)0;

                break;
            }

            // It is expected for --path to error out if the user has
            // not provided a path to a mach-o library path or to a
            // directory where some could be found

            if (local_architectures != 0 || local_architecture_overrides != 0 || local_platform != macho::utils::tbd::platform::none || local_options != 0 || local_tbd_version != (enum macho::utils::tbd::version)0) {
                fputs("Please provide a path to a mach-o library file or to a directory to recurse through\n", stderr);
                return 1;
            }
        } else if (strcmp(option, "platform") == 0) {
            if (is_last_argument) {
                fputs("Please provide a platform-string. Run --list-platform to see a list of platforms\n", stderr);
                return 1;
            }

            i++;

            const auto &platform_string = argv[i];
            platform = macho::utils::tbd::platform_from_string(platform_string);

            if (platform == macho::utils::tbd::platform::none) {
                fprintf(stderr, "Platform-string (%s) is invalid\n", platform_string);
                return 1;
            }
        } else if (strcmp(option, "remove-objc-constraint") == 0) {
            options |= macho::utils::tbd::options::remove_objc_constraint;
        } else if (strcmp(option, "replace-path-extension") == 0) {
            options |= replace_path_extension;
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
                version = macho::utils::tbd::version::v1;
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
    } else {
        // Remove any duplicates
        for (auto tbd_iter = tbds.begin(); tbd_iter != tbds.end(); tbd_iter++) {
            const auto &tbd = *tbd_iter;
            const auto &tbd_path = tbd.path;

            for (auto tbd_inner_iter = tbd_iter + 1; tbd_inner_iter != tbds.end();) {
                const auto &tbd_inner = *tbd_inner_iter;
                const auto &tbd_inner_path = tbd_inner.path;

                if (path::compare(tbd_path.begin(), tbd_path.end(), tbd_inner_path.begin(), tbd_inner_path.end()) != 0) {
                    continue;
                }

                auto tbd_options = tbd.options;
                auto tbd_inner_options = tbd_inner.options;

                if (tbd_inner_options & recurse_subdirectories) {
                    tbd_options |= recurse_subdirectories;
                    tbd_inner_options &= ~recurse_subdirectories;
                }

                if (tbd_inner_options != 0) {
                    tbd_inner_iter++;
                } else {
                    tbd_inner_iter = tbds.erase(tbd_inner_iter);
                }
            }
        }
    }

    for (auto &tbd : tbds) {
        auto &tbd_path = tbd.path;
        auto &tbd_output_path = tbd.output_path;

        auto &tbd_flags = tbd.flags;
        if ((tbd_flags & macho::utils::tbd::flags::flat_namespace) != macho::utils::tbd::flags::none) {
            tbd_flags |= macho::utils::tbd::flags::flat_namespace;
        }

        if ((tbd_flags & macho::utils::tbd::flags::not_app_extension_safe) != macho::utils::tbd::flags::none) {
            tbd_flags |= macho::utils::tbd::flags::not_app_extension_safe;
        }

        auto &tbd_options = tbd.options;
        if (options & macho::utils::tbd::options::allow_all_private_symbols) {
            tbd_options |= macho::utils::tbd::options::allow_all_private_symbols;
        } else {
            if (options & macho::utils::tbd::options::allow_private_normal_symbols) {
                tbd_options |= macho::utils::tbd::options::allow_private_normal_symbols;
            }

            if (options & macho::utils::tbd::options::allow_private_weak_symbols) {
                tbd_options |= macho::utils::tbd::options::allow_private_weak_symbols;
            }

            if (options & macho::utils::tbd::options::allow_private_objc_symbols) {
                tbd_options |= macho::utils::tbd::options::allow_private_objc_symbols;
            } else {
                if (options & macho::utils::tbd::options::allow_private_objc_classes) {
                    tbd_options |= macho::utils::tbd::options::allow_private_objc_classes;
                }

                if (options & macho::utils::tbd::options::allow_private_objc_ivars) {
                    tbd_options |= macho::utils::tbd::options::allow_private_objc_ivars;
                }
            }
        }

        if (options & dont_print_warnings) {
            tbd_options |= dont_print_warnings;
        }

        if (tbd_options & recurse_directories) {
            if (tbd_output_path.empty()) {
                fprintf(stderr, "Cannot output mach-o files found while recursing directory (at path %s) to stdout. Please provide a directory to output .tbd files to\n", tbd_path.data());
                return 1;
            }

            if (options & replace_path_extension) {
                tbd_options |= replace_path_extension;
            }

            const auto tbd_path_length = tbd_path.length();
            const auto tbd_output_path_length = tbd_output_path.length();

            auto outputted_any_macho_libraries = false;
            auto recurse_options = uint64_t();

            if (!(tbd_options & dont_print_warnings)) {
                recurse_options |= recurse::options::print_warnings;
            }

            if (tbd_options & recurse_subdirectories) {
                recurse_options |= recurse::options::recurse_subdirectories;
            }

            auto recursion_result = recurse::macho_libraries(tbd_path.data(), recurse_options, [&](std::string &library_path, macho::file &file) {
                auto output_path_front = std::string::npos;
                if (tbd_options & maintain_directories) {
                    output_path_front = tbd_path_length;
                } else {
                    output_path_front = path::find_last_slash(library_path.begin(), library_path.end()) - library_path.begin();
                }

                auto output_path = library_path.substr(output_path_front);

                auto output_path_length = output_path.length();
                auto output_path_new_length = output_path_length + tbd_output_path_length + 4;

                output_path.reserve(output_path_new_length);
                output_path.insert(0, tbd_output_path);

                if (tbd_options & replace_path_extension) {
                    auto output_path_end = output_path.end();
                    auto path_extension = path::find_extension(output_path.begin(), output_path_end);

                    if (path_extension != output_path_end) {
                        output_path.erase(path_extension, output_path_end);
                    }
                }

                output_path.append(".tbd");

                auto output_file_descriptor = -1;
                auto recursive_directory_creation_ptr = recursively_create_directories_from_file_path_creating_last_as_file(output_path.data(), tbd_output_path_length, &output_file_descriptor);

                auto output_file = (FILE *)nullptr;
                if (output_file_descriptor != -1) {
                    output_file = fdopen(output_file_descriptor, "w");
                } else {
                    output_file = fopen(output_path.data(), "w");
                }

                if (!output_file) {
                    // should_print_paths is always true for recursing,
                    // so a check here is unnecessary

                    fprintf(stderr, "Failed to open file (at path %s) for writing, failing with error: %s\n", output_path.data(), strerror(errno));
                    return;
                }

                auto tbd_creation_options = creation_handling_print_paths | creation_handling_ignore_no_provided_architectures;
                if (tbd_options & dont_print_warnings) {
                    tbd_creation_options |= creation_handling_dont_print_warnings;
                }

                if (tbd.constraint == macho::utils::tbd::objc_constraint::no_value) {
                    tbd.constraint = objc_constraint;
                }

                if (tbd.platform == macho::utils::tbd::platform::none) {
                    tbd.platform = platform;
                }

                if (tbd.version == (enum macho::utils::tbd::version)0) {
                    tbd.version = version;
                }

                if (!tbd.architectures) {
                    tbd.architectures = architectures;
                }

                if (!tbd.architecture_overrides) {
                    tbd.architecture_overrides = architecture_overrides;
                }

                const auto result = create_tbd_file(library_path.data(), file, output_path.data(), output_file, tbd_options & 0xff, tbd.flags, tbd.constraint, tbd.platform, tbd.version, tbd.architectures, tbd.architecture_overrides, tbd_creation_options);
                if (!result) {
                    if (recursive_directory_creation_ptr != nullptr) {
                        recursively_remove_directories_from_file_path(output_path.data(), recursive_directory_creation_ptr);
                    }
                }

                fclose(output_file);
                outputted_any_macho_libraries = true;
            });

            switch (recursion_result) {
                case recurse::operation_result::ok:
                    if (!outputted_any_macho_libraries) {
                        if (tbd_options & recurse_subdirectories) {
                            fprintf(stderr, "No mach-o files were found for outputting while recursing through directory (at path %s)\n", tbd_path.data());
                        } else {
                            fprintf(stderr, "No mach-o files were found for outputting while recursing once through directory (at path %s)\n", tbd_path.data());
                        }
                    }

                    break;

                case recurse::operation_result::failed_to_open_directory:
                    fprintf(stderr, "Warning: Failed to open directory (at path %s) for recursing, failing with error: %s\n", tbd_path.data(), strerror(errno));
                    break;
            }
        } else {
            auto library_file = macho::file();
            auto library_file_open_result = macho::file::open_result::ok;

            if (tbd_path == "stdin") {
                library_file_open_result = library_file.open_from_dynamic_library(stdin);
            } else {
                library_file_open_result = library_file.open_from_dynamic_library(tbd_path.data());
            }

            switch (library_file_open_result) {
                case macho::file::open_result::ok:
                    break;

                case macho::file::open_result::failed_to_open_stream:
                    if (should_print_paths) {
                        if (tbd_path == "stdin") {
                            fprintf(stderr, "Failed to open file (in stdin) for reading, failing with error: %s\n", strerror(errno));
                        } else {
                            fprintf(stderr, "Failed to open file (at path %s) for reading, failing with error: %s\n", tbd_path.data(), strerror(errno));
                        }
                    } else {
                        fprintf(stderr, "Failed to open file at provided path for reading, failing with error: %s\n", strerror(errno));
                    }

                    break;

                case macho::file::open_result::failed_to_allocate_memory:
                    if (should_print_paths) {
                        if (tbd_path == "stdin") {
                            fputs("Failed to allocate memory necessary for processing file (in stdin)\n", stderr);
                        } else {
                            fprintf(stderr, "Failed to allocate memory necessary for processing file (at path %s)\n", tbd_path.data());
                        }
                    } else {
                        fputs("Failed to allocate memory necessary for processing file at provided path\n", stderr);
                    }

                    break;

                case macho::file::open_result::stream_seek_error:
                case macho::file::open_result::stream_read_error:
                    if (should_print_paths) {
                        if (tbd_path == "stdin") {
                            fprintf(stderr, "Encountered an error while reading through file (in stdin), likely not a valid mach-o. Reading failed with error: %s\n", strerror(ferror(library_file.stream)));
                        } else {
                            fprintf(stderr, "Encountered an error while reading through file (at path %s), likely not a valid mach-o. Reading failed with error: %s\n", tbd_path.data(), strerror(ferror(library_file.stream)));
                        }
                    } else {
                        fprintf(stderr, "Encountered an error while reading through file at provided path, likely not a valid mach-o. Reading failed with error: %s\n", strerror(ferror(library_file.stream)));
                    }

                    break;

                case macho::file::open_result::zero_architectures: {
                    if (should_print_paths) {
                        if (tbd_path == "stdin") {
                            fputs("Fat mach-o file (in stdin) does not have any architectures\n", stderr);
                        } else {
                            fprintf(stderr, "Fat mach-o file (at path %s) does not have any architectures\n", tbd_path.data());
                        }
                    } else {
                        fputs("Fat mach-o file at provided path does not have any architectures\n", stderr);
                    }

                    break;
                }

                case macho::file::open_result::invalid_container: {
                    if (should_print_paths) {
                        if (tbd_path == "stdin") {
                            fputs("Mach-o file (in stdin) is invalid\n", stderr);
                        } else {
                            fprintf(stderr, "Mach-o file (at path %s) is invalid\n", tbd_path.data());
                        }
                    } else {
                        fputs("Mach-o file at provided path is invalid\n", stderr);
                    }

                    break;
                }

                case macho::file::open_result::not_a_macho: {
                    if (should_print_paths) {
                        if (tbd_path == "stdin") {
                            fputs("File (in stdin) is not a valid mach-o\n", stderr);
                        } else {
                            fprintf(stderr, "File (at path %s) is not a valid mach-o\n", tbd_path.data());
                        }
                    } else {
                        fputs("File at provided path is not a valid mach-o\n", stderr);
                    }

                    break;
                }

                case macho::file::open_result::not_a_library: {
                    if (should_print_paths) {
                        if (tbd_path == "stdin") {
                            fputs("Mach-o file (in stdin) is not a mach-o library\n", stderr);
                        } else {
                            fputs("Mach-o file (at path %s) is not a mach-o library\n", stderr);
                        }
                    } else {
                        fputs("Mach-o file at provided path is not a valid mach-o library\n", stderr);
                    }

                    break;
                }

                case macho::file::open_result::not_a_dynamic_library: {
                    if (should_print_paths) {
                        if (tbd_path == "stdin") {
                            fputs("Mach-o file (in stdin) is not a mach-o dynamic library\n", stderr);
                        } else {
                            fputs("Mach-o file (at path %s) is not a mach-o dynamic library\n", stderr);
                        }
                    } else {
                        fputs("Mach-o file at provided path is not a valid mach-o dynamic library\n", stderr);
                    }

                    break;
                }
            }

            if (library_file_open_result != macho::file::open_result::ok) {
                continue;
            }

            auto output_file = stdout;
            auto recursive_directory_creation_ptr = (char *)nullptr;

            if (!tbd_output_path.empty()) {
                recursive_directory_creation_ptr = recursively_create_directories_from_file_path_creating_last_as_file(tbd_output_path.data(), 0);
                output_file = fopen(tbd_output_path.data(), "w");

                if (!output_file) {
                    if (should_print_paths) {
                        fprintf(stderr, "Failed to open file (at path %s) for writing, failing with error: %s\n", tbd_output_path.data(), strerror(errno));
                    } else {
                        fprintf(stderr, "Failed to open file at provided output-path for writing, failing with error: %s\n", strerror(errno));
                    }

                    continue;
                }
            }

            auto tbd_creation_options = static_cast<uint64_t>(creation_handling_print_paths);
            if (tbd_options & dont_print_warnings) {
                tbd_creation_options |= creation_handling_dont_print_warnings;
            }

            if (tbd.constraint == macho::utils::tbd::objc_constraint::no_value) {
                tbd.constraint = objc_constraint;
            }

            if (tbd.platform == macho::utils::tbd::platform::none) {
                tbd.platform = platform;
            }

            if (tbd.version == (enum macho::utils::tbd::version)0) {
                tbd.version = version;
            }

            if (!tbd.architectures) {
                tbd.architectures = architectures;
            }

            if (!tbd.architecture_overrides) {
                tbd.architecture_overrides = architecture_overrides;
            }

            const auto result = create_tbd_file(tbd_path.data(), library_file, tbd_output_path.data(), output_file, tbd_options & 0xff, tbd.flags, tbd.constraint, tbd.platform, tbd.version, tbd.architectures, tbd.architecture_overrides, tbd_creation_options);
            if (!tbd_output_path.empty()) {
                if (!result) {
                    recursively_remove_directories_from_file_path(tbd_output_path.data(), recursive_directory_creation_ptr);
                }

                fclose(output_file);
            }
        }
    }

    return 0;
}
