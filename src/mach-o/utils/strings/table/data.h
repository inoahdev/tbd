//
//  src/mach-o/utils/strings/table/data.h
//  tbd
//
//  Created by inoahdev on 1/1/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#pragma once

#include <cstdint>
#include <cstring>

#include "../../load_commands/data.h"

namespace macho::utils::strings::table {
    struct data {
        struct iterator {
            typedef const char &reference;
            typedef reference const_reference;
            
            typedef const char *pointer;
            typedef pointer const_pointer;
            
            explicit iterator(const const_pointer &ptr) noexcept : ptr(ptr) {}
            
            inline const_pointer operator+(uint64_t amount) const noexcept {
                auto ptr = this->ptr;
                for (; amount != 0; amount--) {
                    ptr += (strlen(ptr) + 1);
                }
                
                return ptr;
            }
            
            inline const_pointer operator-(uint64_t amount) const noexcept {
                auto ptr = this->ptr;
                for (; amount != 0; amount--) {
                    ptr -= 2;
                    while (*ptr != '\0') {
                        ptr--;
                    }
                }
                
                return ptr;
            }
            
            inline iterator &operator++() noexcept {
                ptr += (strlen(ptr) + 1);
                return *this;
            }
            
            inline iterator &operator--() noexcept {
                ptr -= 2;
                while (*ptr != '\0') {
                    ptr--;
                }
                
                return *this;
            }

            inline iterator &operator++(int) noexcept { return ++(*this); }
            inline iterator &operator--(int) noexcept { return --(*this); }

            inline const_reference operator[](int64_t index) const noexcept { return *(ptr + index); }
            
            inline bool operator==(const iterator &iter) const noexcept { return ptr == iter.ptr; }
            inline bool operator!=(const iterator &iter) const noexcept { return ptr != iter.ptr; }
            
            inline const_pointer data() const noexcept { return ptr; }
            
        protected:
            const_pointer ptr;
        };
        
        iterator begin = iterator(nullptr);
        iterator end = iterator(nullptr);
        
        struct options {
            enum class shifts : uint64_t {
                // Validate string-table
                // starts with terminator
                
                // Important for use with
                // subtraction of iterators
                
                validate_starts_null = 1 << 0
            };
            
            uint64_t bits = uint64_t();
            
            inline bool has_none() const noexcept {
                return this->bits == 0;
            }
            
            inline bool has_shift(shifts shift) const noexcept {
                return this->bits & (1ull << static_cast<uint64_t>(shift));
            }
            
            inline bool validate_starts_null() const noexcept {
                return this->has_shift(shifts::validate_starts_null);
            }
            
            inline void add_shift(shifts shift) noexcept {
                this->bits |= (1ull << static_cast<uint64_t>(shift));
            }
            
            inline void remove_shift(shifts shift) noexcept {
                this->bits &= ~(1ull << static_cast<uint64_t>(shift));
            }
            
            inline void clear() noexcept {
                this->bits = 0;
            }
        
            inline void set_should_validate_starts_null() noexcept {
                this->add_shift(shifts::validate_starts_null);
            }
            
            inline void set_should_not_validate_starts_null() noexcept {
                this->remove_shift(shifts::validate_starts_null);
            }
        };
        
        enum class creation_result {
            ok,
            
            failed_to_retrieve_load_commands,
            failed_to_find_symbol_table,
            
            invalid_symbol_table,
            multiple_symbol_tables,
            
            invalid_string_table,
            empty_string_table,
            
            stream_seek_error,
            stream_read_error,
            
            doesnt_start_null
        };
        
        creation_result create(const macho::container &container, const options &options) noexcept;
        creation_result create(const macho::container &container, const load_commands::data &data, const options &options) noexcept;
        creation_result create(const macho::container &container, const symtab_command &symtab, const options &options) noexcept;

        ~data() noexcept;
        
        inline const char *string() const noexcept { return begin.data(); }
        inline size_t size() const noexcept { return end.data() - begin.data(); }
        
        inline iterator::const_reference operator[](uint32_t index) const noexcept { return string()[index]; }
    };
}
