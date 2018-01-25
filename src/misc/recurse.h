//
//  src/misc/recurse.h
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include <cerrno>
#include <string>

#include "../utils/directory.h"
#include "../mach-o/file.h"

namespace misc::recurse {
    struct options {
        explicit inline options() = default;
        explicit inline options(uint64_t value) : value(value) {};

        union {
            uint64_t value = 0;

            struct {
                bool recurse_subdirectories : 1;
                bool print_warnings         : 1;
            } __attribute__((packed));
        };

        inline bool has_none() noexcept { return this->value == 0; }
        inline void clear() noexcept { this->value = 0; }

        inline bool operator==(const options &options) const noexcept { return this->value == options.value; }
        inline bool operator!=(const options &options) const noexcept { return this->value != options.value; }
    };

    struct filetypes {
        explicit inline filetypes() = default;
        explicit inline filetypes(uint64_t value) : value(value) {};

        union {
            uint64_t value = 0;

            struct {
                bool file            : 1;
                bool library         : 1;
                bool dynamic_library : 1;
            } __attribute__((packed));
        };

        inline bool has_none() const noexcept { return this->value == 0; }
        inline void clear() noexcept { this->value = 0; }

        inline bool operator==(const filetypes &filetypes) const noexcept { return this->value == filetypes.value; }
        inline bool operator!=(const filetypes &filetypes) const noexcept { return this->value != filetypes.value; }
    };

    bool is_valid_file_for_filetypes(macho::file &macho_file, const filetypes &filetypes);
    bool is_valid_macho_file_at_path(macho::file &file, const char *path, const filetypes &filetypes, const options &options);

    enum class operation_result {
        ok,
        failed_to_open_directory,
        found_no_matching_files
    };

    template <typename T>
    operation_result macho_files(const char *directory_path, const filetypes &filetypes, options options, T &&callback) noexcept { // <void(std::string &, macho::file &)>
        if (filetypes.has_none()) {
            return operation_result::ok;
        }

        auto failed_to_open_directory = false;
        if (!recurse_directory(directory_path, filetypes, options, callback, false, &failed_to_open_directory)) {
            if (failed_to_open_directory) {
                return operation_result::failed_to_open_directory;
            }

            return operation_result::found_no_matching_files;
        }

        return operation_result::ok;
    }

    template <typename T>
    bool recurse_directory(const char *directory_path, const filetypes &filetypes, const options &options, T &&callback, bool is_subdirectory, bool *failed_to_open_directory = nullptr) noexcept {
        auto directory = utils::directory();
        auto directory_open_result = directory.open(directory_path);

        switch (directory_open_result) {
            case utils::directory::open_result::ok:
                break;

            case utils::directory::open_result::failed_to_open_directory:
                if (options.print_warnings) {
                    if (is_subdirectory) {
                        fprintf(stderr, "Warning: Failed to open sub-directory at path (%s), failing with error: %s\n", directory_path, strerror(errno));
                    }
                }

                if (failed_to_open_directory != nullptr) {
                    *failed_to_open_directory = true;
                }

                return false;
        }

        auto found_valid_files = false;
        struct dots {
            union {
                uint64_t value = 0;

                struct {
                    bool skipped_current : 1;
                    bool skipped_parent  : 1;
                } __attribute__((packed));
            };
        } has_skipped_past_dots;

        for (auto iter = directory.entries.begin; iter != directory.entries.end; iter++) {
            auto should_exit = false;
            switch (iter->d_type) {
                case DT_REG: {
                    auto file = macho::file();
                    auto path = iter.entry_path_from_directory_path(directory_path);

                    if (is_valid_macho_file_at_path(file, path.c_str(), filetypes, options)) {
                        found_valid_files = true;
                        should_exit = !callback(file, path);
                    }

                    break;
                }

                case DT_DIR: {
                    if (!has_skipped_past_dots.skipped_current) {
                        if (strcmp(iter->d_name, ".") == 0) {
                            has_skipped_past_dots.skipped_current = 1;
                            continue;
                        }
                    }

                    if (!has_skipped_past_dots.skipped_parent) {
                        if (strcmp(iter->d_name, "..") == 0) {
                            has_skipped_past_dots.skipped_parent = true;
                            continue;
                        }
                    }

                    auto path = iter.entry_path_from_directory_path(directory_path);
                    if (recurse_directory(path.c_str(), filetypes, options, callback, true)) {
                        found_valid_files = true;
                    }

                    break;
                }

                default:
                    break;
            }

            if (should_exit) {
                break;
            }
        }

        return found_valid_files;
    }
}
