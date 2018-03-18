//
//  src/mach-o/file.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <vector>

#include "container.h"

namespace macho {
    class file {
    public:
        explicit file() = default;

        stream::file::shared stream;
        magic magic;

        std::vector<container> containers = std::vector<container>();

        enum class open_result {
            ok,

            not_a_macho,
            invalid_macho,

            failed_to_open_stream,

            stream_seek_error,
            stream_read_error,

            zero_containers,
            too_many_containers,

            overlapping_containers,
            invalid_container,
        };

        open_result open(const char *path) noexcept;
        open_result open(const char *path, const char *mode) noexcept;
        open_result open(FILE *file) noexcept;

        open_result open_copy(const file &file, const char *mode) noexcept;

    protected:
        open_result load_containers() noexcept;
    };
}
