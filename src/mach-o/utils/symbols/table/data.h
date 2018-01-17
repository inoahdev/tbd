//
//  src/mach-o/utils/symbols/table/data.h
//  tbd
//
//  Created by inoahdev on 12/12/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include "../../../headers/symbol_table.h"
#include "../../load_commands/data.h"

namespace macho::utils::symbols::table {
    struct data {
        struct iterator {
            typedef struct symbol_table::entry &reference;
            typedef const struct symbol_table::entry &const_reference;

            typedef struct symbol_table::entry *pointer;
            typedef const struct symbol_table::entry *const_pointer;

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

            enum class shifts : uint64_t {
                is_big_endian,
            };

            explicit flags() = default;

            inline bool has_shift(shifts shift) const noexcept {
                return this->bits & (1ull << static_cast<uint64_t>(shift));
            }

            inline bool is_little_endian() const noexcept {
                return !this->is_big_endian();
            }

            inline bool is_big_endian() const noexcept {
                return this->has_shift(shifts::is_big_endian);
            }

            inline bool operator==(const flags &flags) const noexcept { return bits == flags.bits; }
            inline bool operator!=(const flags &flags) const noexcept { return bits != flags.bits; }

        protected:
            uint64_t bits = uint64_t();

            inline void add_shift(shifts shift) noexcept {
                this->bits |= (1ull << static_cast<uint64_t>(shift));
            }

            inline void set_is_big_endian() noexcept {
                this->add_shift(shifts::is_big_endian);
            }

            inline void remove_shift(shifts shift) noexcept {
                this->bits &= ~(1ull << static_cast<uint64_t>(shift));
            }

            inline void set_is_little_endian() noexcept {
                this->remove_shift(shifts::is_big_endian);
            }
        };

        typedef const iterator const_iterator;

        iterator begin = iterator(nullptr);
        iterator end = iterator(nullptr);

        flags flags;

        struct options {
            enum class shifts : uint64_t {
                none,
                convert_to_little_endian,
                convert_to_big_endian
            };

            uint64_t bits = uint64_t();

            inline bool has_shift(shifts shift) const noexcept {
                return this->bits & (1ull << static_cast<uint64_t>(shift));
            }

            inline bool convert_to_little_endian() const noexcept {
                return this->has_shift(shifts::convert_to_little_endian);
            }

            inline bool convert_to_big_endian() const noexcept {
                return this->has_shift(shifts::convert_to_big_endian);
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

            inline void set_convert_to_little_endian() noexcept {
                this->add_shift(shifts::convert_to_little_endian);
            }

            inline void set_convert_to_big_endian() noexcept {
                this->add_shift(shifts::convert_to_big_endian);
            }

            inline bool operator==(const options &options) const noexcept { return this->bits == options.bits; }
            inline bool operator!=(const options &options) const noexcept { return this->bits != options.bits; }
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
