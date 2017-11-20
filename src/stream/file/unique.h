//
//  src/stream/file/unique.h
//  tbd
//
//  Created by inoahdev on 11/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include "seek_type.h"

namespace stream::file {
    class unique {
    public:
        explicit unique() = default;

        explicit unique(const unique &) = delete;
        explicit unique(unique &&) noexcept;

        unique &operator=(const unique &) = delete;
        unique &operator=(unique &&) noexcept;

        ~unique() noexcept;

        enum class open_result {
            ok,
            failed_to_open_file
        };

        open_result open(const char *path, const char *mode) noexcept;
        open_result open(int descriptor, const char *mode) noexcept;

        open_result open_copy(const unique &file) noexcept;

        bool seek(long location, seek_type seek) const noexcept;

        bool read(void *buffer, size_t size) const noexcept;
        bool write(const void *buffer, size_t size) const noexcept;

        long position() const noexcept;
        long size() const noexcept;

        int error() const noexcept;

        inline bool is_open() const noexcept { return stream_ != nullptr; }
        inline bool is_closed() const noexcept { return stream_ == nullptr; }

        inline FILE *stream() const noexcept { return stream_; }

        void close();

    private:
        FILE *stream_ = nullptr;
        const char *mode_ = nullptr;
    };
}
