//
//  src/macho/container.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/loader.h>
#include <mach-o/fat.h>

#include <cstdio>
#include <functional>

namespace macho {
    class container {
    public:
        explicit container(FILE *file, long base);
        explicit container(FILE *file, long base, const struct fat_arch &architecture);
        explicit container(FILE *file, long base, const struct fat_arch_64 &architecture);

        ~container();

        void iterate_load_commands(const std::function<bool(const struct load_command *load_cmd)> &callback);
        void iterate_symbols(const std::function<bool(const struct nlist_64 &, const char *)> &callback);

        inline uint32_t &swap_value(uint32_t &value) const noexcept {
            if (should_swap_) {
                value = ((value >> 8) & 0x00ff00ff) | ((value << 8) & 0xff00ff00);
                value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);
            }

            return value;
        }

        inline const FILE *file() const noexcept { return file_; }
        inline const struct mach_header &header() const noexcept { return header_; }

        inline const bool should_swap() const noexcept { return should_swap_; }

        inline const long base() const noexcept { return base_; }
        inline const long size() const noexcept { return size_; }

        inline const bool is_32_bit() const noexcept { return header_.magic == MH_MAGIC || header_.magic == MH_CIGAM; }
        inline const bool is_64_bit() const noexcept { return header_.magic == MH_MAGIC_64 || header_.magic == MH_CIGAM_64; }

    private:
        FILE *file_;

        long base_ = 0;
        long size_ = 0;

        struct mach_header header_;
        bool should_swap_ = false;

        char *cached_ = nullptr;
        bool swapped_cache = false;

        char *string_table_ = nullptr;

        void validate();
    };
}
