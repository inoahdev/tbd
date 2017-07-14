//
//  src/macho/file.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

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

        inline const FILE *file_pointer() const noexcept { return file_; }
        inline std::vector<container> &containers() noexcept { return containers_; }

        inline const bool is_thin() const noexcept { return magic_ == MH_MAGIC || magic_ == MH_CIGAM || magic_ == MH_MAGIC_64 || magic_ == MH_CIGAM_64; }
        inline const bool is_fat() const noexcept { return magic_ == FAT_MAGIC || magic_ == FAT_CIGAM || magic_ == FAT_MAGIC_64 || magic_ == FAT_CIGAM_64; }

    private:
        FILE *file_;
        uint32_t magic_;

        std::vector<container> containers_ = std::vector<container>();

        inline static uint32_t &swap_value(uint32_t &value) noexcept {
            value = ((value >> 8) & 0x00ff00ff) | ((value << 8) & 0xff00ff00);
            value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);

            return value;
        }
        
        static bool has_library_command(int descriptor, const struct mach_header &header) noexcept;
        void validate();
    };
}
