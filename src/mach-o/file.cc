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
    file::file(file &&file) noexcept :
    stream(file.stream), magic(file.magic), containers(std::move(file.containers)) {
        file.stream = nullptr;
        file.magic = magic::normal;
    }

    file::open_result file::open(const char *path) noexcept {
        mode_ = "r";
        stream = fopen(path, mode_);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers();
    }

    file::open_result file::open(const char *path, const char *mode) noexcept {
        mode_ = mode;
        stream = fopen(path, mode);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers();
    }

    file::open_result file::open(FILE *file, const char *mode) noexcept {
        mode_ = mode;
        stream = file;

        return load_containers();
    }

    file::open_result file::open_from_library(const char *path) noexcept {
        mode_ = "r";
        stream = fopen(path, mode_);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::library>();
    }

    file::open_result file::open_from_library(const char *path, const char *mode) noexcept {
        mode_ = mode;
        stream = fopen(path, mode);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::library>();
    }

    file::open_result file::open_from_library(FILE *file, const char *mode) noexcept {
        mode_ = mode;
        stream = file;

        return load_containers<type::library>();
    }

    file::open_result file::open_from_dynamic_library(const char *path) noexcept {
        mode_ = "r";
        stream = fopen(path, mode_);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::dynamic_library>();
    }

    file::open_result file::open_from_dynamic_library(const char *path, const char *mode) noexcept {
        mode_ = mode;
        stream = fopen(path, mode);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers<type::dynamic_library>();
    }

    file::open_result file::open_from_dynamic_library(FILE *file, const char *mode) noexcept {
        mode_ = mode;
        stream = file;

        return load_containers<type::dynamic_library>();
    }

    file::open_result file::open_copy(const file &file) {
        if (!file.stream) {
            if (stream != nullptr) {
                fclose(stream);
                stream = nullptr;
            }

            magic = magic::normal;
            containers.clear();

            // Copied an empty file object
            return open_result::ok;
        }

        mode_ = file.mode_;
        stream = freopen(nullptr, mode_, file.stream);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        magic = file.magic;
        containers.reserve(file.containers.size());

        for (auto &container : file.containers) {
            auto new_container = macho::container();
            auto new_container_open_result = new_container.open_copy(container);

            switch (new_container_open_result) {
                case container::open_result::ok:
                    break;

                case container::open_result::invalid_range:
                case container::open_result::stream_seek_error:
                case container::open_result::stream_read_error:
                case container::open_result::fat_container:
                case container::open_result::not_a_macho:
                case container::open_result::invalid_macho:
                    return open_result::invalid_container;

                case container::open_result::not_a_library:
                case container::open_result::not_a_dynamic_library:
                    break;
            }

            new_container.stream = stream;
            containers.emplace_back(std::move(new_container));
        }

        return open_result::ok;
    }

    file::open_result file::open_copy(const file &file, const char *mode) {
        if (!file.stream) {
            if (stream != nullptr) {
                fclose(stream);
                stream = nullptr;
            }

            magic = magic::normal;
            containers.clear();

            // Copied an empty file object
            return open_result::ok;
        }

        stream = freopen(nullptr, mode, file.stream);
        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        magic = file.magic;
        containers.reserve(file.containers.size());

        for (auto &container : file.containers) {
            auto new_container = macho::container();
            auto new_container_open_result = new_container.open_copy(container);

            switch (new_container_open_result) {
                case container::open_result::ok:
                    break;

                case container::open_result::invalid_range:
                case container::open_result::stream_seek_error:
                case container::open_result::stream_read_error:
                case container::open_result::fat_container:
                case container::open_result::not_a_macho:
                case container::open_result::invalid_macho:
                    return open_result::invalid_container;

                case container::open_result::not_a_library:
                case container::open_result::not_a_dynamic_library:
                    break;
            }

            new_container.stream = stream;
            containers.emplace_back(std::move(new_container));
        }

        return open_result::ok;
    }

    file &file::operator=(file &&file) noexcept {
        stream = file.stream;
        magic = file.magic;

        containers = std::move(file.containers);

        stream = nullptr;
        magic = file.magic;

        return *this;
    }

    file::~file() noexcept {
        if (stream != nullptr) {
            fclose(stream);
        }

        containers.clear();
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

        const auto load_commands_size = header->sizeofcmds;
        if (!load_commands_size) {
            return false;
        }

        const auto load_commands = (uint8_t *)malloc(load_commands_size);
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

            return false;
        }

        auto index = uint32_t();
        auto size_left = load_commands_size;

        const auto header_magic_is_big_endian = header->magic == magic::big_endian || header->magic == magic::bits64_big_endian;
        const auto &ncmds = header->ncmds;

        for (auto i = uint32_t(); i < ncmds; i++) {
            auto &load_cmd = *(struct load_command *)&load_commands[index];
            if (header_magic_is_big_endian) {
                swap_load_command(load_cmd);
            }

            auto cmdsize = load_cmd.cmdsize;
            if (cmdsize < sizeof(struct load_command)) {
                return false;
            }

            if (cmdsize > size_left || (cmdsize == size_left && i != ncmds - 1)) {
                return false;
            }

            if (load_cmd.cmd == load_commands::identification_dylib) {
                if (load_cmd.cmdsize < sizeof(dylib_command)) {
                    // Make sure load-command is valid
                    return false;
                }

                return true;
            }

            index += cmdsize;
        }

        return false;
    }
}
