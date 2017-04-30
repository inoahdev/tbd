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

    exit(0);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fputs("Please run -h or -u to see a list of options\n", stderr);
        return 1;
    }

    auto architectures = std::vector<const NXArchInfo *>();
    auto tbds = std::vector<tbd>();

    auto platform_string = std::string();
    auto version = 2;

    auto current_directory = std::string();
    auto parse_architectures = [&](std::vector<const NXArchInfo *> &architectures, int &index) {
        while (index < argc) {
            const auto &architecture_string = argv[index];
            if (*architecture_string == '-' || *architecture_string == '/') {
                break;
            }

            const auto architecture = NXGetArchInfoFromName(architecture_string);
            if (!architecture) {
                break;
            }

            architectures.emplace_back(architecture);
            index++;
        }
    };

    auto output_paths_index = 0;
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

        const auto is_last_argument = i == argc - 1;
        if (strcmp(option, "a") == 0 || strcmp(option, "archs") == 0) {
            if (is_last_argument) {
                fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                return 1;
            }

            parse_architectures(architectures, i);
            i--;
        } else if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
            if (i != 1 || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_usage();
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
                    fprintf(stderr, "No coresponding mach-o files for output-path (%s)\n", path.data());
                    return 1;
                }

                auto &tbd = tbds.at(output_paths_index);
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

                struct stat sbuf;
                if (stat(path.data(), &sbuf) == 0) {
                    if (S_ISDIR(sbuf.st_mode)) {
                        const auto &macho_files = tbd.macho_files();
                        if (macho_files.size() < 2) {
                            fprintf(stderr, "Cannot output tbd-file to a directory at path (%s), please provide a full path to a file to output to\n", path.data());
                            return 1;
                        } else if (path == "stdout") {
                            fputs("Cannot output recursive mach-o files to stdout. Please provide a directory to output to", stderr);
                            return 1;
                        }

                        for (const auto &macho_file : tbd.macho_files()) {
                            const auto macho_file_iter = macho_file.find_last_of('/');
                            auto macho_file_output_path = macho_file.substr(macho_file_iter);

                            macho_file_output_path.insert(0, path);
                            macho_file_output_path.append(".tbd");

                            tbd.add_output_file(macho_file_output_path);
                        }
                    } else if (S_ISREG(sbuf.st_mode)) {
                        const auto &macho_files = tbd.macho_files();
                        if (macho_files.size() > 1) {
                            fprintf(stderr, "Can't output multiple mach-o files and output to file at path (%s)\n", path.data());
                            return 1;
                        }

                        tbd.add_output_file(path);
                    }
                } else {
                    const auto &macho_files = tbd.macho_files();
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

            auto is_recursive = false;
            for (i++; i < argc; i++) {
                const auto &argument = argv[i];
                if (*argument == '-') {
                    const char *option = &argument[1];
                    if (*option == '-') {
                        option = &option[1];
                    }

                    const auto is_last_argument = i == argc - 1;
                    if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                        is_recursive = true;
                    } else if (strcmp(option, "a") == 0 || strcmp(option, "archs") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                            return 1;
                        }

                        i++;

                        parse_architectures(local_architectures, i);
                        i--;
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
                    if (!is_recursive) {
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

                    std::function<void(DIR *, const std::string &)> loop_directory = [&](DIR *directory, const std::string &directory_path) {
                        auto directory_entry = readdir(directory);
                        while (directory_entry != nullptr) {
                            if (directory_entry->d_type == DT_DIR) {
                                if (strncmp(directory_entry->d_name, ".", directory_entry->d_namlen) == 0 ||
                                    strncmp(directory_entry->d_name, "..", directory_entry->d_namlen) == 0) {
                                    directory_entry = readdir(directory);
                                    continue;
                                }

                                auto sub_directory_path = directory_path;

                                sub_directory_path.append(directory_entry->d_name);
                                sub_directory_path.append(1, '/');

                                const auto sub_directory = opendir(sub_directory_path.data());
                                if (!sub_directory) {
                                    fprintf(stderr, "Failed to open sub-directory at path (%s), failing with error (%s)\n", sub_directory_path.data(), strerror(errno));
                                    exit(1);
                                }

                                loop_directory(sub_directory, sub_directory_path);
                                closedir(sub_directory);
                            } else if (directory_entry->d_type == DT_REG) {
                                const auto &directory_entry_path = directory_path + directory_entry->d_name;

                                if (macho::file::is_valid_library(directory_entry_path)) {
                                    tbd.add_macho_file(directory_entry_path);
                                }
                            }

                            directory_entry = readdir(directory);
                        }
                    };

                    loop_directory(directory, path);
                    closedir(directory);
                } else if (S_ISREG(path_sbuf.st_mode)) {
                    if (is_recursive) {
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

                tbds.emplace_back(tbd);

                local_architectures.clear();
                local_platform.clear();

                local_tbd_version = 0;
                is_recursive = false;

                break;
            }

            if (is_recursive || local_architectures.size() != 0 || local_platform.size() != 0 || local_tbd_version != 0) {
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
            if (strcmp(platform_string_arg, "ios") != 0 && strcmp(platform_string_arg, "macosx") != 0 && strcmp(platform_string_arg, "watchos") != 0 && strcmp(platform_string_arg, "tvos") != 0) {
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

    if (!tbds.size()) {
        fputs("No mach-o files have been provided", stderr);
        return 1;
    }

    for (auto &tbd : tbds) {
        const auto &platform = tbd.platform();
        const auto &macho_files = tbd.macho_files();

        auto path = macho_files.front();
        if (macho_files.size() > 1) {
            const auto path_position = path.find_last_of('/');
            path.erase(path_position + 1);
        }

        if (platform == (enum tbd::platform)-1) {
            while (platform_string.empty() || (platform_string != "ios" && platform_string != "macosx" && platform_string != "watchos" && platform_string != "tvos")) {
                if (path.back() == '/') {
                    fprintf(stdout, "Please provide a platform for files in directory at path (%s) (ios, macosx, watchos, or tvos): ", path.data());
                } else {
                    fprintf(stdout, "Please provide a platform for file at path (%s) (ios, macosx, watchos, or tvos): ", path.data());
                }

                getline(std::cin, platform_string);
            }

            tbd.set_platform(tbd::string_to_platform(platform_string.data()));
        }

        auto &output_files = tbd.output_files();
        if (output_files.size() != 0) {
            continue;
        }

        if (macho_files.size() > 1) {
            for (const auto &macho_file : macho_files) {
                tbd.add_output_file(macho_file + ".tbd");
            }
        } else {
            tbd.add_output_file("stdout");
        }
    }

    for (auto &tbd : tbds) {
        tbd.run();
    }
}
