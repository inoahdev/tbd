//
//  src/macho/container.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/swap.h>
#include <cstdlib>

#include "container.h"

namespace macho {
    container::container(FILE *file, long base)
    : file_(file), base_(base), is_architecture_(false) {
        const auto position = ftell(file);
        fseek(file, 0, SEEK_END);

        this->size_ = ftell(file);

        fseek(file, position, SEEK_SET);
        this->validate();
    }

    container::container(FILE *file, long macho_base, const struct fat_arch &architecture)
    : file_(file), base_(macho_base), size_(architecture.size), is_architecture_(true) {
        this->validate();
    }

    container::container(FILE *file, long macho_base, const struct fat_arch_64 &architecture)
    : file_(file), base_(architecture.offset), size_(architecture.size), is_architecture_(true) {
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
        auto &should_swap = should_swap_;
        
        auto position = ftell(file);
        
        fseek(file, base, SEEK_SET);
        fread(&magic, sizeof(uint32_t), 1, file);

        const auto is_regular_macho_file = magic == MH_MAGIC || magic == MH_CIGAM || magic == MH_MAGIC_64 || magic == MH_CIGAM_64;
        if (is_regular_macho_file) {
            fread(&header.cputype, sizeof(struct mach_header) - sizeof(uint32_t), 1, file);
            if (magic == MH_CIGAM || magic == MH_CIGAM_64) {
                should_swap = true;
                swap_mach_header(&header, NX_LittleEndian);
            }
        } else {
            const auto is_fat_macho_file = magic == MH_MAGIC || magic == MH_CIGAM || magic == MH_MAGIC_64 || magic == MH_CIGAM_64;
            if (is_fat_macho_file) {
                fprintf(stderr, "Architecture at offset (%ld) cannot be a fat mach-o file itself\n", base);
                exit(1);
            } else {
                fprintf(stderr, "Architecture at offset (%ld) is not a valid mach-o base\n", base);
                exit(1);
            }
        }

        fseek(file, position, SEEK_SET);
    }

    void container::iterate_load_commands(const std::function<bool (const struct load_command *)> &callback) {
        const auto &base = base_;
        const auto &header = header_;
        const auto &file = file_;
        
        const auto &is_architecture = is_architecture_;
        const auto &should_swap = should_swap_;
        
        const auto &ncmds = header.ncmds;
        const auto &sizeofcmds = header.sizeofcmds;
        
        auto &cached = cached_;
        if (!cached) {
            cached = new char[sizeofcmds];

            auto load_command_base = base + sizeof(struct mach_header);
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
            if (should_swap && !swapped_cache) {
                swap_load_command(load_cmd, NX_LittleEndian);
            }

            const auto &cmdsize = load_cmd->cmdsize;
            if (cmdsize < sizeof(struct load_command)) {
                if (is_architecture) {
                    fprintf(stderr, "Load-command (at index %d) of architecture is too small to be valid\n", i);
                } else {
                    fprintf(stderr, "Load-command (at index %d) of mach-o file is too small to be valid\n", i);
                }

                exit(1);
            }

            if (cmdsize >= sizeofcmds) {
                if (is_architecture) {
                    fprintf(stderr, "Load-command (at index %d) of architecture is larger than/or equal to entire area allocated for load-commands\n", i);
                } else {
                    fprintf(stderr, "Load-command (at index %d) of mach-o file is larger than/or equal to entire area allocated for load-commands\n", i);
                }

                exit(1);
            }

            size_used += cmdsize;
            if (size_used > sizeofcmds) {
                if (is_architecture) {
                    fprintf(stderr, "Load-command (at index %d) of architecture goes past end of area allocated for load-commands\n", i);
                } else {
                    fprintf(stderr, "Load-command (at index %d) of mach-o file goes past end of area allocated for load-commands\n", i);
                }

                exit(1);
            } else if (size_used == sizeofcmds && i != ncmds - 1) {
                if (is_architecture) {
                    fprintf(stderr, "Load-command (at index %d) of architecture takes up all of the remaining space allocated for load-commands\n", i);
                } else {
                    fprintf(stderr, "Load-command (at index %d) of mach-o file takes up all of the remaining space allocated for load-commands\n", i);
                }

                exit(1);
            }

            if (should_callback) {
                auto result = callback(load_cmd);
                if (!result) {
                    should_callback = false;
                }
            } else if (swapped_cache) {
                break;
            }

            cached_index += cmdsize;
        }

        swapped_cache = true;
    }

    void container::iterate_symbols(const std::function<bool (const struct nlist_64 &, const char *)> &callback) {
        auto &base = base_;
        auto &file = file_;
        auto &is_architecture = is_architecture_;
        auto &should_swap = should_swap_;
        
        auto position = ftell(file);

        struct symtab_command *symtab_command = nullptr;
        iterate_load_commands([&](const struct load_command *load_cmd) {
            if (load_cmd->cmd != LC_SYMTAB) {
                return true;
            }

            symtab_command = (struct symtab_command *)load_cmd;
            if (should_swap) {
                swap_symtab_command(symtab_command, NX_LittleEndian);
            }

            return false;
        });

        if (!symtab_command) {
            if (is_architecture) {
                fputs("Architecture does not have a symbol-table\n", stderr);
            } else {
                fputs("Mach-o file does not have a symbol-table\n", stderr);
            }

            exit(1);
        }

        const auto &string_table_offset = symtab_command->stroff;
        if (string_table_offset > size_) {
            if (is_architecture) {
                fputs("Architecture has a string-table outside of its container\n", stderr);
            } else {
                fputs("Mach-o file has a string-table outside of its container\n", stderr);
            }

            exit(1);
        }

        const auto &string_table_size = symtab_command->strsize;
        if (string_table_offset + string_table_size > size_) {
            if (is_architecture) {
                fputs("Architecture has a string-table that goes outside of its container\n", stderr);
            } else {
                fputs("Mach-o file has a string-table that goes outside of its container\n", stderr);
            }

            exit(1);
        }

        const auto &symbol_table_offset = symtab_command->symoff;
        if (symbol_table_offset > size_) {
            if (is_architecture) {
                fputs("Architecture has a symbol-table that is outside of its container\n", stderr);
            } else {
                fputs("Mach-o file has a symbol-table that is outside of its container\n", stderr);
            }

            exit(1);
        }
        
        auto &string_table = string_table_;
        if (!string_table) {
            string_table = new char[string_table_size];

            fseek(file, base + string_table_offset, SEEK_SET);
            fread(string_table, string_table_size, 1, file);
        }

        const auto &symbol_table_count = symtab_command->nsyms;
        fseek(file, base + symbol_table_offset, SEEK_SET);

        if (this->is_64_bit()) {
            const auto symbol_table_size = sizeof(struct nlist_64) * symbol_table_count;
            if (symbol_table_offset + symbol_table_size > size_) {
                if (is_architecture) {
                    fputs("Architecture has a symbol-table that goes outside of its container\n", stderr);
                } else {
                    fputs("Mach-o file has a symbol-table that goes outside of its container\n", stderr);
                }

                exit(1);
            }

            const auto symbol_table = new struct nlist_64[symbol_table_count];
            fread(symbol_table, symbol_table_size, 1, file);

            if (should_swap) {
                swap_nlist_64(symbol_table, symbol_table_count, NX_LittleEndian);
            }

            for (auto i = 0; i < symbol_table_count; i++) {
                const auto &symbol_table_entry = symbol_table[i];
                const auto &symbol_table_entry_string_table_index = symbol_table_entry.n_un.n_strx;

                if (symbol_table_entry_string_table_index > (string_table_size - 1)) {
                    fprintf(stderr, "Symbol-table entry (at index %d) has symbol-string past end of string-table\n", i);
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
            if (symbol_table_offset + symbol_table_size > size_) {
                if (is_architecture) {
                    fputs("Architecture has a symbol-table that goes outside of its container\n", stderr);
                } else {
                    fputs("Mach-o file has a symbol-table that goes outside of its container\n", stderr);
                }

                exit(1);
            }

            const auto symbol_table = new struct nlist[symbol_table_count];
            fread(symbol_table, symbol_table_size, 1, file);

            if (should_swap) {
                swap_nlist(symbol_table, symbol_table_count, NX_LittleEndian);
            }

            for (auto i = 0; i < symbol_table_count; i++) {
                const auto &symbol_table_entry = symbol_table[i];
                const auto &symbol_table_entry_string_table_index = symbol_table_entry.n_un.n_strx;

                if (symbol_table_entry_string_table_index > (string_table_size - 1)) {
                    fprintf(stderr, "Symbol-table entry (at index %d) has symbol-string past end of string-table\n", i);
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
