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

            inline bool is_little_endian() const noexcept {
                return !this->bits.is_big_endian;
            }

            inline bool is_big_endian() const noexcept {
                return this->bits.is_big_endian;
            }

            inline bool is_32_bit() const noexcept {
                return !this->bits.is_64_bit;
            }

            inline bool is_64_bit() const noexcept {
                return this->bits.is_64_bit;
            }

            inline bool has_none() const noexcept { return this->bits.value == 0; }

            inline bool operator==(const flags &flags) const noexcept { return this->bits.value == flags.bits.value; }
            inline bool operator!=(const flags &flags) const noexcept { return this->bits.value == flags.bits.value; }

        protected:
            union {
                uint64_t value = 0;

                struct {
                    bool is_big_endian : 1;
                    bool is_64_bit     : 1;
                } __attribute__((packed));
            } bits;

            inline void clear() noexcept { this->bits.value = 0; }
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
                this->ptr =
                    reinterpret_cast<iterator::const_pointer>(reinterpret_cast<const uint8_t *>(ptr) + this->cmdsize());

                return *this;
            }

            inline iterator &operator++(int) noexcept { return ++(*this); }

            inline bool operator==(const iterator::const_pointer &ptr) const noexcept { return this->ptr == ptr; }
            inline bool operator!=(const iterator::const_pointer &ptr) const noexcept { return this->ptr != ptr; }

            inline bool operator==(const iterator &iter) const noexcept {
                return this->ptr == iter.ptr && this->flags == iter.flags;
            }

            inline bool operator!=(const iterator &iter) const noexcept {
                return this->ptr != iter.ptr || this->flags != iter.flags;
            }

            inline const_pointer load_command() const noexcept { return this->ptr; }

        protected:
            const_pointer ptr;
        };

        typedef const iterator const_iterator;

        iterator begin = iterator(nullptr);
        iterator::const_pointer end = nullptr;

        inline const flags &flags() const noexcept { return begin.flags; }

        struct options {
            explicit inline options() = default;
            explicit inline options(uint64_t value) : value(value) {};

            union {
                uint64_t value = 0;

                struct {
                    bool convert_to_little_endian : 1;
                    bool convert_to_big_endian    : 1;

                    // Don't allow misaligned
                    // load-command buffer

                    bool enforce_strict_alignment : 1;
                } __attribute__((packed));
            };

            inline bool has_none() const noexcept { return this->value == 0; }
            inline void clear() noexcept { this->value = 0; }

            inline bool operator==(const options &options) const noexcept { return this->value == options.value; }
            inline bool operator!=(const options &options) const noexcept { return this->value != options.value; }
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
