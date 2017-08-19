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
        explicit container() = default;

        FILE *stream = nullptr;

        long base = 0;
        size_t size = 0;

        struct header header = {};

        enum class creation_result {
            ok,
            stream_seek_error,
            stream_read_error,
            fat_container,
            not_a_macho,
        };

        static creation_result create(container *container, FILE *stream, long base) noexcept;
        static creation_result create(container *container, FILE *stream, long base, size_t size) noexcept;

        enum class load_command_iteration_result {
            ok,
            stream_seek_error,
            stream_read_error,
            load_command_is_too_small,
            load_command_is_too_large
        };

        load_command_iteration_result iterate_load_commands(const std::function<bool(const struct load_command *, const struct load_command *)> &callback) noexcept;

        enum class symbols_iteration_result {
            ok,

            stream_seek_error,
            stream_read_error,

            no_symbol_table_load_command,
            invalid_string_table,
            invalid_symbol_table,
            invalid_symbol_table_entry
        };

        symbols_iteration_result iterate_symbols(const std::function<bool(const struct nlist_64 &, const char *)> &callback) noexcept;

        inline uint32_t &swap_value(uint32_t &value) const noexcept {
            const auto is_big_endian = this->is_big_endian();
            if (is_big_endian) {
                swap_uint32(&value);
            }

            return value;
        }

        inline const bool is_big_endian() const noexcept { return magic_is_big_endian(header.magic); }

        inline const bool is_32_bit() const noexcept { return magic_is_32_bit(header.magic); }
        inline const bool is_64_bit() const noexcept { return magic_is_64_bit(header.magic); }

    private:
        uint8_t *cached_load_commands_ = nullptr;
        uint8_t *cached_symbol_table_ = nullptr;

        struct symtab_command *symbol_table_ = nullptr;
        char *cached_string_table_ = nullptr;

        creation_result validate() noexcept;
    };
}
