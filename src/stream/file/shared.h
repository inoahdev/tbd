//
//  src/stream/file/shared.h
//  tbd
//
//  Created by inoahdev on 11/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <memory>
#include "seek_type.h"

namespace stream::file {
    class shared {
    public:
        explicit shared() = default;

        enum class open_result {
            ok,
            failed_to_open_file
        };

        open_result open(const char *path, const char *mode) noexcept;
        open_result open(int descriptor, const char *mode) noexcept;

        open_result open(FILE *file) noexcept;

        bool seek(long location, seek_type seek) const noexcept;

        bool read(void *buffer, size_t size) const noexcept;
        bool write(const void *buffer, size_t size) const noexcept;

        long position() const noexcept;
        int error() const noexcept;

        inline bool is_open() const noexcept { return stream() != nullptr; }
        inline bool is_closed() const noexcept { return stream() == nullptr; }

        inline FILE *stream() const noexcept { return stream_.get(); }

        void close();

    private:
        std::shared_ptr<FILE> stream_;
    };
}
