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
            not_a_macho,
            invalid_container,
        };

        static open_result open(file *file, const std::string &path) noexcept;
        ~file() noexcept;

        enum check_error {
            ok,

            failed_to_open_descriptor,
            failed_to_close_descriptor,

            failed_to_seek_descriptor,
            failed_to_read_descriptor
        };

        static bool is_valid_file(const std::string &path, check_error *error = nullptr) noexcept;
        static bool is_valid_library(const std::string &path, check_error *error = nullptr) noexcept;

        inline static uint32_t &swap_value(uint32_t &value) noexcept {
            value = ((value >> 8) & 0x00ff00ff) | ((value << 8) & 0xff00ff00);
            value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);

            return value;
        }

    private:
        static bool has_library_command(int descriptor, const struct header *header, check_error *error) noexcept;
        open_result validate() noexcept;
    };
}
