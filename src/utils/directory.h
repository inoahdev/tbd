//
//  src/utils/directory.h
//  tbd
//
//  Created by inoahdev on 10/17/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <dirent.h>
#include <string>

#include "path.h"

namespace utils {
    struct directory {
        struct iterator {
            friend struct directory;

            typedef struct dirent &reference;
            typedef const struct dirent &const_reference;

            typedef struct dirent *pointer;
            typedef const struct dirent *const_pointer;

            explicit iterator(const_pointer ptr) : ptr(ptr) {}

            std::string entry_path_from_directory_path(const char *root) const noexcept;

            inline const_reference operator*() const noexcept { return *this->ptr; }
            inline const_pointer operator->() const noexcept { return this->ptr; }

            inline const_pointer entry() const noexcept { return this->ptr; }
            inline iterator &operator=(const_pointer ptr) noexcept { this->ptr = ptr; return *this; }

            inline iterator &operator++() noexcept { this->ptr = readdir(this->dir); return *this; }
            inline iterator &operator++(int) noexcept { return ++(*this); }

            inline bool operator==(const_pointer ptr) const noexcept { return this->ptr == ptr; }
            inline bool operator==(const iterator &iter) const noexcept { return this->ptr == iter.ptr; }

            inline bool operator!=(const_pointer ptr) const noexcept { return this->ptr != ptr; }
            inline bool operator!=(const iterator &iter) const noexcept { return this->ptr != iter.ptr; }

        protected:
            DIR *dir;
            const_pointer ptr;
        };

        inline ~directory() noexcept { this->close(); }

        struct {
            iterator begin = iterator(nullptr);
            iterator::const_pointer end = nullptr;
        } entries;

        enum class open_result {
            ok,
            failed_to_open_directory
        };

        open_result open(const char *path) noexcept;
        open_result open(const std::string &path) noexcept;

        open_result open(std::string &&path) noexcept;

        void close() noexcept;
    };
}
