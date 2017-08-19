//
//  src/mach-o/container.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cstdio>
#include <cstdlib>

#include "headers/symbol_table.h"
#include "container.h"

namespace macho {
    container::creation_result container::create(container *container, FILE *stream, long base) noexcept {
        container->stream = stream;
        container->base = base;

        const auto position = ftell(stream);
        if (fseek(stream, 0, SEEK_END) != 0) {
            return container::creation_result::stream_seek_error;
        }

        container->size = ftell(stream);

        if (fseek(stream, position, SEEK_SET) != 0) {
            return container::creation_result::stream_seek_error;
        }

        return container->validate();
    }

    container::creation_result container::create(container *container, FILE *stream, long base, size_t size) noexcept {
        container->stream = stream;

        container->base = base;
        container->size = size;

        return container->validate();
    }

    container::creation_result container::validate() noexcept {
        auto &magic = header.magic;

        const auto is_big_endian = this->is_big_endian();
        const auto stream_position = ftell(stream);

        if (fseek(stream, base, SEEK_SET) != 0) {
            return container::creation_result::stream_seek_error;
        }

        if (fread(&magic, sizeof(magic), 1, stream) != 1) {
            return container::creation_result::stream_read_error;
        }

        const auto macho_stream_is_regular = magic_is_thin(magic);
        if (macho_stream_is_regular) {
            if (fread(&header.cputype, sizeof(header) - sizeof(header.magic), 1, stream) != 1) {
                return container::creation_result::stream_read_error;
            }

            if (is_big_endian) {
                swap_mach_header(&header);
            }
        } else {
            const auto macho_stream_is_fat = magic_is_fat(magic);
            if (macho_stream_is_fat) {
                return container::creation_result::fat_container;
            } else {
                return container::creation_result::not_a_macho;
            }
        }

        if (fseek(stream, stream_position, SEEK_SET) != 0) {
            return container::creation_result::stream_seek_error;
        }

        return container::creation_result::ok;
    }

    container::load_command_iteration_result container::iterate_load_commands(const std::function<bool (const struct load_command *, const struct load_command *)> &callback) noexcept {
        const auto is_big_endian = this->is_big_endian();

        const auto &ncmds = header.ncmds;
        const auto &sizeofcmds = header.sizeofcmds;

        auto &cached_load_commands = cached_load_commands_;
        auto created_load_commands_cache = cached_load_commands != nullptr;

        if (!created_load_commands_cache) {
            cached_load_commands = new uint8_t[sizeofcmds];

            auto load_command_base = base + sizeof(header);
            if (is_64_bit()) {
                load_command_base += sizeof(uint32_t);
            }

            const auto position = ftell(stream);

            if (fseek(stream, load_command_base, SEEK_SET) != 0) {
                return container::load_command_iteration_result::stream_seek_error;
            }

            if (fread(cached_load_commands, sizeofcmds, 1, stream) != 1) {
                return container::load_command_iteration_result::stream_read_error;
            }

            if (fseek(stream, position, SEEK_SET) != 0) {
                return container::load_command_iteration_result::stream_seek_error;
            }
        }

        auto size_used = 0;
        auto should_callback = true;

        for (auto i = 0, cached_load_commands_index = 0; i < ncmds; i++) {
            auto load_cmd = (struct load_command *)&cached_load_commands[cached_load_commands_index];
            const auto &cmdsize = load_cmd->cmdsize;

            if (created_load_commands_cache) {
                auto load_command = *load_cmd;
                if (is_big_endian) {
                    swap_load_command(&load_command);
                }

                const auto &cmdsize = load_command.cmdsize;
                if (cmdsize < sizeof(struct load_command)) {
                    return container::load_command_iteration_result::load_command_is_too_small;
                }

                if (cmdsize >= sizeofcmds) {
                    return container::load_command_iteration_result::load_command_is_too_large;
                }

                size_used += cmdsize;
                if (size_used > sizeofcmds) {
                    return container::load_command_iteration_result::load_command_is_too_large;
                } else if (size_used == sizeofcmds && i != ncmds - 1) {
                    return container::load_command_iteration_result::load_command_is_too_large;
                }

                if (should_callback) {
                    should_callback = callback(&load_command, load_cmd);
                }

                cached_load_commands_index += cmdsize;
            } else {
                should_callback = callback(load_cmd, load_cmd);
                if (!should_callback) {
                    break;
                }

                cached_load_commands_index += cmdsize;
            }
        }

        return container::load_command_iteration_result::ok;
    }

    container::symbols_iteration_result container::iterate_symbols(const std::function<bool (const struct nlist_64 &, const char *)> &callback) noexcept {
        const auto is_big_endian = this->is_big_endian();
        const auto position = ftell(stream);

        auto &symbol_table = symbol_table_;
        if (!symbol_table) {
            iterate_load_commands([&](const struct load_command *load_cmd, const struct load_command *load_command) {
                if (load_cmd->cmd != load_commands::symbol_table) {
                    return true;
                }

                symbol_table = (struct symtab_command *)load_command;
                return false;
            });

            if (!symbol_table) {
                return container::symbols_iteration_result::ok;
            }
        }

        auto &cached_string_table = cached_string_table_;
        if (!cached_string_table) {
            auto string_table_location = symbol_table->stroff;
            if (is_big_endian) {
                macho::swap_uint32(&string_table_location);
            }

            if (string_table_location > size) {
                return container::symbols_iteration_result::invalid_string_table;
            }

            const auto &string_table_size = symbol_table->strsize;
            if (string_table_size > size) {
                return container::symbols_iteration_result::invalid_string_table;
            }

            const auto string_table_end = string_table_location + string_table_size;
            if (string_table_end > size) {
                return container::symbols_iteration_result::invalid_string_table;
            }

            cached_string_table = new char[string_table_size];

            if (fseek(stream, base + string_table_location, SEEK_SET) != 0) {
                return container::symbols_iteration_result::stream_seek_error;
            }

            if (fread(cached_string_table, string_table_size, 1, stream) != 1) {
                return container::symbols_iteration_result::stream_read_error;
            }
        }

        auto &cached_symbol_table = cached_symbol_table_;
        if (!cached_symbol_table) {
            auto symbol_table_count = symbol_table->nsyms;
            auto symbol_table_location = symbol_table->symoff;

            if (is_big_endian) {
                macho::swap_uint32(&symbol_table_count);
                macho::swap_uint32(&symbol_table_location);
            }

            if (symbol_table_location > size) {
                return container::symbols_iteration_result::invalid_symbol_table;
            }

            if (fseek(stream, base + symbol_table_location, SEEK_SET) != 0) {
                return container::symbols_iteration_result::stream_seek_error;
            }

            const auto is_64_bit = this->is_64_bit();
            if (is_64_bit) {
                const auto symbol_table_size = sizeof(struct nlist_64) * symbol_table_count;
                if (symbol_table_size > size) {
                    return container::symbols_iteration_result::invalid_symbol_table;
                }

                const auto symbol_table_end = symbol_table_location + symbol_table_size;
                if (symbol_table_end > size) {
                    return container::symbols_iteration_result::invalid_symbol_table;
                }

                cached_symbol_table = new uint8_t[symbol_table_size];

                if (fread(cached_symbol_table, symbol_table_size, 1, stream) != 1) {
                    return container::symbols_iteration_result::stream_read_error;
                }

                if (is_big_endian) {
                    swap_nlist_64((struct nlist_64 *)cached_symbol_table, symbol_table_count);
                }
            } else {
                const auto symbol_table_size = sizeof(struct nlist) * symbol_table_count;
                if (symbol_table_size > size) {
                    return container::symbols_iteration_result::invalid_symbol_table;
                }

                const auto symbol_table_end = symbol_table_location + symbol_table_size;
                if (symbol_table_end > size) {
                    return container::symbols_iteration_result::invalid_symbol_table;
                }

                cached_symbol_table = new uint8_t[symbol_table_size];

                if (fread(cached_symbol_table, symbol_table_size, 1, stream) != 1) {
                    return container::symbols_iteration_result::stream_read_error;
                }

                if (is_big_endian) {
                    swap_nlist((struct nlist *)cached_symbol_table, symbol_table_count);
                }
            }

            if (fseek(stream, position, SEEK_SET) != 0) {
                return container::symbols_iteration_result::stream_seek_error;
            }
        }

        const auto &symbol_table_count = symbol_table->nsyms;
        const auto &string_table_size = symbol_table->strsize;

        const auto string_table_max_index = string_table_size - 1;
        const auto container_is_64_bit = this->is_64_bit();

        if (container_is_64_bit) {
            for (auto i = 0; i < symbol_table_count; i++) {
                const auto cached_symbol_table_index = sizeof(struct nlist_64) * i;

                const auto &symbol_table_entry = (struct nlist_64 *)&cached_symbol_table[cached_symbol_table_index];
                const auto &symbol_table_entry_string_table_index = symbol_table_entry->n_un.n_strx;

                if (symbol_table_entry_string_table_index > string_table_max_index) {
                    return container::symbols_iteration_result::invalid_symbol_table_entry;
                }

                const auto symbol_table_string_table_string = &cached_string_table[symbol_table_entry_string_table_index];
                const auto result = callback(*symbol_table_entry, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }
        } else {
            for (auto i = 0; i < symbol_table_count; i++) {
                const auto cached_symbol_table_index = sizeof(struct nlist) * i;

                const auto &symbol_table_entry = (struct nlist *)&cached_symbol_table[cached_symbol_table_index];
                const auto &symbol_table_entry_string_table_index = symbol_table_entry->n_un.n_strx;

                if (symbol_table_entry_string_table_index > string_table_max_index) {
                    return container::symbols_iteration_result::invalid_symbol_table_entry;
                }

                const struct nlist_64 symbol_table_entry_64 = { { symbol_table_entry->n_un.n_strx }, symbol_table_entry->n_type, symbol_table_entry->n_sect, (uint16_t)symbol_table_entry->n_desc, symbol_table_entry->n_value };

                const auto symbol_table_string_table_string = &cached_string_table[symbol_table_entry_string_table_index];
                const auto result = callback(symbol_table_entry_64, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }
        }

        return container::symbols_iteration_result::ok;
    }
}
