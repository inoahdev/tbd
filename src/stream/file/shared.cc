//
//  src/stream/file/shared.cc
//  tbd
//
//  Created by inoahdev on 11/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "shared.h"

namespace stream::file {
    shared::open_result shared::open(const char *path, const char *mode) noexcept {
        const auto file = fopen(path, mode);
        if (!file) {
            return open_result::failed_to_open_file;
        }

        stream_.reset(file, [](FILE *file) {
            fclose(file);
        });

        return open_result::ok;
    }

    shared::open_result shared::open(int descriptor, const char *mode) noexcept {
        const auto file = fdopen(descriptor, mode);
        if (!file) {
            return open_result::failed_to_open_file;
        }

        stream_.reset(file, [](FILE *file) {
            fclose(file);
        });

        return open_result::ok;
    }

    shared::open_result shared::open(FILE *file) noexcept {
        stream_.reset(file);
        return open_result::ok;
    }

    void shared::close() {
        stream_.reset();
    }

    bool shared::seek(long location, seek_type seek) const noexcept {
        const auto stream = this->stream();
        if (!stream) {
            return false;
        }

        return fseek(stream, location, static_cast<int>(seek)) == 0;
    }

    bool shared::read(void *buffer, size_t size) const noexcept {
        const auto stream = this->stream();
        if (!stream) {
            return false;
        }

        return fread(buffer, size, 1, stream) == 1;
    }

    bool shared::write(const void *buffer, size_t size) const noexcept {
        const auto stream = this->stream();
        if (!stream) {
            return false;
        }

        return fwrite(buffer, size, 1, stream) == 1;
    }

    long shared::position() const noexcept {
        const auto stream = this->stream();
        if (!stream) {
            return -1;
        }

        return ftell(stream);
    }

    int shared::error() const noexcept {
        const auto stream = this->stream();
        if (!stream) {
            return 0;
        }

        return ferror(stream);
    }
}
