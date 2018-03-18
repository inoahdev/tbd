//
//  src/mach-o/container.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include <cstdlib>
#include "../stream/file/shared.h"

#include "../utils/swap.h"
#include "utils/swap.h"

namespace macho {
    class container {
    public:
        explicit container() = default;

        stream::file::shared stream;

        long base = long();
        size_t size = size_t();

        struct header header;

        enum class open_result {
            ok,

            stream_seek_error,
            stream_read_error,

            fat_container,

            not_a_macho,
            invalid_macho,
        };

        open_result open(const stream::file::shared &stream, long base, size_t size) noexcept;
        open_result open_copy(const container &container) noexcept;

        inline const bool is_32_bit() const noexcept { return header.magic.is_32_bit(); }
        inline const bool is_64_bit() const noexcept { return header.magic.is_64_bit(); }

        inline const bool is_little_endian() const noexcept { return header.magic.is_little_endian(); }
        inline const bool is_big_endian() const noexcept { return header.magic.is_big_endian(); }

    protected:
        open_result validate_and_load_data() noexcept;
    };
}
