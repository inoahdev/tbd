//
//  src/mach-o/container.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <cstdio>
#include "swap.h"

namespace macho {
    class container {
    public:
        explicit container() = default;

        explicit container(const container &) = delete;
        explicit container(container &&) noexcept;

        container &operator=(const container &) = delete;
        container &operator=(container &&) noexcept;

        ~container();

        FILE *stream = nullptr;

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

        open_result open(FILE *stream, long base = 0, size_t size = 0) noexcept;
        open_result open_from_library(FILE *stream, long base = 0, size_t size = 0) noexcept;
        open_result open_from_dynamic_library(FILE *stream, long base = 0, size_t size = 0) noexcept;

        open_result open_copy(const container &container) noexcept;

        enum class load_command_iteration_result {
            ok,
            no_load_commands,
            stream_seek_error,
            stream_read_error,
            load_command_is_too_small,
            load_command_is_too_large
        };

        struct load_command *find_first_of_load_command(load_commands cmd, load_command_iteration_result *result = nullptr);

        template <typename T>
        load_command_iteration_result iterate_load_commands(T &&callback) noexcept { // <bool(long, const struct load_command *, const struct load_command *)>
            const auto magic_is_big_endian = is_big_endian();

            const auto &ncmds = header.ncmds;
            const auto &sizeofcmds = header.sizeofcmds;

            if (!ncmds || !sizeofcmds) {
                return load_command_iteration_result::no_load_commands;
            }

            auto &cached_load_commands = cached_load_commands_;
            const auto created_cached_load_commands = !cached_load_commands;

            auto load_command_base = base + sizeof(header);
            auto magic_is_64_bit = is_64_bit();

            if (magic_is_64_bit) {
                load_command_base += sizeof(uint32_t);
            }

            if (!cached_load_commands) {
                const auto position = ftell(stream);

                if (fseek(stream, load_command_base, SEEK_SET) != 0) {
                    return load_command_iteration_result::stream_seek_error;
                }

                cached_load_commands = new uint8_t[sizeofcmds];

                if (fread(cached_load_commands, sizeofcmds, 1, stream) != 1) {
                    delete[] cached_load_commands;
                    cached_load_commands = nullptr;

                    return load_command_iteration_result::stream_read_error;
                }

                if (fseek(stream, position, SEEK_SET) != 0) {
                    return load_command_iteration_result::stream_seek_error;
                }
            }

            auto size_used = uint32_t();
            auto should_callback = true;

            for (auto i = uint32_t(), cached_load_commands_index = uint32_t(); i < ncmds; i++) {
                auto load_cmd = (struct load_command *)&cached_load_commands[cached_load_commands_index];
                auto swapped_load_command = *load_cmd;
                
                if (magic_is_big_endian) {
                    swap_load_command(swapped_load_command);
                }
                
                auto cmdsize = swapped_load_command.cmdsize;
                if (created_cached_load_commands) {
                    if (cmdsize < sizeof(struct load_command)) {
                        return load_command_iteration_result::load_command_is_too_small;
                    }

                    size_used += cmdsize;
                    if (size_used > sizeofcmds || (size_used == sizeofcmds && i != ncmds - 1)) {
                        return load_command_iteration_result::load_command_is_too_large;
                    }
                }
                
                if (should_callback) {
                    should_callback = callback(load_command_base + cached_load_commands_index, &swapped_load_command, load_cmd);
                }

                cached_load_commands_index += cmdsize;
            }

            return load_command_iteration_result::ok;
        }

        enum class symbols_iteration_result {
            ok,

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

            const auto position = ftell(stream);
            const auto symbol_table = (symtab_command *)find_first_of_load_command(load_commands::symbol_table);

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

                if (fseek(stream, base + string_table_location, SEEK_SET) != 0) {
                    return symbols_iteration_result::stream_seek_error;
                }

                cached_string_table = new char[string_table_size];

                if (fread(cached_string_table, string_table_size, 1, stream) != 1) {
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

                if (fseek(stream, base + symbol_table_location, SEEK_SET) != 0) {
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

                    cached_symbol_table = new uint8_t[symbol_table_size];

                    if (fread(cached_symbol_table, symbol_table_size, 1, stream) != 1) {
                        delete[] cached_symbol_table;
                        cached_symbol_table = nullptr;

                        return symbols_iteration_result::stream_read_error;
                    }

                    if (magic_is_big_endian) {
                        swap_nlists_64((struct nlist_64 *)cached_symbol_table, symbol_table_count);
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

                    cached_symbol_table = new uint8_t[symbol_table_size];

                    if (fread(cached_symbol_table, symbol_table_size, 1, stream) != 1) {
                        delete[] cached_symbol_table;
                        cached_symbol_table = nullptr;

                        return symbols_iteration_result::stream_read_error;
                    }

                    if (magic_is_big_endian) {
                        swap_nlists((struct nlist *)cached_symbol_table, symbol_table_count);
                    }
                }

                if (fseek(stream, position, SEEK_SET) != 0) {
                    return symbols_iteration_result::stream_seek_error;
                }
            }

            const auto &symbol_table_count = symbol_table->nsyms;
            const auto &string_table_size = symbol_table->strsize;

            const auto string_table_max_index = string_table_size - 1;
            if (magic_is_64_bit) {
                for (auto i = uint32_t(); i < symbol_table_count; i++) {
                    const auto &symbol_table_entry = &((struct nlist_64 *)cached_symbol_table)[i];
                    const auto &symbol_table_entry_string_table_index = symbol_table_entry->n_un.n_strx;

                    if (symbol_table_entry_string_table_index > string_table_max_index) {
                        return symbols_iteration_result::invalid_symbol_table_entry;
                    }

                    const auto symbol_table_string_table_string = &cached_string_table[symbol_table_entry_string_table_index];
                    const auto result = callback(*symbol_table_entry, symbol_table_string_table_string);

                    if (!result) {
                        break;
                    }
                }
            } else {
                for (auto i = uint32_t(); i < symbol_table_count; i++) {
                    const auto &symbol_table_entry = &((struct nlist *)cached_symbol_table)[i];
                    const auto &symbol_table_entry_string_table_index = symbol_table_entry->n_un.n_strx;

                    if (symbol_table_entry_string_table_index > string_table_max_index) {
                        return symbols_iteration_result::invalid_symbol_table_entry;
                    }

                    const struct nlist_64 symbol_table_entry_64 = { { symbol_table_entry->n_un.n_strx }, symbol_table_entry->n_type, symbol_table_entry->n_sect, static_cast<uint16_t>(symbol_table_entry->n_desc), symbol_table_entry->n_value };

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

        open_result validate_and_load_data(validation_type type = validation_type::none) noexcept;
    };
}
