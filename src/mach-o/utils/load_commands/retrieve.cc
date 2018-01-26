//
//  src/mach-o/utils/load_commands/retrieve.cc
//  tbd
//
//  Created by inoahdev on 11/23/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "../../../utils/swap.h"
#include "../swap.h"

#include "retrieve.h"

namespace macho::utils::load_commands {
    struct load_command *retrieve_first_of_load_command_in_container(const container &container, enum load_commands cmd, retrieve_result *result) noexcept {
        auto load_commands_count = container.header.ncmds;
        auto load_commands_size = container.header.sizeofcmds;

        if (!load_commands_count) {
            if (result != nullptr) {
                *result = retrieve_result::no_load_commands;
            }

            return nullptr;
        }

        if (!load_commands_size) {
            if (result != nullptr) {
                *result = retrieve_result::load_commands_area_is_too_small;
            }

            return nullptr;
        }

        if (container.is_big_endian()) {
            ::utils::swap::uint32(load_commands_count);
            ::utils::swap::uint32(load_commands_size);
        }

        if (load_commands_size < sizeof(load_command)) {
            if (result != nullptr) {
                *result = retrieve_result::load_commands_area_is_too_small;
            }

            return nullptr;
        }

        const auto load_commands_minimum_size = sizeof(load_command) * load_commands_count;
        const auto container_available_size = container.size - sizeof(container.header);

        if (load_commands_minimum_size > container_available_size) {
            if (result != nullptr) {
                *result = retrieve_result::too_many_load_commands;
            }

            return nullptr;
        }

        if (load_commands_size < load_commands_minimum_size) {
            if (result != nullptr) {
                *result = retrieve_result::load_commands_area_is_too_small;
            }

            return nullptr;
        }

        if (load_commands_size > container_available_size) {
            if (result != nullptr) {
                *result = retrieve_result::load_commands_area_is_too_large;
            }

            return nullptr;
        }

        auto load_commands_begin = container.base + sizeof(container.header);
        if (container.is_64_bit()) {
            load_commands_begin += sizeof(uint32_t);
        }

        const auto container_seek_result = container.stream.seek(load_commands_begin, stream::file::seek_type::beginning);
        if (!container_seek_result) {
            if (result != nullptr) {
                *result = retrieve_result::stream_seek_error;
            }

            return nullptr;
        }

        const auto load_commands_max_index = load_commands_count - 1;

        auto size_taken = uint32_t();
        for (auto index = uint32_t(); index < load_commands_count; index++) {
            auto load_command = macho::load_command();
            auto load_command_read_result = container.stream.read(&load_command, sizeof(load_command));

            if (!load_command_read_result) {
                if (result != nullptr) {
                    *result = retrieve_result::stream_read_error;
                }

                return nullptr;
            }

            if (container.is_big_endian()) {
                utils::swap::load_commands::base(load_command);
            }

            if (load_command.cmdsize < sizeof(load_command)) {
                if (result != nullptr) {
                    *result = retrieve_result::invalid_load_command;
                }

                return nullptr;
            }

            if (load_command.cmdsize > load_commands_size) {
                if (result != nullptr) {
                    *result = retrieve_result::invalid_load_command;
                }

                return nullptr;
            }

            if (size_taken + load_command.cmdsize < size_taken) {
                if (result != nullptr) {
                    *result = retrieve_result::invalid_load_command;
                }

                return nullptr;
            }

            size_taken += load_command.cmdsize;
            if (size_taken > load_commands_size || (size_taken == load_commands_size && index != load_commands_max_index)) {
                if (result != nullptr) {
                    *result = retrieve_result::invalid_load_command;
                }

                return nullptr;
            }

            if (load_command.cmd != cmd) {
                const auto load_command_remaining_size = load_command.cmdsize - sizeof(load_command);
                const auto container_seek_result = container.stream.seek(load_command_remaining_size, stream::file::seek_type::current);

                if (!container_seek_result) {
                    if (result != nullptr) {
                        *result = retrieve_result::stream_seek_error;
                    }

                    return nullptr;
                }

                continue;
            }

            const auto full_load_command = reinterpret_cast<struct load_command *>(new uint8_t[load_command.cmdsize]);
            const auto load_command_remaining_size = load_command.cmdsize - sizeof(load_command);

            if (load_command_remaining_size != 0) {
                const auto load_command_remaining_data = &full_load_command[1];
                const auto container_read_result = container.stream.read(load_command_remaining_data, load_command_remaining_size);

                if (!container_read_result) {
                    if (result != nullptr) {
                        *result = retrieve_result::stream_read_error;
                    }

                    delete[] full_load_command;
                    return nullptr;
                }
            }

            if (container.is_big_endian()) {
                swap::load_commands::base(load_command);
            }

            *full_load_command = load_command;
            return full_load_command;
        }

        if (result != nullptr) {
            *result = retrieve_result::no_instance_of_specified_load_command;
        }

        return nullptr;
    }

    retrieve_result find_first_of_load_command_in_container(const container &container, enum load_commands cmd, struct load_command *buffer, uint32_t cmdsize) noexcept {
        if (cmdsize < sizeof(struct load_command)) {
            return retrieve_result::cmdsize_too_small_to_hold_load_command;
        }

        auto load_commands_count = container.header.ncmds;
        auto load_commands_size = container.header.sizeofcmds;

        if (!load_commands_count) {
            return retrieve_result::no_load_commands;
        }

        if (!load_commands_size) {
            return retrieve_result::load_commands_area_is_too_small;
        }

        if (container.is_big_endian()) {
            ::utils::swap::uint32(load_commands_count);
            ::utils::swap::uint32(load_commands_size);
        }

        if (load_commands_size < sizeof(load_command)) {
            return retrieve_result::load_commands_area_is_too_small;
        }

        const auto load_commands_minimum_size = sizeof(load_command) * load_commands_count;

        auto container_available_size = container.size - sizeof(container.header);
        if (container.is_64_bit()) {
            container_available_size -= sizeof(uint32_t);
        }

        if (load_commands_minimum_size > container_available_size) {
            return retrieve_result::too_many_load_commands;
        }

        if (load_commands_size < load_commands_minimum_size) {
            return retrieve_result::load_commands_area_is_too_small;
        }

        if (load_commands_size > container_available_size) {
            return retrieve_result::load_commands_area_is_too_large;
        }

        auto load_commands_begin = container.base + sizeof(container.header);
        if (container.is_64_bit()) {
            load_commands_begin += sizeof(uint32_t);
        }

        const auto container_seek_result = container.stream.seek(load_commands_begin, stream::file::seek_type::beginning);
        if (!container_seek_result) {
            return retrieve_result::stream_seek_error;
        }

        const auto load_commands_max_index = load_commands_count - 1;

        auto size_taken = uint32_t();
        for (auto index = uint32_t(); index < load_commands_count; index++) {
            auto load_command = macho::load_command();
            auto load_command_read_result = container.stream.read(&load_command, sizeof(load_command));

            if (!load_command_read_result) {
                return retrieve_result::stream_read_error;
            }

            if (container.is_big_endian()) {
                utils::swap::load_commands::base(load_command);
            }

            if (load_command.cmdsize < sizeof(load_command)) {
                return retrieve_result::invalid_load_command;
            }

            if (load_command.cmdsize > load_commands_size) {
                return retrieve_result::invalid_load_command;
            }

            if (size_taken + load_command.cmdsize < size_taken) {
                return retrieve_result::invalid_load_command;
            }

            size_taken += load_command.cmdsize;
            if (size_taken > load_commands_size) {
                return retrieve_result::invalid_load_command;
            }

            if (index != load_commands_max_index) {
                if (size_taken == load_commands_size) {
                    return retrieve_result::invalid_load_command;
                }
            }

            if (load_command.cmd != cmd) {
                const auto load_command_remaining_size = load_command.cmdsize - sizeof(load_command);
                if (load_command_remaining_size != 0) {
                    const auto container_seek_result = container.stream.seek(load_command_remaining_size, stream::file::seek_type::current);
                    if (!container_seek_result) {
                        return retrieve_result::stream_seek_error;
                    }
                }

                continue;
            }

            if (cmdsize < load_command.cmdsize) {
                return retrieve_result::cmdsize_too_small_to_hold_load_command;
            }

            const auto load_command_remaining_size = load_command.cmdsize - sizeof(load_command);
            if (load_command_remaining_size != 0) {
                const auto load_command_remaining_data = &buffer[1];
                const auto load_command_remaining_data_read_result = container.stream.read(load_command_remaining_data, load_command_remaining_size);

                if (!load_command_remaining_data_read_result) {
                    return retrieve_result::stream_read_error;
                }
            }

            if (container.is_big_endian()) {
                swap::load_commands::base(load_command);
            }

            *buffer = load_command;
            return retrieve_result::ok;
        }

        return retrieve_result::no_instance_of_specified_load_command;
    }

    struct load_command *retrieve_only_load_command_of_cmd_in_container(const container &container, enum load_commands cmd, retrieve_result *result) noexcept {
        auto load_commands_count = container.header.ncmds;
        auto load_commands_size = container.header.sizeofcmds;

        if (!load_commands_count) {
            if (result != nullptr) {
                *result = retrieve_result::no_load_commands;
            }

            return nullptr;
        }

        if (!load_commands_size) {
            if (result != nullptr) {
                *result = retrieve_result::load_commands_area_is_too_small;
            }

            return nullptr;
        }

        if (container.is_big_endian()) {
            ::utils::swap::uint32(load_commands_count);
            ::utils::swap::uint32(load_commands_size);
        }

        if (load_commands_size < sizeof(load_command)) {
            if (result != nullptr) {
                *result = retrieve_result::load_commands_area_is_too_small;
            }

            return nullptr;
        }

        const auto load_commands_minimum_size = sizeof(load_command) * load_commands_count;
        const auto container_available_size = container.size - sizeof(container.header);

        if (load_commands_minimum_size > container_available_size) {
            if (result != nullptr) {
                *result = retrieve_result::too_many_load_commands;
            }

            return nullptr;
        }

        if (load_commands_size < load_commands_minimum_size) {
            if (result != nullptr) {
                *result = retrieve_result::load_commands_area_is_too_small;
            }

            return nullptr;
        }

        if (load_commands_size > container_available_size) {
            if (result != nullptr) {
                *result = retrieve_result::load_commands_area_is_too_large;
            }

            return nullptr;
        }

        auto load_commands_begin = container.base + sizeof(container.header);
        if (container.is_64_bit()) {
            load_commands_begin += sizeof(uint32_t);
        }

        const auto container_seek_result = container.stream.seek(load_commands_begin, stream::file::seek_type::beginning);
        if (!container_seek_result) {
            if (result != nullptr) {
                *result = retrieve_result::stream_seek_error;
            }

            return nullptr;
        }

        const auto load_commands_max_index = load_commands_count - 1;

        auto size_taken = uint32_t();
        auto resulting_load_command = static_cast<struct load_command *>(nullptr);

        for (auto index = uint32_t(); index < load_commands_count; index++) {
            auto load_command = macho::load_command();
            auto load_command_read_result = container.stream.read(&load_command, sizeof(load_command));

            if (!load_command_read_result) {
                if (result != nullptr) {
                    *result = retrieve_result::stream_read_error;
                }

                return nullptr;
            }

            if (container.is_big_endian()) {
                utils::swap::load_commands::base(load_command);
            }

            if (load_command.cmdsize < sizeof(load_command)) {
                if (result != nullptr) {
                    *result = retrieve_result::invalid_load_command;
                }

                return nullptr;
            }

            if (load_command.cmdsize > load_commands_size) {
                if (result != nullptr) {
                    *result = retrieve_result::invalid_load_command;
                }

                return nullptr;
            }

            if (size_taken + load_command.cmdsize < size_taken) {
                if (result != nullptr) {
                    *result = retrieve_result::invalid_load_command;
                }

                return nullptr;
            }

            size_taken += load_command.cmdsize;
            if (size_taken > load_commands_size || (size_taken == load_commands_size && index != load_commands_max_index)) {
                if (result != nullptr) {
                    *result = retrieve_result::invalid_load_command;
                }

                return nullptr;
            }

            if (load_command.cmd != cmd) {
                const auto load_command_remaining_size = load_command.cmdsize - sizeof(load_command);
                const auto container_seek_result = container.stream.seek(load_command_remaining_size, stream::file::seek_type::current);

                if (!container_seek_result) {
                    if (result != nullptr) {
                        *result = retrieve_result::stream_seek_error;
                    }

                    return nullptr;
                }

                continue;
            }

            if (resulting_load_command != nullptr) {
                if (result != nullptr) {
                    *result = retrieve_result::multiple_load_commands;
                }

                return nullptr;
            }

            const auto full_load_command = reinterpret_cast<struct load_command *>(new uint8_t[load_command.cmdsize]);
            const auto load_command_remaining_size = load_command.cmdsize - sizeof(load_command);

            if (load_command_remaining_size != 0) {
                const auto load_command_remaining_data = &full_load_command[1];
                const auto container_read_result = container.stream.read(load_command_remaining_data, load_command_remaining_size);

                if (!container_read_result) {
                    if (result != nullptr) {
                        *result = retrieve_result::stream_read_error;
                    }

                    delete[] full_load_command;
                    return nullptr;
                }
            }

            if (container.is_big_endian()) {
                swap::load_commands::base(load_command);
            }

            *full_load_command = load_command;
            resulting_load_command = full_load_command;
        }

        if (result != nullptr) {
            *result = retrieve_result::no_instance_of_specified_load_command;
        }

        return resulting_load_command;
    }

    retrieve_result retrieve_only_load_command_of_cmd_in_container(const container &container, enum load_commands cmd, struct load_command *buffer, uint32_t cmdsize) noexcept {
        if (cmdsize < sizeof(struct load_command)) {
            return retrieve_result::cmdsize_too_small_to_hold_load_command;
        }

        auto load_commands_count = container.header.ncmds;
        auto load_commands_size = container.header.sizeofcmds;

        if (!load_commands_count) {
            return retrieve_result::no_load_commands;
        }

        if (!load_commands_size) {
            return retrieve_result::load_commands_area_is_too_small;
        }

        if (container.is_big_endian()) {
            ::utils::swap::uint32(load_commands_count);
            ::utils::swap::uint32(load_commands_size);
        }

        if (load_commands_size < sizeof(load_command)) {
            return retrieve_result::load_commands_area_is_too_small;
        }

        const auto load_commands_minimum_size = sizeof(load_command) * load_commands_count;

        auto container_available_size = container.size - sizeof(container.header);
        if (container.is_64_bit()) {
            container_available_size -= sizeof(uint32_t);
        }

        if (load_commands_minimum_size > container_available_size) {
            return retrieve_result::too_many_load_commands;
        }

        if (load_commands_size < load_commands_minimum_size) {
            return retrieve_result::load_commands_area_is_too_small;
        }

        if (load_commands_size > container_available_size) {
            return retrieve_result::load_commands_area_is_too_large;
        }

        auto load_commands_begin = container.base + sizeof(container.header);
        if (container.is_64_bit()) {
            load_commands_begin += sizeof(uint32_t);
        }

        const auto container_seek_result = container.stream.seek(load_commands_begin, stream::file::seek_type::beginning);
        if (!container_seek_result) {
            return retrieve_result::stream_seek_error;
        }

        const auto load_commands_max_index = load_commands_count - 1;

        auto size_taken = uint32_t();
        auto found = false;

        for (auto index = uint32_t(); index < load_commands_count; index++) {
            auto load_command = macho::load_command();
            auto load_command_read_result = container.stream.read(&load_command, sizeof(load_command));

            if (!load_command_read_result) {
                return retrieve_result::stream_read_error;
            }

            if (container.is_big_endian()) {
                utils::swap::load_commands::base(load_command);
            }

            if (load_command.cmdsize < sizeof(load_command)) {
                return retrieve_result::invalid_load_command;
            }

            if (load_command.cmdsize > load_commands_size) {
                return retrieve_result::invalid_load_command;
            }

            if (size_taken + load_command.cmdsize < size_taken) {
                return retrieve_result::invalid_load_command;
            }

            size_taken += load_command.cmdsize;
            if (size_taken > load_commands_size) {
                return retrieve_result::invalid_load_command;
            }

            if (index != load_commands_max_index) {
                if (size_taken == load_commands_size) {
                    return retrieve_result::invalid_load_command;
                }
            }

            if (load_command.cmd != cmd) {
                const auto load_command_remaining_size = load_command.cmdsize - sizeof(load_command);
                if (load_command_remaining_size != 0) {
                    const auto container_seek_result = container.stream.seek(load_command_remaining_size, stream::file::seek_type::current);
                    if (!container_seek_result) {
                        return retrieve_result::stream_seek_error;
                    }
                }

                continue;
            }

            if (found) {
                return retrieve_result::multiple_load_commands;
            }

            if (cmdsize < load_command.cmdsize) {
                return retrieve_result::cmdsize_too_small_to_hold_load_command;
            }

            const auto load_command_remaining_size = load_command.cmdsize - sizeof(load_command);
            if (load_command_remaining_size != 0) {
                const auto load_command_remaining_data = &buffer[1];
                const auto load_command_remaining_data_read_result = container.stream.read(load_command_remaining_data, load_command_remaining_size);

                if (!load_command_remaining_data_read_result) {
                    return retrieve_result::stream_read_error;
                }
            }

            if (container.is_big_endian()) {
                swap::load_commands::base(load_command);
            }

            *buffer = load_command;
            found = true;
        }

        if (!found) {
            return retrieve_result::no_instance_of_specified_load_command;
        }

        return retrieve_result::ok;
    }
}
