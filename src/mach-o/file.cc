//
//  src/mach-o/file.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>
#include "file.h"

namespace macho {
    file::open_result file::open(const char *path) noexcept {
        const auto result = stream.open(path, "r");
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;;
        }

        return load_containers();
    }

    file::open_result file::open(const char *path, const char *mode) noexcept {
        const auto result = stream.open(path, mode);
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return load_containers();
    }

    file::open_result file::open(FILE *file) noexcept {
        const auto result = stream.open(file);
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return load_containers();
    }

    file::open_result file::open_from_library(const char *path) noexcept {
        const auto result = stream.open(path, "r");
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::library>();
    }

    file::open_result file::open_from_library(const char *path, const char *mode) noexcept {
        const auto result = stream.open(path, mode);
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::library>();
    }

    file::open_result file::open_from_library(FILE *file, const char *mode) noexcept {
        const auto result = stream.open(file);
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::library>();
    }

    file::open_result file::open_from_dynamic_library(const char *path) noexcept {
        const auto result = stream.open(path, "r");
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::dynamic_library>();
    }

    file::open_result file::open_from_dynamic_library(const char *path, const char *mode) noexcept {
        const auto result = stream.open(path, mode);
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::dynamic_library>();
    }

    file::open_result file::open_from_dynamic_library(FILE *file, const char *mode) noexcept {
        const auto result = stream.open(file);
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::dynamic_library>();
    }

    bool file::is_valid_file(const char *path, check_error *error) noexcept {
        return is_valid_file_of_file_type(path, error);
    }

    bool file::is_valid_library(const char *path, check_error *error) noexcept {
        return is_valid_file_of_file_type<type::library>(path, error);
    }

    bool file::is_valid_dynamic_library(const char *path, check_error *error) noexcept {
        return is_valid_file_of_file_type<type::dynamic_library>(path, error);
    }

    bool file::has_library_command(int descriptor, const struct header *header, check_error *error) noexcept {
        const auto header_magic_is_64_bit = magic_is_64_bit(header->magic);

        if (header_magic_is_64_bit) {
            if (lseek(descriptor, sizeof(magic), SEEK_CUR) == -1) {
                if (error != nullptr) {
                    *error = check_error::failed_to_seek_descriptor;
                }

                return false;
            }
        } else {
            const auto header_magic_is_32_bit = magic_is_32_bit(header->magic);
            if (!header_magic_is_32_bit) {
                return false;
            }
        }

        const auto &load_commands_size = header->sizeofcmds;
        const auto &load_commands_count = header->ncmds;

        if (!load_commands_count || load_commands_size < sizeof(load_command)) {
            return false;
        }

        const auto load_commands_minimum_size = sizeof(load_command) * load_commands_count;
        if (load_commands_size < load_commands_minimum_size) {
            return false;
        }

        const auto load_commands = static_cast<uint8_t *>(malloc(load_commands_size));
        if (!load_commands) {
            if (error != nullptr) {
                *error = check_error::failed_to_allocate_memory;
            }

            return false;
        }

        if (read(descriptor, load_commands, load_commands_size) == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_read_descriptor;
            }

            free(load_commands);
            return false;
        }

        auto index = uint32_t();
        auto size_left = load_commands_size;

        const auto header_magic_is_big_endian = header->magic == magic::big_endian || header->magic == magic::bits64_big_endian;

        for (auto i = uint32_t(); i < load_commands_count; i++) {
            auto &load_command = *reinterpret_cast<struct load_command *>(&load_commands[index]);
            if (header_magic_is_big_endian) {
                swap_load_command(load_command);
            }

            const auto cmdsize = load_command.cmdsize;
            if (cmdsize < sizeof(struct load_command)) {
                free(load_commands);
                return false;
            }

            if (cmdsize > size_left || (cmdsize == size_left && i != load_commands_count - 1)) {
                free(load_commands);
                return false;
            }

            if (load_command.cmd == load_commands::identification_dylib) {
                free(load_commands);
                if (load_command.cmdsize < sizeof(dylib_command)) {
                    // Make sure load-command is valid
                    return false;
                }

                return true;
            }

            index += cmdsize;
        }

        free(load_commands);
        return false;
    }

    bool file::is_library() noexcept {
        auto is_library = false;
        for (auto &container : containers) {
            is_library = container.is_library();
            if (!is_library) {
                break;
            }
        }

        return is_library;
    }
    
    bool file::is_dynamic_library() noexcept {
        auto is_dynamic_library = false;
        for (auto &container : containers) {
            is_dynamic_library = container.is_dynamic_library();
            if (!is_dynamic_library) {
                break;
            }
        }

        return is_dynamic_library;
    }
}
