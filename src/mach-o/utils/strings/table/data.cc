//
//  src/mach-o/utils/strings/table/data.cc
//  tbd
//
//  Created by inoahdev on 1/1/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "../../../../utils/range.h"
#include "../../load_commands/retrieve.h"

#include "data.h"

namespace macho::utils::strings::table {
    data::creation_result data::create(const macho::container &container, const options &options) noexcept {
        auto symbol_table = symtab_command();

        const auto retrieve_result = load_commands::retrieve_only_load_command_of_cmd_in_container(container, macho::load_commands::symbol_table, &symbol_table, sizeof(symbol_table));
        switch (retrieve_result) {
            case load_commands::retrieve_result::ok:
                break;

            case load_commands::retrieve_result::no_load_commands:
            case load_commands::retrieve_result::too_many_load_commands:
            case load_commands::retrieve_result::load_commands_area_is_too_small:
            case load_commands::retrieve_result::load_commands_area_is_too_large:
            case load_commands::retrieve_result::stream_seek_error:
            case load_commands::retrieve_result::stream_read_error:
            case load_commands::retrieve_result::invalid_load_command:
                return creation_result::failed_to_retrieve_load_commands;

            case load_commands::retrieve_result::no_instance_of_specified_load_command:
                return creation_result::failed_to_find_symbol_table;

            case load_commands::retrieve_result::cmdsize_too_small_to_hold_load_command:
                return creation_result::invalid_symbol_table;

            case load_commands::retrieve_result::multiple_load_commands:
                return creation_result::multiple_symbol_tables;
        }

        // A check here is necessary as retrieve_only_load_command_of_cmd_in_container()
        // checks provided buffer-size solely to make sure it can copy the whole
        // load-command over

        if (symbol_table.cmdsize != sizeof(symbol_table)) {
            return creation_result::invalid_symbol_table;
        }

        return this->create(container, symbol_table, options);
    }

    data::creation_result data::create(const macho::container &container, const load_commands::data &data, const options &options) noexcept {
        auto symbol_table = static_cast<const symtab_command *>(nullptr);
        for (auto iter = data.begin; iter != data.end; iter++) {
            const auto cmd = iter.cmd();
            if (cmd != macho::load_commands::symbol_table) {
                continue;
            }

            const auto cmdsize = iter.cmdsize();
            if (cmdsize != sizeof(symtab_command)) {
                return creation_result::invalid_symbol_table;
            }

            symbol_table = static_cast<const struct symtab_command *>(iter.load_command());
        }

        if (!symbol_table) {
            return creation_result::failed_to_find_symbol_table;
        }

        return this->create(container, *symbol_table, options);
    }

    data::creation_result data::create(const container &container, const symtab_command &symtab, const options &options) noexcept {
        auto string_table_size = symtab.strsize;
        auto string_table_location = symtab.stroff;

        if (container.is_big_endian()) {
            ::utils::swap::uint32(string_table_size);
            ::utils::swap::uint32(string_table_location);
        }

        // XXX: string_table_location should be validated to
        // make sure it does not overlap with the load-commands buffer

        if (string_table_location < sizeof(header) || string_table_location >= container.size) {
            return creation_result::invalid_string_table;
        }

        if (string_table_size > container.size) {
            return creation_result::invalid_string_table;
        }

        if (!::utils::range::is_valid_size_based_range(string_table_location, string_table_size)) {
            return creation_result::invalid_string_table;
        }

        const auto string_table_range = ::utils::range(string_table_location, string_table_size);
        if (string_table_range.is_or_goes_past_end(container.size)) {
            return creation_result::invalid_string_table;
        }

        // No check required here for overflows when adding container.base and string_table_location
        // as string_table_location is guaranteed here to be less than container.size, which is guaranteed
        // to be not overflow with container.base

        // Also no check is needed to make sure container.base goes past end of container.stream as string_table_location
        // is guarantted to be in the container's range, which is guaranteed to be in the stream's range

        if (!container.stream.seek(container.base + string_table_location, stream::file::seek_type::beginning)) {
            return creation_result::stream_seek_error;
        }

        const auto string_table = new char[string_table_size];

        if (!container.stream.read(string_table, string_table_size)) {
            delete[] string_table;
            return creation_result::stream_read_error;
        }

        if (options.validate_starts_null) {
            if (*string_table != '\0') {
                delete[] string_table;
                return creation_result::doesnt_start_null;
            }
        }

        this->begin = iterator(string_table);
        this->end = iterator(&string_table[string_table_size]);

        return creation_result::ok;
    }

    data::~data() noexcept {
        if (this->string() != nullptr) {
            delete[] const_cast<char *>(this->string());
        }
    }
}
