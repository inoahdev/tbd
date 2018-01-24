//
//  src/mach-o/utils/segments/sections_iterator.h
//  tbd
//
//  Created by inoahdev on 11/23/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include "../../headers/segments.h"
#include "../../container.h"

namespace macho::utils::segments {
    struct sections_iterator {
        typedef struct macho::segments::section &reference;
        typedef const struct macho::segments::section &const_reference;
        
        typedef struct macho::segments::section *pointer;
        typedef const struct macho::segments::section *const_pointer;
        
        explicit sections_iterator(const_pointer ptr) : ptr(ptr) {}
        
        inline const_reference operator*() const noexcept { return *this->ptr; }
        inline const_pointer operator->() const noexcept { return this->ptr; }
        
        inline sections_iterator operator+(uint64_t index) noexcept { return sections_iterator(this->ptr + index); }
        inline sections_iterator operator-(uint64_t index) noexcept { return sections_iterator(this->ptr - index); }

        inline sections_iterator &operator++() noexcept { this->ptr++; return *this; }
        inline sections_iterator &operator++(int) noexcept { return ++(*this); }
        
        inline sections_iterator &operator--() noexcept { this->ptr--; return *this; }
        inline sections_iterator &operator--(int) noexcept { return --(*this); }
        
        inline const_pointer operator+(uint64_t index) const noexcept { return this->ptr + index; }
        inline const_pointer operator-(uint64_t index) const noexcept { return this->ptr - index; }

        inline const_reference operator[](uint64_t index) const noexcept { return this->ptr[index]; }
        inline const_pointer section() const noexcept { return this->ptr; }
        
        inline sections_iterator &operator=(const_pointer ptr) noexcept { this->ptr = ptr; return *this; }
        
        inline bool operator==(const_pointer ptr) const noexcept { return this->ptr == ptr; }
        inline bool operator==(const sections_iterator &iter) const noexcept { return this->ptr == iter.ptr; }

        inline bool operator!=(const_pointer ptr) const noexcept { return this->ptr != ptr; }
        inline bool operator!=(const sections_iterator &iter) const noexcept { return this->ptr != iter.ptr; }
        
    protected:
        const_pointer ptr;
    };
    
    struct sections_64_iterator {
        typedef struct macho::segments::section_64 &reference;
        typedef const struct macho::segments::section_64 &const_reference;
        
        typedef struct macho::segments::section_64 *pointer;
        typedef const struct macho::segments::section_64 *const_pointer;
        
        explicit sections_64_iterator(const_pointer ptr) : ptr(ptr) {}

        inline const_reference operator*() const noexcept { return *ptr; }
        inline const_pointer operator->() const noexcept { return ptr; }
        
        inline sections_64_iterator operator+(uint64_t index) noexcept { return sections_64_iterator(this->ptr + index); }
        inline sections_64_iterator operator-(uint64_t index) noexcept { return sections_64_iterator(this->ptr - index); }
        
        inline sections_64_iterator &operator++() noexcept { this->ptr++; return *this; }
        inline sections_64_iterator &operator++(int) noexcept { return ++(*this); }
        
        inline sections_64_iterator &operator--() noexcept { this->ptr--; return *this; }
        inline sections_64_iterator &operator--(int) noexcept { return --(*this); }
        
        inline const_pointer operator+(uint64_t index) const noexcept { return this->ptr + index; }
        inline const_pointer operator-(uint64_t index) const noexcept { return this->ptr + index; }
        
        inline const_reference operator[](uint64_t index) const noexcept { return this->ptr[index]; }
        inline const_pointer section() const noexcept { return this->ptr; }
        
        inline sections_64_iterator &operator=(const_pointer ptr) noexcept { this->ptr = ptr; return *this; }
        
        inline bool operator==(const_pointer ptr) const noexcept { return this->ptr == ptr; }
        inline bool operator==(sections_64_iterator iter) const noexcept { return this->ptr == iter.ptr; }
        
        inline bool operator!=(const_pointer ptr) const noexcept { return this->ptr != ptr; }
        inline bool operator!=(sections_64_iterator iter) const noexcept { return this->ptr != iter.ptr; }
        
    protected:
        const_pointer ptr;
    };
}
