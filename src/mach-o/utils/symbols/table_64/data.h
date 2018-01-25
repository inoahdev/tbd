//
//  src/mach-o/utils/symbols/table_64/data.h
//  tbd
//
//  Created by inoahdev on 12/28/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include "../../../container.h"
#include "../../../headers/symbol_table.h"

#include "../../load_commands/data.h"

namespace macho::utils::symbols::table_64 {
    struct data {
        struct iterator {
            typedef struct symbol_table::entry_64 &reference;
            typedef const struct symbol_table::entry_64 &const_reference;

            typedef struct symbol_table::entry_64 *pointer;
            typedef const struct symbol_table::entry_64 *const_pointer;

            explicit iterator(const const_pointer &ptr) noexcept : ptr(ptr) {}

            inline const_reference operator*() const noexcept { return *ptr; }
            inline const_pointer operator->() const noexcept { return ptr; }

            inline const_pointer operator+(uint64_t index) const noexcept { return ptr + index; }
            inline const_pointer operator-(uint64_t index) const noexcept { return ptr - index; }

            inline iterator &operator++() noexcept { ++ptr; return *this; }
            inline iterator &operator--() noexcept { --ptr; return *this; }

            inline iterator &operator++(int) noexcept { return ++(*this); }
            inline iterator &operator--(int) noexcept { return --(*this); }

            inline const_reference operator[](int64_t index) const noexcept { return ptr[index]; }

            inline bool operator==(const iterator &iter) const noexcept { return ptr == iter.ptr; }
            inline bool operator!=(const iterator &iter) const noexcept { return ptr != iter.ptr; }

            inline const_pointer entry() const noexcept { return ptr; }

        protected:
            const_pointer ptr;
        };

        struct flags {
            friend struct data;

            inline bool is_little_endian() const noexcept {
                return !is_big_endian();
            }

            inline bool is_big_endian() const noexcept {
                return this->bits.is_big_endian;
            }

            inline bool operator==(const flags &flags) const noexcept { return this->bits.value == flags.bits.value; }
            inline bool operator!=(const flags &flags) const noexcept { return this->bits.value != flags.bits.value; }

        protected:
            union {
                uint64_t value = 0;
                struct {
                    bool is_big_endian : 1;
                }  __attribute__((packed));
            } bits;
        };

        typedef const iterator const_iterator;

        iterator begin = iterator(nullptr);
        iterator end = iterator(nullptr);

        flags flags;

        struct options {
            union {
                uint64_t value = 0;

                struct {
                    bool convert_to_little_endian : 1;
                    bool convert_to_big_endian    : 1;
                } __attribute__((packed));
            };

            inline bool has_none() noexcept { return this->value == 0; }
            inline void clear() noexcept { this->value = 0; }

            inline bool operator==(const options &options) const noexcept { return this->value == options.value; }
            inline bool operator!=(const options &options) const noexcept { return this->value != options.value; }
        };

        enum class creation_result {
            ok,

            failed_to_retrieve_symbol_table,
            failed_to_find_symbol_table,

            invalid_symbol_table,
            multiple_symbol_tables,

            no_symbols,
            invalid_symbol_entry_table,

            stream_seek_error,
            stream_read_error
        };

        creation_result create(const container &container, const options &options) noexcept;
        creation_result create(const container &container, const load_commands::data &data, const options &options) noexcept;
        creation_result create(const container &container, const symtab_command &symtab, const options &options) noexcept;

        ~data() noexcept;
    };
}
