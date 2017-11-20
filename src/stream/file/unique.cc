//
//  src/stream/file/unique.cc
//  tbd
//
//  Created by inoahdev on 11/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <sys/stat.h>
#include "unique.h"

namespace stream::file {
    unique::unique(unique &&unique) noexcept :
    stream_(unique.stream_), mode_(unique.mode_) {
        unique.stream_ = nullptr;
        unique.mode_ = nullptr;
    }

    unique &unique::operator=(unique &&unique) noexcept {
        stream_ = unique.stream_;
        mode_ = unique.mode_;

        unique.stream_ = nullptr;
        unique.mode_ = nullptr;

        return *this;
    }

    unique::~unique() noexcept {
        close();
    }

    unique::open_result unique::open(const char *path, const char *mode) noexcept {
        if (is_open()) {
            close();
        }
        
        const auto file = fopen(path, mode);
        if (!file) {
            return open_result::failed_to_open_file;
        }

        stream_ = file;
        return open_result::ok;
    }

    unique::open_result unique::open(int descriptor, const char *mode) noexcept {
        if (is_open()) {
            close();
        }
        
        const auto file = fdopen(descriptor, mode);
        if (!file) {
            return open_result::failed_to_open_file;
        }

        stream_ = file;
        return open_result::ok;
    }

    unique::open_result unique::open_copy(const unique &file) noexcept {
        if (file.stream_ != nullptr) {
            stream_ = freopen(nullptr, file.mode_, file.stream_);
            if (!stream_) {
                return open_result::failed_to_open_file;
            }

        } else {
            stream_ = nullptr;
        }

        return open_result::ok;
    }

    void unique::close() {
        fclose(stream_);
    }

    bool unique::seek(long location, seek_type seek) const noexcept {
        if (is_closed()) {
            return -1;
        }

        return fseek(stream_, location, static_cast<int>(seek)) == 0;
    }

    bool unique::read(void *buffer, size_t size) const noexcept {
        if (is_closed()) {
            return false;
        }

        return fread(buffer, size, 1, stream_) == 1;
    }

    bool unique::write(const void *buffer, size_t size) const noexcept {
        if (is_closed()) {
            return false;
        }

        return fwrite(buffer, size, 1, stream_) == 1;
    }

    long unique::position() const noexcept {
        if (is_closed()) {
            return -1;
        }

        return ftell(stream_);
    }

    long unique::size() const noexcept {
        if (is_closed()) {
            return 0;
        }

        const auto descriptor = fileno(stream_);
        if (descriptor == -1) {
            return 0;
        }

        struct stat sbuf;
        if (fstat(descriptor, &sbuf) != 0) {
            return 0;
        }

        return sbuf.st_size;
    }

    int unique::error() const noexcept {
        if (is_closed()) {
            return 0;
        }

        return ferror(stream_);
    }
}
