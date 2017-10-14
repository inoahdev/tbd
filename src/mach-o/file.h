//
//  src/mach-o/file.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

#include "container.h"

namespace macho {
    class file {
    public:
        explicit file() = default;

        FILE *stream = nullptr;
        magic magic = magic::normal;

        std::vector<container> containers = std::vector<container>();

        explicit file(const file &) = delete;
        explicit file(file &&) noexcept;

        file &operator=(const file &) = delete;
        file &operator=(file &&) noexcept;

        ~file() noexcept;

        enum class open_result {
            ok,

            failed_to_open_stream,
            failed_to_allocate_memory,

            stream_seek_error,
            stream_read_error,

            zero_architectures,
            invalid_container,

            not_a_macho,

            not_a_library,
            not_a_dynamic_library
        };

        open_result open(const char *path) noexcept;
        inline open_result open(const std::string &path) noexcept {
            return open(path.data());
        }

        open_result open(const char *path, const char *mode) noexcept;
        inline open_result open(const std::string &path, const char *mode) noexcept {
            return open(path.data(), mode);
        }

        open_result open(FILE *file, const char *mode = "r") noexcept;

        open_result open_from_library(const char *path) noexcept;
        inline open_result open_from_library(const std::string &path) noexcept {
            return open_from_library(path.data());
        }

        open_result open_from_library(const char *path, const char *mode) noexcept;
        inline open_result open_from_library(const std::string &path, const char *mode) noexcept {
            return open_from_library(path.data(), mode);
        }

        open_result open_from_library(FILE *file, const char *mode = "r") noexcept;

        open_result open_from_dynamic_library(const char *path) noexcept;
        inline open_result open_from_dynamic_library(const std::string &path) noexcept {
            return open_from_dynamic_library(path.data());
        }

        open_result open_from_dynamic_library(const char *path, const char *mode) noexcept;
        inline open_result open_from_dynamic_library(const std::string &path, const char *mode) noexcept {
            return open_from_dynamic_library(path.data(), mode);
        }

        open_result open_from_dynamic_library(FILE *file, const char *mode = "r") noexcept;

        open_result open_copy(const file &file);
        open_result open_copy(const file &file, const char *mode);

        enum class check_error {
            ok,
            failed_to_allocate_memory,

            failed_to_open_descriptor,
            failed_to_close_descriptor,

            failed_to_seek_descriptor,
            failed_to_read_descriptor
        };

        static bool is_valid_file(const char *path, check_error *error = nullptr) noexcept;
        static bool is_valid_file(const std::string &path, check_error *error = nullptr) noexcept {
            return is_valid_file(path.data());
        }

        static bool is_valid_library(const char *path, check_error *error = nullptr) noexcept;
        static bool is_valid_library(const std::string &path, check_error *error = nullptr) noexcept {
            return is_valid_library(path.data());
        }

        static bool is_valid_dynamic_library(const char *path, check_error *error = nullptr) noexcept;
        static bool is_valid_dynamic_library(const std::string &path, check_error *error = nullptr) noexcept {
            return is_valid_dynamic_library(path.data());
        }

    private:
        const char *mode_ = nullptr; // for copies

        static bool has_library_command(int descriptor, const struct header *header, check_error *error) noexcept;
        static int get_library_file_descriptor(const char *path, check_error *error);

        enum class file_type : uint64_t {
            none,
            library,
            dynamic_library
        };
        
        static bool is_valid_file_of_file_type(const char *path, file_type type, check_error *error = nullptr) noexcept;
        open_result load_containers(file_type type = file_type::none) noexcept;
    };
}
