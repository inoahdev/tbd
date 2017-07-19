//
//  src/macho/container.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <functional>
#include "swap.h"

namespace macho {
    class container {
    public:
        explicit container(FILE *file, long base);
        explicit container(FILE *file, long base, size_t size);

        ~container();

        void iterate_load_commands(const std::function<bool(const struct load_command *load_cmd)> &callback);
        void iterate_symbols(const std::function<bool(const struct nlist_64 &, const char *)> &callback);

        inline uint32_t &swap_value(uint32_t &value) const noexcept {
            const auto magic_is_big_endian = this->magic_is_big_endian();
            if (magic_is_big_endian) {
                swap_uint32(&value);
            }

            return value;
        }

        inline const FILE *file() const noexcept { return file_; }
        inline const header &header() const noexcept { return header_; }

        inline const bool magic_is_big_endian() const noexcept { return macho::magic_is_big_endian(header_.magic); }

        inline const long &base() const noexcept { return base_; }
        inline const size_t &size() const noexcept { return size_; }

        inline const bool is_32_bit() const noexcept { return magic_is_32_bit(header_.magic); }
        inline const bool is_64_bit() const noexcept { return magic_is_64_bit(header_.magic); }

    private:
        FILE *file_;

        long base_ = 0;
        size_t size_ = 0;

        struct header header_;

        char *cached_ = nullptr;
        char *string_table_ = nullptr;

        void validate();
    };
}
