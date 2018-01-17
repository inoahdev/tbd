//
//  src/stream/file/shared.cc
//  tbd
//
//  Created by inoahdev on 11/10/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <sys/stat.h>
#include "shared.h"

namespace stream::file {
    shared::open_result shared::open(const char *path, const char *mode) noexcept {
        const auto file = fopen(path, mode);
        if (!file) {
            // Close the current stream 
            // even on open failure

            this->stream_.reset();
            return open_result::failed_to_open_file;
        }

        this->stream_.reset(file, [](FILE *file) {
            fclose(file);
        });

        return open_result::ok;
    }

    shared::open_result shared::open(int descriptor, const char *mode) noexcept {        
        const auto file = fdopen(descriptor, mode);
        if (!file) {
            // Close the current stream 
            // even on open failure

            this->stream_.reset();
            return open_result::failed_to_open_file;
        }

        this->stream_.reset(file, [](FILE *file) {
            fclose(file);
        });

        return open_result::ok;
    }
    
    shared::open_result shared::open_copy(FILE *file, const char *mode) noexcept {
        const auto file_copy = freopen(nullptr, mode, file);
        if (!file_copy) {
            // Close the current stream
            // even on open failure
            
            this->stream_.reset();
            return open_result::failed_to_open_file;
        }
        
        this->stream_.reset(file_copy, [](FILE *file) {
            fclose(file);
        });
        
        return open_result::ok;
    }
    
    shared::open_result shared::open_copy(const shared &shared, const char *mode) noexcept {
        const auto file = freopen(nullptr, mode, shared.stream());
        if (!file) {
            // Close the current stream
            // even on open failure
            
            this->stream_.reset();
            return open_result::failed_to_open_file;
        }
        
        this->stream_.reset(file, [](FILE *file) {
            fclose(file);
        });
        
        return open_result::ok;
    }

    shared::open_result shared::open(FILE *file) noexcept {
        this->stream_.reset(file);
        return open_result::ok;
    }

    void shared::close() {
        this->stream_.reset();
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

    long shared::size() const noexcept {
        const auto stream = this->stream();
        if (!stream) {
            return 0;
        }

        const auto descriptor = fileno(stream);
        if (descriptor == -1) {
            return 0;
        }

        struct stat sbuf;
        if (fstat(descriptor, &sbuf) != 0) {
            return 0;
        }

        return sbuf.st_size;
    }

    int shared::error() const noexcept {
        const auto stream = this->stream();
        if (!stream) {
            return 0;
        }

        return ferror(stream);
    }
}
