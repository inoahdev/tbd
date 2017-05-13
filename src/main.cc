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

void print_usage() {
    fputs("Usage: tbd [-p file-paths] [-v/--version v2] [-a/--archs architectures] [-o/-output output-paths-or-stdout]\n", stdout);
    fputs("Main options:\n", stdout);
    fputs("    -a, --archs,    Specify Architecture(s) to use, instead of the ones in the provieded mach-o file(s)\n", stdout);
    fputs("    -h, --help,     Print this message\n", stdout);
    fputs("    -o, --output,   Path(s) to output file(s) to write converted .tbd. If provided file(s) already exists, contents will get overrided. Can also provide \"stdout\" to print to stdout\n", stdout);
    fputs("    -p, --path,     Path(s) to mach-o file(s) to convert to a .tbd\n", stdout);
    fputs("    -u, --usage,    Print this message\n", stdout);
    fputs("    -v, --version,  Set version of tbd to convert to (default is v2)\n", stdout);

    fputs("\n", stdout);
    fputs("Extra options:\n", stdout);
    fputs("        --platform, Specify platform for all mach-o files provided\n", stdout);
    fputs("    -r, --recurse,  Specify directory to recurse and find mach-o files in. Use in conjunction with -p (ex. -p -r /path/to/directory)\n", stdout);
    fputs("        --versions, Print a list of all valid tbd-versions\n", stdout);

    fputs("\n", stdout);
    fputs("List options:\n", stdout);
    fputs("        --list-architectures,   List all valid architectures for tbd-files\n", stdout);
    fputs("        --list-macho-libraries, List all valid mach-o libraries in current-directory (or at provided path(s))\n", stdout);

    exit(0);
}

enum class recurse {
    none,
    once,
    all
};

void loop_directory_for_libraries(DIR *directory, const std::string &directory_path, const recurse &recurse_type, const std::function<void(const std::string &)> &callback) {
    auto directory_entry = readdir(directory);
    while (directory_entry != nullptr) {
        if (directory_entry->d_type == DT_DIR && recurse_type == recurse::all) {
            if (strncmp(directory_entry->d_name, ".", directory_entry->d_namlen) == 0 ||
                strncmp(directory_entry->d_name, "..", directory_entry->d_namlen) == 0) {
                directory_entry = readdir(directory);
                continue;
            }

            auto sub_directory_path = directory_path;

            sub_directory_path.append(directory_entry->d_name, &directory_entry->d_name[directory_entry->d_namlen]);
            sub_directory_path.append(1, '/');

            const auto sub_directory = opendir(sub_directory_path.data());
            if (!sub_directory) {
                fprintf(stderr, "Failed to open sub-directory at path (%s), failing with error (%s)\n", sub_directory_path.data(), strerror(errno));
                exit(1);
            }

            loop_directory_for_libraries(sub_directory, sub_directory_path, recurse_type, callback);
            closedir(sub_directory);
        } else if (directory_entry->d_type == DT_REG) {
            const auto &directory_entry_path = directory_path + directory_entry->d_name;

            if (macho::file::is_valid_library(directory_entry_path)) {
                callback(directory_entry_path);
            }
        }

        directory_entry = readdir(directory);
    }
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fputs("Please run -h or -u to see a list of options\n", stderr);
        return 1;
    }

    typedef struct tbd_recursive {
        tbd tbd;
        recurse recurse;
    } tbd_recursive;

    auto architectures = std::vector<const NXArchInfo *>();
    auto tbds = std::vector<tbd_recursive>();

    auto platform_string = std::string();
    auto current_directory = std::string();

    auto version = 2;
    auto parse_architectures = [&](std::vector<const NXArchInfo *> &architectures, int &index) {
        while (index < argc) {
            const auto &architecture_string = argv[index];
            if (*architecture_string == '-' || *architecture_string == '/') {
                if (architectures.empty()) {
                    fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                    exit(1);
                }

                break;
            }

            const auto architecture = NXGetArchInfoFromName(architecture_string);
            if (!architecture) {
                if (architectures.empty()) {
                    fprintf(stderr, "Unrecognized architecture with name (%s)\n", architecture_string);
                    exit(1);
                }

                break;
            }

            architectures.emplace_back(architecture);
            index++;
        }

        index--;
    };

    auto output_paths_index = 0;
    auto provided_macho_files = false;

    for (auto i = 1; i < argc; i++) {
        const auto &argument = argv[i];
        if (*argument != '-') {
            fprintf(stderr, "Unrecognized argument (%s)\n", argument);
            return 1;
        }

        auto option = &argument[1];
        if (*option == '-') {
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
            parse_architectures(architectures, i);
        } else if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_usage();
        } else if (strcmp(option, "list-architectures") == 0) {
            if (!is_first_argument || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            auto architectures = NXGetAllArchInfos();
            while (architectures->name != NULL) {
                fprintf(stdout, "%s\n", architectures->name);
                architectures++;
            }

            return 0;
        } else if (strcmp(option, "list-macho-libraries") == 0) {
            if (!is_first_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            auto paths = std::vector<std::pair<std::string, recurse>>();
            if (is_last_argument) {
                const auto current_directory_string = getcwd(nullptr, 0);
                if (!current_directory_string) {
                    fprintf(stdout, "Failed to retrieve current-directory, failing with error (%s)\n", strerror(errno));
                    return 1;
                }

                current_directory = current_directory_string;
                if (current_directory.back() != '/') {
                    current_directory.append(1, '/');
                }

                paths.emplace_back(current_directory, recurse::all);
            } else {
                auto recurse_type = recurse::none;
                for (i++; i < argc; i++) {
                    const auto &argument = argv[i];
                    if (*argument == '-') {
                        auto option = &argument[1];
                        if (*option == '-') {
                            option++;
                        }

                        if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                            recurse_type = recurse::all;
                        } else if (strncmp(option, "r=", 2) == 0 || strncmp(option, "recurse=", 8) == 0) {
                            const auto recurse_type_string = strchr(option, '=') + 1;
                            if (!*recurse_type_string) {
                                fputs("Please provide a recurse type", stderr);
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
                            fprintf(stderr, "Unrecognized argument (%s)\n", argument);
                            return 1;
                        }

                        continue;
                    }

                    if (*argument != '/') {
                        if (current_directory.empty()) {
                            const auto current_directory_string = getcwd(nullptr, 0);
                            if (!current_directory_string) {
                                fprintf(stderr, "Failed to get current-working-directory, failing with error (%s)\n", strerror(errno));
                                return 1;
                            }

                            current_directory = current_directory_string;
                            if (current_directory.back() != '/') {
                                current_directory.append(1, '/');
                            }
                        }

                        auto path = std::string(argument);
                        path.insert(0, current_directory);

                        paths.emplace_back(path, recurse_type);
                    } else {
                        paths.emplace_back(argument, recurse_type);
                    }

                    recurse_type = recurse::none;
                }

                if (paths.empty()) {
                    if (current_directory.empty()) {
                        const auto current_directory_string = getcwd(nullptr, 0);
                        if (!current_directory_string) {
                            fprintf(stdout, "Failed to retrieve current-directory, failing with error (%s)\n", strerror(errno));
                            return 1;
                        }

                        current_directory = current_directory_string;
                        if (current_directory.back() != '/') {
                            current_directory.append(1, '/');
                        }
                    }

                    paths.emplace_back(current_directory, recurse_type);
                }
            }

            for (const auto &pair : paths) {
                const auto &path = pair.first;
                const auto &recurse_type = pair.second;

                if (access(path.data(), F_OK) != 0) {
                    fprintf(stderr, "Object at path (%s) does not exist\n", path.data());
                    return 1;
                }

                struct stat sbuf;
                if (stat(path.data(), &sbuf) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object at path (%s), failing with error (%s)\n", path.data(), strerror(errno));
                    return 1;
                }

                if (S_ISDIR(sbuf.st_mode) && recurse_type == recurse::none) {
                    fprintf(stderr, "Cannot open directory at path (%s) as a macho-file, use -r (or -r=) to recurse the directory\n", path.data());
                    return 1;
                } else if (!S_ISDIR(sbuf.st_mode) && recurse_type != recurse::none) {
                    fprintf(stderr, "Cannot recurse file at path (%s)\n", path.data());
                    return 1;
                }
            }

            for (const auto &pair : paths) {
                const auto &path = pair.first;
                const auto &recurse_type = pair.second;

                if (recurse_type != recurse::none) {
                    const auto directory = opendir(path.data());
                    if (!directory) {
                        fprintf(stderr, "Failed to open directory at path (%s), failing with error (%s)\n", path.data(), strerror(errno));
                        exit(1);
                    }

                    auto library_paths = std::vector<std::string>();
                    loop_directory_for_libraries(directory, path, recurse_type, [&](const std::string &path) {
                        library_paths.emplace_back(path);
                    });

                    if (library_paths.empty()) {
                        switch (recurse_type) {
                            case recurse::none:
                                break;

                            case recurse::once:
                                fprintf(stdout, "No mach-o library files were found while recursing once through path (%s)\n", path.data());
                                break;

                            case recurse::all:
                                fprintf(stdout, "No mach-o library files were found while recursing through path (%s)\n", path.data());
                                break;
                        }
                    } else {
                        switch (recurse_type) {
                            case recurse::none:
                                break;

                            case recurse::once:
                                fprintf(stdout, "Found the following mach-o libraries while recursing once through path (%s)\n", path.data());
                                break;

                            case recurse::all:
                                fprintf(stdout, "Found the following mach-o libraries while recursing through path (%s)\n", path.data());
                                break;
                        }

                        for (auto &library_path : library_paths) {
                            library_path.erase(library_path.begin(), library_path.begin() + path.length());
                            fprintf(stdout, "%s\n", library_path.data());
                        }

                        fputs("\n", stdout);
                    }
                } else {
                    const auto path_is_library = macho::file::is_valid_library(path);
                    if (path_is_library) {
                        fprintf(stdout, "Mach-o file at path (%s) is a library\n", path.data());
                    } else {
                        fprintf(stdout, "Mach-o file at path (%s) is not a library\n", path.data());
                    }
                }
            }

            return 0;
        } else if (strcmp(option, "o") == 0 || strcmp(option, "output") == 0) {
            if (is_last_argument) {
                fputs("Please provide path(s) to output files\n", stderr);
                return 1;
            }

            for (i++; i < argc; i++) {
                auto path = std::string(argv[i]);
                const auto &path_front = path.front();

                if (path_front == '-') {
                    break;
                }

                if (output_paths_index >= tbds.size()) {
                    fprintf(stderr, "No coresponding mach-o files for output-path (%s, at index %d)\n", path.data(), output_paths_index);
                    return 1;
                }

                auto &tbd_recursive = tbds.at(output_paths_index);
                auto &tbd = tbd_recursive.tbd;

                auto &macho_files = tbd.macho_files();
                if (path_front != '/' && path != "stdout") {
                    if (current_directory.empty()) {
                        const auto current_directory_string = getcwd(nullptr, 0);
                        if (!current_directory_string) {
                            fprintf(stderr, "Failed to get current-working-directory, failing with error (%s)\n", strerror(errno));
                            return 1;
                        }

                        current_directory = current_directory_string;
                        if (current_directory.back() != '/') {
                            current_directory.append(1, '/');
                        }
                    }

                    path.insert(0, current_directory);
                } else if (path == "stdout" && macho_files.size() > 1) {
                    fputs("Can't output multiple mach-o files to stdout\n", stderr);
                    return 1;
                }

                const auto &tbd_recurse_type = tbd_recursive.recurse;

                struct stat sbuf;
                if (stat(path.data(), &sbuf) == 0) {
                    if (S_ISDIR(sbuf.st_mode)) {
                        if (tbd_recurse_type == recurse::none) {
                            fprintf(stderr, "Cannot output tbd-file to a directory at path (%s), please provide a full path to a file to output to\n", path.data());
                            return 1;
                        } else if (path == "stdout") {
                            fputs("Cannot output recursive mach-o files to stdout. Please provide a directory to output to", stderr);
                            return 1;
                        }

                        for (const auto &macho_file : macho_files) {
                            const auto macho_file_iter = macho_file.find_last_of('/');
                            auto macho_file_output_path = macho_file.substr(macho_file_iter);

                            macho_file_output_path.insert(0, path);
                            macho_file_output_path.append(".tbd");

                            tbd.add_output_file(macho_file_output_path);
                        }
                    } else if (S_ISREG(sbuf.st_mode)) {
                        if (macho_files.size() > 1) {
                            fprintf(stderr, "Can't output multiple mach-o files and output to file at path (%s)\n", path.data());
                            return 1;
                        }

                        tbd.add_output_file(path);
                    }
                } else {
                    if (macho_files.size() > 1) {
                        fprintf(stderr, "Directory at path (%s) does not exist\n", path.data());
                        return 1;
                    }

                    tbd.add_output_file(path);
                }

                output_paths_index++;
            }

            i--;
        } else if (strcmp(option, "p") == 0 || strcmp(option, "path") == 0) {
            if (is_last_argument) {
                fputs("Please provide path(s) to mach-o files\n", stderr);
                return 1;
            }

            auto local_architectures = std::vector<const NXArchInfo *>();

            auto local_platform = std::string();
            auto local_tbd_version = 0;

            auto recurse_type = recurse::none;
            for (i++; i < argc; i++) {
                const auto &argument = argv[i];
                if (*argument == '-') {
                    auto option = &argument[1];
                    if (*option == '-') {
                        option++;
                    }

                    const auto is_last_argument = i == argc - 1;
                    if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                        recurse_type = recurse::all;
                    } else if (strncmp(option, "r=", 2) == 0 || strncmp(option, "recurse=", 8) == 0) {
                        const auto recurse_type_string = strchr(option, '=') + 1;
                        if (!*recurse_type_string) {
                            fputs("Please provide a recurse type", stderr);
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
                    } else if (strcmp(option, "a") == 0 || strcmp(option, "archs") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                            return 1;
                        }

                        i++;
                        parse_architectures(local_architectures, i);
                    } else if (strcmp(option, "platform") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a platform-string (ios, macosx, tvos, watchos)", stderr);
                            return 1;
                        }

                        i++;

                        const auto &platform_string_arg = argv[i];
                        if (strcmp(platform_string_arg, "ios") != 0 && strcmp(platform_string_arg, "macosx") != 0 && strcmp(platform_string_arg, "watchos") != 0 && strcmp(platform_string_arg, "tvos") != 0) {
                            fprintf(stderr, "Platform-string (%s) is invalid\n", platform_string_arg);
                            return 1;
                        }

                        platform_string = platform_string_arg;
                    } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a tbd-version\n", stderr);
                            return 1;
                        }

                        i++;

                        local_tbd_version = (int)tbd::string_to_version(argv[i]);
                        if (!local_tbd_version) {
                            fprintf(stderr, "(%s) is not a valid tbd-version\n", argv[i]);
                            return 1;
                        }
                    } else {
                        break;
                    }

                    continue;
                }

                auto path = std::string(argument);
                if (path.front() != '/') {
                    if (current_directory.empty()) {
                        const auto current_directory_string = getcwd(nullptr, 0);
                        if (!current_directory_string) {
                            fprintf(stderr, "Failed to get current-working-directory, failing with error (%s)\n", strerror(errno));
                            return 1;
                        }

                        current_directory = current_directory_string;
                        if (current_directory.back() != '/') {
                            current_directory.append(1, '/');
                        }
                    }

                    path.insert(0, current_directory);
                }

                struct stat path_sbuf;
                if (stat(path.data(), &path_sbuf) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object at path (%s), failing with error (%s)\n", path.data(), strerror(errno));
                    return 1;
                }

                auto tbd = ::tbd();

                if (S_ISDIR(path_sbuf.st_mode)) {
                    if (recurse_type == recurse::none) {
                        fprintf(stderr, "Cannot open directory at path (%s) as a macho-file, use -r to recurse the directory\n", path.data());
                        return 1;
                    }

                    if (path.back() != '/') {
                        path.append(1, '/');
                    }

                    const auto directory = opendir(path.data());
                    if (!directory) {
                        fprintf(stderr, "Failed to open directory at path (%s), failing with error (%s)\n", path.data(), strerror(errno));
                        return 1;
                    }

                    loop_directory_for_libraries(directory, path, recurse_type, [&](const std::string &path) {
                        tbd.add_macho_file(path);
                    });

                    closedir(directory);
                } else if (S_ISREG(path_sbuf.st_mode)) {
                    if (recurse_type != recurse::none) {
                        fprintf(stderr, "Cannot recurse file at path (%s)\n", path.data());
                        return 1;
                    }

                    if (!macho::file::is_valid_library(path)) {
                        fprintf(stderr, "File at path (%s) is not a valid mach-o library\n", path.data());
                        return 1;
                    }

                    tbd.add_macho_file(path);
                } else {
                    fprintf(stderr, "Object at path (%s) is not a regular file\n", path.data());
                    return 1;
                }

                provided_macho_files = true;

                const auto &tbd_macho_files = tbd.macho_files();
                if (tbd_macho_files.empty()) {
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

                auto tbd_architectures = &local_architectures;
                if (tbd_architectures->empty()) {
                    tbd_architectures = &architectures;
                }

                auto tbd_platform = &local_platform;
                if (tbd_platform->empty()) {
                    tbd_platform = &platform_string;
                }

                auto tbd_version = &local_tbd_version;
                if (!*tbd_version) {
                    tbd_version = &version;
                }

                tbd.set_architectures(*tbd_architectures);
                tbd.set_platform(tbd::string_to_platform(tbd_platform->data()));
                tbd.set_version(*(enum tbd::version *)tbd_version);

                auto tbd_recurse = tbd_recursive({ tbd, recurse_type });
                tbds.emplace_back(tbd_recurse);

                local_architectures.clear();
                local_platform.clear();

                local_tbd_version = 0;
                recurse_type = recurse::none;

                break;
            }

            if (recurse_type != recurse::none || local_architectures.size() != 0 || local_platform.size() != 0 || local_tbd_version != 0) {
                fputs("Please provide a path to a directory to recurse through\n", stderr);
                return 1;
            }
        } else if (strcmp(option, "platform") == 0) {
            if (is_last_argument) {
                fputs("Please provide a platform-string (ios, macosx, tvos, watchos)", stderr);
                return 1;
            }

            i++;

            const auto &platform_string_arg = argv[i];
            if ((int)tbd::string_to_platform(platform_string_arg) == -1) {
                fprintf(stderr, "Platform-string (%s) is invalid\n", platform_string_arg);
                return 1;
            }

            platform_string = platform_string_arg;
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (i != 1 || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_usage();
        } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
            if (is_last_argument) {
                fputs("Please provide a tbd-version\n", stderr);
                return 1;
            }

            i++;

            const auto &version_string = argv[i];
            if (*version_string == '-') {
                fputs("Please provide a tbd-version\n", stderr);
                return 1;
            }

            if (strcmp(version_string, "v1") == 0) {
                version = 1;
            } else if (strcmp(version_string, "v2") != 0) {
                fprintf(stderr, "tbd-version (%s) is invalid\n", version_string);
                return 1;
            }
        } else if (strcmp(option, "versions") == 0) {
            if (i != 1 || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            fputs("v1\nv2 (default)\n", stdout);
            return 0;
        } else {
            fprintf(stderr, "Unrecognized argument: %s\n", argument);
            return 1;
        }
    }

    if (!provided_macho_files) {
        fputs("No mach-o files have been provided\n", stderr);
        return 1;
    }

    auto tbd_recursive_index = 0;
    for (auto &tbd_recursive : tbds) {
        auto &tbd = tbd_recursive.tbd;

        const auto &platform = tbd.platform();
        const auto &macho_files = tbd.macho_files();

        auto path = macho_files.front();

        const auto &tbd_recurse_type = tbd_recursive.recurse;
        if (tbd_recurse_type != recurse::none) {
            const auto path_position = path.find_last_of('/');
            path.erase(path_position + 1);
        }

        if (platform == (enum tbd::platform)-1) {
            auto tbd_platform = (enum tbd::platform)-1;
            while (platform_string.empty() || (tbd_platform = tbd::string_to_platform(platform_string.data())) == (enum tbd::platform)-1) {
                if (path.back() == '/') {
                    fprintf(stdout, "Please provide a platform for files in directory at path (%s) (ios, macosx, watchos, or tvos): ", path.data());
                } else {
                    fprintf(stdout, "Please provide a platform for file at path (%s) (ios, macosx, watchos, or tvos): ", path.data());
                }

                getline(std::cin, platform_string);
            }

            tbd.set_platform(tbd_platform);
        }

        auto &output_files = tbd.output_files();
        if (output_files.size() != 0) {
            continue;
        }

        if (tbd_recurse_type != recurse::none) {
            for (const auto &macho_file : macho_files) {
                tbd.add_output_file(macho_file + ".tbd");
            }
        }

        tbd_recursive_index++;
    }

    for (auto &tbd_recursive : tbds) {
        auto &tbd = tbd_recursive.tbd;
        tbd.run();
    }
}
