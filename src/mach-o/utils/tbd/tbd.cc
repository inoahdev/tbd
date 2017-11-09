//
//  src/tbd/tbd.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>

#include "../../../misc/bits.h"
#include "../../../objc/image_info.h"

#include "../../headers/build.h"
#include "../../headers/flags.h"
#include "../../headers/segment.h"

#include "tbd.h"

namespace macho::utils::tbd {
    class reexport {
    public:
        explicit reexport() = default;

        explicit reexport(const reexport &) = delete;
        explicit reexport(reexport &&reexport) noexcept
        : bits(std::move(reexport.bits)), string(reexport.string) {
            reexport.string = nullptr;
        }

        reexport &operator=(const reexport &) = delete;
        reexport &operator=(reexport &&reexport) noexcept {
            bits = std::move(reexport.bits);
            string = reexport.string;

            reexport.string = nullptr;
            return *this;
        }

        enum class creation_result {
            ok,
            failed_to_allocate_memory
        };

        creation_result create(const char *string) noexcept {
            this->string = string;

            auto bits_creation_result = bits.create_stack_max();
            switch (bits_creation_result) {
                case bits::creation_result::ok:
                    break;

                case bits::creation_result::failed_to_allocate_memory:
                    return creation_result::failed_to_allocate_memory;
            }

            return creation_result::ok;
        }

        creation_result create_copy(const reexport &reexport) noexcept {
            string = reexport.string;

            auto bits_creation_result = bits.create_copy(reexport.bits);
            switch (bits_creation_result) {
                case bits::creation_result::ok:
                    break;

                case bits::creation_result::failed_to_allocate_memory:
                    return creation_result::failed_to_allocate_memory;
            }

            return creation_result::ok;
        }

        inline void add_architecture(bits_integer_t index) noexcept { bits.cast(index, true); }

        bits bits;
        const char *string;

        inline bool operator==(const char *string) const noexcept { return strcmp(this->string, string) == 0; }
        inline bool operator==(const reexport &reexport) const noexcept { return strcmp(string, reexport.string) == 0; }
    };

    class symbol {
    public:
        enum class type {
            symbols,
            weak_symbols,
            objc_classes,
            objc_ivars
        };

        explicit symbol() = default;

        explicit symbol(const symbol &) = delete;
        explicit symbol(symbol &&symbol) noexcept
        : bits(std::move(symbol.bits)), string(symbol.string), type(symbol.type), weak(symbol.weak) {
            symbol.string = nullptr;
            symbol.type = type::symbols;

            symbol.weak = false;
        }

        symbol &operator=(const symbol &) = delete;
        symbol &operator=(symbol &&symbol) noexcept {
            bits = std::move(symbol.bits);
            string = symbol.string;

            weak = symbol.weak;
            type = symbol.type;

            symbol.string = nullptr;
            symbol.type = type::symbols;

            symbol.weak = false;
            return *this;
        }

        enum class creation_result {
            ok,
            failed_to_allocate_memory
        };

        creation_result create(const char *string, bool weak, enum type type) noexcept {
            this->string = string;

            this->type = type;
            this->weak = weak;

            auto bits_creation_result = bits.create_stack_max();
            switch (bits_creation_result) {
                case bits::creation_result::ok:
                    break;

                case bits::creation_result::failed_to_allocate_memory:
                    return creation_result::failed_to_allocate_memory;
            }

            return creation_result::ok;
        }

        creation_result create_copy(const symbol &symbol) noexcept {
            string = symbol.string;

            type = symbol.type;
            weak = symbol.weak;

            auto bits_creation_result = bits.create_copy(symbol.bits);
            switch (bits_creation_result) {
                case bits::creation_result::ok:
                    break;

                case bits::creation_result::failed_to_allocate_memory:
                    return creation_result::failed_to_allocate_memory;
            }

            return creation_result::ok;
        }

        bits bits;
        const char *string;

        enum type type;
        bool weak;

        inline void add_architecture(bits_integer_t index) noexcept { bits.cast(index, true); }

        inline bool operator==(const char *string) const noexcept { return strcmp(this->string, string) == 0; }
        inline bool operator==(const symbol &symbol) const noexcept { return strcmp(string, symbol.string) == 0; }
    };

    class group {
    public:
        explicit group() = default;

        explicit group(const group &) = delete;
        explicit group(group &&group) noexcept
        : bits(std::move(group.bits)), reexports_count(group.reexports_count), symbols_count(group.symbols_count) {
            group.reexports_count = 0;
            group.symbols_count = 0;
        }

        group &operator=(const group &) = delete;
        group &operator=(group &&group) noexcept {
            bits = std::move(group.bits);

            reexports_count = group.reexports_count;
            symbols_count = group.symbols_count;

            group.reexports_count = 0;
            group.symbols_count = 0;

            return *this;
        }

        enum class creation_result {
            ok,
            failed_to_allocate_memory
        };

        creation_result create(const bits &bits) {
            auto bits_creation_result = this->bits.create_copy(bits);
            switch (bits_creation_result) {
                case bits::creation_result::ok:
                    break;

                case bits::creation_result::failed_to_allocate_memory:
                    return creation_result::failed_to_allocate_memory;
            }

            return creation_result::ok;
        }

        bits bits;
        std::vector<std::string> clients;

        uint64_t reexports_count = 0;
        uint64_t symbols_count = 0;

        inline bool operator==(const group &group) const noexcept { return bits == group.bits; }
        inline bool operator!=(const group &group) const noexcept { return bits != group.bits; }
    };

    const char *objc_constraint_to_string(const objc_constraint &constraint) noexcept {
        switch (constraint) {
            case objc_constraint::no_value:
                break;

            case objc_constraint::none:
                return "none";

            case objc_constraint::retain_release:
                return "retain_release";

            case objc_constraint::retain_release_for_simulator:
                return "retain_release_for_simulator";

            case objc_constraint::retain_release_or_gc:
                return "retain_release_or_gc";

            case objc_constraint::gc:
                return "gc";
        }

        return nullptr;
    }

    objc_constraint objc_constraint_from_string(const char *string) noexcept {
        if (strcmp(string, "none") == 0) {
            return objc_constraint::none;
        } else if (strcmp(string, "retain_release") == 0) {
            return objc_constraint::retain_release;
        } else if (strcmp(string, "retain_release_for_simulator") == 0) {
            return objc_constraint::retain_release_for_simulator;
        } else if (strcmp(string, "gc") == 0) {
            return objc_constraint::gc;
        }

        return objc_constraint::no_value;
    }

    const char *platform_to_string(const enum platform &platform) noexcept {
        switch (platform) {
            case platform::none:
                return "none";

            case platform::aix:
                return "aix";

            case platform::amdhsa:
                return "amdhsa";

            case platform::ananas:
                return "ananas";

            case platform::cloudabi:
                return "cloudabi";

            case platform::cnk:
                return "cnk";

            case platform::contiki:
                return "contiki";

            case platform::cuda:
                return "cuda";

            case platform::darwin:
                return "darwin";

            case platform::dragonfly:
                return "dragonfly";

            case platform::elfiamcu:
                return "elfiamcu";

            case platform::freebsd:
                return "freebsd";

            case platform::fuchsia:
                return "fuchsia";

            case platform::haiku:
                return "haiku";

            case platform::ios:
                return "ios";

            case platform::kfreebsd:
                return "kfreebsd";

            case platform::linux:
                return "linux";

            case platform::lv2:
                return "lv2";

            case platform::macosx:
                return "macosx";

            case platform::mesa3d:
                return "mesa3d";

            case platform::minix:
                return "minix";

            case platform::nacl:
                return "nacl";

            case platform::netbsd:
                return "netbsd";

            case platform::nvcl:
                return "nvcl";

            case platform::openbsd:
                return "openbsd";

            case platform::ps4:
                return "ps4";

            case platform::rtems:
                return "rtems";

            case platform::solaris:
                return "solaris";

            case platform::tvos:
                return "tvos";

            case platform::watchos:
                return "watchos";

            case platform::windows:
                return "windows";
        }

        return nullptr;
    }

    enum platform platform_from_string(const char *platform) noexcept {
        if (strcmp(platform, "none") == 0) {
            return platform::none;
        } else if (strcmp(platform, "aix") == 0) {
            return platform::aix;
        } else if (strcmp(platform, "amdhsa") == 0) {
            return platform::amdhsa;
        } else if (strcmp(platform, "ananas") == 0) {
            return platform::ananas;
        } else if (strcmp(platform, "cloudabi") == 0) {
            return platform::cloudabi;
        } else if (strcmp(platform, "cnk") == 0) {
            return platform::cnk;
        } else if (strcmp(platform, "contiki") == 0) {
            return platform::contiki;
        } else if (strcmp(platform, "cuda") == 0) {
            return platform::cuda;
        } else if (strcmp(platform, "darwin") == 0) {
            return platform::darwin;
        } else if (strcmp(platform, "dragonfly") == 0) {
            return platform::dragonfly;
        } else if (strcmp(platform, "elfiamcu") == 0) {
            return platform::elfiamcu;
        } else if (strcmp(platform, "freebsd") == 0) {
            return platform::freebsd;
        } else if (strcmp(platform, "fuchsia") == 0) {
            return platform::fuchsia;
        } else if (strcmp(platform, "haiku") == 0) {
            return platform::haiku;
        } else if (strcmp(platform, "ios") == 0) {
            return platform::ios;
        } else if (strcmp(platform, "kfreebsd") == 0) {
            return platform::kfreebsd;
        } else if (strcmp(platform, "linux") == 0) {
            return platform::linux;
        } else if (strcmp(platform, "lv2") == 0) {
            return platform::lv2;
        } else if (strcmp(platform, "macosx") == 0) {
            return platform::macosx;
        } else if (strcmp(platform, "mesa3d") == 0) {
            return platform::mesa3d;
        } else if (strcmp(platform, "minix") == 0) {
            return platform::minix;
        } else if (strcmp(platform, "nacl") == 0) {
            return platform::nacl;
        } else if (strcmp(platform, "netbsd") == 0) {
            return platform::netbsd;
        } else if (strcmp(platform, "nvcl") == 0) {
            return platform::nvcl;
        } else if (strcmp(platform, "openbsd") == 0) {
            return platform::openbsd;
        } else if (strcmp(platform, "ps4") == 0) {
            return platform::ps4;
        } else if (strcmp(platform, "rtems") == 0) {
            return platform::rtems;
        } else if (strcmp(platform, "solaris") == 0) {
            return platform::solaris;
        } else if (strcmp(platform, "tvos") == 0) {
            return platform::tvos;
        } else if (strcmp(platform, "watchos") == 0) {
            return platform::watchos;
        } else if (strcmp(platform, "windows") == 0) {
            return platform::windows;
        }

        return platform::none;
    }

    enum version version_from_string(const char *version) noexcept {
        if (strcmp(version, "v1") == 0) {
            return version::v1;
        } else if (strcmp(version, "v2") == 0) {
            return version::v2;
        }

        return version::none;
    }

    const char *get_parsed_symbol_string(const char *string, bool is_weak, enum symbol::type *type) {
        if (is_weak) {
            *type = symbol::type::weak_symbols;
            return string;
        }

        if (strncmp(string, "_OBJC_CLASS_$", 13) == 0) {
            *type = symbol::type::objc_classes;
            return &string[13];
        }

        if (strncmp(string, ".objc_class_name", 16) == 0) {
            *type = symbol::type::objc_classes;
            return &string[16];
        }

        if (strncmp(string, "_OBJC_METACLASS_$", 17) == 0) {
            *type = symbol::type::objc_classes;
            return &string[17];
        }

        if (strncmp(string, "_OBJC_IVAR_$", 12) == 0) {
            *type = symbol::type::objc_ivars;
            return &string[12];
        }

        *type = symbol::type::symbols;
        return string;
    }

    template <typename T>
    struct stream_helper;

    template <>
    struct stream_helper<int> {
        int descriptor = 0;
        explicit stream_helper(int descriptor) : descriptor(descriptor) {}

        inline auto print(const char *str) const noexcept {
            dprintf(descriptor, "%s", str);
        }
        
        __attribute__((format(printf, 2, 3))) inline auto printf(const char *str, ...) const noexcept {
            va_list list;

            va_start(list, str);
            vdprintf(descriptor, str, list);
            va_end(list);
            
        }

        inline auto print(const char &ch) const noexcept {
            printf("%c", ch);
        }
    };

    template <>
    struct stream_helper<FILE *> {
        FILE *file = nullptr;
        explicit stream_helper(FILE *file) : file(file) {}

        inline auto print(const char &ch) const noexcept {
            fputc(ch, file);
        }

        inline auto print(const char *str) const noexcept {
            fputs(str, file);
        }
        
        __attribute__((format(printf, 2, 3))) inline auto printf(const char *str, ...) const noexcept {
            va_list list;

            va_start(list, str);
            vfprintf(file, str, list);
            va_end(list);
        }
    };

    static inline constexpr const auto line_length_max = 105;

    template <typename T>
    inline void print_string_to_tbd_output_array(const stream_helper<T> &helper, const char *string, unsigned long &current_line_length) {
        const auto string_length = strlen(string);

        auto new_line_length = string_length + 2;
        auto new_current_line_length = current_line_length + new_line_length;

        // A line that is printed is allowed to go upto line_length_max. An exception
        // is made when one string is longer than line_length_max. When calculating
        // additional line length for a string, in addition to the string-length,
        // 2 is added for the comma and the space behind it.

        if (current_line_length >= line_length_max || (current_line_length != 0 && new_current_line_length > line_length_max)) {
            helper.printf(",\n%-26s", "");
            new_current_line_length = new_line_length;
        } else if (current_line_length != 0) {
            helper.print(", ");
            new_current_line_length += 2;
        }

        auto needs_quotes = strncmp(string, "$ld", 3) == 0;
        if (needs_quotes) {
            helper.print('\'');
        }

        helper.print(string);

        if (needs_quotes) {
            helper.print('\'');
        }

        current_line_length = new_current_line_length;
    }

    enum print_architectures_array_to_tbd_output_options {
        print_architectures_array_to_tbd_output_option_with_dash = 1 << 0
    };

    template <typename T>
    inline void print_architectures_array_to_tbd_output(const stream_helper<T> &helper, uint64_t architectures, uint64_t architecture_overrides, int options = 0) {
        const auto architecture_info_table = get_architecture_info_table();
        const auto architecture_info_table_size = get_architecture_info_table_size();

        if (architecture_overrides != 0) {
            auto index = uint64_t();
            for (; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & (1ull << index))) {
                    continue;
                }

                if (options & print_architectures_array_to_tbd_output_option_with_dash) {
                    helper.printf("  - archs:%-14s[ %s", "", architecture_info_table[index].name);
                } else {
                    helper.printf("archs:%-17s[ %s", "", architecture_info_table[index].name);
                }

                break;
            }

            if (index != architecture_info_table_size) {
                for (index++; index < architecture_info_table_size; index++) {
                    if (!(architecture_overrides & (1ull << index))) {
                        continue;
                    }

                    helper.printf(", %s", architecture_info_table[index].name);
                }

                helper.print(" ]\n");
            }
        } else {
            auto index = uint64_t();
            for (; index < architecture_info_table_size; index++) {
                if (!(architectures & (1ull << index))) {
                    continue;
                }

                if (options & print_architectures_array_to_tbd_output_option_with_dash) {
                    helper.printf("  - archs:%-14s[ %s", "", architecture_info_table[index].name);
                } else {
                    helper.printf("archs:%-17s[ %s", "", architecture_info_table[index].name);
                }

                break;
            }

            if (index != architecture_info_table_size) {
                for (index++; index < architecture_info_table_size; index++) {
                    if (!(architectures & (1ull << index))) {
                        continue;
                    }

                    helper.printf(", %s", architecture_info_table[index].name);
                }

                helper.print(" ]\n");
            }
        }
    }

    template <typename T>
    inline void print_export_group_to_tbd_output(const stream_helper<T> &helper, uint64_t architectures, uint64_t architecture_overrides, const bits &bits, const std::vector<std::string> &clients, const std::vector<reexport> &reexports, const std::vector<symbol> &symbols, version version) {
        const auto should_check_bits = bits.was_created();

        auto reexports_end = reexports.end();
        auto reexports_begin = reexports_end;

        if (!reexports.empty()) {
            reexports_begin = reexports.begin();

            if (should_check_bits) {
                for (; reexports_begin != reexports_end; reexports_begin++) {
                    const auto &reexport = *reexports_begin;
                    const auto &reexport_bits = reexport.bits;

                    if (reexport_bits != bits) {
                        continue;
                    }

                    break;
                }
            }
        }

        const auto symbols_end = symbols.end();

        auto symbols_begin_symbols_iter = symbols.begin();
        auto symbols_begin_objc_classes_iter = symbols_begin_symbols_iter;
        auto symbols_begin_objc_ivars_iter = symbols_begin_symbols_iter;
        auto symbols_begin_weak_symbols_iter = symbols_begin_symbols_iter;

        if (!symbols.empty()) {
            for (; symbols_begin_symbols_iter != symbols_end; symbols_begin_symbols_iter++) {
                const auto &symbol = *symbols_begin_symbols_iter;
                if (symbol.type != symbol::type::symbols) {
                    continue;
                }

                if (should_check_bits) {
                    const auto &symbol_bits = symbol.bits;
                    if (symbol_bits != bits) {
                        continue;
                    }
                }

                break;
            }

            for (; symbols_begin_objc_classes_iter != symbols_end; symbols_begin_objc_classes_iter++) {
                const auto &symbol = *symbols_begin_objc_classes_iter;
                if (symbol.type != symbol::type::objc_classes) {
                    continue;
                }

                if (should_check_bits) {
                    const auto &symbol_bits = symbol.bits;
                    if (symbol_bits != bits) {
                        continue;
                    }
                }

                break;
            }

            for (; symbols_begin_objc_ivars_iter != symbols_end; symbols_begin_objc_ivars_iter++) {
                const auto &symbol = *symbols_begin_objc_ivars_iter;
                if (symbol.type != symbol::type::objc_ivars) {
                    continue;
                }

                if (should_check_bits) {
                    const auto &symbol_bits = symbol.bits;
                    if (symbol_bits != bits) {
                        continue;
                    }
                }

                break;
            }

            for (; symbols_begin_weak_symbols_iter != symbols_end; symbols_begin_weak_symbols_iter++) {
                const auto &symbol = *symbols_begin_weak_symbols_iter;
                if (symbol.type != symbol::type::weak_symbols) {
                    continue;
                }

                if (should_check_bits) {
                    const auto &symbol_bits = symbol.bits;
                    if (symbol_bits != bits) {
                        continue;
                    }
                }

                break;
            }
        }

        if (clients.empty() && symbols_begin_symbols_iter == symbols_end && symbols_begin_objc_classes_iter == symbols_end && symbols_begin_objc_ivars_iter == symbols_end && symbols_begin_weak_symbols_iter == symbols_end) {
            return;
        }

        print_architectures_array_to_tbd_output(helper, architectures, architecture_overrides, print_architectures_array_to_tbd_output_option_with_dash);

        if (!clients.empty()) {
            auto clients_begin = clients.begin();
            auto clients_end = clients.end();

            switch (version) {
                case none:
                    break;

                case version::v1:
                    helper.printf("%-4sallowed-clients:%-3s[ %s", "",  "", clients_begin->data());
                    break;

                case version::v2:
                    helper.printf("%-4sallowable-clients: [ %s", "", clients_begin->data());
                    break;
            }

            for (clients_begin++; clients_begin != clients_end; clients_begin++) {
                helper.printf(", %s", clients_begin->data());
            }

            helper.print(" ]\n");
        }

        if (reexports_begin != reexports_end) {
            helper.printf("%-4sre-exports:%9s[ %s", "", "", reexports_begin->string);

            auto current_line_length = strlen(reexports_begin->string);
            for (auto reexports_iter = reexports_begin + 1; reexports_iter != reexports_end; reexports_iter++) {
                const auto &reexport = *reexports_iter;
                const auto &reexport_bits = reexport.bits;

                if (should_check_bits) {
                    if (reexport_bits != bits) {
                        continue;
                    }
                }

                print_string_to_tbd_output_array(helper, reexport.string, current_line_length);
            }

            helper.print(" ]\n");
        }

        if (symbols_begin_symbols_iter != symbols_end) {
            helper.printf("%-4ssymbols:%12s[ ", "", "");

            auto current_line_length = size_t();
            print_string_to_tbd_output_array(helper, symbols_begin_symbols_iter->string, current_line_length);

            for (symbols_begin_symbols_iter++; symbols_begin_symbols_iter != symbols_end; symbols_begin_symbols_iter++) {
                const auto &symbol = *symbols_begin_symbols_iter;
                if (symbol.type != symbol::type::symbols) {
                    continue;
                }

                if (should_check_bits) {
                    const auto &symbol_bits = symbol.bits;
                    if (symbol_bits != bits) {
                        continue;
                    }
                }

                print_string_to_tbd_output_array(helper, symbol.string, current_line_length);
            }

            helper.print(" ]\n");
        }

        if (symbols_begin_objc_classes_iter != symbols_end) {
            helper.printf("%-4sobjc-classes:%7s[ ", "", "");

            auto current_line_length = size_t();
            print_string_to_tbd_output_array(helper, symbols_begin_objc_classes_iter->string, current_line_length);

            for (symbols_begin_objc_classes_iter++; symbols_begin_objc_classes_iter != symbols_end; symbols_begin_objc_classes_iter++) {
                const auto &symbol = *symbols_begin_objc_classes_iter;
                if (symbol.type != symbol::type::objc_classes) {
                    continue;
                }

                if (should_check_bits) {
                    const auto &symbol_bits = symbol.bits;
                    if (symbol_bits != bits) {
                        continue;
                    }
                }

                print_string_to_tbd_output_array(helper, symbol.string, current_line_length);
            }

            helper.print(" ]\n");
        }

        if (symbols_begin_objc_ivars_iter != symbols_end) {
            helper.printf("%-4sobjc-ivars:%9s[ ", "", "");

            auto current_line_length = size_t();
            print_string_to_tbd_output_array(helper, symbols_begin_objc_ivars_iter->string, current_line_length);

            for (symbols_begin_objc_ivars_iter++; symbols_begin_objc_ivars_iter != symbols_end; symbols_begin_objc_ivars_iter++) {
                const auto &symbol = *symbols_begin_objc_ivars_iter;
                if (symbol.type != symbol::type::objc_ivars) {
                    continue;
                }

                if (should_check_bits) {
                    const auto &symbol_bits = symbol.bits;
                    if (symbol_bits != bits) {
                        continue;
                    }
                }

                print_string_to_tbd_output_array(helper, symbol.string, current_line_length);
            }

            helper.print(" ]\n");
        }

        if (symbols_begin_weak_symbols_iter != symbols_end) {
            helper.printf("%-4sweak-def-symbols:%3s[ ", "", "");

            auto current_line_length = size_t();
            print_string_to_tbd_output_array(helper, symbols_begin_weak_symbols_iter->string, current_line_length);

            for (symbols_begin_weak_symbols_iter++; symbols_begin_weak_symbols_iter != symbols_end; symbols_begin_weak_symbols_iter++) {
                const auto &symbol = *symbols_begin_weak_symbols_iter;
                if (symbol.type != symbol::type::weak_symbols) {
                    continue;
                }

                if (should_check_bits) {
                    const auto &symbol_bits = symbol.bits;
                    if (symbol_bits != bits) {
                        continue;
                    }
                }

                print_string_to_tbd_output_array(helper, symbol.string, current_line_length);
            }

            helper.print(" ]\n");
        }
    }

    creation_result get_required_load_command_data(container &container, uint64_t architecture_info_index, uint32_t &current_version, uint32_t &compatibility_version, uint32_t &swift_version, const char *&installation_name, platform &platform, objc_constraint &constraint, std::vector<reexport> &reexports, std::vector<std::string> &sub_clients, std::string &parent_umbrella, uint8_t *&uuid) {
        auto failure_result = creation_result::ok;

        const auto container_is_big_endian = container.is_big_endian();
        const auto should_find_library_platform = platform == platform::none;

        const auto container_base = container.base;
        const auto container_size = container.size;

        auto container_stream = container.stream;
        auto container_load_command_iteration_result = container.iterate_load_commands([&](long location, const load_command swapped, const load_command *load_command) {
            switch (swapped.cmd) {
                case load_commands::build_version: {
                    if (!should_find_library_platform) {
                        break;
                    }

                    const auto build_version_command = (macho::build_version_command *)load_command;
                    auto build_version_platform = build_version_command->platform;

                    if (container_is_big_endian) {
                        swap_uint32(build_version_platform);
                    }

                    auto build_version_parsed_platform = platform::none;
                    switch (build_version::platform(build_version_platform)) {
                        case build_version::platform::macos:
                            build_version_parsed_platform = platform::macosx;
                            break;

                        case build_version::platform::ios:
                            build_version_parsed_platform = platform::ios;
                            break;

                        case build_version::platform::tvos:
                            build_version_parsed_platform = platform::tvos;
                            break;

                        case build_version::platform::watchos:
                            build_version_parsed_platform = platform::watchos;
                            break;

                        default:
                            failure_result = creation_result::platform_not_supported;
                            return false;
                    }

                    if (platform != platform::none) {
                        if (platform != build_version_parsed_platform) {
                            failure_result = creation_result::multiple_platforms;
                            return false;
                        }
                    } else {
                        platform = build_version_parsed_platform;
                    }

                    break;
                }

                case load_commands::identification_dylib: {
                    auto identification_dylib_command = (dylib_command *)load_command;
                    auto identification_dylib_installation_name_string_index = identification_dylib_command->name.offset;

                    if (container_is_big_endian) {
                        swap_uint32(identification_dylib_installation_name_string_index);
                    }

                    if (identification_dylib_installation_name_string_index >= swapped.cmdsize) {
                        failure_result = creation_result::invalid_load_command;
                        return false;
                    }

                    const auto &identification_dylib_installation_name_string = &((char *)identification_dylib_command)[identification_dylib_installation_name_string_index];
                    const auto identification_dylib_installation_name_string_end = &identification_dylib_installation_name_string[strlen(identification_dylib_installation_name_string)];

                    if (std::all_of(identification_dylib_installation_name_string, identification_dylib_installation_name_string_end, isspace)) {
                        failure_result = creation_result::empty_installation_name;
                        return false;
                    }

                    if (installation_name != nullptr) {
                        if (strcmp(installation_name, identification_dylib_installation_name_string) != 0) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }
                    }

                    installation_name = identification_dylib_installation_name_string;

                    auto identification_dylib_current_version = identification_dylib_command->current_version;
                    auto identification_dylib_compatibility_version = identification_dylib_command->compatibility_version;

                    if (container_is_big_endian) {
                        swap_uint32(identification_dylib_current_version);
                        swap_uint32(identification_dylib_compatibility_version);
                    }

                    if (current_version != -1) {
                        if (current_version != identification_dylib_current_version) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }
                    }

                    if (compatibility_version != -1) {
                        if (compatibility_version != identification_dylib_compatibility_version) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }
                    }

                    current_version = identification_dylib_current_version;
                    compatibility_version = identification_dylib_compatibility_version;

                    break;
                }

                case load_commands::reexport_dylib: {
                    auto reexport_dylib_command = (dylib_command *)load_command;
                    auto reexport_dylib_string_index = reexport_dylib_command->name.offset;

                    if (container_is_big_endian) {
                        swap_uint32(reexport_dylib_string_index);
                    }

                    if (reexport_dylib_string_index > swapped.cmdsize) {
                        failure_result = creation_result::invalid_load_command;
                        return false;
                    }

                    const auto &reexport_dylib_string = &((char *)reexport_dylib_command)[reexport_dylib_string_index];
                    const auto reexports_iter = std::find(reexports.begin(), reexports.end(), reexport_dylib_string);

                    if (reexports_iter != reexports.end()) {
                        reexports_iter->add_architecture(architecture_info_index);
                    } else {
                        auto reexport = tbd::reexport();
                        auto reexport_creation_result = reexport.create(reexport_dylib_string);

                        switch (reexport_creation_result) {
                            case reexport::creation_result::ok:
                                break;

                            case reexport::creation_result::failed_to_allocate_memory:
                                failure_result = creation_result::failed_to_allocate_memory;
                                return false;
                        }

                        reexport.add_architecture(architecture_info_index);
                        reexports.emplace_back(std::move(reexport));
                    }

                    break;
                }

                case load_commands::segment: {
                    const auto segment_command = (macho::segment_command *)load_command;
                    if (strncmp(segment_command->segname, "__DATA", 16) != 0 && strncmp(segment_command->segname, "__DATA_CONST", 16) != 0 && strncmp(segment_command->segname, "__DATA_DIRTY", 16) != 0 && strncmp(segment_command->segname, "__OBJC", 16) != 0) {
                        break;
                    }

                    const auto container_is_64_bit = container.is_64_bit();
                    if (container_is_64_bit) {
                        break;
                    }

                    auto segment_sections_count = segment_command->nsects;
                    if (container_is_big_endian) {
                        swap_uint32(segment_sections_count);
                    }

                    if (segment_sections_count != 0) {
                        const auto segment_sections_size = sizeof(segments::section) * segment_sections_count;
                        const auto expected_command_size_without_data = sizeof(macho::segment_command) + segment_sections_size;

                        if (segment_command->cmdsize < expected_command_size_without_data) {
                            failure_result = creation_result::invalid_segment;
                            return false;
                        }

                        auto segment_section = reinterpret_cast<segments::section *>(&segment_command[1]);

                        auto objc_image_info = objc::image_info();
                        auto found_objc_image_info = false;

                        const auto segment_sections_end = reinterpret_cast<segments::section *>((uint64_t)&segment_command[1] + segment_sections_size);
                        while (segment_section != segment_sections_end) {
                            if (strncmp(segment_section->sectname, "__objc_imageinfo", 16) != 0 && strncmp(segment_section->sectname, "__image_info", 16) != 0) {
                                segment_section = &segment_section[1];
                                continue;
                            }

                            auto segment_section_data_offset = segment_section->offset;
                            if (container_is_big_endian) {
                                swap_uint32(segment_section_data_offset);
                            }

                            if (segment_section_data_offset >= container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            auto segment_section_data_size = segment_section->size;
                            if (container_is_big_endian) {
                                swap_uint32(segment_section_data_offset);
                            }

                            if (segment_section_data_size >= container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            const auto segment_section_data_end = segment_section_data_offset + segment_section_data_size;
                            if (segment_section_data_end >= container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            const auto library_container_stream_position = ftell(container_stream);

                            if (fseek(container_stream, container_base + segment_section_data_offset, SEEK_SET) != 0) {
                                failure_result = creation_result::stream_seek_error;
                                return false;
                            }

                            if (fread(&objc_image_info, sizeof(objc_image_info), 1, container_stream) != 1) {
                                failure_result = creation_result::stream_seek_error;
                                return false;
                            }

                            if (fseek(container_stream, library_container_stream_position, SEEK_SET) != 0) {
                                failure_result = creation_result::stream_seek_error;
                                return false;
                            }

                            found_objc_image_info = true;
                            break;
                        }

                        if (!found_objc_image_info) {
                            break;
                        }

                        const auto &objc_image_info_flags = objc_image_info.flags;

                        if (objc_image_info_flags & objc::image_info::flags::requires_gc) {
                            if (constraint != objc_constraint::no_value) {
                                if (constraint != objc_constraint::gc) {
                                    failure_result = creation_result::contradictary_load_command_information;
                                    return false;
                                }
                            } else {
                                constraint = objc_constraint::gc;
                            }
                        } else if (objc_image_info_flags & objc::image_info::flags::supports_gc) {
                            if (constraint != objc_constraint::no_value) {
                                if (constraint != objc_constraint::retain_release_or_gc) {
                                    failure_result = creation_result::contradictary_load_command_information;
                                    return false;
                                }
                            } else {
                                constraint = objc_constraint::retain_release_or_gc;
                            }
                        } else if (objc_image_info_flags & objc::image_info::flags::is_simulated) {
                            if (constraint != objc_constraint::no_value) {
                                if (constraint != objc_constraint::retain_release_for_simulator) {
                                    failure_result = creation_result::contradictary_load_command_information;
                                    return false;
                                }
                            } else {
                                constraint = objc_constraint::retain_release_for_simulator;
                            }
                        } else {
                            constraint = objc_constraint::retain_release;
                        }

                        auto objc_image_info_flags_swift_version = (objc_image_info_flags >> objc::image_info::swift_version::shift) & objc::image_info::swift_version::mask;
                        if (swift_version != 0) {
                            if (objc_image_info_flags_swift_version != swift_version) {
                                failure_result = creation_result::contradictary_load_command_information;
                                return false;
                            }
                        } else {
                            swift_version = objc_image_info_flags_swift_version;
                        }
                    }

                    break;
                }

                case load_commands::segment_64: {
                    const auto segment_command = (segment_command_64 *)load_command;
                    if (strncmp(segment_command->segname, "__DATA", 16) != 0 && strncmp(segment_command->segname, "__DATA_CONST", 16) != 0 && strncmp(segment_command->segname, "__DATA_DIRTY", 16) != 0 && strncmp(segment_command->segname, "__OBJC", 16) != 0) {
                        break;
                    }

                    const auto library_container_is_32_bit = container.is_32_bit();
                    if (library_container_is_32_bit) {
                        break;
                    }

                    auto segment_sections_count = segment_command->nsects;
                    if (container_is_big_endian) {
                        swap_uint32(segment_sections_count);
                    }

                    if (segment_sections_count != 0) {
                        const auto segment_sections_size = sizeof(segments::section_64) * segment_sections_count;
                        const auto expected_command_size_without_data = sizeof(macho::segment_command) + segment_sections_size;

                        if (segment_command->cmdsize < expected_command_size_without_data) {
                            failure_result = creation_result::invalid_segment;
                            return false;
                        }

                        auto segment_section = reinterpret_cast<segments::section_64 *>(&segment_command[1]);

                        auto objc_image_info = objc::image_info();
                        auto found_objc_image_info = false;

                        const auto segment_sections_end = reinterpret_cast<segments::section_64 *>((uint64_t)segment_section + segment_sections_size);
                        while (segment_section != segment_sections_end) {
                            if (strncmp(segment_section->sectname, "__objc_imageinfo", 16) != 0 && strncmp(segment_section->sectname, "__image_info", 16) != 0) {
                                segment_section = &segment_section[1];
                                continue;
                            }

                            auto segment_section_data_offset = segment_section->offset;
                            if (container_is_big_endian) {
                                swap_uint32(segment_section_data_offset);
                            }

                            if (segment_section_data_offset >= container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            auto segment_section_data_size = segment_section->size;
                            if (container_is_big_endian) {
                                swap_uint32(segment_section_data_offset);
                            }

                            if (segment_section_data_size >= container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            const auto segment_section_data_end = segment_section_data_offset + segment_section_data_size;
                            if (segment_section_data_end >= container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            const auto library_container_stream_position = ftell(container_stream);

                            if (fseek(container_stream, container_base + segment_section_data_offset, SEEK_SET) != 0) {
                                failure_result = creation_result::stream_seek_error;
                                return false;
                            }

                            if (fread(&objc_image_info, sizeof(objc_image_info), 1, container_stream) != 1) {
                                failure_result = creation_result::stream_seek_error;
                                return false;
                            }

                            if (fseek(container_stream, library_container_stream_position, SEEK_SET) != 0) {
                                failure_result = creation_result::stream_seek_error;
                                return false;
                            }

                            found_objc_image_info = true;
                            break;
                        }

                        if (!found_objc_image_info) {
                            break;
                        }

                        const auto &objc_image_info_flags = objc_image_info.flags;

                        if (objc_image_info_flags & objc::image_info::flags::requires_gc) {
                            if (constraint != objc_constraint::no_value) {
                                if (constraint != objc_constraint::gc) {
                                    failure_result = creation_result::contradictary_load_command_information;
                                    return false;
                                }
                            } else {
                                constraint = objc_constraint::gc;
                            }
                        } else if (objc_image_info_flags & objc::image_info::flags::supports_gc) {
                            if (constraint != objc_constraint::no_value) {
                                if (constraint != objc_constraint::retain_release_or_gc) {
                                    failure_result = creation_result::contradictary_load_command_information;
                                    return false;
                                }
                            } else {
                                constraint = objc_constraint::retain_release_or_gc;
                            }
                        } else if (objc_image_info_flags & objc::image_info::flags::is_simulated) {
                            if (constraint != objc_constraint::no_value) {
                                if (constraint != objc_constraint::retain_release_for_simulator) {
                                    failure_result = creation_result::contradictary_load_command_information;
                                    return false;
                                }
                            } else {
                                constraint = objc_constraint::retain_release_for_simulator;
                            }
                        } else {
                            constraint = objc_constraint::retain_release;
                        }

                        auto objc_image_info_flags_swift_version = (objc_image_info_flags >> objc::image_info::swift_version::shift) & objc::image_info::swift_version::mask;
                        if (swift_version != 0) {
                            if (objc_image_info_flags_swift_version != swift_version) {
                                failure_result = creation_result::contradictary_load_command_information;
                                return false;
                            }
                        } else {
                            swift_version = objc_image_info_flags_swift_version;
                        }
                    }

                    break;
                }

                case load_commands::sub_client: {
                    const auto sub_client_command = (macho::sub_client_command *)load_command;

                    auto sub_client_command_cmdsize = sub_client_command->cmdsize;
                    if (container_is_big_endian) {
                        swap_uint32(sub_client_command_cmdsize);
                    }

                    if (sub_client_command_cmdsize < sizeof(macho::sub_client_command)) {
                        failure_result = creation_result::invalid_sub_client;
                        return false;
                    }

                    auto sub_client_command_client_location = sub_client_command->client.offset;
                    if (container_is_big_endian) {
                        swap_uint32(sub_client_command_cmdsize);
                    }

                    if (sub_client_command_client_location < sizeof(macho::sub_client_command) || sub_client_command_client_location > sub_client_command_cmdsize) {
                        failure_result = creation_result::invalid_sub_client;
                        return false;
                    }

                    const auto sub_client_command_client_size = sub_client_command_cmdsize - sub_client_command_client_location;
                    if (sub_client_command_client_size != 0) {
                        const auto library_container_stream_position = ftell(container_stream);

                        if (fseek(container_stream, location + sub_client_command_client_location, SEEK_SET) != 0) {
                            failure_result = creation_result::stream_seek_error;
                            return false;
                        }

                        auto sub_client_command_client_string = static_cast<char *>(nullptr);
                        if (sub_client_command_client_size > sizeof(char *)) {
                            sub_client_command_client_string = static_cast<char *>(malloc(sub_client_command_client_size));
                            if (!sub_client_command_client_string) {
                                failure_result = creation_result::failed_to_allocate_memory;
                                return false;
                            }

                            if (fread(sub_client_command_client_string, sub_client_command_client_size, 1, container_stream) != 1) {
                                failure_result = creation_result::stream_read_error;
                                return false;
                            }
                        } else {
                            if (fread(&sub_client_command_client_string, sub_client_command_client_size, 1, container_stream) != 1) {
                                failure_result = creation_result::stream_read_error;
                                return false;
                            }
                        }

                        auto sub_clients_end = sub_clients.cend();
                        auto sub_clients_iter = sub_clients_end;

                        if (sub_client_command_client_size > sizeof(char *)) {
                            sub_clients_iter = std::find(sub_clients.cbegin(), sub_clients_end, sub_client_command_client_string);
                        } else {
                            sub_clients_iter = std::find(sub_clients.cbegin(), sub_clients_end, reinterpret_cast<char *>(&sub_client_command_client_string));
                        }

                        if (sub_clients_iter == sub_clients_end) {
                            if (sub_client_command_client_size > sizeof(char *)) {
                                sub_clients.emplace_back(sub_client_command_client_string);
                            } else {
                                sub_clients.emplace_back(reinterpret_cast<char *>(&sub_client_command_client_string));
                            }
                        }

                        if (sub_client_command_client_size > sizeof(char *)) {
                            free(sub_client_command_client_string);
                        }

                        if (fseek(container_stream, library_container_stream_position, SEEK_SET) != 0) {
                            failure_result = creation_result::stream_seek_error;
                            return false;
                        }
                    }

                    break;
                }

                case load_commands::sub_umbrella: {
                    const auto sub_umbrella_command = (macho::sub_umbrella_command *)load_command;

                    auto sub_umbrella_command_cmdsize = sub_umbrella_command->cmdsize;
                    if (container_is_big_endian) {
                        swap_uint32(sub_umbrella_command_cmdsize);
                    }

                    if (sub_umbrella_command_cmdsize < sizeof(macho::sub_umbrella_command)) {
                        failure_result = creation_result::invalid_sub_umbrella;
                        return false;
                    }

                    const auto sub_umbrella_command_umbrella_location = sub_umbrella_command->sub_umbrella.offset;
                    if (sub_umbrella_command_umbrella_location < sizeof(macho::sub_umbrella_command) || sub_umbrella_command_umbrella_location > sub_umbrella_command_cmdsize) {
                        failure_result = creation_result::invalid_sub_umbrella;
                        return false;
                    }

                    const auto sub_umbrella_command_umbrella_size = sub_umbrella_command_cmdsize - sub_umbrella_command_umbrella_location;
                    if (sub_umbrella_command_umbrella_size != 0) {
                        const auto library_container_stream_position = ftell(container_stream);

                        if (fseek(container_stream, location + sub_umbrella_command_umbrella_location, SEEK_SET) != 0) {
                            failure_result = creation_result::stream_seek_error;
                            return false;
                        }

                        auto sub_umbrella_command_umbrella_string = static_cast<char *>(nullptr);
                        if (sub_umbrella_command_umbrella_size > sizeof(char *)) {
                            sub_umbrella_command_umbrella_string = static_cast<char *>(malloc(sub_umbrella_command_umbrella_size));
                            if (!sub_umbrella_command_umbrella_string) {
                                failure_result = creation_result::failed_to_allocate_memory;
                                return false;
                            }

                            if (fread(sub_umbrella_command_umbrella_string, sub_umbrella_command_umbrella_size, 1, container_stream) != 1) {
                                failure_result = creation_result::stream_read_error;
                                return false;
                            }
                        } else {
                            if (fread(&sub_umbrella_command_umbrella_string, sub_umbrella_command_umbrella_size, 1, container_stream) != 1) {
                                failure_result = creation_result::stream_read_error;
                                return false;
                            }
                        }

                        if (fseek(container_stream, library_container_stream_position, SEEK_SET) != 0) {
                            failure_result = creation_result::stream_seek_error;
                            return false;
                        }

                        if (!parent_umbrella.empty() && sub_umbrella_command_umbrella_string != parent_umbrella) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }

                        if (sub_umbrella_command_umbrella_size > sizeof(char *)) {
                            parent_umbrella = reinterpret_cast<char *>(&sub_umbrella_command_umbrella_string);
                            free(sub_umbrella_command_umbrella_string);
                        } else {
                            parent_umbrella = sub_umbrella_command_umbrella_string;
                        }
                    }

                    break;
                }

                case load_commands::uuid: {
                    auto &load_command_uuid = ((uuid_command *)load_command)->uuid;

                    // Check if multiple uuid load-commands were
                    // found in the same container

                    if (uuid != nullptr) {
                        if (memcmp(&load_command_uuid, uuid, 16) != 0) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }

                        break;
                    }

                    uuid = reinterpret_cast<uint8_t *>(&load_command_uuid);
                    break;
                }

                case load_commands::version_min_macosx: {
                    if (!should_find_library_platform) {
                        break;
                    }

                    if (platform != platform::none) {
                        if (platform != platform::macosx) {
                            failure_result = creation_result::multiple_platforms;
                            return false;
                        }
                    } else {
                        platform = platform::macosx;
                    }

                    break;
                }

                case load_commands::version_min_iphoneos: {
                    if (!should_find_library_platform) {
                        break;
                    }

                    if (platform != platform::none) {
                        if (platform != platform::ios) {
                            failure_result = creation_result::multiple_platforms;
                            return false;
                        }
                    } else {
                        platform = platform::ios;
                    }

                    break;
                }

                case load_commands::version_min_watchos: {
                    if (!should_find_library_platform) {
                        break;
                    }

                    if (platform != platform::none) {
                        if (platform != platform::watchos) {
                            failure_result = creation_result::multiple_platforms;
                            return false;
                        }
                    } else {
                        platform = platform::watchos;
                    }

                    break;
                }

                case load_commands::version_min_tvos: {
                    if (!should_find_library_platform) {
                        break;
                    }

                    if (platform != platform::none) {
                        if (platform != platform::tvos) {
                            failure_result = creation_result::multiple_platforms;
                            return false;
                        }
                    } else {
                        platform = platform::tvos;
                    }

                    break;
                }

                default:
                    break;
            }

            return true;
        });

        switch (container_load_command_iteration_result) {
            case container::load_command_iteration_result::ok:
                break;

            case container::load_command_iteration_result::no_load_commands:
            case container::load_command_iteration_result::stream_seek_error:
            case container::load_command_iteration_result::stream_read_error:
            case container::load_command_iteration_result::load_command_is_too_small:
            case container::load_command_iteration_result::load_command_is_too_large:
                return creation_result::failed_to_iterate_load_commands;
        }

        return failure_result;
    }

    creation_result get_symbols(container &container, uint64_t architecture_info_index, std::vector<symbol> &symbols, options options) {
        auto symbols_iteration_failure_result = creation_result::ok;
        auto library_container_symbols_iteration_result = container.iterate_symbol_table([&](const nlist_64 &symbol_table_entry, const char *symbol_string) {
            const auto &symbol_table_entry_type = symbol_table_entry.n_type;
            if ((symbol_table_entry_type & symbol_table::flags::type) != symbol_table::type::section) {
                return true;
            }

            enum symbol::type symbol_type;

            const auto symbol_is_weak = symbol_table_entry.n_desc & symbol_table::description::weak_definition;
            const auto parsed_symbol_string = get_parsed_symbol_string(symbol_string, symbol_is_weak, &symbol_type);

            if ((options & options::allow_all_private_symbols) == options::none) {
                switch (symbol_type) {
                    case symbol::type::symbols:
                        if ((options & options::allow_private_normal_symbols) == options::none) {
                            if (!(symbol_table_entry_type & symbol_table::flags::external)) {
                                return true;
                            }
                        }

                        break;

                    case symbol::type::weak_symbols:
                        if ((options & options::allow_private_weak_symbols) == options::none) {
                            if (!(symbol_table_entry_type & symbol_table::flags::external)) {
                                return true;
                            }
                        }

                        break;

                    case symbol::type::objc_classes:
                        if ((options & options::allow_private_objc_symbols) == options::none) {
                            if ((options & options::allow_private_objc_classes) == options::none) {
                                if (!(symbol_table_entry_type & symbol_table::flags::external)) {
                                    return true;
                                }
                            }
                        }

                        break;

                    case symbol::type::objc_ivars:
                        if ((options & options::allow_private_objc_symbols) == options::none) {
                            if ((options & options::allow_private_objc_ivars) == options::none) {
                                if (!(symbol_table_entry_type & symbol_table::flags::external)) {
                                    return true;
                                }
                            }
                        }

                        break;
                }
            }

            const auto symbols_iter = std::find(symbols.begin(), symbols.end(), parsed_symbol_string);
            if (symbols_iter != symbols.end()) {
                symbols_iter->add_architecture(architecture_info_index);
            } else {
                auto symbol = tbd::symbol();
                auto symbol_creation_result = symbol.create(parsed_symbol_string, symbol_is_weak, symbol_type);

                switch (symbol_creation_result) {
                    case symbol::creation_result::ok:
                        break;

                    case symbol::creation_result::failed_to_allocate_memory:
                        symbols_iteration_failure_result = creation_result::failed_to_allocate_memory;
                        return false;
                }

                symbol.add_architecture(architecture_info_index);
                symbols.emplace_back(std::move(symbol));
            }

            return true;
        });

        switch (library_container_symbols_iteration_result) {
            case container::symbols_iteration_result::ok:
            case container::symbols_iteration_result::no_symbols:
            case container::symbols_iteration_result::no_symbol_table_load_command:
                break;

            case container::symbols_iteration_result::stream_seek_error:
            case container::symbols_iteration_result::stream_read_error:
            case container::symbols_iteration_result::invalid_symbol_table_load_command:
            case container::symbols_iteration_result::invalid_symbol_table_entry:
                return creation_result::failed_to_iterate_symbols;
        }

        return creation_result::ok;
    }

    template <typename T>
    inline creation_result create_from_macho_library(file &library, const stream_helper<T> &helper, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides) {
        uint32_t library_current_version = -1;
        uint32_t library_compatibility_version = -1;
        uint32_t library_swift_version = 0;
        uint32_t library_flags = 0;

        auto library_installation_name = static_cast<const char *>(nullptr);
        auto library_objc_constraint = objc_constraint::no_value;
        auto library_parent_umbrella = std::string();

        auto library_reexports = std::vector<reexport>();
        auto library_symbols = std::vector<symbol>();

        auto &library_containers = library.containers;
        auto library_containers_index = 0;

        const auto library_containers_size = library_containers.size();

        auto library_uuids = std::vector<std::pair<uint64_t, const uint8_t *>>();
        auto library_container_architectures = uint64_t();

        library_uuids.reserve(library_containers_size);

        auto has_found_architectures_provided = false;
        for (auto &library_container : library_containers) {
            const auto &library_container_header = library_container.header;

            const auto library_container_header_filetype = library_container_header.filetype;
            const auto library_container_header_flags = library_container_header.flags;

            const auto library_container_header_cputype = library_container_header.cputype;
            const auto library_container_header_cpusubtype = library_container_header.cpusubtype;

            if (library_container.is_big_endian()) {
                swap_uint32(*(uint32_t *)&library_container_header_filetype);
                swap_uint32(*(uint32_t *)&library_container_header_flags);

                swap_uint32(*(uint32_t *)&library_container_header_cputype);
                swap_uint32(*(uint32_t *)&library_container_header_cpusubtype);
            }

            if (!filetype_is_dynamic_library(library_container_header_filetype)) {
                return creation_result::unsupported_filetype;
            }

            if ((options & options::remove_flags) == options::none) {
                if (library_container_header_flags & macho::flags::two_level_namespaces) {
                    if (!(library_flags & macho::flags::two_level_namespaces)) {
                        if (library_containers_index != 0) {
                            return creation_result::inconsistent_flags;
                        }

                        library_flags |= macho::flags::two_level_namespaces;
                    }
                }

                if (!(library_container_header_flags & macho::flags::app_extension_safe)) {
                    if (library_flags & macho::flags::app_extension_safe) {
                        if (library_containers_index != 0) {
                            return creation_result::inconsistent_flags;
                        }

                        library_flags &= ~macho::flags::app_extension_safe;
                    }
                }
            }

            const auto library_container_header_subtype = subtype_from_cputype(library_container_header_cputype, library_container_header.cpusubtype);
            if (library_container_header_subtype == subtype::none) {
                return creation_result::invalid_subtype;
            }

            const auto library_container_architecture_info_table_index = architecture_info_index_from_cputype(library_container_header_cputype, library_container_header_subtype);
            if (library_container_architecture_info_table_index == -1) {
                return creation_result::invalid_cputype;
            }

            if (architectures != 0) {
                // any is the first architecture info and if set is stored in the LSB
                if (!(architectures & 1)) {
                    if (!(architectures & (1ull << library_container_architecture_info_table_index))) {
                        continue;
                    }
                }

                has_found_architectures_provided = true;
            }

            if (architecture_overrides == 0) {
                library_container_architectures |= 1ull << library_container_architecture_info_table_index;
            }

            uint32_t local_current_version = -1;
            uint32_t local_compatibility_version = -1;
            uint32_t local_swift_version = 0;

            auto local_installation_name = static_cast<const char *>(nullptr);
            auto local_objc_constraint = objc_constraint::no_value;
            auto local_parent_umbrella = std::string();

            auto local_sub_clients = std::vector<std::string>();
            auto local_uuid = static_cast<uint8_t *>(nullptr);

            const auto failure_result = get_required_load_command_data(library_container, library_container_architecture_info_table_index, local_current_version, local_compatibility_version, local_swift_version, local_installation_name, platform, local_objc_constraint, library_reexports, local_sub_clients, local_parent_umbrella, local_uuid);

            if (failure_result != creation_result::ok) {
                return failure_result;
            }

            if (platform == platform::none) {
                return creation_result::platform_not_found;
            }

            if (!local_installation_name) {
                return creation_result::not_a_library;
            }

            if (library_installation_name != nullptr) {
                if (strcmp(library_installation_name, local_installation_name) != 0) {
                    return creation_result::contradictary_container_information;
                }
            } else {
                library_installation_name = local_installation_name;
            }

            if ((options & options::remove_current_version) == options::none) {
                if (library_current_version != -1) {
                    if (local_current_version != library_current_version) {
                        return creation_result::contradictary_container_information;
                    }
                } else {
                    library_current_version = local_current_version;
                }
            }

            if ((options & options::remove_compatibility_version) == options::none) {
                if (library_compatibility_version != -1) {
                    if (library_compatibility_version != local_compatibility_version) {
                        return creation_result::contradictary_container_information;
                    }
                } else {
                    library_compatibility_version = local_compatibility_version;
                }
            }

            if ((options & options::remove_swift_version) == options::none) {
                if (library_swift_version != 0) {
                    if (library_swift_version != local_swift_version) {
                        return creation_result::contradictary_container_information;
                    }
                } else {
                    library_swift_version = local_swift_version;
                }
            }

            if ((options & options::remove_objc_constraint) == options::none) {
                if (library_objc_constraint != objc_constraint::no_value) {
                    if (library_objc_constraint != local_objc_constraint) {
                        return creation_result::contradictary_container_information;
                    }
                } else {
                    library_objc_constraint = local_objc_constraint;
                }
            }

            if (version == version::v2) {
                if ((options & options::remove_parent_umbrella) == options::none) {
                    if (!library_parent_umbrella.empty()) {
                        if (library_parent_umbrella != local_parent_umbrella) {
                            return creation_result::contradictary_container_information;
                        }
                    } else if (!local_parent_umbrella.empty()) {
                        // In the scenario where one container had no (or empty) parent-umbrella
                        // and another container did not

                        if (library_containers_index != 0) {
                            return creation_result::contradictary_container_information;
                        }

                        library_parent_umbrella = std::move(local_parent_umbrella);
                    }
                }

                if ((options & options::remove_uuids) == options::none) {
                    if (!local_uuid) {
                        if ((options & options::ignore_missing_uuids) == options::none) {
                            return creation_result::has_no_uuid;
                        }
                    } else {
                        if ((options & options::ignore_non_unique_uuid) == options::none) {
                            const auto library_uuids_iter = std::find_if(library_uuids.begin(), library_uuids.end(), [&](const std::pair<uint64_t, const uint8_t *> &rhs) {
                                return memcmp(local_uuid, rhs.second, 16) == 0;
                            });

                            const auto &uuids_end = library_uuids.end();
                            if (library_uuids_iter != uuids_end) {
                                return creation_result::uuid_is_not_unique;
                            }
                        }

                        library_uuids.emplace_back(library_container_architecture_info_table_index, local_uuid);
                    }
                }
            }

            const auto symbols_iteration_failure_result = get_symbols(library_container, library_container_architecture_info_table_index, library_symbols, options);
            if (symbols_iteration_failure_result != creation_result::ok) {
                return symbols_iteration_failure_result;
            }

            library_containers_index++;
        }

        if (architectures != 0) {
            if (!has_found_architectures_provided) {
                return creation_result::no_provided_architectures;
            }
        }

        if ((options & options::ignore_missing_exports) == options::none) {
            if (library_reexports.empty() && library_symbols.empty()) {
                return creation_result::no_symbols_or_reexports;
            }
        }

        std::sort(library_reexports.begin(), library_reexports.end(), [](const reexport &lhs, const reexport &rhs) {
            const auto lhs_string = lhs.string;
            const auto rhs_string = rhs.string;

            return strcmp(lhs_string, rhs_string) < 0;
        });

        std::sort(library_symbols.begin(), library_symbols.end(), [](const symbol &lhs, const symbol &rhs) {
            const auto lhs_string = lhs.string;
            const auto rhs_string = rhs.string;

            return strcmp(lhs_string, rhs_string) < 0;
        });

        auto groups = std::vector<group>();
        if (architecture_overrides != 0) {
            groups.emplace_back();
        } else {
            for (const auto &library_reexport : library_reexports) {
                const auto &library_reexport_bits = library_reexport.bits;
                const auto group_iter = std::find_if(groups.begin(), groups.end(), [&](const group &lhs) {
                    return lhs.bits == library_reexport_bits;
                });

                if (group_iter != groups.end()) {
                    group_iter->reexports_count++;
                } else {
                    auto group = tbd::group();
                    auto group_creation_result = group.create(library_reexport_bits);

                    switch (group_creation_result) {
                        case group::creation_result::ok:
                            break;

                        case group::creation_result::failed_to_allocate_memory:
                            return creation_result::failed_to_allocate_memory;
                    }

                    group.reexports_count++;
                    groups.emplace_back(std::move(group));
                }
            }

            for (const auto &library_symbol : library_symbols) {
                const auto &library_symbol_bits = library_symbol.bits;
                const auto group_iter = std::find_if(groups.begin(), groups.end(), [&](const group &lhs) {
                    return lhs.bits == library_symbol_bits;
                });

                if (group_iter != groups.end()) {
                    group_iter->symbols_count++;
                } else {
                    auto group = tbd::group();
                    auto group_creation_result = group.create(library_symbol_bits);

                    switch (group_creation_result) {
                        case group::creation_result::ok:
                            break;

                        case group::creation_result::failed_to_allocate_memory:
                            return creation_result::failed_to_allocate_memory;
                    }

                    group.symbols_count++;
                    groups.emplace_back(std::move(group));
                }
            }

            std::sort(groups.begin(), groups.end(), [](const group &lhs, const group &rhs) {
                const auto lhs_symbols_count = lhs.symbols_count;
                const auto rhs_symbols_count = rhs.symbols_count;

                return lhs_symbols_count < rhs_symbols_count;
            });
        }

        helper.print("---");
        if (version == version::v2) {
            helper.print(" !tapi-tbd-v2");
        }

        helper.print('\n');
        print_architectures_array_to_tbd_output(helper, library_container_architectures, architecture_overrides);

        if (architecture_overrides == 0) {
            if (version == version::v2) {
                if ((options & options::remove_uuids) == options::none) {
                    const auto library_uuids_size = library_uuids.size();
                    if (library_uuids_size) {
                        helper.printf("uuids:%-17s[ ", "");

                        const auto architecture_info_table = get_architecture_info_table();
                        for (auto library_uuids_index = 0; library_uuids_index != library_uuids_size; library_uuids_index++) {
                            const auto &library_uuid_pair = library_uuids.at(library_uuids_index);

                            const auto &library_container_architecture_arch_info = architecture_info_table[library_uuid_pair.first];
                            const auto &library_uuid = library_uuid_pair.second;

                            helper.printf("'%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X'", library_container_architecture_arch_info.name, library_uuid[0], library_uuid[1], library_uuid[2], library_uuid[3], library_uuid[4], library_uuid[5], library_uuid[6], library_uuid[7], library_uuid[8], library_uuid[9], library_uuid[10], library_uuid[11], library_uuid[12], library_uuid[13], library_uuid[14], library_uuid[15]);

                            if (library_uuids_index != library_uuids_size - 1) {
                                helper.print(", ");

                                if (library_uuids_index % 2 != 0) {
                                    helper.printf("%-26s", "\n");
                                }
                            }
                        }

                        helper.print(" ]\n");
                    }
                }
            }
        }

        helper.printf("platform:%-14s%s\n", "", platform_to_string(platform));

        if ((options & options::remove_flags) == options::none || flags != flags::none) {
            if (flags != flags::none) {
                if ((flags & flags::flat_namespace) != flags::none) {
                    library_flags |= macho::flags::two_level_namespaces;
                }

                if ((flags & flags::not_app_extension_safe) != flags::none) {
                    library_flags &= ~macho::flags::two_level_namespaces;
                }
            }

            if (library_flags & macho::flags::two_level_namespaces || !(library_flags & macho::flags::app_extension_safe)) {
                helper.printf("flags:%-17s[ ", "");

                if (library_flags & macho::flags::two_level_namespaces) {
                    helper.print("flat_namespace");

                    if (!(library_flags & macho::flags::app_extension_safe)) {
                        helper.print(", not_app_extension_safe");
                    }
                } else if (!(library_flags & macho::flags::app_extension_safe)) {
                    helper.print("not_app_extension_safe");
                }

                helper.print(" ]\n");
            }
        }

        helper.printf("install-name:%-10s%s\n", "", library_installation_name);

        if ((options & options::remove_current_version) == options::none) {
            auto library_current_version_major = library_current_version >> 16;
            auto library_current_version_minor = (library_current_version >> 8) & 0xff;
            auto library_current_version_revision = library_current_version & 0xff;

            helper.printf("current-version:%-7s%u", "", library_current_version_major);

            if (library_current_version_minor != 0) {
                helper.printf(".%u", library_current_version_minor);
            }

            if (library_current_version_revision != 0) {
                if (library_current_version_minor == 0) {
                    helper.print(".0");
                }

                helper.printf(".%u", library_current_version_revision);
            }

            helper.print('\n');
        }

        if ((options & options::remove_compatibility_version) == options::none) {
            auto library_compatibility_version_major = library_compatibility_version >> 16;
            auto library_compatibility_version_minor = (library_compatibility_version >> 8) & 0xff;
            auto library_compatibility_version_revision = library_compatibility_version & 0xff;

            helper.printf("compatibility-version: %u", library_compatibility_version_major);

            if (library_compatibility_version_minor != 0) {
                helper.printf(".%u", library_compatibility_version_minor);
            }

            if (library_compatibility_version_revision != 0) {
                if (library_compatibility_version_minor == 0) {
                    helper.print(".0");
                }

                helper.printf(".%u", library_compatibility_version_revision);
            }

            helper.print('\n');
        }

        if ((options & options::remove_swift_version) == options::none) {
            if (library_swift_version != 0) {
                switch (library_swift_version) {
                    case 1:
                        helper.printf("swift-version:%-9s1\n", "");
                        break;

                    case 2:
                        helper.printf("swift-version:%-9s1.2\n", "");
                        break;

                    default:
                        helper.printf("swift-version:%-9s%u\n", "", library_swift_version - 1);
                        break;
                }
            }
        }

        if ((options & options::remove_objc_constraint) == options::none) {
            if (const auto objc_constraint_string = objc_constraint_to_string(library_objc_constraint); objc_constraint_string != nullptr) {
                helper.printf("objc-constraint:%-7s%s\n", "", objc_constraint_to_string(library_objc_constraint));
            }
        }

        if (version == version::v2) {
            if ((options & options::remove_parent_umbrella) == options::none) {
                if (!library_parent_umbrella.empty()) {
                    helper.printf("parent-umbrella:%-7s%s\n", "", library_parent_umbrella.data());
                }
            }
        }

        helper.print("exports:\n");

        if (architecture_overrides != 0) {
            print_export_group_to_tbd_output(helper, library_container_architectures, architecture_overrides, bits(), std::vector<std::string>(), library_reexports, library_symbols, version);
        } else {
            for (const auto &group : groups) {
                print_export_group_to_tbd_output(helper, library_container_architectures, architecture_overrides, group.bits, group.clients, library_reexports, library_symbols, version);
            }
        }

        helper.print("...\n");
        return creation_result::ok;
    }

    template <typename T>
    inline creation_result create_from_macho_library(container &container, const stream_helper<T> &helper, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides) {
        uint32_t current_version = -1;
        uint32_t compatibility_version = -1;
        uint32_t swift_version = 0;

        auto installation_name = static_cast<const char *>(nullptr);
        auto objc_constraint = objc_constraint::no_value;
        auto parent_umbrella = std::string();

        auto reexports = std::vector<reexport>();
        auto symbols = std::vector<symbol>();

        auto sub_clients = std::vector<std::string>();
        auto uuid = static_cast<uint8_t *>(nullptr);

        const auto &header = container.header;

        const auto header_filetype = header.filetype;
        const auto header_flags = header.flags;

        const auto header_cputype = header.cputype;
        const auto header_cpusubtype = header.cpusubtype;

        if (container.is_big_endian()) {
            swap_uint32(*(uint32_t *)&header_filetype);
            swap_uint32(*(uint32_t *)&header_flags);

            swap_uint32(*(uint32_t *)&header_cputype);
            swap_uint32(*(uint32_t *)&header_cpusubtype);
        }

        if (!filetype_is_dynamic_library(header_filetype)) {
            return creation_result::unsupported_filetype;
        }

        const auto header_subtype = subtype_from_cputype(header_cputype, header_cpusubtype);
        if (header_subtype == subtype::none) {
            return creation_result::invalid_subtype;
        }

        const auto architecture_info = architecture_info_from_cputype(header_cputype, header_subtype);
        if (!architecture_info) {
            return creation_result::invalid_cputype;
        }

        const auto architecture_info_table_index = architecture_info_index_from_cputype(header_cputype, header_subtype);
        if (architectures != 0) {
            // any is the first architecture info and if set is stored in the LSB
            if (!(architectures & 1)) {
                if (!(architectures & (1ull << architecture_info_table_index))) {
                    return creation_result::no_provided_architectures;
                }
            }
        }

        const auto failure_result = get_required_load_command_data(container, architecture_info_table_index, current_version, compatibility_version, swift_version, installation_name, platform, objc_constraint, reexports, sub_clients, parent_umbrella, uuid);
        if (failure_result != creation_result::ok) {
            return failure_result;
        }

        if (platform == platform::none) {
            return creation_result::platform_not_found;
        }

        if (!installation_name) {
            return creation_result::not_a_library;
        }

        if (version == version::v2) {
            if ((options & options::remove_uuids) == options::none || (options & options::ignore_missing_uuids) == options::none) {
                if (!uuid) {
                    return creation_result::has_no_uuid;
                }
            }
        }

        const auto symbols_iteration_failure_result = get_symbols(container, architecture_info_table_index, symbols, options);
        if (symbols_iteration_failure_result != creation_result::ok) {
            return symbols_iteration_failure_result;
        }

        if ((options & options::ignore_missing_exports) == options::none) {
            if (reexports.empty() && symbols.empty()) {
                return creation_result::no_symbols_or_reexports;
            }
        }

        std::sort(reexports.begin(), reexports.end(), [](const reexport &lhs, const reexport &rhs) {
            const auto lhs_string = lhs.string;
            const auto rhs_string = rhs.string;

            return strcmp(lhs_string, rhs_string) < 0;
        });

        std::sort(symbols.begin(), symbols.end(), [](const symbol &lhs, const symbol &rhs) {
            const auto lhs_string = lhs.string;
            const auto rhs_string = rhs.string;

            return strcmp(lhs_string, rhs_string) < 0;
        });

        helper.print("---");
        if (version == version::v2) {
            helper.print(" !tapi-tbd-v2");
        }

        helper.print('\n');
        print_architectures_array_to_tbd_output(helper, 1ull << architecture_info_table_index, architecture_overrides);

        if (architecture_overrides == 0) {
            if (version == version::v2) {
                if ((options & options::remove_uuids) == options::none) {
                    const auto architecture_info_table = get_architecture_info_table();
                    const auto architecture_info_name = architecture_info_table[architecture_info_table_index].name;

                    helper.printf("uuids:%-17s[ '%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X' ]\n", "", architecture_info_name, uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
                }
            }
        }

        helper.printf("platform:%-14s%s\n", "", platform_to_string(platform));

        if ((options & options::remove_flags) == options::none || flags != flags::none) {
            auto container_flags = header_flags;
            if (flags != flags::none) {
                if ((flags & flags::flat_namespace) != flags::none) {
                    container_flags |= macho::flags::two_level_namespaces;
                }

                if ((flags & flags::not_app_extension_safe) != flags::none) {
                    container_flags &= ~macho::flags::two_level_namespaces;
                }
            }

            if (container_flags & macho::flags::two_level_namespaces || !(container_flags & macho::flags::app_extension_safe)) {
                helper.printf("flags:%-17s[ ", "");

                if (header_flags & macho::flags::two_level_namespaces) {
                    helper.print("flat_namespace");

                    if (!(header_flags & macho::flags::app_extension_safe)) {
                        helper.print(", not_app_extension_safe");
                    }
                } else if (!(header_flags & macho::flags::app_extension_safe)) {
                    helper.print("not_app_extension_safe");
                }

                helper.print(" ]\n");
            }
        }

        helper.printf("install-name:%-10s%s\n", "", installation_name);

        if ((options & options::remove_current_version) == options::none) {
            auto library_current_version_major = current_version >> 16;
            auto library_current_version_minor = (current_version >> 8) & 0xff;
            auto library_current_version_revision = current_version & 0xff;

            helper.printf("current-version:%-7s%u", "", library_current_version_major);

            if (library_current_version_minor != 0) {
                helper.printf(".%u", library_current_version_minor);
            }

            if (library_current_version_revision != 0) {
                if (library_current_version_minor == 0) {
                    helper.print(".0");
                }

                helper.printf(".%u", library_current_version_revision);
            }

            helper.print('\n');
        }

        if ((options & options::remove_compatibility_version) == options::none) {
            auto compatibility_version_major = compatibility_version >> 16;
            auto compatibility_version_minor = (compatibility_version >> 8) & 0xff;
            auto compatibility_version_revision = compatibility_version & 0xff;

            helper.printf("compatibility-version: %u", compatibility_version_major);

            if (compatibility_version_minor != 0) {
                helper.printf(".%u", compatibility_version_minor);
            }

            if (compatibility_version_revision != 0) {
                if (compatibility_version_minor == 0) {
                    helper.print(".0");
                }

                helper.printf(".%u", compatibility_version_revision);
            }

            helper.print('\n');
        }

        if ((options & options::remove_swift_version) == options::none) {
            if (swift_version != 0) {
                switch (swift_version) {
                    case 1:
                        helper.printf("swift-version:%-9s1\n", "");
                        break;

                    case 2:
                        helper.printf("swift-version:%-9s1.2\n", "");
                        break;

                    default:
                        helper.printf("swift-version:%-9s%u\n", "", swift_version - 1);
                        break;
                }
            }
        }

        if ((options & options::remove_objc_constraint) == options::none) {
            if (const auto objc_constraint_string = objc_constraint_to_string(objc_constraint); objc_constraint_string != nullptr) {
                helper.printf("objc-constraint:%-7s%s\n", "", objc_constraint_to_string(objc_constraint));
            }
        }

        if (version == version::v2) {
            if ((options & options::remove_parent_umbrella) == options::none) {
                if (!parent_umbrella.empty()) {
                    helper.printf("parent-umbrella:%-7s%s\n", "", parent_umbrella.data());
                }
            }
        }

        helper.print("exports:\n");
        print_export_group_to_tbd_output(helper, 1ull << architecture_info_table_index, architecture_overrides, bits(), sub_clients, reexports, symbols, version);

        helper.print("...\n");
        return creation_result::ok;
    }

    creation_result create_from_macho_library(file &library, int output, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides) {
        return create_from_macho_library(library, stream_helper<int>(output), options, flags, constraint, platform, version, architectures, architecture_overrides);
    }

    creation_result create_from_macho_library(container &container, int output, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides) {
        return create_from_macho_library(container, stream_helper<int>(output), options, flags, constraint, platform, version, architectures, architecture_overrides);
    }

    creation_result create_from_macho_library(file &library, FILE *output, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides) {
        return create_from_macho_library(library, stream_helper<FILE *>(output), options, flags, constraint, platform, version, architectures, architecture_overrides);
    }

    creation_result create_from_macho_library(container &container, FILE *output, options options, flags flags, objc_constraint constraint, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides) {
        return create_from_macho_library(container, stream_helper<FILE *>(output), options, flags, constraint, platform, version, architectures, architecture_overrides);
    }
}
