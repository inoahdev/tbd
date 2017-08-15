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
        explicit file(const std::string &path);
        ~file();

        static bool is_valid_file(const std::string &path) noexcept;
        static bool is_valid_library(const std::string &path) noexcept;

        inline const FILE *stream() const noexcept { return stream_; }
        inline std::vector<container> &containers() noexcept { return containers_; }

        inline const bool is_thin() const noexcept { return magic_is_thin(magic_); }
        inline const bool is_fat() const noexcept { return magic_is_fat(magic_); }

    private:
        FILE *stream_;
        magic magic_;

        std::vector<container> containers_ = std::vector<container>();

        inline static uint32_t &swap_value(uint32_t &value) noexcept {
            value = ((value >> 8) & 0x00ff00ff) | ((value << 8) & 0xff00ff00);
            value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);

            return value;
        }

        static bool has_library_command(int descriptor, const header &header) noexcept;
        void validate();
    };
}
