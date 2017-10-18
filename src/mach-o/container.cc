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
    container::open_result container::open(FILE *stream, long base, size_t size) noexcept {
        this->stream = stream;

        this->base = base;
        this->size = size;

        return validate_and_load_data();
    }

    container::open_result container::open_from_library(FILE *stream, long base, size_t size) noexcept {
        this->stream = stream;

        this->base = base;
        this->size = size;

        return validate_and_load_data(validation_type::as_library);
    }

    container::open_result container::open_from_dynamic_library(FILE *stream, long base, size_t size) noexcept {
        this->stream = stream;

        this->base = base;
        this->size = size;

        return validate_and_load_data(validation_type::as_dynamic_library);
    }

    container::open_result container::open_copy(const container &container) noexcept {
        stream = container.stream;

        base = container.base;
        size = container.size;

        return validate_and_load_data();
    }

    container::open_result container::validate_and_load_data(container::validation_type type) noexcept {
        auto &magic = header.magic;

        const auto magic_is_big_endian = is_big_endian();
        const auto stream_position = ftell(stream);

        if (fseek(stream, base, SEEK_SET) != 0) {
            return open_result::stream_seek_error;
        }

        if (fread(&magic, sizeof(magic), 1, stream) != 1) {
            return open_result::stream_read_error;
        }

        const auto macho_stream_is_regular = magic_is_thin(magic);
        if (macho_stream_is_regular) {
            if (fread(&header.cputype, sizeof(header) - sizeof(header.magic), 1, stream) != 1) {
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

        if (fseek(stream, stream_position, SEEK_SET) != 0) {
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

        if (type == validation_type::as_library || type == validation_type::as_dynamic_library) {
            auto filetype = header.filetype;
            if (magic_is_big_endian) {
                swap_uint32(*(uint32_t *)&filetype);
            }

            switch (type) {
                case validation_type::none:
                    break;

                case validation_type::as_library:
                    if (!filetype_is_library(filetype)) {
                        return open_result::not_a_library;
                    }

                    break;

                case validation_type::as_dynamic_library:
                    if (!filetype_is_dynamic_library(filetype)) {
                        return open_result::not_a_dynamic_library;
                    }

                    break;
            }

            auto iteration_result = load_command_iteration_result::ok;
            auto identification_dylib = (dylib_command *)find_first_of_load_command(load_commands::identification_dylib, &iteration_result);

            switch (iteration_result) {
                case load_command_iteration_result::ok:
                    break;

                case load_command_iteration_result::no_load_commands:
                    break;

                case load_command_iteration_result::stream_seek_error:
                    return open_result::stream_seek_error;

                case load_command_iteration_result::stream_read_error:
                    return open_result::stream_read_error;

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

    container::container(container &&container) noexcept :
    stream(container.stream), base(container.base), size(container.size), header(container.header), cached_load_commands_(container.cached_load_commands_),
    cached_symbol_table_(container.cached_symbol_table_), cached_string_table_(container.cached_string_table_) {
        container.base = 0;
        container.size = 0;

        container.header = {};
        container.cached_load_commands_ = nullptr;

        container.cached_string_table_ = nullptr;
        container.cached_symbol_table_ = nullptr;
    }

    container &container::operator=(container &&container) noexcept {
        stream = container.stream;

        base = container.base;
        size = container.size;

        header = container.header;
        cached_load_commands_ = container.cached_load_commands_;

        cached_string_table_ = container.cached_string_table_;
        cached_symbol_table_ = container.cached_symbol_table_;

        container.stream = nullptr;
        container.base = 0;
        container.size = 0;

        container.header = {};
        container.cached_load_commands_ = nullptr;

        container.cached_string_table_ = nullptr;
        container.cached_symbol_table_ = nullptr;

        return *this;
    }

    size_t container::file_size(container::open_result &result) noexcept {
        const auto position = ftell(stream);
        if (fseek(stream, 0, SEEK_END) != 0) {
            result = open_result::stream_seek_error;
            return 0;
        }

        auto size = static_cast<size_t>(ftell(stream));
        if (size < base) {
            result = open_result::invalid_range;
            return size;
        }

        if (fseek(stream, position, SEEK_SET) != 0) {
            result = open_result::stream_seek_error;
            return size;
        }

        result = open_result::ok;
        return size;
    }

    container::~container() {
        auto &cached_load_commands = cached_load_commands_;
        if (cached_load_commands != nullptr) {
            delete[] cached_load_commands;
        }

        auto &cached_symbol_table = cached_symbol_table_;
        if (cached_symbol_table != nullptr) {
            delete[] cached_symbol_table;
        }

        auto &cached_string_table = cached_string_table_;
        if (cached_string_table != nullptr) {
            delete[] cached_string_table;
        }
    }

    struct load_command *container::find_first_of_load_command(load_commands cmd, container::load_command_iteration_result *result) {
        const auto magic_is_big_endian = is_big_endian();

        const auto &ncmds = header.ncmds;
        const auto &sizeofcmds = header.sizeofcmds;

        if (!ncmds || !sizeofcmds) {
            if (result != nullptr) {
                *result = load_command_iteration_result::no_load_commands;
            }

            return nullptr;
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
                if (result != nullptr) {
                    *result = load_command_iteration_result::stream_seek_error;
                }

                return nullptr;
            }

            cached_load_commands = new uint8_t[sizeofcmds];

            if (fread(cached_load_commands, sizeofcmds, 1, stream) != 1) {
                delete[] cached_load_commands;
                cached_load_commands = nullptr;

                if (result != nullptr) {
                    *result = load_command_iteration_result::stream_read_error;
                }

                return nullptr;
            }

            if (fseek(stream, position, SEEK_SET) != 0) {
                if (result != nullptr) {
                    *result = load_command_iteration_result::stream_seek_error;
                }

                return nullptr;
            }
        }

        auto size_used = uint32_t();
        for (auto i = uint32_t(), cached_load_commands_index = uint32_t(); i < ncmds; i++) {
            auto load_cmd = (struct load_command *)&cached_load_commands[cached_load_commands_index];

            auto load_cmd_cmd = load_cmd->cmd;
            auto load_cmd_cmdsize = load_cmd->cmdsize;

            if (created_cached_load_commands) {
                auto swapped_load_command = *load_cmd;
                if (magic_is_big_endian) {
                    swap_load_command(swapped_load_command);
                }

                load_cmd_cmdsize = swapped_load_command.cmdsize;
                if (load_cmd_cmdsize < sizeof(struct load_command)) {
                    if (result != nullptr) {
                        *result = load_command_iteration_result::load_command_is_too_small;
                    }

                    return nullptr;
                }

                size_used += load_cmd_cmdsize;
                if (size_used > sizeofcmds || (size_used == sizeofcmds && i != ncmds - 1)) {
                    if (result != nullptr) {
                        *result = load_command_iteration_result::load_command_is_too_large;
                    }

                    return nullptr;
                }

                load_cmd_cmd = swapped_load_command.cmd;
            }

            if (load_cmd_cmd == cmd) {
                if (result != nullptr) {
                    *result = load_command_iteration_result::ok;
                }

                return load_cmd;
            }

            cached_load_commands_index += load_cmd_cmdsize;
        }

        if (result != nullptr) {
            *result = load_command_iteration_result::ok;
        }

        return nullptr;
    }
}
