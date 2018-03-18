//
//  src/mach-o/container.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <cstdio>
#include <cstdlib>

#include "headers/symbol_table.h"
#include "container.h"

namespace macho {
    container::open_result container::open(const stream::file::shared &stream, long base, size_t size) noexcept {
        this->stream = stream;

        this->base = base;
        this->size = size;

        return this->validate_and_load_data();
    }

    container::open_result container::open_copy(const container &container) noexcept {
        this->stream = container.stream;

        this->base = container.base;
        this->size = container.size;

        return this->validate_and_load_data();
    }
    
    container::open_result container::validate_and_load_data() noexcept {
        if (this->size < sizeof(this->header.magic)) {
            return open_result::not_a_macho;
        }
        
        if (!this->stream.seek(this->base, stream::file::seek_type::beginning)) {
            return open_result::stream_seek_error;
        }
        
        if (!this->stream.read(&this->header.magic, sizeof(this->header.magic))) {
            return open_result::stream_read_error;
        }
        
        if (this->header.magic.is_fat()) {
            return open_result::fat_container;
        } else if (!this->header.magic.is_thin()){
            return open_result::not_a_macho;
        }
        
        if (this->size < sizeof(header)) {
            return open_result::invalid_macho;
        }
        
        if (!this->stream.read(&this->header.cputype, sizeof(this->header) - sizeof(this->header.magic))) {
            return open_result::stream_read_error;
        }
        
        if (this->is_big_endian()) {
            utils::swap::mach_header(header);
        }
        
        return open_result::ok;
    }
}
