//
//  misc/recurse.cc
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>

#include "../mach-o/file.h"
#include "recurse.h"

namespace recurse {
    void _macho_libraries_subdirectories(DIR *directory, const std::string &directory_path, uint64_t options, const std::function<void (std::string &, macho::file &)> &callback) {
        auto directory_entry = readdir(directory);
        auto directory_path_length = directory_path.length();

        while (directory_entry != nullptr) {
            const auto directory_entry_name = directory_entry->d_name;
            const auto directory_entry_name_length = strlen(directory_entry_name);

            const auto directory_entry_is_directory = directory_entry->d_type == DT_DIR;
            if (directory_entry_is_directory) {
                // Ignore the directory-entry for the directory itself, and that of its
                // superior in the directory hierarchy.

                if (strcmp(directory_entry_name, ".") == 0 || strcmp(directory_entry_name, "..") == 0) {
                    directory_entry = readdir(directory);
                    continue;
                }

                auto sub_directory_path = std::string();
                auto sub_directory_path_length = directory_path_length + directory_entry_name_length + 1;

                // Avoid re-allocations by calculating new total-length, and reserving space
                // accordingly.

                sub_directory_path.reserve(sub_directory_path_length);

                // Add a final forward slash to the path as it is a directory, and reduces work
                // needing to be done if a sub-directory is found and needs to be recursed.

                sub_directory_path.append(directory_path);
                sub_directory_path.append(directory_entry_name, directory_entry_name_length);
                sub_directory_path.append(1, '/');

                const auto sub_directory = opendir(sub_directory_path.data());
                if (sub_directory) {
                    _macho_libraries_subdirectories(sub_directory, sub_directory_path, options, callback);
                    closedir(sub_directory);
                } else {
                    if (options & options::print_warnings) {
                        fprintf(stderr, "Warning: Failed to open sub-directory (at path %s), failing with error: %s\n", sub_directory_path.data(), strerror(errno));
                    }
                }
            } else {
                const auto directory_entry_is_regular_file = directory_entry->d_type == DT_REG;
                if (directory_entry_is_regular_file) {
                    auto directory_entry_path = std::string();
                    auto directory_entry_path_length = directory_path_length + directory_entry_name_length;

                    directory_entry_path.reserve(directory_entry_path_length);

                    directory_entry_path.append(directory_path);
                    directory_entry_path.append(directory_entry_name, directory_entry_name_length);

                    auto directory_entry_library_file = macho::file();
                    auto directory_entry_library_file_open_result = directory_entry_library_file.open_from_library(directory_entry_path.data());

                    switch (directory_entry_library_file_open_result) {
                        case macho::file::open_result::ok:
                            callback(directory_entry_path, directory_entry_library_file);
                            break;

                        case macho::file::open_result::failed_to_open_stream:
                            if (options & options::print_warnings) {
                                fprintf(stderr, "Warning: Failed to open file (at path %s), failing with error: %s\n", directory_entry_path.data(), strerror(errno));
                            }

                            break;

                        default:
                            break;
                    }
                }
            }

            directory_entry = readdir(directory);
        }
    }

    operation_result macho_libraries(const char *directory_path, uint64_t options, const std::function<void (std::string &, macho::file &)> &callback) {
        const auto directory = opendir(directory_path);
        if (!directory) {
            return operation_result::failed_to_open_directory;
        }

        const auto directory_path_length = strlen(directory_path);

        auto directory_entry = readdir(directory);
        while (directory_entry != nullptr) {
            const auto directory_entry_name = directory_entry->d_name;
            const auto directory_entry_name_length = strlen(directory_entry_name);

            const auto directory_entry_is_directory = directory_entry->d_type == DT_DIR;
            if (directory_entry_is_directory) {
                if (options & options::recurse_subdirectories) {
                    // Ignore the directory-entry for the directory itself, and that of its
                    // superior in the directory hierarchy.

                    if (strcmp(directory_entry_name, ".") == 0 || strcmp(directory_entry_name, "..") == 0) {
                        directory_entry = readdir(directory);
                        continue;
                    }

                    auto sub_directory_path = std::string();
                    auto sub_directory_path_length = directory_path_length + directory_entry_name_length + 1;

                    // Avoid re-allocations by calculating new total-length, and reserving space
                    // accordingly.

                    sub_directory_path.reserve(sub_directory_path_length);

                    // Add a final forward slash to the path as it is a directory, and reduces work
                    // needing to be done if a sub-directory is found and needs to be recursed.

                    sub_directory_path.append(directory_path);
                    sub_directory_path.append(directory_entry_name, directory_entry_name_length);
                    sub_directory_path.append(1, '/');

                    const auto sub_directory = opendir(sub_directory_path.data());
                    if (sub_directory) {
                        _macho_libraries_subdirectories(sub_directory, sub_directory_path, options, callback);
                        closedir(sub_directory);
                    } else {
                        if (options & options::print_warnings) {
                            fprintf(stderr, "Warning: Failed to open sub-directory (at path %s), failing with error: %s\n", sub_directory_path.data(), strerror(errno));
                        }
                    }
                }
            } else {
                const auto directory_entry_is_regular_file = directory_entry->d_type == DT_REG;
                if (directory_entry_is_regular_file) {
                    auto directory_entry_path = std::string();
                    auto directory_entry_path_length = directory_path_length + directory_entry_name_length;

                    directory_entry_path.reserve(directory_entry_path_length);

                    directory_entry_path.append(directory_path);
                    directory_entry_path.append(directory_entry_name, directory_entry_name_length);

                    auto directory_entry_library_file = macho::file();
                    auto directory_entry_library_file_open_result = directory_entry_library_file.open_from_library(directory_entry_path.data());

                    switch (directory_entry_library_file_open_result) {
                        case macho::file::open_result::ok:
                            callback(directory_entry_path, directory_entry_library_file);
                            break;

                        case macho::file::open_result::failed_to_open_stream:
                            if (options & options::print_warnings) {
                                fprintf(stderr, "Warning: Failed to open file (at path %s), failing with error: %s\n", directory_entry_path.data(), strerror(errno));
                            }

                            break;

                        default:
                            break;
                    }
                }
            }

            directory_entry = readdir(directory);
        }

        closedir(directory);
        return operation_result::ok;
    }

    void _macho_library_paths_subdirectories(DIR *directory, const std::string &directory_path, uint64_t options, const std::function<void(std::string &)> &callback) {
        auto directory_entry = readdir(directory);
        auto directory_path_length = directory_path.length();

        while (directory_entry != nullptr) {
            const auto directory_entry_name = directory_entry->d_name;
            const auto directory_entry_name_length = strlen(directory_entry_name);

            const auto directory_entry_is_directory = directory_entry->d_type == DT_DIR;
            if (directory_entry_is_directory) {
                // Ignore the directory-entry for the directory itself, and that of its
                // superior in the directory hierarchy.

                if (strcmp(directory_entry_name, ".") == 0 || strcmp(directory_entry_name, "..") == 0) {
                    directory_entry = readdir(directory);
                    continue;
                }

                auto sub_directory_path = std::string();
                auto sub_directory_path_length = directory_path_length + directory_entry_name_length + 1;

                // Avoid re-allocations by calculating new total-length, and reserving space
                // accordingly.

                sub_directory_path.reserve(sub_directory_path_length);

                // Add a final forward slash to the path as it is a directory, and reduces work
                // needing to be done if a sub-directory is found and needs to be recursed.

                sub_directory_path.append(directory_path);
                sub_directory_path.append(directory_entry_name, directory_entry_name_length);
                sub_directory_path.append(1, '/');

                const auto sub_directory = opendir(sub_directory_path.data());
                if (sub_directory) {
                    _macho_library_paths_subdirectories(sub_directory, sub_directory_path, options, callback);
                    closedir(sub_directory);
                } else {
                    if (options & options::print_warnings) {
                        fprintf(stderr, "Warning: Failed to open sub-directory (at path %s), failing with error: %s\n", sub_directory_path.data(), strerror(errno));
                    }
                }
            } else {
                const auto directory_entry_is_regular_file = directory_entry->d_type == DT_REG;
                if (directory_entry_is_regular_file) {
                    auto directory_entry_path = std::string();
                    auto directory_entry_path_length = directory_path_length + directory_entry_name_length;

                    directory_entry_path.reserve(directory_entry_path_length);

                    directory_entry_path.append(directory_path);
                    directory_entry_path.append(directory_entry_name, directory_entry_name_length);

                    auto directory_entry_path_is_valid_library_check_error = macho::file::check_error::ok;
                    const auto directory_entry_path_is_valid_library = macho::file::is_valid_library(directory_entry_path, &directory_entry_path_is_valid_library_check_error);

                    switch (directory_entry_path_is_valid_library_check_error) {
                        case macho::file::check_error::failed_to_open_descriptor:
                            if (options & options::print_warnings) {
                                fprintf(stderr, "Warning: Failed to open file (at path %s), failing with error: %s\n", directory_entry_path.data(), strerror(errno));
                            }

                            break;

                        default:
                            break;
                    }

                    if (directory_entry_path_is_valid_library) {
                        callback(directory_entry_path);
                    }
                }
            }

            directory_entry = readdir(directory);
        }
    }

    operation_result macho_library_paths(const char *directory_path, uint64_t options, const std::function<void (std::string &)> &callback) {
        const auto directory = opendir(directory_path);
        if (!directory) {
            return operation_result::failed_to_open_directory;
        }

        const auto directory_path_length = strlen(directory_path);

        auto directory_entry = readdir(directory);
        while (directory_entry != nullptr) {
            const auto directory_entry_name = directory_entry->d_name;
            const auto directory_entry_name_length = strlen(directory_entry_name);

            const auto directory_entry_is_directory = directory_entry->d_type == DT_DIR;
            if (directory_entry_is_directory) {
                if (options & options::recurse_subdirectories) {
                    // Ignore the directory-entry for the directory itself, and that of its
                    // superior in the directory hierarchy.

                    if (strcmp(directory_entry_name, ".") == 0 || strcmp(directory_entry_name, "..") == 0) {
                        directory_entry = readdir(directory);
                        continue;
                    }

                    auto sub_directory_path = std::string();
                    auto sub_directory_path_length = directory_path_length + directory_entry_name_length + 1;

                    // Avoid re-allocations by calculating new total-length, and reserving space
                    // accordingly.

                    sub_directory_path.reserve(sub_directory_path_length);

                    // Add a final forward slash to the path as it is a directory, and reduces work
                    // needing to be done if a sub-directory is found and needs to be recursed.

                    sub_directory_path.append(directory_path);
                    sub_directory_path.append(directory_entry_name, directory_entry_name_length);
                    sub_directory_path.append(1, '/');

                    const auto sub_directory = opendir(sub_directory_path.data());
                    if (sub_directory) {
                        _macho_library_paths_subdirectories(sub_directory, sub_directory_path, options, callback);
                        closedir(sub_directory);
                    } else {
                        if (options & options::print_warnings) {
                            fprintf(stderr, "Warning: Failed to open sub-directory (at path %s), failing with error: %s\n", sub_directory_path.data(), strerror(errno));
                        }
                    }
                }
            } else {
                const auto directory_entry_is_regular_file = directory_entry->d_type == DT_REG;
                if (directory_entry_is_regular_file) {
                    auto directory_entry_path = std::string();
                    auto directory_entry_path_length = directory_path_length + directory_entry_name_length;

                    directory_entry_path.reserve(directory_entry_path_length);

                    directory_entry_path.append(directory_path);
                    directory_entry_path.append(directory_entry_name, directory_entry_name_length);

                    auto directory_entry_path_is_valid_library_check_error = macho::file::check_error::ok;
                    const auto directory_entry_path_is_valid_library = macho::file::is_valid_library(directory_entry_path, &directory_entry_path_is_valid_library_check_error);

                    switch (directory_entry_path_is_valid_library_check_error) {
                        case macho::file::check_error::failed_to_open_descriptor:
                            if (options & options::print_warnings) {
                                fprintf(stderr, "Warning: Failed to open file (at path %s), failing with error: %s\n", directory_entry_path.data(), strerror(errno));
                            }

                            break;

                        default:
                            break;
                    }

                    if (directory_entry_path_is_valid_library) {
                        callback(directory_entry_path);
                    }
                }
            }

            directory_entry = readdir(directory);
        }

        closedir(directory);
        return operation_result::ok;
    }
}
