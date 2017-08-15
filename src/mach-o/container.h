//
//  src/mach-o/container.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <cstdint>
#include <functional>

#include "swap.h"

namespace macho {
    class container {
    public:
        explicit container(FILE *stream, long base);
        explicit container(FILE *stream, long base, size_t size);

        ~container();

        void iterate_load_commands(const std::function<bool(const struct load_command *, const struct load_command *)> &callback);
        void iterate_symbols(const std::function<bool(const struct nlist_64 &, const char *)> &callback);

        inline uint32_t &swap_value(uint32_t &value) const noexcept {
            const auto is_big_endian = this->is_big_endian();
            if (is_big_endian) {
                swap_uint32(&value);
            }

            return value;
        }

        inline FILE *stream() const noexcept { return stream_; }
        inline const header &header() const noexcept { return header_; }

        inline const bool is_big_endian() const noexcept { return magic_is_big_endian(header_.magic); }

        inline const long &base() const noexcept { return base_; }
        inline const size_t &size() const noexcept { return size_; }

        inline const bool is_32_bit() const noexcept { return magic_is_32_bit(header_.magic); }
        inline const bool is_64_bit() const noexcept { return magic_is_64_bit(header_.magic); }

    private:
        FILE *stream_;

        long base_ = 0;
        size_t size_ = 0;

        struct header header_;

        uint8_t *cached_load_commands_ = nullptr;
        uint8_t *cached_symbol_table_ = nullptr;

        struct symtab_command *symbol_table_ = nullptr;
        char *cached_string_table_ = nullptr;

        void validate();
    };
}
