//
//  src/mach-o/container.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <cstdio>

#include "../stream/file/shared.h"
#include "swap.h"

namespace macho {
    class container {
    public:
        explicit container() = default;

        explicit container(const container &container) noexcept;
        explicit container(container &&container) noexcept;

        container &operator=(const container &container) noexcept;
        container &operator=(container &&container) noexcept;

        ~container() noexcept;

        stream::file::shared stream;

        long base = 0;
        size_t size = 0;

        struct header header = {};

        enum class open_result {
            ok,
            invalid_range,

            stream_seek_error,
            stream_read_error,

            fat_container,

            not_a_macho,
            invalid_macho,

            not_a_library,
            not_a_dynamic_library
        };

        open_result open(const stream::file::shared &stream, long base = 0, size_t size = 0) noexcept;
        open_result open_from_library(const stream::file::shared &stream, long base = 0, size_t size = 0) noexcept;
        open_result open_from_dynamic_library(const stream::file::shared &stream, long base = 0, size_t size = 0) noexcept;

        open_result open_copy(const container &container) noexcept;

        enum class load_command_iteration_result {
            ok,
            failed_to_allocate_memory,

            no_load_commands,
            load_commands_area_is_too_small,

            stream_seek_error,
            stream_read_error,

            load_command_is_too_small,
            load_command_is_too_large
        };

        load_command *find_first_of_load_command(load_commands cmd, load_command_iteration_result *result = nullptr);

        template <typename T>
        load_command_iteration_result iterate_load_commands(T &&callback) noexcept { // <bool(long, const struct load_command *, const struct load_command *)>
            const auto magic_is_big_endian = is_big_endian();

            const auto &ncmds = header.ncmds;
            const auto &sizeofcmds = header.sizeofcmds;

            if (!ncmds || sizeofcmds < sizeof(load_command)) {
                return load_command_iteration_result::no_load_commands;
            }

            const auto load_commands_minimum_size = sizeof(load_command) * ncmds;
            if (sizeofcmds < load_commands_minimum_size) {
                return load_command_iteration_result::load_commands_area_is_too_small;
            }

            const auto created_cached_load_commands = !cached_load_commands_;

            auto load_command_base = base + sizeof(header);
            auto magic_is_64_bit = is_64_bit();

            if (magic_is_64_bit) {
                load_command_base += sizeof(uint32_t);
            }

            if (!cached_load_commands_) {
                const auto position = stream.position();

                if (!stream.seek(load_command_base, stream::file::seek_type::beginning)) {
                    return load_command_iteration_result::stream_seek_error;
                }

                cached_load_commands_ = static_cast<uint8_t *>(malloc(sizeofcmds));
                if (!cached_load_commands_) {
                    return load_command_iteration_result::failed_to_allocate_memory;
                }

                if (!stream.read(cached_load_commands_, sizeofcmds)) {
                    delete[] cached_load_commands_;
                    cached_load_commands_ = nullptr;

                    return load_command_iteration_result::stream_read_error;
                }

                if (!stream.seek(position, stream::file::seek_type::beginning)) {
                    return load_command_iteration_result::stream_seek_error;
                }
            }

            auto size_used = uint32_t();
            auto should_callback = true;

            for (auto i = uint32_t(), cached_load_commands_index = uint32_t(); i < ncmds; i++) {
                auto load_cmd = reinterpret_cast<load_command *>(&cached_load_commands_[cached_load_commands_index]);
                auto swapped_load_command = *load_cmd;

                if (magic_is_big_endian) {
                    swap_load_command(swapped_load_command);
                }

                auto cmdsize = swapped_load_command.cmdsize;
                if (created_cached_load_commands) {
                    if (cmdsize < sizeof(load_command)) {
                        return load_command_iteration_result::load_command_is_too_small;
                    }

                    size_used += cmdsize;
                    if (size_used > sizeofcmds || (size_used == sizeofcmds && i != ncmds - 1)) {
                        return load_command_iteration_result::load_command_is_too_large;
                    }
                }

                if (should_callback) {
                    should_callback = callback(load_command_base + cached_load_commands_index, swapped_load_command, load_cmd);
                }

                cached_load_commands_index += cmdsize;
            }

            return load_command_iteration_result::ok;
        }

        enum class symbols_iteration_result {
            ok,
            failed_to_allocate_memory,

            stream_seek_error,
            stream_read_error,

            no_symbol_table_load_command,
            invalid_symbol_table_load_command,

            no_symbols,
            invalid_symbol_table_entry
        };

        template <typename T>
        symbols_iteration_result iterate_symbol_table(T &&callback) noexcept { // <bool(const struct nlist_64 &, const char *)>
            const auto magic_is_big_endian = is_big_endian();
            const auto magic_is_64_bit = is_64_bit();

            const auto position = stream.position();
            const auto symbol_table = static_cast<symtab_command *>(find_first_of_load_command(load_commands::symbol_table));

            if (!symbol_table) {
                return symbols_iteration_result::no_symbol_table_load_command;
            }

            auto symbol_table_cmdsize = symbol_table->cmdsize;
            if (magic_is_big_endian) {
                swap_uint32(symbol_table_cmdsize);
            }

            if (symbol_table_cmdsize != sizeof(symtab_command)) {
                return symbols_iteration_result::invalid_symbol_table_load_command;
            }

            auto &cached_string_table = cached_string_table_;
            if (!cached_string_table) {
                auto string_table_location = symbol_table->stroff;
                if (magic_is_big_endian) {
                    macho::swap_uint32(string_table_location);
                }

                if (!string_table_location) {
                    return symbols_iteration_result::invalid_symbol_table_load_command;
                }

                if (string_table_location > size) {
                    return symbols_iteration_result::invalid_symbol_table_load_command;
                }

                const auto &string_table_size = symbol_table->strsize;
                if (!string_table_size) {
                    return symbols_iteration_result::invalid_symbol_table_load_command;
                }

                if (string_table_size > size) {
                    return symbols_iteration_result::invalid_symbol_table_load_command;
                }

                const auto string_table_end = string_table_location + string_table_size;
                if (string_table_end > size) {
                    return symbols_iteration_result::invalid_symbol_table_load_command;
                }

                if (!stream.seek(base + string_table_location, stream::file::seek_type::beginning)) {
                    return symbols_iteration_result::stream_seek_error;
                }

                cached_string_table = static_cast<char *>(malloc(string_table_size));
                if (!cached_string_table) {
                    return symbols_iteration_result::failed_to_allocate_memory;
                }

                if (!stream.read(cached_string_table, string_table_size)) {
                    delete[] cached_string_table;
                    cached_string_table = nullptr;

                    return symbols_iteration_result::stream_read_error;
                }
            }

            auto &cached_symbol_table = cached_symbol_table_;
            if (!cached_symbol_table) {
                auto symbol_table_count = symbol_table->nsyms;
                auto symbol_table_location = symbol_table->symoff;

                if (magic_is_big_endian) {
                    macho::swap_uint32(symbol_table_count);
                    macho::swap_uint32(symbol_table_location);
                }

                if (!symbol_table_count) {
                    return symbols_iteration_result::no_symbols;
                }

                if (!symbol_table_location) {
                    return symbols_iteration_result::invalid_symbol_table_load_command;
                }

                if (symbol_table_location > size) {
                    return symbols_iteration_result::invalid_symbol_table_load_command;
                }

                if (!stream.seek(base + symbol_table_location, stream::file::seek_type::beginning)) {
                    return symbols_iteration_result::stream_seek_error;
                }

                if (magic_is_64_bit) {
                    const auto symbol_table_size = sizeof(struct nlist_64) * symbol_table_count;
                    if (symbol_table_size > size) {
                        return symbols_iteration_result::invalid_symbol_table_load_command;
                    }

                    const auto symbol_table_end = symbol_table_location + symbol_table_size;
                    if (symbol_table_end > size) {
                        return symbols_iteration_result::invalid_symbol_table_load_command;
                    }

                    cached_symbol_table = static_cast<uint8_t *>(malloc(symbol_table_size));
                    if (!cached_symbol_table) {
                        return symbols_iteration_result::failed_to_allocate_memory;
                    }

                    if (!stream.read(cached_symbol_table, symbol_table_size)) {
                        delete[] cached_symbol_table;
                        cached_symbol_table = nullptr;

                        return symbols_iteration_result::stream_read_error;
                    }

                    if (magic_is_big_endian) {
                        swap_nlists_64(reinterpret_cast<struct nlist_64 *>(cached_symbol_table), symbol_table_count);
                    }
                } else {
                    const auto symbol_table_size = sizeof(struct nlist) * symbol_table_count;
                    if (symbol_table_size > size) {
                        return symbols_iteration_result::invalid_symbol_table_load_command;
                    }

                    const auto symbol_table_end = symbol_table_location + symbol_table_size;
                    if (symbol_table_end > size) {
                        return symbols_iteration_result::invalid_symbol_table_load_command;
                    }

                    cached_symbol_table = static_cast<uint8_t *>(malloc(symbol_table_size));
                    if (!cached_symbol_table) {
                        return symbols_iteration_result::failed_to_allocate_memory;
                    }

                    if (!stream.read(cached_symbol_table, symbol_table_size)) {
                        delete[] cached_symbol_table;
                        cached_symbol_table = nullptr;

                        return symbols_iteration_result::stream_read_error;
                    }

                    if (magic_is_big_endian) {
                        swap_nlists(reinterpret_cast<struct nlist *>(cached_symbol_table), symbol_table_count);
                    }
                }

                if (!stream.seek(position, stream::file::seek_type::beginning)) {
                    return symbols_iteration_result::stream_seek_error;
                }
            }

            const auto &symbol_table_count = symbol_table->nsyms;
            const auto &string_table_size = symbol_table->strsize;

            const auto string_table_max_index = string_table_size - 1;
            if (magic_is_64_bit) {
                for (auto i = uint32_t(); i < symbol_table_count; i++) {
                    const auto &symbol_table_entry = reinterpret_cast<struct nlist_64 *>(cached_symbol_table)[i];
                    const auto &symbol_table_entry_string_table_index = symbol_table_entry.n_un.n_strx;

                    if (symbol_table_entry_string_table_index > string_table_max_index) {
                        return symbols_iteration_result::invalid_symbol_table_entry;
                    }

                    const auto symbol_table_string_table_string = &cached_string_table[symbol_table_entry_string_table_index];
                    const auto result = callback(symbol_table_entry, symbol_table_string_table_string);

                    if (!result) {
                        break;
                    }
                }
            } else {
                for (auto i = uint32_t(); i < symbol_table_count; i++) {
                    const auto &symbol_table_entry = reinterpret_cast<struct nlist *>(cached_symbol_table)[i];
                    const auto &symbol_table_entry_string_table_index = symbol_table_entry.n_un.n_strx;

                    if (symbol_table_entry_string_table_index > string_table_max_index) {
                        return symbols_iteration_result::invalid_symbol_table_entry;
                    }

                    const struct nlist_64 symbol_table_entry_64 = { { symbol_table_entry.n_un.n_strx }, symbol_table_entry.n_type, symbol_table_entry.n_sect, static_cast<uint16_t>(symbol_table_entry.n_desc), symbol_table_entry.n_value };

                    const auto symbol_table_string_table_string = &cached_string_table[symbol_table_entry_string_table_index];
                    const auto result = callback(symbol_table_entry_64, symbol_table_string_table_string);

                    if (!result) {
                        break;
                    }
                }
            }

            return symbols_iteration_result::ok;
        }

        inline const bool is_big_endian() const noexcept { return header.magic == magic::big_endian || header.magic == magic::bits64_big_endian; }

        inline const bool is_32_bit() const noexcept { return magic_is_32_bit(header.magic); }
        inline const bool is_64_bit() const noexcept { return magic_is_64_bit(header.magic); }

    private:
        uint8_t *cached_load_commands_ = nullptr;
        uint8_t *cached_symbol_table_ = nullptr;

        char *cached_string_table_ = nullptr;
        size_t file_size(open_result &result) noexcept;

        enum class validation_type : uint64_t {
            none,
            as_library,
            as_dynamic_library
        };

        template <validation_type type = validation_type::none>
        inline open_result validate_and_load_data() noexcept {
            auto &magic = header.magic;

            const auto magic_is_big_endian = is_big_endian();
            const auto stream_position = stream.position();

            if (!stream.seek(base, stream::file::seek_type::beginning)) {
                return open_result::stream_seek_error;
            }

            if (!stream.read(&magic, sizeof(magic))) {
                return open_result::stream_read_error;
            }

            const auto macho_stream_is_regular = magic_is_thin(magic);
            if (macho_stream_is_regular) {
                if (!stream.read(&header.cputype, sizeof(header) - sizeof(header.magic))) {
                    return open_result::stream_read_error;
                }

                if (magic_is_big_endian) {
                    swap_mach_header(header);
                }
            } else {
                const auto macho_stream_is_fat = magic_is_fat(magic);
                if (macho_stream_is_fat) {
                    return open_result::fat_container;
                } else {
                    return open_result::not_a_macho;
                }
            }

            if (!stream.seek(stream_position, stream::file::seek_type::beginning)) {
                return open_result::stream_seek_error;
            }

            auto file_size_calculation_result = open_result::ok;

            const auto file_size = this->file_size(file_size_calculation_result);
            const auto max_size = file_size - base;

            if (file_size_calculation_result != open_result::ok) {
                return file_size_calculation_result;
            }

            if (!size) {
                size = max_size;
            } else if (size > max_size) {
                return open_result::invalid_range;
            }

            if constexpr (type == validation_type::as_library || type == validation_type::as_dynamic_library) {
                auto filetype = header.filetype;
                if (magic_is_big_endian) {
                    swap_uint32(*reinterpret_cast<uint32_t *>(&filetype));
                }


                if constexpr (type == validation_type::as_library) {
                    if (!filetype_is_library(filetype)) {
                        return open_result::not_a_library;
                    }
                }

                if constexpr (type == validation_type::as_dynamic_library) {
                    if (!filetype_is_dynamic_library(filetype)) {
                        return open_result::not_a_dynamic_library;
                    }
                }

                auto iteration_result = load_command_iteration_result::ok;
                auto identification_dylib = static_cast<dylib_command *>(find_first_of_load_command(load_commands::identification_dylib, &iteration_result));

                switch (iteration_result) {
                    case load_command_iteration_result::ok:
                    case load_command_iteration_result::failed_to_allocate_memory:
                    case load_command_iteration_result::no_load_commands:
                        break;

                    case load_command_iteration_result::stream_seek_error:
                        return open_result::stream_seek_error;

                    case load_command_iteration_result::stream_read_error:
                        return open_result::stream_read_error;

                    case load_command_iteration_result::load_commands_area_is_too_small:
                    case load_command_iteration_result::load_command_is_too_small:
                    case load_command_iteration_result::load_command_is_too_large:
                        return open_result::invalid_macho;
                }

                if (!identification_dylib) {
                    return open_result::not_a_library;
                }

                auto identification_dylib_cmdsize = identification_dylib->cmdsize;
                if (magic_is_big_endian) {
                    swap_uint32(identification_dylib_cmdsize);
                }

                if (identification_dylib_cmdsize < sizeof(dylib_command)) {
                    return open_result::not_a_library;
                }
            }

            return open_result::ok;
        }
    };
}
