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
        FILE *stream = nullptr;
        magic magic = magic::normal;

        std::vector<container> containers = std::vector<container>();

        enum class open_result {
            ok,
            failed_to_open_stream,

            stream_seek_error,
            stream_read_error,

            zero_architectures,
            invalid_container,

            not_a_macho,
            not_a_library,
        };

        static open_result open(file *file, const char *path) noexcept;
        static open_result open(file *file, const std::string &path) noexcept {
            return open(file, path.data());
        }

        static open_result open_from_library(file *file, const char *path) noexcept;
        static open_result open_from_library(file *file, const std::string &path) noexcept {
            return open(file, path.data());
        }

        ~file() noexcept;

        enum check_error {
            ok,

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

        inline static uint32_t &swap_value(uint32_t &value) noexcept {
            value = ((value >> 8) & 0x00ff00ff) | ((value << 8) & 0xff00ff00);
            value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);

            return value;
        }

    private:
        static bool has_library_command(int descriptor, const struct header *header, check_error *error) noexcept;
        static int get_library_file_descriptor(const char *path, check_error *error);
    };
}
