//
//  src/macho/container.cc
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
    container::container(FILE *stream, long base)
    : stream_(stream), base_(base) {
        const auto position = ftell(stream);
        fseek(stream, 0, SEEK_END);

        this->size_ = ftell(stream);

        fseek(stream, position, SEEK_SET);
        this->validate();
    }

    container::container(FILE *stream, long base, size_t size)
    : stream_(stream), base_(base), size_(size) {
        this->validate();
    }

    container::~container() {
        auto &cached_load_commands = cached_load_commands_;
        if (cached_load_commands) {
            delete[] cached_load_commands;
        }

        auto &cached_string_table = cached_string_table_;
        if (cached_string_table) {
            delete[] cached_string_table;
        }

        auto &cached_symbol_table = cached_symbol_table_;
        if (cached_symbol_table) {
            delete[] cached_symbol_table;
        }
    }

    void container::validate() {
        auto &base = base_;
        auto &stream = stream_;

        auto &header = header_;
        auto &magic = header.magic;

        const auto is_big_endian = this->is_big_endian();
        const auto stream_position = ftell(stream);

        fseek(stream, base, SEEK_SET);
        fread(&magic, sizeof(uint32_t), 1, stream);

        const auto macho_stream_is_regular = magic_is_thin(magic);
        if (macho_stream_is_regular) {
            fread(&header.cputype, sizeof(header) - sizeof(uint32_t), 1, stream);

            if (is_big_endian) {
                swap_mach_header(&header);
            }
        } else {
            const auto macho_stream_is_fat = magic_is_fat(magic);
            if (macho_stream_is_fat) {
                fprintf(stderr, "Architecture at location (0x%.8lX) cannot be a fat mach-o file itself\n", base);
                exit(EXIT_FAILURE);
            } else {
                fprintf(stderr, "Architecture at location (0x%.8lX) is not a valid mach-o base\n", base);
                exit(EXIT_FAILURE);
            }
        }

        fseek(stream, stream_position, SEEK_SET);
    }

    void container::iterate_load_commands(const std::function<bool (const struct load_command *, const struct load_command *)> &callback) {
        const auto &base = base_;
        const auto &stream = stream_;

        const auto &header = header_;
        const auto is_big_endian = this->is_big_endian();

        const auto &ncmds = header.ncmds;
        const auto &sizeofcmds = header.sizeofcmds;

        auto &cached_load_commands = cached_load_commands_;
        auto created_load_commands_cache = !cached_load_commands_;

        if (created_load_commands_cache) {
            cached_load_commands = new uint8_t[sizeofcmds];

            auto load_command_base = base + sizeof(header);
            if (is_64_bit()) {
                load_command_base += sizeof(uint32_t);
            }

            const auto position = ftell(stream);

            fseek(stream, load_command_base, SEEK_SET);
            fread(cached_load_commands, sizeofcmds, 1, stream);

            fseek(stream, position, SEEK_SET);
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
                    fprintf(stderr, "Load-command (at index %d) of mach-o container (at base 0x%.8lX) is too small to be valid\n", i, base);
                    exit(EXIT_FAILURE);
                }

                if (cmdsize >= sizeofcmds) {
                    fprintf(stderr, "Load-command (at index %d) of mach-o container (at base 0x%.8lX) is larger than/or equal to entire area allocated for load-commands\n", i, base);
                    exit(EXIT_FAILURE);
                }

                size_used += cmdsize;
                if (size_used > sizeofcmds) {
                    fprintf(stderr, "Load-command (at index %d) of mach-o container (at base 0x%.8lX) goes past end of area allocated for load-commands\n", i, base);
                    exit(EXIT_FAILURE);
                } else if (size_used == sizeofcmds && i != ncmds - 1) {
                    fprintf(stderr, "Load-command (at index %d) of mach-o container (at base 0x%.8lX) takes up all of the remaining space allocated for load-commands\n", i, base);
                    exit(EXIT_FAILURE);
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
    }

    void container::iterate_symbols(const std::function<bool (const struct nlist_64 &, const char *)> &callback) {
        auto &base = base_;
        auto &size = size_;

        auto &stream = stream_;

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
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) does not have a symbol-table\n", base);
                exit(EXIT_FAILURE);
            }
        }

        auto &cached_string_table = cached_string_table_;
        if (!cached_string_table) {
            auto string_table_location = symbol_table->stroff;
            if (is_big_endian) {
                macho::swap_uint32(&string_table_location);
            }

            if (string_table_location > size) {
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a string-table outside of its container\n", base);
                exit(EXIT_FAILURE);
            }

            const auto &string_table_size = symbol_table->strsize;
            if (string_table_size > size) {
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a string-table with a size larger than that of itself\n", base);
                exit(EXIT_FAILURE);
            }

            const auto string_table_end = string_table_location + string_table_size;
            if (string_table_end > size) {
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a string-table that goes outside of its container\n", base);
                exit(EXIT_FAILURE);
            }

            cached_string_table = new char[string_table_size];

            fseek(stream, base + string_table_location, SEEK_SET);
            fread(cached_string_table, string_table_size, 1, stream);
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
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table that is outside of its container\n", base);
                exit(EXIT_FAILURE);
            }

            fseek(stream, base + symbol_table_location, SEEK_SET);

            const auto is_64_bit = this->is_64_bit();
            if (is_64_bit) {
                const auto symbol_table_size = sizeof(struct nlist_64) * symbol_table_count;
                if (symbol_table_size > size) {
                    fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table with a size larger than itself\n", base);
                    exit(EXIT_FAILURE);
                }

                const auto symbol_table_end = symbol_table_location + symbol_table_size;
                if (symbol_table_end > size) {
                    fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table that goes outside of its boundaries\n", base);
                    exit(EXIT_FAILURE);
                }

                cached_symbol_table = new uint8_t[symbol_table_size];
                fread(cached_symbol_table, symbol_table_size, 1, stream);

                if (is_big_endian) {
                    swap_nlist_64((struct nlist_64 *)cached_symbol_table, symbol_table_count);
                }
            } else {
                const auto symbol_table_size = sizeof(struct nlist) * symbol_table_count;
                if (symbol_table_size > size) {
                    fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table with a size larger than itself\n", base);
                    exit(EXIT_FAILURE);
                }

                const auto symbol_table_end = symbol_table_location + symbol_table_size;
                if (symbol_table_end > size) {
                    fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table that goes outside of its boundaries\n", base);
                    exit(EXIT_FAILURE);
                }

                cached_symbol_table = new uint8_t[symbol_table_size];
                fread(cached_symbol_table, symbol_table_size, 1, stream);

                if (is_big_endian) {
                    swap_nlist((struct nlist *)cached_symbol_table, symbol_table_count);
                }
            }

            fseek(stream, position, SEEK_SET);
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
                    fprintf(stderr, "Symbol-table entry (at index %d) has symbol-string (at index %u) past end of string-table (of size %u) of mach-o container (at base 0x%.8lX)\n", i, symbol_table_entry_string_table_index, string_table_size, base);
                    exit(EXIT_FAILURE);
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
                    fprintf(stderr, "Symbol-table entry (at index %d) has symbol-string (at index %u) past end of string-table (of size %u) of mach-o container (at base 0x%.8lX)\n", i, symbol_table_entry_string_table_index, string_table_size, base);
                    exit(EXIT_FAILURE);
                }

                const struct nlist_64 symbol_table_entry_64 = { { symbol_table_entry->n_un.n_strx }, symbol_table_entry->n_type, symbol_table_entry->n_sect, (uint16_t)symbol_table_entry->n_desc, symbol_table_entry->n_value };

                const auto symbol_table_string_table_string = &cached_string_table[symbol_table_entry_string_table_index];
                const auto result = callback(symbol_table_entry_64, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }
        }
    }
}
