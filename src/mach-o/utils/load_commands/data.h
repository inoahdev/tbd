//
//  src/mach-o/utils/load_commands/data.h
//  tbd
//
//  Created by inoahdev on 11/23/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include "../../headers/load_commands.h"
#include "../../container.h"

namespace macho::utils::load_commands {
    struct data {
        struct flags {
            friend struct data;

            enum class shifts : uint64_t {
                is_big_endian,
                is_64_bit
            };

            inline bool has_shift(shifts shift) const noexcept {
                return this->bits & (1ull << static_cast<uint64_t>(shift));
            }

            inline bool is_little_endian() const noexcept {
                return !this->is_big_endian();
            }

            inline bool is_big_endian() const noexcept {
                return this->has_shift(shifts::is_big_endian);
            }

            inline bool is_32_bit() const noexcept {
                return !this->is_64_bit();
            }

            inline bool is_64_bit() const noexcept {
                return this->has_shift(shifts::is_64_bit);
            }

            inline bool operator==(const flags &flags) const noexcept { return bits == flags.bits; }
            inline bool operator!=(const flags &flags) const noexcept { return bits != flags.bits; }

        protected:
            uint64_t bits = uint64_t();

            inline void add_shift(shifts shift) noexcept {
                this->bits |= 1ull << static_cast<uint64_t>(shift);
            }

            inline void remove_shift(shifts shift) noexcept {
                this->bits &= ~(1ull << static_cast<uint64_t>(shift));
            }

            inline void set_is_little_endian() noexcept {
                this->remove_shift(shifts::is_big_endian);
            }

            inline void set_is_big_endian() noexcept {
                this->add_shift(shifts::is_big_endian);
            }

            inline void set_is_32_bit() noexcept {
                this->remove_shift(shifts::is_64_bit);
            }

            inline void set_is_64_bit() noexcept {
                this->add_shift(shifts::is_64_bit);
            }
        };

        struct iterator {
            typedef struct load_command &reference;
            typedef const struct load_command &const_reference;

            typedef struct load_command *pointer;
            typedef const struct load_command *const_pointer;

            struct flags flags;

            explicit iterator(const const_pointer &ptr) noexcept : ptr(ptr) {}
            explicit iterator(const const_pointer &ptr, const struct flags &flags) noexcept :  flags(flags), ptr(ptr) {}

            enum load_commands cmd() const noexcept;
            uint32_t cmdsize() const noexcept;

            inline bool is_little_endian() const noexcept {
                return this->flags.is_little_endian();
            }

            inline bool is_big_endian() const noexcept {
                return this->flags.is_big_endian();
            }

            inline bool is_32_bit() const noexcept {
                return this->flags.is_32_bit();
            }

            inline bool is_64_bit() const noexcept {
                return this->flags.is_64_bit();
            }

            inline const_reference operator*() const noexcept { return *this->ptr; }
            inline const_pointer operator->() const noexcept { return this->ptr; }

            inline iterator &operator++() noexcept {
                this->ptr = reinterpret_cast<iterator::const_pointer>(reinterpret_cast<const uint8_t *>(ptr) + this->cmdsize());
                return *this;
            }

            inline iterator &operator++(int) noexcept { return ++(*this); }

            inline bool operator==(const iterator::const_pointer &ptr) const noexcept { return this->ptr == ptr; }
            inline bool operator!=(const iterator::const_pointer &ptr) const noexcept { return this->ptr != ptr; }

            inline bool operator==(const iterator &iter) const noexcept { return this->ptr == iter.ptr && this->flags == iter.flags; }
            inline bool operator!=(const iterator &iter) const noexcept { return this->ptr != iter.ptr || this->flags != iter.flags; }

            inline const_pointer load_command() const noexcept { return this->ptr; }

        protected:
            const_pointer ptr;
        };

        typedef const iterator const_iterator;

        iterator begin = iterator(nullptr);
        iterator::const_pointer end = nullptr;

        inline const flags &flags() const noexcept { return begin.flags; }

        struct options {
            enum class shifts : uint64_t {
                none,

                convert_to_little_endian,
                convert_to_big_endian,

                // Don't allow misaligned
                // load-command buffer

                enforce_strict_alignment,
            };

            uint64_t bits = uint64_t();

            inline bool has_none() const noexcept {
                return this->bits == 0;
            }

            inline bool has_shift(shifts shift) const noexcept {
                return this->bits & (1ull << static_cast<uint64_t>(shift));
            }

            inline bool convert_to_little_endian() const noexcept {
                return this->has_shift(shifts::convert_to_little_endian);
            }

            inline bool convert_to_big_endian() const noexcept {
                return this->has_shift(shifts::convert_to_big_endian);
            }

            inline bool enforce_strict_alignment() const noexcept {
                return this->has_shift(shifts::enforce_strict_alignment);
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
            
            inline void set_should_convert_to_little_endian() noexcept {
                this->add_shift(shifts::convert_to_little_endian);
            }

            inline void set_should_not_convert_to_little_endian() noexcept {
                this->remove_shift(shifts::convert_to_little_endian);
            }

            inline void set_should_convert_to_big_endian() noexcept {
                this->add_shift(shifts::convert_to_big_endian);
            }

            inline void set_should_not_convert_to_big_endian() noexcept {
                this->remove_shift(shifts::convert_to_big_endian);
            }

            inline void set_should_enforce_strict_alignment() noexcept {
                this->add_shift(shifts::enforce_strict_alignment);
            }

            inline void set_should_not_enforce_strict_alignment() noexcept {
                this->remove_shift(shifts::enforce_strict_alignment);
            }

            inline bool operator==(const options &options) const noexcept { return bits == options.bits; }
            inline bool operator!=(const options &options) const noexcept { return bits != options.bits; }
        };

        enum class creation_result {
            ok,

            no_load_commands,
            too_many_load_commands,

            load_commands_area_is_too_small,
            load_commands_area_is_too_large,

            stream_seek_error,
            stream_read_error,

            invalid_load_command,
            misaligned_load_commands
        };

        inline ~data() noexcept { _destruct(); }

        creation_result create(const container &container, const options &options) noexcept;
        uint32_t count() const noexcept;

    protected:
        void _destruct() noexcept;

        void _swap_load_command(load_command &load_command);
        void switch_endian_of_load_command_with_cmd(load_command &load_command, macho::load_commands cmd);
    };
}
