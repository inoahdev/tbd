//
//  src/macho/container.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>

#include "header/symbol_table.h"
#include "container.h"

namespace macho {
    container::container(FILE *file, long base)
    : file_(file), base_(base) {
        const auto position = ftell(file);
        fseek(file, 0, SEEK_END);

        this->size_ = ftell(file);

        fseek(file, position, SEEK_SET);
        this->validate();
    }

    container::container(FILE *file, long base, size_t size)
    : file_(file), base_(base), size_(size) {
        this->validate();
    }

    container::~container() {
        auto &cached = cached_;
        if (cached) {
            delete[] cached;
        }

        auto &string_table = string_table_;
        if (string_table) {
            delete[] string_table;
        }
    }

    void container::validate() {
        auto &base = base_;
        auto &file = file_;

        auto &header = header_;
        auto &magic = header.magic;

        const auto is_big_endian = this->is_big_endian();
        const auto position = ftell(file);

        fseek(file, base, SEEK_SET);
        fread(&magic, sizeof(uint32_t), 1, file);

        const auto macho_file_is_regular = magic_is_thin(magic);
        if (macho_file_is_regular) {
            fread(&header.cputype, sizeof(header) - sizeof(uint32_t), 1, file);

            if (is_big_endian) {
                swap_mach_header(&header);
            }
        } else {
            const auto macho_file_is_fat = magic_is_fat(magic);
            if (macho_file_is_fat) {
                fprintf(stderr, "Architecture at location (0x%.8lX) cannot be a fat mach-o file itself\n", base);
                exit(1);
            } else {
                fprintf(stderr, "Architecture at location (0x%.8lX) is not a valid mach-o base\n", base);
                exit(1);
            }
        }

        fseek(file, position, SEEK_SET);
    }

    void container::iterate_load_commands(const std::function<bool (const struct load_command *)> &callback) {
        const auto &base = base_;
        const auto &file = file_;

        const auto &header = header_;
        const auto is_big_endian = this->is_big_endian();

        const auto &ncmds = header.ncmds;
        const auto &sizeofcmds = header.sizeofcmds;

        auto &cached = cached_;
        auto created_cache = false;

        if (!cached) {
            cached = new char[sizeofcmds];
            created_cache = true;

            auto load_command_base = base + sizeof(header);
            if (this->is_64_bit()) {
                load_command_base += sizeof(uint32_t);
            }

            const auto position = ftell(file);

            fseek(file, load_command_base, SEEK_SET);
            fread(cached, sizeofcmds, 1, file);

            fseek(file, position, SEEK_SET);
        }

        auto size_used = 0;
        auto cached_index = 0;

        auto should_callback = true;
        for (auto i = 0; i < ncmds; i++) {
            auto load_cmd = (struct load_command *)&cached[cached_index];
            const auto &cmdsize = load_cmd->cmdsize;

            if (created_cache) {
                if (is_big_endian) {
                    swap_load_command(load_cmd);
                }

                if (cmdsize < sizeof(struct load_command)) {
                    fprintf(stderr, "Load-command (at index %d) of mach-o container (at base 0x%.8lX) is too small to be valid\n", i, base);
                    exit(1);
                }

                if (cmdsize >= sizeofcmds) {
                    fprintf(stderr, "Load-command (at index %d) of mach-o container (at base 0x%.8lX) is larger than/or equal to entire area allocated for load-commands\n", i, base);
                    exit(1);
                }

                size_used += cmdsize;
                if (size_used > sizeofcmds) {
                    fprintf(stderr, "Load-command (at index %d) of mach-o container (at base 0x%.8lX) goes past end of area allocated for load-commands\n", i, base);
                    exit(1);
                } else if (size_used == sizeofcmds && i != ncmds - 1) {
                    fprintf(stderr, "Load-command (at index %d) of mach-o container (at base 0x%.8lX) takes up all of the remaining space allocated for load-commands\n", i, base);
                    exit(1);
                }
            }

            if (should_callback) {
                auto result = callback(load_cmd);
                if (!result) {
                    should_callback = false;
                }
            }

            if (!should_callback && !created_cache) {
                break;
            }

            cached_index += cmdsize;
        }
    }

    void container::iterate_symbols(const std::function<bool (const struct nlist_64 &, const char *)> &callback) {
        auto &base = base_;
        auto &file = file_;

        const auto is_big_endian = this->is_big_endian();
        const auto position = ftell(file);

        struct symtab_command *symtab_command = nullptr;
        iterate_load_commands([&](const struct load_command *load_cmd) {
            if (load_cmd->cmd != load_commands::symbol_table) {
                return true;
            }

            symtab_command = (struct symtab_command *)load_cmd;
            if (is_big_endian) {
                swap_symtab_command(symtab_command);
            }

            return false;
        });

        if (!symtab_command) {
            fprintf(stderr, "Mach-o container (at base 0x%.8lX) does not have a symbol-table\n", base);
            exit(1);
        }

        const auto &string_table_location = symtab_command->stroff;
        if (string_table_location > size_) {
            fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a string-table outside of its container\n", base);
            exit(1);
        }

        const auto &string_table_size = symtab_command->strsize;
        if (string_table_size > size_) {
            fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a string-table with a size larger than that of itself\n", base);
            exit(1);
        }

        const auto string_table_end = string_table_location + string_table_size;
        if (string_table_end > size_) {
            fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a string-table that goes outside of its container\n", base);
            exit(1);
        }

        const auto &symbol_table_location = symtab_command->symoff;
        if (symbol_table_location > size_) {
            fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table that is outside of its container\n", base);
            exit(1);
        }

        auto &string_table = string_table_;
        if (!string_table) {
            string_table = new char[string_table_size];

            fseek(file, base + string_table_location, SEEK_SET);
            fread(string_table, string_table_size, 1, file);
        }

        const auto &symbol_table_count = symtab_command->nsyms;
        const auto string_table_max_index = string_table_size - 1;

        fseek(file, base + symbol_table_location, SEEK_SET);

        const auto is_64_bit = this->is_64_bit();
        if (is_64_bit) {
            const auto symbol_table_size = sizeof(struct nlist_64) * symbol_table_count;
            if (symbol_table_size > size_) {
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table with a size larger than itself\n", base);
                exit(1);
            }

            const auto symbol_table_end = symbol_table_location + symbol_table_size;
            if (symbol_table_end > size_) {
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table that goes outside of its boundaries\n", base);
                exit(1);
            }

            const auto symbol_table = new struct nlist_64[symbol_table_count];
            fread(symbol_table, symbol_table_size, 1, file);

            if (is_big_endian) {
                swap_nlist_64(symbol_table, symbol_table_count);
            }

            for (auto i = 0; i < symbol_table_count; i++) {
                const auto &symbol_table_entry = symbol_table[i];
                const auto &symbol_table_entry_string_table_index = symbol_table_entry.n_un.n_strx;

                if (symbol_table_entry_string_table_index > string_table_max_index) {
                    fprintf(stderr, "Symbol-table entry (at index %d) has symbol-string past end of string-table of mach-o container (at base 0x%.8lX)\n", i, base);
                    exit(1);
                }

                const auto symbol_table_string_table_string = &string_table[symbol_table_entry_string_table_index];
                const auto result = callback(symbol_table_entry, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }

            delete[] symbol_table;
        } else {
            const auto symbol_table_size = sizeof(struct nlist) * symbol_table_count;
            if (symbol_table_size > size_) {
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table with a size larger than itself\n", base);
                exit(1);
            }

            const auto symbol_table_end = symbol_table_location + symbol_table_size;
            if (symbol_table_end > size_) {
                fprintf(stderr, "Mach-o container (at base 0x%.8lX) has a symbol-table that goes outside of its boundaries\n", base);
                exit(1);
            }

            const auto symbol_table = new struct nlist[symbol_table_count];
            fread(symbol_table, symbol_table_size, 1, file);

            if (is_big_endian) {
                swap_nlist(symbol_table, symbol_table_count);
            }

            for (auto i = 0; i < symbol_table_count; i++) {
                const auto &symbol_table_entry = symbol_table[i];
                const auto &symbol_table_entry_string_table_index = symbol_table_entry.n_un.n_strx;

                if (symbol_table_entry_string_table_index > string_table_max_index) {
                    fprintf(stderr, "Symbol-table entry (at index %d) has symbol-string past end of string-table of mach-o container (at base 0x%.8lX)\n", i, base);
                    exit(1);
                }

                const struct nlist_64 symbol_table_entry_64 = { { symbol_table_entry.n_un.n_strx }, symbol_table_entry.n_type, symbol_table_entry.n_sect, (uint16_t)symbol_table_entry.n_desc, symbol_table_entry.n_value };

                const auto symbol_table_string_table_string = &string_table[symbol_table_entry_string_table_index];
                const auto result = callback(symbol_table_entry_64, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }

            delete[] symbol_table;
        }

        fseek(file, position, SEEK_SET);
    }
}
