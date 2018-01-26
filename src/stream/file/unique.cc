//
//  src/stream/file/unique.cc
//  tbd
//
//  Created by inoahdev on 11/10/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include <sys/stat.h>
#include "unique.h"

namespace stream::file {
    unique::unique(unique &&unique) noexcept :
    stream_(unique.stream_) {
        unique.stream_ = nullptr;
    }

    unique &unique::operator=(unique &&unique) noexcept {
        this->stream_ = unique.stream_;
        unique.stream_ = nullptr;

        return *this;
    }

    unique::~unique() noexcept {
        this->close();
    }

    unique::open_result unique::open(const char *path, const char *mode) noexcept {
        if (this->is_open()) {
            this->close();
        }

        const auto file = fopen(path, mode);
        if (!file) {
            return open_result::failed_to_open_file;
        }

        this->stream_ = file;
        return open_result::ok;
    }

    unique::open_result unique::open(int descriptor, const char *mode) noexcept {
        if (this->is_open()) {
            this->close();
        }

        const auto file = fdopen(descriptor, mode);
        if (!file) {
            return open_result::failed_to_open_file;
        }

        this->stream_ = file;
        return open_result::ok;
    }

    void unique::close() {
        fclose(this->stream());
    }

    bool unique::seek(long location, seek_type seek) const noexcept {
        if (this->is_closed()) {
            return -1;
        }

        return fseek(this->stream(), location, static_cast<int>(seek)) == 0;
    }

    bool unique::read(void *buffer, size_t size) const noexcept {
        if (this->is_closed()) {
            return false;
        }

        return fread(buffer, size, 1, this->stream()) == 1;
    }

    bool unique::write(const void *buffer, size_t size) const noexcept {
        if (this->is_closed()) {
            return false;
        }

        return fwrite(buffer, size, 1, this->stream()) == 1;
    }

    long unique::position() const noexcept {
        if (this->is_closed()) {
            return -1;
        }

        return ftell(this->stream());
    }

    long unique::size() const noexcept {
        if (this->is_closed()) {
            return 0;
        }

        const auto descriptor = fileno(this->stream());
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
        if (this->is_closed()) {
            return 0;
        }

        return ferror(this->stream());
    }
}
