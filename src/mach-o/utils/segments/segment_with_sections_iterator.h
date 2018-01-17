//
//  src/mach-o/utils/segments/segment_with_sections_iterator.h
//  tbd
//
//  Created by inoahdev on 11/25/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include "../../headers/load_commands.h"
#include "../load_commands/data.h"

#include "sections_iterator.h"

namespace macho::utils::segments {
    struct command {
        explicit command(const macho::segment_command *segment, bool is_big_endian) noexcept;

        struct {
            sections_iterator begin = sections_iterator(nullptr);
            sections_iterator end = sections_iterator(nullptr);
        } sections;
    };
    
    struct command_64 {
        explicit command_64(const macho::segment_command_64 *segment, bool is_big_endian) noexcept;
        
        struct {
            sections_64_iterator begin = sections_64_iterator(nullptr);
            sections_64_iterator end = sections_64_iterator(nullptr);
        } sections;
    };
}
