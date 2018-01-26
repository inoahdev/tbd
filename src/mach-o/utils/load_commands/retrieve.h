//
//  src/mach-o/utils/load_commands/retrieve.h
//  tbd
//
//  Created by inoahdev on 11/23/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once
#include "../../container.h"

namespace macho::utils::load_commands {
    enum class retrieve_result {
        ok,

        no_load_commands,
        too_many_load_commands,

        load_commands_area_is_too_small,
        load_commands_area_is_too_large,

        stream_seek_error,
        stream_read_error,

        invalid_load_command,

        no_instance_of_specified_load_command,
        cmdsize_too_small_to_hold_load_command,

        multiple_load_commands
    };

    struct load_command *retrieve_first_of_load_command_in_container(const container &container, enum load_commands cmd, retrieve_result *result = nullptr) noexcept;
    retrieve_result retrieve_first_of_load_command_in_container(const container &container, enum load_commands cmd, struct load_command *buffer, uint32_t cmdsize) noexcept;

    struct load_command *retrieve_only_load_command_of_cmd_in_container(const container &container, enum load_commands cmd, retrieve_result *result = nullptr) noexcept;
    retrieve_result retrieve_only_load_command_of_cmd_in_container(const container &container, enum load_commands cmd, struct load_command *buffer, uint32_t cmdsize) noexcept;
}
