//
//  src/mach-o/utils/segments/segment_with_sections_iterator.cc
//  tbd
//
//  Created by inoahdev on 11/25/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "segment_with_sections_iterator.h"

namespace macho::utils::segments {
    command::command(const macho::segment_command *segment, bool is_big_endian) noexcept {
        auto segment_sections_count = segment->nsects;
        if (!segment_sections_count) {
            return;
        }

        if (is_big_endian) {
            ::utils::swap::uint32(segment_sections_count);
        }
        
        const auto segment_sections_size = sizeof(sections_iterator::const_reference) * segment_sections_count;
        const auto expected_command_size_without_data = sizeof(macho::segment_command) + segment_sections_size;

        auto cmdsize = segment->cmdsize;
        if (is_big_endian) {
            ::utils::swap::uint32(cmdsize);
        }

        if (cmdsize < expected_command_size_without_data) {
            return;
        }

        this->sections.begin = reinterpret_cast<sections_iterator::const_pointer>(&segment[1]);
        this->sections.end = &sections.begin[segment_sections_count];
    }

    command_64::command_64(const macho::segment_command_64 *segment, bool is_big_endian) noexcept {
        auto segment_sections_count = segment->nsects;
        if (!segment_sections_count) {
            return;
        }

        if (is_big_endian) {
            ::utils::swap::uint32(segment_sections_count);
        }

        const auto segment_sections_size = sizeof(sections_64_iterator::const_reference) * segment_sections_count;
        const auto expected_command_size_without_data = sizeof(macho::segment_command_64) + segment_sections_size;

        auto cmdsize = segment->cmdsize;
        if (is_big_endian) {
            ::utils::swap::uint32(cmdsize);
        }

        if (cmdsize < expected_command_size_without_data) {
            return;
        }

        this->sections.begin = reinterpret_cast<sections_64_iterator::const_pointer>(&segment[1]);
        this->sections.end = &sections.begin[segment_sections_count];
    }
}
