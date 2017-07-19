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

#include "tbd/tbd.h"

enum class recurse {
    none,
    once,
    all
};

void loop_subdirectories_for_libraries(DIR *directory, const std::string &directory_path, const std::function<void(const std::string &)> &callback) {
    auto directory_entry = readdir(directory);
    auto directory_path_length = directory_path.length();

    while (directory_entry != nullptr) {
        const auto directory_entry_is_directory = directory_entry->d_type == DT_DIR;
        if (directory_entry_is_directory) {
            // Ignore the directory-entry for the directory itself, and that of its
            // superior in the directory hierarchy.

            if (strncmp(directory_entry->d_name, ".", directory_entry->d_namlen) == 0 ||
                strncmp(directory_entry->d_name, "..", directory_entry->d_namlen) == 0) {
                directory_entry = readdir(directory);
                continue;
            }

            auto sub_directory_path = std::string();
            auto sub_directory_path_length = directory_path_length + directory_entry->d_namlen + 1;

            // Avoid re-allocations by calculating new total-length, and reserving space
            // accordingly.

            sub_directory_path.reserve(sub_directory_path_length);

            // Add a final forward slash to the path as it is a directory, and reduces work
            // needing to be done if a sub-directory is found and needs to be recursed.

            sub_directory_path.append(directory_path);
            sub_directory_path.append(directory_entry->d_name, &directory_entry->d_name[directory_entry->d_namlen]);
            sub_directory_path.append(1, '/');

            const auto sub_directory = opendir(sub_directory_path.data());
            if (sub_directory) {
                loop_subdirectories_for_libraries(sub_directory, sub_directory_path, callback);
                closedir(sub_directory);
            } else {
                // In the context of recursing through files and directories, Erroring out here would
                // be a mistake. Warning the user that opening the directory failed is good enough.

                fprintf(stderr, "Warning: Failed to open sub-directory at path (%s), failing with error (%s)\n", sub_directory_path.data(), strerror(errno));
            }
        } else {
            const auto directory_entry_is_regular_file = directory_entry->d_type == DT_REG;
            if (directory_entry_is_regular_file) {
                // Only append the valid part of the directory_entry->d_name buffer,
                // which is directory_entry->d_namlen long.

                auto directory_entry_path = std::string();
                auto directory_entry_path_length = directory_path_length + directory_entry->d_namlen;

                directory_entry_path.reserve(directory_entry_path_length);

                directory_entry_path.append(directory_path);
                directory_entry_path.append(directory_entry->d_name, directory_entry->d_namlen);

                const auto directory_entry_path_is_valid_library = macho::file::is_valid_library(directory_entry_path);
                if (directory_entry_path_is_valid_library) {
                    callback(directory_entry_path);
                }
            }
        }

        directory_entry = readdir(directory);
    }
}

void loop_directory_for_libraries(const char *directory_path, const recurse &recurse_type, const std::function<void(const std::string &)> &callback) {
    const auto directory = opendir(directory_path);
    if (!directory) {
        fprintf(stderr, "Failed to open directory at path (%s), failing with error (%s)\n", directory_path, strerror(errno));
        exit(1);
    }

    const auto directory_path_length = strlen(directory_path);
    const auto should_recurse_all_of_directory_entry = recurse_type == recurse::all;

    auto directory_entry = readdir(directory);
    while (directory_entry != nullptr) {
        const auto directory_entry_is_directory = directory_entry->d_type == DT_DIR;
        if (directory_entry_is_directory && should_recurse_all_of_directory_entry) {
            // Ignore the directory-entry for the directory itself, and that of its
            // superior in the directory hierarchy.

            if (strncmp(directory_entry->d_name, ".", directory_entry->d_namlen) == 0 ||
                strncmp(directory_entry->d_name, "..", directory_entry->d_namlen) == 0) {
                directory_entry = readdir(directory);
                continue;
            }

            auto sub_directory_path = std::string();
            auto sub_directory_path_length = directory_path_length + directory_entry->d_namlen + 1;

            // Avoid re-allocations by calculating new total-length, and reserving space
            // accordingly.

            sub_directory_path.reserve(sub_directory_path_length);

            // Add a final forward slash to the path as it is a directory, and reduces work
            // needing to be done if a sub-directory is found and needs to be recursed.

            sub_directory_path.append(directory_path);
            sub_directory_path.append(directory_entry->d_name, &directory_entry->d_name[directory_entry->d_namlen]);
            sub_directory_path.append(1, '/');

            const auto sub_directory = opendir(sub_directory_path.data());
            if (sub_directory) {
                loop_subdirectories_for_libraries(sub_directory, sub_directory_path, callback);
                closedir(sub_directory);
            } else {
                // In the context of recursing through files and directories, Erroring out here would
                // be a mistake. Warning the user that opening the directory failed is good enough.

                fprintf(stderr, "Warning: Failed to open sub-directory at path (%s), failing with error (%s)\n", sub_directory_path.data(), strerror(errno));
            }
        } else {
            const auto directory_entry_is_regular_file = directory_entry->d_type == DT_REG;
            if (directory_entry_is_regular_file) {
                // Only append the valid part of the directory_entry->d_name buffer,
                // which is directory_entry->d_namlen long.

                auto directory_entry_path = std::string();
                auto directory_entry_path_length = directory_path_length + directory_entry->d_namlen;

                directory_entry_path.reserve(directory_entry_path_length);

                directory_entry_path.append(directory_path);
                directory_entry_path.append(directory_entry->d_name, &directory_entry->d_name[directory_entry->d_namlen]);

                const auto directory_entry_path_is_valid_library = macho::file::is_valid_library(directory_entry_path);
                if (directory_entry_path_is_valid_library) {
                    callback(directory_entry_path);
                }
            }
        }

        directory_entry = readdir(directory);
    }
}

const char *retrieve_current_directory() {
    // Store current-directory as a static variable to avoid
    // calling getcwd more times than necessary.

    static const char *current_directory = nullptr;
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

            strcpy(new_current_directory_string, current_directory_string);
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

void parse_architectures_list(std::vector<const macho::architecture_info *> &architectures, int &index, int argc, const char *argv[]) {
    while (index < argc) {
        const auto &architecture_string = argv[index];
        const auto &architecture_string_front = architecture_string[0];

        // Quickly filter out an option or path instead of a (relatively)
        // expensive call to macho::architecture_info_from_name().

        if (architecture_string_front == '-' || architecture_string_front == '/') {
            // If the architectures vector is empty, the user did not provide any architectures
            // but did provided the architecture option, which requires at leasr one architecture
            // being provided.

            if (architectures.empty()) {
                fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                exit(1);
            }

            break;
        }

        const auto architecture = macho::architecture_info_from_name(architecture_string);
        if (!architecture) {
            // macho::architecture_info_from_name() returning nullptr can be the result of one
            // of two scenarios, The string stored in architecture being invalid,
            // such as being mispelled, or the string being the path object inevitebly
            // following the architecture argument.

            // If the architectures vector is empty, the user did not provide any architectures
            // but did provided the architecture option, which requires at leasr one architecture
            // being provided.

            if (architectures.empty()) {
                fprintf(stderr, "Unrecognized architecture with name (%s)\n", architecture_string);
                exit(1);
            }

            break;
        }

        architectures.emplace_back(architecture);
        index++;
    }

    // As the caller of this function is in a for loop,
    // the index is incremented once again once this function
    // returns. To make up for this, decrement index by 1.

    index--;
}

void recursively_create_directories_from_file_path(char *path, bool create_last_as_directory) {
    // If the path starts off with a forward slash, it will
    // result in the while loop running on a path that is
    // empty. To avoid this, start the search at the first
    // index.

    auto slash = strchr(&path[1], '/');
    while (slash != nullptr) {
        // In order to avoid unnecessary (and expensive) allocations,
        // terminate the string at the location of the forward slash
        // and revert back after use.

        slash[0] = '\0';

        if (access(path, F_OK) != 0) {
            if (mkdir(path, 0755) != 0) {
                fprintf(stderr, "Failed to create directory at path (%s) with mode (0755), failing with error (%s)\n", path, strerror(errno));
                exit(1);
            }
        }

        slash[0] = '/';
        slash = strchr(&slash[1], '/');
    }

    if (create_last_as_directory) {
        if (access(path, F_OK) != 0) {
            if (mkdir(path, 0755) != 0) {
                fprintf(stderr, "Failed to create directory at path (%s) with mode (0755), failing with error (%s)\n", path, strerror(errno));
                exit(1);
            }
        }
    }
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
    fputs("Usage: tbd -p [-a/--archs architectures] [--platform ios/macosx/watchos/tvos] [-r/--recurse/ -r=once/all / --recurse=once/all] [-v/--version v1/v2] /path/to/macho/library\n", stdout);
    fputs("    -a, --archs,    Specify architecture(s) to use, instead of the ones in the provieded mach-o file(s)\n", stdout);
    fputs("        --platform, Specify platform for all mach-o library files provided\n", stdout);
    fputs("    -r, --recurse,  Specify directory to recurse and find mach-o library files in\n", stdout);
    fputs("    -v, --version,  Specify version of tbd to convert to (default is v2)\n", stdout);

    fputc('\n', stdout);
    fputs("Outputting options:\n", stdout);
    fputs("Usage: tbd -o [--maintain-directories] /path/to/output/file\n", stdout);
    fputs("        --maintain-directories, Maintain directories where mach-o library files were found in (subtracting the path provided)\n", stdout);

    fputc('\n', stdout);
    fputs("Global options:\n", stdout);
    fputs("    -a, --archs,    Specify architecture(s) to use, replacing default architectures (where default architectures were not already provided)\n", stdout);
    fputs("        --platform, Specify platform for all mach-o library files provided (applying to all mach-o library files where platform was not provided)\n", stdout);
    fputs("    -v, --version,  Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tnd-version was not provided)\n", stdout);

    fputc('\n', stdout);
    fputs("List options:\n", stdout);
    fputs("        --list-architectures,   List all valid architectures for tbd-files\n", stdout);
    fputs("        --list-macho-libraries, List all valid mach-o libraries in current-directory (or at provided path(s))\n", stdout);
    fputs("        --list-recurse,         List all valid recurse options for parsing directories\n", stdout);
    fputs("        --list-versions,        List all valid versions for tbd-files\n", stdout);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fputs("Please run -h or -u to see a list of options\n", stderr);
        return 1;
    }

    typedef struct tbd_recursive {
        const std::string provided_path;

        tbd tbd;
        recurse recurse;
    } tbd_recursive;

    auto architectures = std::vector<const macho::architecture_info *>();
    auto tbds = std::vector<tbd_recursive>();

    auto current_directory = std::string();
    auto output_paths_index = 0;

    auto platform = (enum tbd::platform)-1;
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

        if (strcmp(option, "a") == 0 || strcmp(option, "archs") == 0) {
            if (is_last_argument) {
                fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                return 1;
            }

            i++;
            parse_architectures_list(architectures, i, argc, argv);
        } else if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_usage();
            return 0;
        } else if (strcmp(option, "list-architectures") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            auto architecture_info = macho::get_architecture_info_table();
            while (architecture_info->name != nullptr) {
                fprintf(stdout, "%s\n", architecture_info->name);
                architecture_info++;
            }

            return 0;
        } else if (strcmp(option, "list-macho-libraries") == 0) {
            if (!is_first_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            auto paths = std::vector<std::pair<std::string, recurse>>();
            if (is_last_argument) {
                // If a path was not provided, --list-macho-libraries is
                // expected to instead recurse the current-directory.

                paths.emplace_back(retrieve_current_directory(), recurse::all);
            } else {
                auto recurse_type = recurse::none;
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
                            recurse_type = recurse::all;
                        } else if (strncmp(option, "r=", 2) == 0 || strncmp(option, "recurse=", 8) == 0) {
                            const auto recurse_type_string = strchr(option, '=') + 1;
                            const auto &recurse_type_string_front = recurse_type_string[0];

                            if (!recurse_type_string_front) {
                                fputs("Please provide a recurse type\n", stderr);
                                return 1;
                            }

                            if (strcmp(recurse_type_string, "once") == 0) {
                                recurse_type = recurse::once;
                            } else if (strcmp(recurse_type_string, "all") == 0) {
                                recurse_type = recurse::all;
                            } else {
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

                        paths.emplace_back(std::move(path), recurse_type);
                    } else {
                        paths.emplace_back(argument, recurse_type);
                    }

                    recurse_type = recurse::none;
                }

                if (paths.empty()) {
                    // If no path was provided, --list-macho-libraries is expected
                    // to instead recurse the current-directory.

                    paths.emplace_back(retrieve_current_directory(), recurse_type);
                }
            }

            const auto &paths_back = paths.back();
            for (const auto &pair : paths) {
                const auto &path = pair.first;
                const auto path_data = path.data();

                struct stat sbuf;
                if (stat(path_data, &sbuf) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object at path (%s), failing with error (%s)\n", path_data, strerror(errno));
                    return 1;
                }

                const auto path_is_directory = S_ISDIR(sbuf.st_mode);
                const auto &recurse_type = pair.second;

                if (recurse_type != recurse::none) {
                    if (!path_is_directory) {
                        fprintf(stderr, "Cannot recurse file at path (%s)\n", path_data);
                        return 1;
                    }

                    auto library_paths = std::vector<std::string>();
                    loop_directory_for_libraries(path_data, recurse_type, [&](const std::string &path) {
                        library_paths.emplace_back(std::move(path));
                    });

                    if (library_paths.empty()) {
                        switch (recurse_type) {
                            case recurse::none:
                                break;

                            case recurse::once:
                                fprintf(stdout, "No mach-o library files were found while recursing once through path (%s)\n", path_data);
                                break;

                            case recurse::all:
                                fprintf(stdout, "No mach-o library files were found while recursing through path (%s)\n", path_data);
                                break;
                        }
                    } else {
                        switch (recurse_type) {
                            case recurse::none:
                                break;

                            case recurse::once:
                                fprintf(stdout, "Found the following mach-o libraries while recursing once through path (%s)\n", path_data);
                                break;

                            case recurse::all:
                                fprintf(stdout, "Found the following mach-o libraries while recursing through path (%s)\n", path_data);
                                break;
                        }

                        const auto &path_length = path.length();
                        for (auto &library_path : library_paths) {
                            // --list-macho-libraries is expected to trim off the provided path
                            // from the resulting mach-o library paths to avoid extremely long
                            // paths being printed unnecessarily.

                            library_path.erase(library_path.begin(), library_path.begin() + path_length);
                            fprintf(stdout, "%s\n", library_path.data());
                        }
                    }

                    // Print a newline between each pair for readibility
                    // purposes, But an extra new-line is not needed for the
                    // last pair

                    if (pair != paths_back) {
                        fputc('\n', stdout);
                    }
                } else {
                    // Provided a recurse type of none requires only a file be provided.

                    if (path_is_directory) {
                        fprintf(stderr, "Cannot open directory at path (%s) as a macho-file, use -r (or -r=) to recurse the directory\n", path_data);
                        return 1;
                    }

                    const auto path_is_library = macho::file::is_valid_library(path);
                    if (path_is_library) {
                        fprintf(stdout, "Mach-o file at path (%s) is a library\n", path_data);
                    } else {
                        // As the user provided only one path to a specific mach-o library file,
                        // --list-macho-libraries is expected to explicity print out whether or
                        // not the provided mach-o library file is valid.

                        fprintf(stdout, "Mach-o file at path (%s) is not a library\n", path_data);
                    }
                }
            }

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
                auto &path_front = path.front();

                const auto tbds_size = tbds.size();
                if (output_paths_index >= tbds_size) {
                    fprintf(stderr, "No coresponding mach-o files for output-path (%s, at index %d)\n", path.data(), output_paths_index);
                    return 1;
                }

                auto &tbd_recursive = tbds.at(output_paths_index);
                auto &tbd = tbd_recursive.tbd;

                auto &macho_files = tbd.macho_files();
                auto macho_files_size = macho_files.size();

                if (path_front != '/' && path != "stdout") {
                    // If the user-provided path-string does not begin with
                    // a forward slash, it is assumed that the path exists
                    // in the current-directory.

                    path.insert(0, retrieve_current_directory());
                } else if (path == "stdout" && macho_files_size > 1) {
                    fputs("Can't output multiple mach-o files to stdout\n", stderr);
                    return 1;
                }

                auto &tbd_recurse_type = tbd_recursive.recurse;
                auto &output_files = tbd.output_files();

                auto create_output_files_for_paths = [&](const std::vector<std::string> &paths, const std::string &provided_path) {
                    const auto provided_path_length = provided_path.length();
                    for (const auto &path : paths) {
                        auto path_iter = std::string::npos;
                        if (should_maintain_directories) {
                            // --maintain-directories requires that directories not
                            // present in the provided path be created in the provided
                            // output-path's directory along with the tbd-file.

                            path_iter = tbd_recursive.provided_path.length();
                        } else {
                            // Resulting tbd-files are stored with the name
                            // of the mach-o library file

                            path_iter = path.find_last_of('/');
                        }

                        auto path_output_path = path.substr(path_iter);

                        auto path_output_path_length = path_output_path.length();
                        auto path_output_path_new_length = path_output_path_length + provided_path_length + 4;

                        path_output_path.reserve(path_output_path_new_length);

                        path_output_path.insert(0, provided_path);
                        path_output_path.append(".tbd");

                        // Recursively create directories in the final output-path for the mach-o
                        // library file as required by --maintain-directories.

                        recursively_create_directories_from_file_path((char *)path_output_path.data(), false);

                        // To avoid unnecessary allocation, Use std::move to move the output-path
                        // data to the new string object in the output_files vector.

                        output_files.emplace_back(std::move(path_output_path));
                    }
                };

                struct stat sbuf;
                if (stat(path.data(), &sbuf) == 0) {
                    const auto path_is_directory = S_ISDIR(sbuf.st_mode);
                    if (path_is_directory) {
                        if (tbd_recurse_type == recurse::none) {
                            fprintf(stderr, "Cannot output tbd-file to a directory at path (%s), please provide a full path to a file to output to\n", path.data());
                            return 1;
                        }

                        const auto &path_back = path.back();
                        if (path_back != '/') {
                            path.append(1, '/');
                        }

                        const auto output_files_size = output_files.size();
                        const auto output_files_new_size = output_files_size + macho_files_size;

                        output_files.reserve(output_files_new_size);
                        create_output_files_for_paths(macho_files, path);
                    } else {
                        const auto path_is_regular_file = S_ISREG(sbuf.st_mode);
                        if (path_is_regular_file) {
                            if (macho_files_size > 1) {
                                fprintf(stderr, "Can't output multiple mach-o files to file at path (%s)\n", path.data());
                                return 1;
                            }

                            // If an output-file at path does not exist, it is expected
                            // to be created. This is extended to the directories where the
                            // output-file is to exist.

                            recursively_create_directories_from_file_path((char *)path.data(), false);
                            output_files.emplace_back(path);
                        }
                    }
                } else {
                    if (macho_files_size > 1) {
                        const auto &path_back = path.back();
                        if (path_back != '/') {
                            path.append(1, '/');
                        }

                        if (access(path.data(), F_OK) != 0) {
                            // If an output-directory does not exist, it is expected
                            // to be created.

                            recursively_create_directories_from_file_path((char *)path.data(), true);
                        }

                        create_output_files_for_paths(macho_files, path);
                    } else {
                        output_files.emplace_back(path);
                    }

                }

                provided_output_path = true;
                break;
            }

            // To support the current format of providing output options,
            // a single output option only supports a single output-path

            if (!provided_output_path) {
                fputs("Please provide path(s) to output files\n", stderr);
                exit(1);
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

            auto local_architectures = std::vector<const macho::architecture_info *>();

            auto local_platform = (enum tbd::platform)-1;
            auto local_tbd_version = (enum tbd::version)0;

            auto recurse_type = recurse::none;
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
                    if (strcmp(option, "a") == 0 || strcmp(option, "archs") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                            return 1;
                        }

                        i++;
                        parse_architectures_list(local_architectures, i, argc, argv);
                    } else if (strcmp(option, "p") == 0) {
                        fprintf(stderr, "Please provide a path for option (%s)\n", argument);
                        return 1;
                    } else if (strcmp(option, "platform") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a platform-string (ios, macosx, tvos, watchos)\n", stderr);
                            return 1;
                        }

                        i++;

                        const auto &platform_string = argv[i];
                        local_platform = tbd::string_to_platform(platform_string);

                        if (local_platform == (enum tbd::platform)-1) {
                            fprintf(stderr, "Platform-string (%s) is invalid\n", platform_string);
                            return 1;
                        }
                    } else if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                        recurse_type = recurse::all;
                    } else if (strncmp(option, "r=", 2) == 0 || strncmp(option, "recurse=", 8) == 0) {
                        const auto recurse_type_string = strchr(option, '=') + 1;
                        const auto &recurse_type_string_front = recurse_type_string[0];

                        if (!recurse_type_string_front) {
                            fputs("Please provide a recurse type\n", stderr);
                            return 1;
                        }

                        if (strcmp(recurse_type_string, "once") == 0) {
                            recurse_type = recurse::once;
                        } else if (strcmp(recurse_type_string, "all") == 0) {
                            recurse_type = recurse::all;
                        } else {
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
                        if (local_tbd_version == (enum tbd::version)-1) {
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
                    fprintf(stderr, "Failed to retrieve information on object at path (%s), failing with error (%s)\n", path.data(), strerror(errno));
                    return 1;
                }

                auto tbd = ::tbd();
                auto &macho_files = tbd.macho_files();

                const auto path_is_directory = S_ISDIR(sbuf.st_mode);
                if (path_is_directory) {
                    if (recurse_type == recurse::none) {
                        fprintf(stderr, "Cannot open directory at path (%s) as a macho-file, use -r to recurse the directory\n", path.data());
                        return 1;
                    }

                    const auto &path_back = path.back();
                    if (path_back != '/') {
                        path.append(1, '/');
                    }

                    loop_directory_for_libraries(path.data(), recurse_type, [&](const std::string &path) {
                        macho_files.emplace_back(std::move(path));
                    });
                } else {
                    const auto path_is_regular_file = S_ISREG(sbuf.st_mode);
                    if (path_is_regular_file) {
                        if (recurse_type != recurse::none) {
                            fprintf(stderr, "Cannot recurse file at path (%s)\n", path.data());
                            return 1;
                        }

                        const auto path_is_valid_library = macho::file::is_valid_library(path);
                        if (!path_is_valid_library) {
                            fprintf(stderr, "File at path (%s) is not a valid mach-o library\n", path.data());
                            return 1;
                        }

                        macho_files.emplace_back(path);
                    } else {
                        fprintf(stderr, "Object at path (%s) is not a regular file\n", path.data());
                        return 1;
                    }
                }

                const auto &tbd_macho_files = tbd.macho_files();
                const auto tbd_macho_files_is_empty = tbd_macho_files.empty();

                if (tbd_macho_files_is_empty) {
                    switch (recurse_type) {
                        case recurse::none:
                            fprintf(stdout, "File at path (%s) is not a mach-o library file\n", path.data());
                            break;

                        case recurse::once:
                            fprintf(stdout, "No mach-o library files were found while recursing once in directory at path (%s)\n", path.data());
                            break;

                        case recurse::all:
                            fprintf(stdout, "No mach-o library files were found while recursing through all files and directories in directory at path (%s)\n", path.data());
                            break;
                    }

                    return 1;
                }

                // A fallback to global options (or defaults) is expected
                // in the absence of local user input

                auto tbd_architectures = &local_architectures;
                if (tbd_architectures->empty()) {
                    tbd_architectures = &architectures;
                }

                auto tbd_platform = local_platform;
                auto tbd_version = local_tbd_version;

                tbd.set_architectures(*tbd_architectures);
                tbd.set_platform(tbd_platform);
                tbd.set_version(tbd_version);

                auto &output_files = tbd.output_files();
                output_files.reserve(macho_files.size());

                auto tbd_recurse = tbd_recursive({ path, tbd, recurse_type });
                tbds.emplace_back(tbd_recurse);

                // Clear the local option fields to signal that a
                // path was provided

                local_architectures.clear();
                local_platform = (enum tbd::platform)-1;
                local_tbd_version = (enum tbd::version)-1;

                recurse_type = recurse::none;

                break;
            }

            // It is expected for --path to error out if the user has
            // not provided a path to a mach-o library path or to a
            // directory where some could be found

            if (local_architectures.size() != 0 || local_platform != (enum tbd::platform)-1 || local_tbd_version != (enum tbd::version)-1 || recurse_type != recurse::none) {
                fputs("Please provide a path to a directory to recurse through\n", stderr);
                return 1;
            }
        } else if (strcmp(option, "platform") == 0) {
            if (is_last_argument) {
                fputs("Please provide a platform-string (ios, macosx, tvos, watchos)\n", stderr);
                return 1;
            }

            i++;

            const auto &platform_string = argv[i];
            platform = tbd::string_to_platform(platform_string);

            if (platform == (enum tbd::platform)-1) {
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

    for (auto &tbd_recursive : tbds) {
        auto &tbd = tbd_recursive.tbd;

        // If a global tbd-version was provided after providing
        // path(s) to mach-o library files, it is expected to apply
        // to mach-o library files where version was not set locally

        auto tbd_version = tbd.version();
        if (tbd_version == (enum tbd::version)-1) {
            tbd_version = version;
            tbd.set_version(version);
        }

        const auto &tbd_architectures = tbd.architectures();

        const auto architectures_size = architectures.size();
        const auto tbd_architectures_size = tbd_architectures.size();

        // Support for multiple architectures is available to only
        // tbd-version v1 as tbd-version v2 requires a uuid to be
        // associated with the architecture

        if (tbd_version == tbd::version::v2) {
            if (tbd_architectures_size != 0 || architectures_size != 0) {
                fputs("Cannot have custom architectures on tbd-version v2, Please specify tbd-version v1\n", stderr);
                return 1;
            }
        } else if (!tbd_architectures_size && architectures_size != 0) {
            // If global custom architectures was provided after providing
            // path(s) to mach-o library files, it is expected to apply
            // to mach-o library files where custom architectures were not
            // set locally, and where tbd-version is v1

            tbd.set_architectures(architectures);
        }

        const auto &path = tbd_recursive.provided_path;
        auto tbd_platform = tbd.platform();

        // If a global platform was provided after providing
        // path(s) to mach-o library files, it is expected to apply
        // to mach-o library files where version was not set locally

        if (tbd_platform == (enum tbd::platform)-1) {
            if (platform != (enum tbd::platform)-1) {
                tbd_platform = platform;
            } else {
                // If a global platform was not provided, it is then expected
                // to give a prompt to the user requiring them to specify their
                // platform

                const auto path_is_directory = path.back() == '/';
                auto platform_string = std::string();

                do {
                    if (path_is_directory) {
                        fprintf(stdout, "Please provide a platform for files in directory at path (%s) (ios, macosx, watchos, or tvos): ", path.data());
                    } else {
                        fprintf(stdout, "Please provide a platform for file at path (%s) (ios, macosx, watchos, or tvos): ", path.data());
                    }

                    getline(std::cin, platform_string);
                    tbd_platform = tbd::string_to_platform(platform_string.data());
                } while (tbd_platform == (enum tbd::platform)-1);
            }

            tbd.set_platform(tbd_platform);
        }

        auto &output_files = tbd.output_files();
        auto output_files_size = output_files.size();

        if (output_files_size != 0) {
            continue;
        }

        const auto &macho_files = tbd.macho_files();
        const auto tbd_recurse_type = tbd_recursive.recurse;

        if (tbd_recurse_type != recurse::none) {
            // If no output-files were provided for multiple
            // mach-o files, it is expected the tbd-files are
            // outputted to the same directory with the same
            // file-name (with the file-extension .tbd)

            output_files.reserve(macho_files.size());
            for (const auto &macho_file : macho_files) {
                output_files.emplace_back(macho_file + ".tbd");
            }
        }
    }

    for (auto &tbd_recursive : tbds) {
        auto &tbd = tbd_recursive.tbd;
        tbd.run();
    }
}
