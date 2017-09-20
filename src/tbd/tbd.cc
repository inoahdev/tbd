//
//  src/tbd/tbd.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>

#include "../mach-o/headers/build.h"
#include "../mach-o/headers/segment.h"

#include "../misc/flags.h"
#include "../objc/image_info.h"

#include "tbd.h"

namespace tbd {
    class group {
    public:
        explicit group() = default;
        explicit group(const flags &flags) noexcept
        : flags(flags) {}

        class flags flags;

        uint64_t reexports_count = 0;
        uint64_t symbols_count = 0;

        inline bool operator==(const group &group) const noexcept { return flags == group.flags; }
        inline bool operator!=(const group &group) const noexcept { return flags != group.flags; }
    };

    class reexport {
    public:
        explicit reexport(const char *string, flags_integer_t flags_length) noexcept
        : flags(flags_length), string(string) {}

        inline void add_architecture(flags_integer_t index) noexcept { flags.cast(index, true); }

        class flags flags;
        const char *string;

        inline bool operator==(const char *string) const noexcept { return strcmp(this->string, string) == 0; }
        inline bool operator==(const reexport &reexport) const noexcept { return strcmp(string, reexport.string) == 0; }

        inline bool operator!=(const char *string) const noexcept { return strcmp(this->string, string) != 0; }
        inline bool operator!=(const reexport &reexport) const noexcept { return strcmp(string, reexport.string) != 0; }
    };

    class symbol {
    public:
        enum class type {
            symbols,
            weak_symbols,
            objc_classes,
            objc_ivars
        };

        explicit symbol(const char *string, bool weak, flags_integer_t flags_length, enum type type) noexcept
        : flags(flags_length), string(string), type(type), weak(weak) {}

        class flags flags;
        const char *string;

        enum type type;
        bool weak;

        inline void add_architecture(flags_integer_t index) noexcept { flags.cast(index, true); }

        inline bool operator==(const char *string) const noexcept { return strcmp(this->string, string) == 0; }
        inline bool operator==(const symbol &symbol) const noexcept { return strcmp(string, symbol.string) == 0; }

        inline bool operator!=(const char *string) const noexcept { return strcmp(this->string, string) != 0; }
        inline bool operator!=(const symbol &symbol) const noexcept { return strcmp(string, symbol.string) != 0; }
    };

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

            default:
                return nullptr;
        }
    }

    enum platform string_to_platform(const char *platform) noexcept {
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

    enum version string_to_version(const char *version) noexcept {
        if (strcmp(version, "v1") == 0) {
            return version::v1;
        } else if (strcmp(version, "v2") == 0) {
            return version::v2;
        }

        return (enum version)0;
    }

    inline const char *get_parsed_symbol_string(const char *string, bool is_weak, enum symbol::type *type) {
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

    void print_reexports_to_tbd_output(FILE *output, std::vector<reexport> &reexports) {
        if (reexports.empty()) {
            return;
        }

        fprintf(output, "%-4sre-exports:%7s", "", "");

        auto reexports_begin = reexports.begin();
        auto reexports_end = reexports.end();

        auto reexports_begin_string = reexports_begin->string;

        fprintf(output, "[ %s", reexports_begin_string);

        const auto line_length_max = 105;
        auto current_line_length = strlen(reexports_begin_string);

        for (reexports_begin++; reexports_begin < reexports_end; reexports_begin++) {
            const auto &reexport = *reexports_begin;

            const auto reexport_string = reexport.string;
            const auto reexport_string_length = strlen(reexport_string);

            auto new_line_length = reexport_string_length + 2;
            auto new_current_line_length = current_line_length + new_line_length;

            // A line that is printed is allowed to go up to a line_length_max. An exception
            // is made when one reexport-string is longer than line_length_max. When calculating
            // additional line length for a reexport-string, in addition to the reexport-string-length,
            // 2 is added for the comma and the space behind it.

            if (current_line_length >= line_length_max || (new_current_line_length != new_line_length && new_current_line_length > line_length_max)) {
                fprintf(output, ",\n%-24s", "");
                new_current_line_length = new_line_length;
            } else {
                fputs(", ", output);
                new_current_line_length++;
            }

            fputs(reexport_string, output);
            current_line_length = new_current_line_length;
        }

        fputs(" ]\n", output);
    }

    void print_reexports_to_tbd_output(FILE *output, const flags &flags, std::vector<reexport> &reexports) {
        if (reexports.empty()) {
            return;
        }

        fprintf(output, "%-4sre-exports:%7s", "", "");

        auto reexports_begin = reexports.begin();
        auto reexports_end = reexports.end();

        auto reexports_begin_string = reexports_begin->string;

        fprintf(output, "[ %s", reexports_begin_string);

        const auto line_length_max = 105;
        const auto should_check_flags = flags.was_created();

        auto current_line_length = strlen(reexports_begin_string);

        for (reexports_begin++; reexports_begin < reexports_end; reexports_begin++) {
            const auto &reexport = *reexports_begin;
            if (should_check_flags) {
                const auto &reexport_flags = reexport.flags;
                if (reexport_flags != flags) {
                    continue;
                }
            }

            const auto reexport_string = reexport.string;
            const auto reexport_string_length = strlen(reexport_string);

            auto new_line_length = reexport_string_length + 2;
            auto new_current_line_length = current_line_length + new_line_length;

            // A line that is printed is allowed to go up to a line_length_max. An exception
            // is made when one reexport-string is longer than line_length_max. When calculating
            // additional line length for a reexport-string, in addition to the reexport-string-length,
            // 2 is added for the comma and the space behind it.

            if (current_line_length >= line_length_max || (new_current_line_length != new_line_length && new_current_line_length > line_length_max)) {
                fprintf(output, ",\n%-24s", "");
                new_current_line_length = new_line_length;
            } else {
                fputs(", ", output);
                new_current_line_length++;
            }

            fputs(reexport_string, output);
            current_line_length = new_current_line_length;
        }

        fputs(" ]\n", output);
    }

    void print_symbols_to_tbd_output(FILE *output, std::vector<symbol> &symbols, enum symbol::type type) {
        if (symbols.empty()) {
            return;
        }

        // Find the first valid symbol (as determined by flags and symbol-type)
        // as printing a symbol-list requires a special format-string being print
        // for the first string, and a different one from the rest.

        auto symbols_begin = symbols.begin();
        auto symbols_end = symbols.end();

        for (; symbols_begin < symbols_end; symbols_begin++) {
            const auto &symbol = *symbols_begin;
            if (symbol.type != type) {
                continue;
            }

            break;
        }

        // If no valid symbols were found, print_symbols should
        // return immediately.

        if (symbols_begin == symbols_end) {
            return;
        }

        switch (type) {
            case symbol::type::symbols:
                fprintf(output, "%-4ssymbols:%10s", "", "");
                break;

            case symbol::type::weak_symbols:
                fprintf(output, "%-4sweak-def-symbols: ", "");
                break;

            case symbol::type::objc_classes:
                fprintf(output, "%-4sobjc-classes:%5s", "", "");
                break;

            case symbol::type::objc_ivars:
                fprintf(output, "%-4sobjc-ivars:%7s", "", "");
                break;
        }

        const auto line_length_max = 105;
        auto symbols_begin_string = symbols_begin->string;

        fputs("[ ", output);

        const auto symbol_string_begin_needs_quotes = strncmp(symbols_begin_string, "$ld", 3) == 0;
        if (symbol_string_begin_needs_quotes) {
            fputc('\'', output);
        }

        fputs(symbols_begin_string, output);
        if (symbol_string_begin_needs_quotes) {
            fputc('\'', output);
        }

        auto current_line_length = strlen(symbols_begin_string);
        for (symbols_begin++; symbols_begin < symbols_end; symbols_begin++) {
            const auto &symbol = *symbols_begin;
            if (symbol.type != type) {
                continue;
            }

            // If a symbol-string has a dollar sign, quotes must be
            // added around the string.

            const auto symbol_string = symbol.string;
            const auto symbol_string_needs_quotes = strncmp(symbol_string, "$ld", 3) == 0;

            const auto symbol_string_length = strlen(symbol_string);

            auto new_line_length = symbol_string_length + 2;
            if (symbol_string_needs_quotes) {
                new_line_length += 2;
            }

            auto new_current_line_length = current_line_length + new_line_length;

            // A line that is printed is allowed to go up to a line_length_max. When
            // calculating additional line length for a symbol, in addition to the
            // symbol-length, 2 is added for the comma and the space behind it
            // exception is made only when one symbol is longer than line_length_max.

            if (current_line_length >= line_length_max || (new_current_line_length != new_line_length && new_current_line_length > line_length_max)) {
                fprintf(output, ",\n%-24s", "");
                new_current_line_length = new_line_length;
            } else {
                fputs(", ", output);
                new_current_line_length++;
            }

            if (symbol_string_needs_quotes) {
                fputc('\'', output);
            }

            fputs(symbol_string, output);

            if (symbol_string_needs_quotes) {
                fputc('\'', output);
            }

            current_line_length = new_current_line_length;
        }

        fputs(" ]\n", output);
    }

    void print_symbols_to_tbd_output(FILE *output, const flags &flags, std::vector<symbol> &symbols, enum symbol::type type) {
        if (symbols.empty()) {
            return;
        }

        // Find the first valid symbol (as determined by flags and symbol-type)
        // as printing a symbol-list requires a special format-string being print
        // for the first string, and a different one from the rest.

        auto symbols_begin = symbols.begin();
        auto symbols_end = symbols.end();

        const auto should_check_flags = flags.was_created();

        for (; symbols_begin < symbols_end; symbols_begin++) {
            const auto &symbol = *symbols_begin;
            if (symbol.type != type) {
                continue;
            }

            if (should_check_flags) {
                const auto &symbol_flags = symbol.flags;
                if (symbol_flags != flags) {
                    continue;
                }
            }

            break;
        }

        // If no valid symbols were found, print_symbols should
        // return immediately.

        if (symbols_begin == symbols_end) {
            return;
        }

        switch (type) {
            case symbol::type::symbols:
                fprintf(output, "%-4ssymbols:%10s", "", "");
                break;

            case symbol::type::weak_symbols:
                fprintf(output, "%-4sweak-def-symbols: ", "");
                break;

            case symbol::type::objc_classes:
                fprintf(output, "%-4sobjc-classes:%5s", "", "");
                break;

            case symbol::type::objc_ivars:
                fprintf(output, "%-4sobjc-ivars:%7s", "", "");
                break;
        }

        const auto line_length_max = 105;
        auto symbols_begin_string = symbols_begin->string;

        fputs("[ ", output);

        const auto symbol_string_begin_needs_quotes = strncmp(symbols_begin_string, "$ld", 3) == 0;
        if (symbol_string_begin_needs_quotes) {
            fputc('\'', output);
        }

        fputs(symbols_begin_string, output);
        if (symbol_string_begin_needs_quotes) {
            fputc('\'', output);
        }

        auto current_line_length = strlen(symbols_begin_string);
        for (symbols_begin++; symbols_begin < symbols_end; symbols_begin++) {
            const auto &symbol = *symbols_begin;
            if (symbol.type != type) {
                continue;
            }

            if (should_check_flags) {
                const auto &symbol_flags = symbol.flags;
                if (symbol_flags != flags) {
                    continue;
                }
            }

            // If a symbol-string has a dollar sign, quotes must be
            // added around the string.

            const auto symbol_string = symbol.string;
            const auto symbol_string_needs_quotes = strncmp(symbol_string, "$ld", 3) == 0;

            const auto symbol_string_length = strlen(symbol_string);

            auto new_line_length = symbol_string_length + 2;
            if (symbol_string_needs_quotes) {
                new_line_length += 2;
            }

            auto new_current_line_length = current_line_length + new_line_length;

            // A line that is printed is allowed to go up to a line_length_max. When
            // calculating additional line length for a symbol, in addition to the
            // symbol-length, 2 is added for the comma and the space behind it
            // exception is made only when one symbol is longer than line_length_max.

            if (current_line_length >= line_length_max || (new_current_line_length != new_line_length && new_current_line_length > line_length_max)) {
                fprintf(output, ",\n%-24s", "");
                new_current_line_length = new_line_length;
            } else {
                fputs(", ", output);
                new_current_line_length++;
            }

            if (symbol_string_needs_quotes) {
                fputc('\'', output);
            }

            fputs(symbol_string, output);

            if (symbol_string_needs_quotes) {
                fputc('\'', output);
            }

            current_line_length = new_current_line_length;
        }

        fputs(" ]\n", output);
    }

    creation_result create_from_macho_library(macho::file &library, FILE *output, uint64_t options, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides) {
        const auto has_provided_architecture = architectures != 0;
        const auto has_architecture_overrides = architecture_overrides != 0;

        uint32_t library_current_version = -1;
        uint32_t library_compatibility_version = -1;
        uint32_t library_swift_version = 0;

        const char *library_installation_name = nullptr;

        auto library_reexports = std::vector<reexport>();
        auto library_symbols = std::vector<symbol>();

        auto &library_containers = library.containers;
        auto library_containers_index = 0;

        auto library_uuids = std::vector<const uint8_t *>();
        const auto library_containers_size = library_containers.size();

        library_uuids.reserve(library_containers_size);

        auto library_container_architectures = std::vector<const macho::architecture_info *>();
        if (!has_architecture_overrides) {
            library_container_architectures.reserve(library_containers_size);
        }

        auto has_found_architectures_provided = false;

        for (auto &library_container : library_containers) {
            const auto &library_container_header = library_container.header;

            const auto library_container_header_cputype = macho::cputype(library_container_header.cputype);
            const auto library_container_header_subtype = macho::subtype_from_cputype(library_container_header_cputype, library_container_header.cpusubtype);

            if (library_container_header_subtype == macho::subtype::none) {
                return creation_result::invalid_subtype;
            }

            const auto library_container_architecture_info = macho::architecture_info_from_cputype(library_container_header_cputype, library_container_header_subtype);
            if (!library_container_architecture_info) {
                return creation_result::invalid_cputype;
            }

            if (has_provided_architecture) {
                // any is the first architecture info and if set is stored in the LSB
                if (!(architectures & 1)) {
                    const auto architecture_info_table = macho::get_architecture_info_table();
                    const auto architecture_info_table_index = ((uint64_t)library_container_architecture_info - (uint64_t)architecture_info_table) / sizeof(macho::architecture_info);

                    if (!(architectures & ((uint64_t)1 << architecture_info_table_index))) {
                        continue;
                    }
                }

                has_found_architectures_provided = true;
            }

            if (!has_architecture_overrides) {
                library_container_architectures.emplace_back(library_container_architecture_info);
            }

            uint32_t local_current_version = -1;
            uint32_t local_compatibility_version = -1;
            uint32_t local_swift_version = 0;

            const char *local_installation_name = nullptr;
            auto failure_result = creation_result::ok;

            const auto library_container_is_big_endian = library_container.is_big_endian();
            const auto should_find_library_platform = platform == platform::none;

            const auto library_container_base = library_container.base;
            const auto library_container_size = library_container.size;

            auto library_container_stream = library_container.stream;
            auto library_container_load_command_iteration_result = library_container.iterate_load_commands([&](const macho::load_command *swapped, const macho::load_command *load_command) {
                switch (swapped->cmd) {
                    case macho::load_commands::build_version: {
                        if (!should_find_library_platform) {
                            break;
                        }

                        const auto build_version_command = (macho::build_version_command *)load_command;
                        auto build_version_platform = build_version_command->platform;

                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&build_version_platform);
                        }

                        auto build_version_parsed_platform = platform::none;
                        switch (macho::build_version::platform(build_version_platform)) {
                            case macho::build_version::platform::macos:
                                build_version_parsed_platform = platform::macosx;
                                break;

                            case macho::build_version::platform::ios:
                                build_version_parsed_platform = platform::ios;
                                break;

                            case macho::build_version::platform::tvos:
                                build_version_parsed_platform = platform::tvos;
                                break;

                            case macho::build_version::platform::watchos:
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

                    case macho::load_commands::identification_dylib: {
                        auto identification_dylib_command = (macho::dylib_command *)load_command;
                        auto identification_dylib_installation_name_string_index = identification_dylib_command->name.offset;

                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&identification_dylib_installation_name_string_index);
                        }

                        if (identification_dylib_installation_name_string_index >= swapped->cmdsize) {
                            failure_result = creation_result::invalid_load_command;
                            return false;
                        }

                        const auto &identification_dylib_installation_name_string = &((char *)identification_dylib_command)[identification_dylib_installation_name_string_index];

                        if (local_installation_name != nullptr) {
                            if (strcmp(local_installation_name, identification_dylib_installation_name_string) != 0) {
                                failure_result = creation_result::contradictary_load_command_information;
                                return false;
                            }
                        }

                        local_installation_name = identification_dylib_installation_name_string;

                        auto identification_dylib_current_version = identification_dylib_command->current_version;
                        auto identification_dylib_compatibility_version = identification_dylib_command->compatibility_version;

                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&identification_dylib_current_version);
                            macho::swap_uint32(&identification_dylib_compatibility_version);
                        }

                        if (local_current_version != -1) {
                            if (local_current_version != identification_dylib_current_version) {
                                failure_result = creation_result::contradictary_load_command_information;
                                return false;
                            }
                        }

                        if (local_compatibility_version != -1) {
                            if (local_compatibility_version != identification_dylib_compatibility_version) {
                                failure_result = creation_result::contradictary_load_command_information;
                                return false;
                            }
                        }

                        local_current_version = identification_dylib_current_version;
                        local_compatibility_version = identification_dylib_compatibility_version;

                        break;
                    }

                    case macho::load_commands::reexport_dylib: {
                        auto reexport_dylib_command = (macho::dylib_command *)load_command;
                        if (library_container_is_big_endian) {
                            macho::swap_dylib_command(reexport_dylib_command);
                        }

                        auto reexport_dylib_string_index = reexport_dylib_command->name.offset;
                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&reexport_dylib_string_index);
                        }

                        if (reexport_dylib_string_index >= swapped->cmdsize) {
                            failure_result = creation_result::invalid_load_command;
                            return false;
                        }

                        const auto &reexport_dylib_string = &((char *)reexport_dylib_command)[reexport_dylib_string_index];
                        const auto library_reexports_iter = std::find(library_reexports.begin(), library_reexports.end(), reexport_dylib_string);

                        if (library_reexports_iter != library_reexports.end()) {
                            library_reexports_iter->add_architecture(library_containers_index);
                        } else {
                            auto &library_reexport = library_reexports.emplace_back(reexport_dylib_string, library_containers_size);
                            library_reexport.add_architecture(library_containers_index);
                        }

                        break;
                    }

                    case macho::load_commands::segment: {
                        const auto segment_command = (macho::segment_command *)load_command;
                        if (strcmp(segment_command->segname, "__DATA") != 0 && strcmp(segment_command->segname, "__DATA_CONST") != 0 && strcmp(segment_command->segname, "__DATA_DIRTY") != 0) {
                            break;
                        }

                        const auto library_container_is_64_bit = library_container.is_64_bit();
                        if (library_container_is_64_bit) {
                            break;
                        }

                        auto segment_sections_count = segment_command->nsects;
                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&segment_sections_count);
                        }

                        auto segment_section = (macho::segments::section *)((uint64_t)segment_command + sizeof(macho::segment_command));

                        auto objc_image_info = objc::image_info();
                        auto found_objc_image_info = false;

                        while (segment_sections_count != 0) {
                            if (strncmp(segment_section->sectname, "__objc_imageinfo", 16) != 0) {
                                segment_section = (macho::segments::section *)((uint64_t)segment_section + sizeof(macho::segments::section));
                                segment_sections_count--;

                                continue;
                            }

                            auto segment_section_data_offset = segment_section->offset;
                            if (library_container_is_big_endian) {
                                macho::swap_uint32(&segment_section_data_offset);
                            }

                            if (segment_section_data_offset >= library_container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            auto segment_section_data_size = segment_section->size;
                            if (library_container_is_big_endian) {
                                macho::swap_uint32(&segment_section_data_offset);
                            }

                            if (segment_section_data_size >= library_container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            const auto segment_section_data_end = segment_section_data_offset + segment_section_data_size;
                            if (segment_section_data_end >= library_container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            const auto library_container_stream_position = ftell(library_container_stream);

                            fseek(library_container_stream, library_container_base + segment_section_data_offset, SEEK_SET);
                            fread(&objc_image_info, sizeof(objc_image_info), 1, library_container_stream);

                            fseek(library_container_stream, library_container_stream_position, SEEK_SET);

                            found_objc_image_info = true;
                            break;
                        }

                        if (!found_objc_image_info) {
                            break;
                        }

                        const auto &objc_image_info_flags = objc_image_info.flags;
                        auto objc_image_info_flags_swift_version = (objc_image_info_flags >> objc::image_info::flags::swift_version_shift) & objc::image_info::flags::swift_version_mask;

                        if (local_swift_version != 0) {
                            if (objc_image_info_flags_swift_version != local_swift_version) {
                                failure_result = creation_result::contradictary_load_command_information;
                                return false;
                            }
                        } else {
                            local_swift_version = objc_image_info_flags_swift_version;
                        }

                        break;
                    }

                    case macho::load_commands::segment_64: {
                        const auto segment_command = (macho::segment_command_64 *)load_command;
                        if (strcmp(segment_command->segname, "__DATA") != 0 && strcmp(segment_command->segname, "__DATA_CONST") != 0 && strcmp(segment_command->segname, "__DATA_DIRTY") != 0) {
                            break;
                        }

                        const auto library_container_is_32_bit = library_container.is_32_bit();
                        if (library_container_is_32_bit) {
                            break;
                        }

                        auto segment_sections_count = segment_command->nsects;
                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&segment_sections_count);
                        }

                        auto segment_section = (macho::segments::section_64 *)((uint64_t)segment_command + sizeof(macho::segment_command_64));

                        auto objc_image_info = objc::image_info();
                        auto found_objc_image_info = false;

                        while (segment_sections_count != 0) {
                            if (strncmp(segment_section->sectname, "__objc_imageinfo", 16) != 0) {
                                segment_section = (macho::segments::section_64 *)((uint64_t)segment_section + sizeof(macho::segments::section_64));
                                segment_sections_count--;

                                continue;
                            }

                            auto segment_section_data_offset = segment_section->offset;
                            if (library_container_is_big_endian) {
                                macho::swap_uint32(&segment_section_data_offset);
                            }

                            if (segment_section_data_offset >= library_container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            auto segment_section_data_size = segment_section->size;
                            if (library_container_is_big_endian) {
                                macho::swap_uint32(&segment_section_data_offset);
                            }

                            if (segment_section_data_size >= library_container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            const auto segment_section_data_end = segment_section_data_offset + segment_section_data_size;
                            if (segment_section_data_end >= library_container_size) {
                                failure_result = creation_result::invalid_segment;
                                return false;
                            }

                            const auto library_container_stream_position = ftell(library_container_stream);

                            fseek(library_container_stream, library_container_base + segment_section_data_offset, SEEK_SET);
                            fread(&objc_image_info, sizeof(objc_image_info), 1, library_container_stream);

                            fseek(library_container_stream, library_container_stream_position, SEEK_SET);

                            found_objc_image_info = true;
                            break;
                        }

                        if (!found_objc_image_info) {
                            break;
                        }

                        const auto &objc_image_info_flags = objc_image_info.flags;
                        auto objc_image_info_flags_swift_version = (objc_image_info_flags >> objc::image_info::flags::swift_version_shift) & objc::image_info::flags::swift_version_mask;

                        if (local_swift_version != 0) {
                            if (objc_image_info_flags_swift_version != local_swift_version) {
                                failure_result = creation_result::contradictary_load_command_information;
                                return false;
                            }
                        } else {
                            local_swift_version = objc_image_info_flags_swift_version;
                        }

                        break;
                    }

                    case macho::load_commands::uuid: {
                        if (version != version::v2) {
                            break;
                        }

                        const auto &library_uuid = ((macho::uuid_command *)load_command)->uuid;
                        const auto library_uuids_size = library_uuids.size();

                        // Check if multiple uuid load-commands were
                        // found in the same container

                        if (library_containers_index != library_uuids_size) {
                            const auto &library_uuids_back = library_uuids.back();
                            if (memcmp(library_uuids_back, library_uuid, 16) != 0) {
                                failure_result = creation_result::contradictary_load_command_information;
                                return false;
                            }

                            return true;
                        }

                        const auto library_uuids_iter = std::find_if(library_uuids.begin(), library_uuids.end(), [&](const uint8_t *rhs) {
                            return memcmp(&library_uuid, rhs, 16) == 0;
                        });

                        const auto &library_uuids_end = library_uuids.end();
                        if (library_uuids_iter != library_uuids_end) {
                            failure_result = creation_result::uuid_is_not_unique;
                            return false;
                        }

                        library_uuids.emplace_back((uint8_t *)&library_uuid);
                        break;
                    }

                    case macho::load_commands::version_min_macosx: {
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

                    case macho::load_commands::version_min_iphoneos: {
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

                    case macho::load_commands::version_min_watchos: {
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

                    case macho::load_commands::version_min_tvos: {
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

            switch (library_container_load_command_iteration_result) {
                case macho::container::load_command_iteration_result::ok:
                    break;

                case macho::container::load_command_iteration_result::stream_seek_error:
                case macho::container::load_command_iteration_result::stream_read_error:
                case macho::container::load_command_iteration_result::load_command_is_too_small:
                case macho::container::load_command_iteration_result::load_command_is_too_large:
                    return creation_result::failed_to_iterate_load_commands;
            }

            if (failure_result != creation_result::ok) {
                return failure_result;
            }

            if (platform == platform::none) {
                return creation_result::platform_not_found;
            }

            if (local_current_version == -1 || local_compatibility_version == -1 || !local_installation_name) {
                return creation_result::not_a_library;
            }

            if (library_installation_name != nullptr) {
                if (strcmp(library_installation_name, local_installation_name) != 0) {
                    return creation_result::contradictary_container_information;
                }
            } else {
                library_installation_name = local_installation_name;
            }

            if (library_current_version != -1) {
                if (local_current_version != library_current_version) {
                    return creation_result::contradictary_container_information;
                }
            } else {
                library_current_version = local_current_version;
            }

            if (library_compatibility_version != -1) {
                if (local_compatibility_version != library_compatibility_version) {
                    return creation_result::contradictary_container_information;
                }
            } else {
                library_compatibility_version = local_compatibility_version;
            }

            if (library_swift_version != 0) {
                if (local_swift_version != library_swift_version) {
                    return creation_result::contradictary_container_information;
                }
            } else {
                library_swift_version = local_swift_version;
            }

            auto library_container_symbols_iteration_result = library_container.iterate_symbols([&](const macho::nlist_64 &symbol_table_entry, const char *symbol_string) {
                const auto &symbol_table_entry_type = symbol_table_entry.n_type;
                if ((symbol_table_entry_type & macho::symbol_table::flags::type) != macho::symbol_table::type::section) {
                    return true;
                }

                enum symbol::type symbol_type;

                const auto symbol_is_weak = symbol_table_entry.n_desc & macho::symbol_table::description::weak_definition;
                const auto parsed_symbol_string = get_parsed_symbol_string(symbol_string, symbol_is_weak, &symbol_type);

                if (!(options & symbol_options::allow_all_private_symbols)) {
                    switch (symbol_type) {
                        case symbol::type::symbols:
                            if (!(options & symbol_options::allow_private_normal_symbols)) {
                                if (!(symbol_table_entry_type & macho::symbol_table::flags::external)) {
                                    return true;
                                }
                            }

                            break;

                        case symbol::type::weak_symbols:
                            if (!(options & symbol_options::allow_private_weak_symbols)) {
                                if (!(symbol_table_entry_type & macho::symbol_table::flags::external)) {
                                    return true;
                                }
                            }

                            break;

                        case symbol::type::objc_classes:
                            if (!(options & symbol_options::allow_private_objc_symbols)) {
                                if (!(options & symbol_options::allow_private_objc_classes)) {
                                    if (!(symbol_table_entry_type & macho::symbol_table::flags::external)) {
                                        return true;
                                    }
                                }
                            }

                            break;

                        case symbol::type::objc_ivars:
                            if (!(options & symbol_options::allow_private_objc_symbols)) {
                                if (!(options & symbol_options::allow_private_objc_ivars)) {
                                    if (!(symbol_table_entry_type & macho::symbol_table::flags::external)) {
                                        return true;
                                    }
                                }
                            }

                            break;

                        default:
                            break;
                    }
                }

                const auto symbols_iter = std::find(library_symbols.begin(), library_symbols.end(), parsed_symbol_string);

                if (symbols_iter != library_symbols.end()) {
                    symbols_iter->add_architecture(library_containers_index);
                } else {
                    auto &library_symbol = library_symbols.emplace_back(parsed_symbol_string, symbol_is_weak, library_containers_size, symbol_type);
                    library_symbol.add_architecture(library_containers_index);
                }

                return true;
            });

            switch (library_container_symbols_iteration_result) {
                case macho::container::symbols_iteration_result::ok:
                case macho::container::symbols_iteration_result::no_symbols:
                case macho::container::symbols_iteration_result::no_symbol_table_load_command:
                    break;

                case macho::container::symbols_iteration_result::stream_seek_error:
                case macho::container::symbols_iteration_result::stream_read_error:
                case macho::container::symbols_iteration_result::invalid_string_table:
                case macho::container::symbols_iteration_result::invalid_symbol_table:
                case macho::container::symbols_iteration_result::invalid_symbol_table_entry:
                    return creation_result::failed_to_iterate_symbols;
            }

            library_containers_index++;
        }

        if (has_provided_architecture) {
            if (!has_found_architectures_provided) {
                return creation_result::no_provided_architectures;
            }
        }

        if (library_reexports.empty() && library_symbols.empty()) {
            return creation_result::no_symbols_or_reexports;
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
        if (has_architecture_overrides) {
            groups.emplace_back();
        } else {
            for (const auto &library_reexport : library_reexports) {
                const auto &library_reexport_flags = library_reexport.flags;
                const auto group_iter = std::find_if(groups.begin(), groups.end(), [&](const group &lhs) {
                    return lhs.flags == library_reexport_flags;
                });

                if (group_iter != groups.end()) {
                    group_iter->reexports_count++;
                } else {
                    auto &group = groups.emplace_back(library_reexport_flags);
                    group.reexports_count++;
                }
            }

            for (const auto &library_symbol : library_symbols) {
                const auto &library_symbol_flags = library_symbol.flags;
                const auto group_iter = std::find_if(groups.begin(), groups.end(), [&](const group &lhs) {
                    return lhs.flags == library_symbol_flags;
                });

                if (group_iter != groups.end()) {
                    group_iter->symbols_count++;
                } else {
                    auto &group = groups.emplace_back(library_symbol_flags);
                    group.symbols_count++;
                }
            }

            std::sort(groups.begin(), groups.end(), [](const group &lhs, const group &rhs) {
                const auto lhs_symbols_count = lhs.symbols_count;
                const auto rhs_symbols_count = rhs.symbols_count;

                return lhs_symbols_count < rhs_symbols_count;
            });
        }

        fputs("---", output);
        if (version == version::v2) {
            fputs(" !tapi-tbd-v2", output);
        }

        fprintf(output, "\narchs:%-17s[ ", "");

        if (has_architecture_overrides) {
            const auto architecture_info_table = macho::get_architecture_info_table();
            const auto architecture_info_table_size = macho::get_architecture_info_table_size();

            auto index = uint64_t();
            for (; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & ((uint64_t)1 << index))) {
                    continue;
                }

                fputs(architecture_info_table[index].name, output);
                break;
            }

            for (index++; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & ((uint64_t)1 << index))) {
                    continue;
                }

                fprintf(output, ", %s", architecture_info_table[index].name);
            }

            fputs(" ]\n", output);
        } else if (has_provided_architecture) {
            const auto architecture_info_table = macho::get_architecture_info_table();
            const auto architecture_info_table_size = macho::get_architecture_info_table_size();

            auto index = uint64_t();
            for (; index < architecture_info_table_size; index++) {
                if (!(architectures & ((uint64_t)1 << index))) {
                    continue;
                }

                fputs(architecture_info_table[index].name, output);
                break;
            }

            for (index++; index < architecture_info_table_size; index++) {
                if (!(architectures & ((uint64_t)1 << index))) {
                    continue;
                }

                fprintf(output, ", %s", architecture_info_table[index].name);
            }

            fputs(" ]\n", output);
        } else {
            const auto library_container_architectures_begin = library_container_architectures.begin();
            const auto library_container_architectures_begin_arch_info = *library_container_architectures_begin;

            fputs(library_container_architectures_begin_arch_info->name, output);

            const auto library_container_architectures_end = library_container_architectures.end();
            for (auto library_container_architectures_iter = library_container_architectures_begin + 1; library_container_architectures_iter != library_container_architectures_end; library_container_architectures_iter++) {
                const auto library_container_architecture_arch_info = *library_container_architectures_iter;
                const auto library_container_architecture_arch_info_name = library_container_architecture_arch_info->name;

                fprintf(output, ", %s", library_container_architecture_arch_info_name);
            }

            fputs(" ]\n", output);
        }

        if (version == version::v2) {
            if (!has_architecture_overrides) {
                const auto library_uuids_size = library_uuids.size();
                if (library_uuids_size) {
                    fprintf(output, "uuids:%-17s[ ", "");

                    auto library_container_architectures_iter = library_container_architectures.begin();
                    for (auto library_uuids_index = 0; library_uuids_index < library_uuids_size; library_uuids_index++, library_container_architectures_iter++) {
                        const auto &library_container_architecture_arch_info = *library_container_architectures_iter;
                        const auto &library_uuid = library_uuids.at(library_uuids_index);

                        fprintf(output, "'%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X'", library_container_architecture_arch_info->name, library_uuid[0], library_uuid[1], library_uuid[2], library_uuid[3], library_uuid[4], library_uuid[5], library_uuid[6], library_uuid[7], library_uuid[8], library_uuid[9], library_uuid[10], library_uuid[11], library_uuid[12], library_uuid[13], library_uuid[14], library_uuid[15]);

                        if (library_uuids_index != library_uuids_size - 1) {
                            fputs(", ", output);

                            if (library_uuids_index % 2 != 0) {
                                fprintf(output, "%-26s", "\n");
                            }
                        }
                    }

                    fputs(" ]\n", output);
                }
            }
        }

        fprintf(output, "platform:%-14s%s\n", "", platform_to_string(platform));
        fprintf(output, "install-name:%-10s%s\n", "", library_installation_name);

        auto library_current_version_major = library_current_version >> 16;
        auto library_current_version_minor = (library_current_version >> 8) & 0xff;
        auto library_current_version_revision = library_current_version & 0xff;

        fprintf(output, "current-version:%-7s%u", "", library_current_version_major);

        if (library_current_version_minor != 0) {
            fprintf(output, ".%u", library_current_version_minor);
        }

        if (library_current_version_revision != 0) {
            if (library_current_version_minor == 0) {
                fputs(".0", output);
            }

            fprintf(output, ".%u", library_current_version_revision);
        }

        auto library_compatibility_version_major = library_compatibility_version >> 16;
        auto library_compatibility_version_minor = (library_compatibility_version >> 8) & 0xff;
        auto library_compatibility_version_revision = library_compatibility_version & 0xff;

        fprintf(output, "\ncompatibility-version: %u", library_compatibility_version_major);

        if (library_compatibility_version_minor != 0) {
            fprintf(output, ".%u", library_compatibility_version_minor);
        }

        if (library_compatibility_version_revision != 0) {
            if (library_compatibility_version_minor == 0) {
                fputs(".0", output);
            }

            fprintf(output, ".%u", library_compatibility_version_revision);
        }

        fputc('\n', output);

        if (library_swift_version != 0) {
            switch (library_swift_version) {
                case 1:
                    fprintf(output, "swift-version:%-9s1\n", "");
                    break;

                case 2:
                    fprintf(output, "swift-version:%-9s1.2\n", "");
                    break;

                default:
                    fprintf(output, "swift-version:%-9s%u\n", "", library_swift_version - 1);
                    break;
            }
        }

        fputs("exports:\n", output);

        if (has_architecture_overrides) {
            const auto architecture_info_table = macho::get_architecture_info_table();
            const auto architecture_info_table_size = macho::get_architecture_info_table_size();

            auto index = uint64_t();
            for (; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & ((uint64_t)1 << index))) {
                    continue;
                }

                fprintf(output, "  - archs:%-12s[ %s", "", architecture_info_table[index].name);
                break;
            }

            for (index++; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & ((uint64_t)1 << index))) {
                    continue;
                }

                fprintf(output, ", %s", architecture_info_table[index].name);
            }

            fputs(" ]\n", output);

            const auto &group = groups.front();
            const auto &group_flags = group.flags;

            print_reexports_to_tbd_output(output, group_flags, library_reexports);

            print_symbols_to_tbd_output(output, group_flags, library_symbols, symbol::type::symbols);
            print_symbols_to_tbd_output(output, group_flags, library_symbols, symbol::type::weak_symbols);
            print_symbols_to_tbd_output(output, group_flags, library_symbols, symbol::type::objc_classes);
            print_symbols_to_tbd_output(output, group_flags, library_symbols, symbol::type::objc_ivars);

        } else {
            for (auto &group : groups) {
                const auto &group_flags = group.flags;

                auto library_container_architectures_index = 0;
                const auto library_container_architectures_size = library_container_architectures.size();

                for (; library_container_architectures_index < library_container_architectures_size; library_container_architectures_index++) {
                    const auto architecture = group_flags.at(library_container_architectures_index);
                    if (!architecture) {
                        continue;
                    }

                    break;
                }

                if (library_container_architectures_index != library_container_architectures_size) {
                    const auto &library_container_architecture_info = library_container_architectures.at(library_container_architectures_index);
                    const auto &library_container_architecture_info_name = library_container_architecture_info->name;

                    fprintf(output, "  - archs:%-12s[ %s", "", library_container_architecture_info_name);

                    for (library_container_architectures_index++; library_container_architectures_index < library_container_architectures_size; library_container_architectures_index++) {
                        const auto architecture = group_flags.at(library_container_architectures_index);
                        if (!architecture) {
                            continue;
                        }

                        const auto &library_container_architecture_info = library_container_architectures.at(library_container_architectures_index);
                        const auto &library_container_architecture_info_name = library_container_architecture_info->name;

                        fprintf(output, ", %s", library_container_architecture_info_name);
                    }

                    fputs(" ]\n", output);
                }

                print_reexports_to_tbd_output(output, group_flags, library_reexports);

                print_symbols_to_tbd_output(output, group_flags, library_symbols, symbol::type::symbols);
                print_symbols_to_tbd_output(output, group_flags, library_symbols, symbol::type::weak_symbols);
                print_symbols_to_tbd_output(output, group_flags, library_symbols, symbol::type::objc_classes);
                print_symbols_to_tbd_output(output, group_flags, library_symbols, symbol::type::objc_ivars);
            }
        }

        fputs("...\n", output);
        return creation_result::ok;
    }

    creation_result create_from_macho_library(macho::container &library, FILE *output, uint64_t options, platform platform, version version, uint64_t architectures, uint64_t architecture_overrides) {
        const auto has_provided_architecture = architectures != 0;
        const auto has_architecture_overrides = architecture_overrides != 0;

        uint32_t current_version = -1;
        uint32_t compatibility_version = -1;
        uint32_t swift_version = 0;

        const char *installation_name = nullptr;

        auto reexports = std::vector<reexport>();
        auto symbols = std::vector<symbol>();

        auto uuid = (uint8_t *)nullptr;

        const auto &header = library.header;

        const auto header_cputype = macho::cputype(header.cputype);
        const auto header_subtype = macho::subtype_from_cputype(header_cputype, header.cpusubtype);

        if (header_subtype == macho::subtype::none) {
            return creation_result::invalid_subtype;
        }

        const auto architecture_info = macho::architecture_info_from_cputype(header_cputype, header_subtype);
        if (!architecture_info) {
            return creation_result::invalid_cputype;
        }

        if (has_provided_architecture) {
            // any is the first architecture info and if set is stored in the LSB
            if (!(architectures & 1)) {
                const auto architecture_info_table = macho::get_architecture_info_table();
                const auto architecture_info_table_index = ((uint64_t)architecture_info - (uint64_t)architecture_info_table) / sizeof(macho::architecture_info);

                if (!(architectures & ((uint64_t)1 << architecture_info_table_index))) {
                    return creation_result::no_provided_architectures;
                }
            }
        }

        const auto library_container_is_big_endian = library.is_big_endian();
        const auto should_find_library_platform = platform == platform::none;

        const auto library_container_base = library.base;
        const auto library_container_size = library.size;

        auto library_container_stream = library.stream;
        auto failure_result = creation_result::ok;

        auto library_container_load_command_iteration_result = library.iterate_load_commands([&](long location, const macho::load_command *swapped, const macho::load_command *load_command) {
            switch (swapped->cmd) {
                case macho::load_commands::build_version: {
                    if (!should_find_library_platform) {
                        break;
                    }

                    const auto build_version_command = (macho::build_version_command *)load_command;
                    auto build_version_platform = build_version_command->platform;

                    if (library_container_is_big_endian) {
                        macho::swap_uint32(&build_version_platform);
                    }

                    auto build_version_parsed_platform = platform::none;
                    switch (macho::build_version::platform(build_version_platform)) {
                        case macho::build_version::platform::macos:
                            build_version_parsed_platform = platform::macosx;
                            break;

                        case macho::build_version::platform::ios:
                            build_version_parsed_platform = platform::ios;
                            break;

                        case macho::build_version::platform::tvos:
                            build_version_parsed_platform = platform::tvos;
                            break;

                        case macho::build_version::platform::watchos:
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

                case macho::load_commands::identification_dylib: {
                    auto identification_dylib_command = (macho::dylib_command *)load_command;
                    auto identification_dylib_installation_name_string_index = identification_dylib_command->name.offset;

                    if (library_container_is_big_endian) {
                        macho::swap_uint32(&identification_dylib_installation_name_string_index);
                    }

                    if (identification_dylib_installation_name_string_index >= swapped->cmdsize) {
                        failure_result = creation_result::invalid_load_command;
                        return false;
                    }

                    const auto &identification_dylib_installation_name_string = &((char *)identification_dylib_command)[identification_dylib_installation_name_string_index];

                    if (installation_name != nullptr) {
                        if (strcmp(installation_name, identification_dylib_installation_name_string) != 0) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }
                    } else {
                        installation_name = identification_dylib_installation_name_string;
                    }

                    auto identification_dylib_current_version = identification_dylib_command->current_version;
                    auto identification_dylib_compatibility_version = identification_dylib_command->compatibility_version;

                    if (library_container_is_big_endian) {
                        macho::swap_uint32(&identification_dylib_current_version);
                        macho::swap_uint32(&identification_dylib_compatibility_version);
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

                case macho::load_commands::reexport_dylib: {
                    auto reexport_dylib_command = (macho::dylib_command *)load_command;
                    if (library_container_is_big_endian) {
                        macho::swap_dylib_command(reexport_dylib_command);
                    }

                    auto reexport_dylib_string_index = reexport_dylib_command->name.offset;
                    if (library_container_is_big_endian) {
                        macho::swap_uint32(&reexport_dylib_string_index);
                    }

                    if (reexport_dylib_string_index >= swapped->cmdsize) {
                        failure_result = creation_result::invalid_load_command;
                        return false;
                    }

                    const auto &reexport_dylib_string = &((char *)reexport_dylib_command)[reexport_dylib_string_index];
                    const auto reexport_iter = std::find(reexports.begin(), reexports.end(), reexport_dylib_string);

                    if (reexport_iter == reexports.end()) {
                        // Only add unique reexports to reexports
                        // vector

                        reexports.emplace_back(reexport_dylib_string, 0);
                    }

                    break;
                }

                case macho::load_commands::segment: {
                    const auto segment_command = (macho::segment_command *)load_command;
                    if (strcmp(segment_command->segname, "__DATA") != 0 && strcmp(segment_command->segname, "__DATA_CONST") != 0 && strcmp(segment_command->segname, "__DATA_DIRTY") != 0) {
                        break;
                    }

                    const auto library_container_is_64_bit = library.is_64_bit();
                    if (library_container_is_64_bit) {
                        break;
                    }

                    auto segment_sections_count = segment_command->nsects;
                    if (library_container_is_big_endian) {
                        macho::swap_uint32(&segment_sections_count);
                    }

                    auto segment_section = (macho::segments::section *)((uint64_t)segment_command + sizeof(macho::segment_command));

                    auto objc_image_info = objc::image_info();
                    auto found_objc_image_info = false;

                    while (segment_sections_count != 0) {
                        if (strncmp(segment_section->sectname, "__objc_imageinfo", 16) != 0) {
                            segment_section = (macho::segments::section *)((uint64_t)segment_section + sizeof(macho::segments::section));
                            segment_sections_count--;

                            continue;
                        }

                        auto segment_section_data_offset = segment_section->offset;
                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&segment_section_data_offset);
                        }

                        if (segment_section_data_offset >= library_container_size) {
                            failure_result = creation_result::invalid_segment;
                            return false;
                        }

                        auto segment_section_data_size = segment_section->size;
                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&segment_section_data_offset);
                        }

                        if (segment_section_data_size >= library_container_size) {
                            failure_result = creation_result::invalid_segment;
                            return false;
                        }

                        const auto segment_section_data_end = segment_section_data_offset + segment_section_data_size;
                        if (segment_section_data_end >= library_container_size) {
                            failure_result = creation_result::invalid_segment;
                            return false;
                        }

                        const auto library_container_stream_position = ftell(library_container_stream);

                        fseek(library_container_stream, library_container_base + segment_section_data_offset, SEEK_SET);
                        fread(&objc_image_info, sizeof(objc_image_info), 1, library_container_stream);

                        fseek(library_container_stream, library_container_stream_position, SEEK_SET);

                        found_objc_image_info = true;
                        break;
                    }

                    if (!found_objc_image_info) {
                        break;
                    }

                    const auto &objc_image_info_flags = objc_image_info.flags;
                    auto objc_image_info_flags_swift_version = (objc_image_info_flags >> objc::image_info::flags::swift_version_shift) & objc::image_info::flags::swift_version_mask;

                    if (swift_version != 0) {
                        if (swift_version != objc_image_info_flags_swift_version) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }
                    } else {
                        swift_version = objc_image_info_flags_swift_version;
                    }

                    break;
                }

                case macho::load_commands::segment_64: {
                    const auto segment_command = (macho::segment_command_64 *)load_command;
                    if (strcmp(segment_command->segname, "__DATA") != 0 && strcmp(segment_command->segname, "__DATA_CONST") != 0 && strcmp(segment_command->segname, "__DATA_DIRTY") != 0) {
                        break;
                    }

                    const auto library_container_is_32_bit = library.is_32_bit();
                    if (library_container_is_32_bit) {
                        break;
                    }

                    auto segment_sections_count = segment_command->nsects;
                    if (library_container_is_big_endian) {
                        macho::swap_uint32(&segment_sections_count);
                    }

                    auto segment_section = (macho::segments::section_64 *)((uint64_t)segment_command + sizeof(macho::segment_command_64));

                    auto objc_image_info = objc::image_info();
                    auto found_objc_image_info = false;

                    while (segment_sections_count != 0) {
                        if (strncmp(segment_section->sectname, "__objc_imageinfo", 16) != 0) {
                            segment_section = (macho::segments::section_64 *)((uint64_t)segment_section + sizeof(macho::segments::section_64));
                            segment_sections_count--;

                            continue;
                        }

                        auto segment_section_data_offset = segment_section->offset;
                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&segment_section_data_offset);
                        }

                        if (segment_section_data_offset >= library_container_size) {
                            failure_result = creation_result::invalid_segment;
                            return false;
                        }

                        auto segment_section_data_size = segment_section->size;
                        if (library_container_is_big_endian) {
                            macho::swap_uint32(&segment_section_data_offset);
                        }

                        if (segment_section_data_size >= library_container_size) {
                            failure_result = creation_result::invalid_segment;
                            return false;
                        }

                        const auto segment_section_data_end = segment_section_data_offset + segment_section_data_size;
                        if (segment_section_data_end >= library_container_size) {
                            failure_result = creation_result::invalid_segment;
                            return false;
                        }

                        const auto library_container_stream_position = ftell(library_container_stream);

                        fseek(library_container_stream, library_container_base + segment_section_data_offset, SEEK_SET);
                        fread(&objc_image_info, sizeof(objc_image_info), 1, library_container_stream);

                        fseek(library_container_stream, library_container_stream_position, SEEK_SET);

                        found_objc_image_info = true;
                        break;
                    }

                    if (!found_objc_image_info) {
                        break;
                    }

                    const auto &objc_image_info_flags = objc_image_info.flags;
                    auto objc_image_info_flags_swift_version = (objc_image_info_flags >> objc::image_info::flags::swift_version_shift) & objc::image_info::flags::swift_version_mask;

                    if (swift_version != 0) {
                        if (swift_version != objc_image_info_flags_swift_version) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }
                    } else {
                        swift_version = objc_image_info_flags_swift_version;
                    }

                    break;
                }

                case macho::load_commands::uuid: {
                    if (version != version::v2) {
                        break;
                    }

                    const auto &library_uuid = ((macho::uuid_command *)load_command)->uuid;

                    // Check if multiple uuid load-commands
                    // were found

                    if (uuid != nullptr) {
                        if (memcmp(uuid, library_uuid, 16) != 0) {
                            failure_result = creation_result::contradictary_load_command_information;
                            return false;
                        }

                        return true;
                    } else {
                        uuid = (uint8_t *)&library_uuid;
                    }

                    break;
                }

                case macho::load_commands::version_min_macosx: {
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

                case macho::load_commands::version_min_iphoneos: {
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

                case macho::load_commands::version_min_watchos: {
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

                case macho::load_commands::version_min_tvos: {
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

        switch (library_container_load_command_iteration_result) {
            case macho::container::load_command_iteration_result::ok:
                break;

            case macho::container::load_command_iteration_result::stream_seek_error:
            case macho::container::load_command_iteration_result::stream_read_error:
            case macho::container::load_command_iteration_result::load_command_is_too_small:
            case macho::container::load_command_iteration_result::load_command_is_too_large:
                return creation_result::failed_to_iterate_load_commands;
        }

        if (failure_result != creation_result::ok) {
            return failure_result;
        }

        if (platform == platform::none) {
            return creation_result::platform_not_found;
        }

        if (current_version == -1 || compatibility_version == -1 || !installation_name) {
            return creation_result::not_a_library;
        }

        if (version == version::v2 && !uuid) {
            return creation_result::has_no_uuid;
        }

        auto library_container_symbols_iteration_result = library.iterate_symbols([&](const macho::nlist_64 &symbol_table_entry, const char *symbol_string) {
            const auto &symbol_table_entry_type = symbol_table_entry.n_type;
            if ((symbol_table_entry_type & macho::symbol_table::flags::type) != macho::symbol_table::type::section) {
                return true;
            }

            enum symbol::type symbol_type;

            const auto symbol_is_weak = symbol_table_entry.n_desc & macho::symbol_table::description::weak_definition;
            const auto parsed_symbol_string = get_parsed_symbol_string(symbol_string, symbol_is_weak, &symbol_type);

            if (!(options & symbol_options::allow_all_private_symbols)) {
                switch (symbol_type) {
                    case symbol::type::symbols:
                        if (!(options & symbol_options::allow_private_normal_symbols)) {
                            if (!(symbol_table_entry_type & macho::symbol_table::flags::external)) {
                                return true;
                            }
                        }

                        break;

                    case symbol::type::weak_symbols:
                        if (!(options & symbol_options::allow_private_weak_symbols)) {
                            if (!(symbol_table_entry_type & macho::symbol_table::flags::external)) {
                                return true;
                            }
                        }

                        break;

                    case symbol::type::objc_classes:
                        if (!(options & symbol_options::allow_private_objc_symbols)) {
                            if (!(options & symbol_options::allow_private_objc_classes)) {
                                if (!(symbol_table_entry_type & macho::symbol_table::flags::external)) {
                                    return true;
                                }
                            }
                        }

                        break;

                    case symbol::type::objc_ivars:
                        if (!(options & symbol_options::allow_private_objc_symbols)) {
                            if (!(options & symbol_options::allow_private_objc_ivars)) {
                                if (!(symbol_table_entry_type & macho::symbol_table::flags::external)) {
                                    return true;
                                }
                            }
                        }

                        break;

                    default:
                        break;
                }
            }

            const auto symbols_iter = std::find(symbols.begin(), symbols.end(), parsed_symbol_string);
            if (symbols_iter == symbols.end()) {
                symbols.emplace_back(parsed_symbol_string, symbol_is_weak, 0, symbol_type);
            }

            return true;
        });

        switch (library_container_symbols_iteration_result) {
            case macho::container::symbols_iteration_result::ok:
            case macho::container::symbols_iteration_result::no_symbols:
            case macho::container::symbols_iteration_result::no_symbol_table_load_command:
                break;

            case macho::container::symbols_iteration_result::stream_seek_error:
            case macho::container::symbols_iteration_result::stream_read_error:
            case macho::container::symbols_iteration_result::invalid_string_table:
            case macho::container::symbols_iteration_result::invalid_symbol_table:
            case macho::container::symbols_iteration_result::invalid_symbol_table_entry:
                return creation_result::failed_to_iterate_symbols;
        }

        if (reexports.empty() && symbols.empty()) {
            return creation_result::no_symbols_or_reexports;
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

        fputs("---", output);
        if (version == version::v2) {
            fputs(" !tapi-tbd-v2", output);
        }

        fprintf(output, "\narchs:%-17s[ ", "");

        if (has_architecture_overrides) {
            const auto architecture_info_table = macho::get_architecture_info_table();
            const auto architecture_info_table_size = macho::get_architecture_info_table_size();

            auto index = uint64_t();
            for (; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & ((uint64_t)1 << index))) {
                    continue;
                }

                fputs(architecture_info_table[index].name, output);
                break;
            }

            for (index++; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & ((uint64_t)1 << index))) {
                    continue;
                }

                fprintf(output, ", %s", architecture_info_table[index].name);
            }

            fputs(" ]\n", output);
        } else {
            fprintf(output, "[ %s ]\n", architecture_info->name);
        }

        if (version == version::v2) {
            if (!has_architecture_overrides) {
                fprintf(output, "uuids:%-17s[ '%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X' ]\n", "", architecture_info->name, uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
            }
        }

        fprintf(output, "platform:%-14s%s\n", "", platform_to_string(platform));
        fprintf(output, "install-name:%-10s%s\n", "", installation_name);

        auto library_current_version_major = current_version >> 16;
        auto library_current_version_minor = (current_version >> 8) & 0xff;
        auto library_current_version_revision = current_version & 0xff;

        fprintf(output, "current-version:%-7s%u", "", library_current_version_major);

        if (library_current_version_minor != 0) {
            fprintf(output, ".%u", library_current_version_minor);
        }

        if (library_current_version_revision != 0) {
            if (library_current_version_minor == 0) {
                fputs(".0", output);
            }

            fprintf(output, ".%u", library_current_version_revision);
        }

        auto compatibility_version_major = compatibility_version >> 16;
        auto compatibility_version_minor = (compatibility_version >> 8) & 0xff;
        auto compatibility_version_revision = compatibility_version & 0xff;

        fprintf(output, "\ncompatibility-version: %u", compatibility_version_major);

        if (compatibility_version_minor != 0) {
            fprintf(output, ".%u", compatibility_version_minor);
        }

        if (compatibility_version_revision != 0) {
            if (compatibility_version_minor == 0) {
                fputs(".0", output);
            }

            fprintf(output, ".%u", compatibility_version_revision);
        }

        fputc('\n', output);

        if (swift_version != 0) {
            switch (swift_version) {
                case 1:
                    fprintf(output, "swift-version:%-9s1\n", "");
                    break;

                case 2:
                    fprintf(output, "swift-version:%-9s1.2\n", "");
                    break;

                default:
                    fprintf(output, "swift-version:%-9s%u\n", "", swift_version - 1);
                    break;
            }
        }

        fputs("exports:\n", output);

        if (has_architecture_overrides) {
            const auto architecture_info_table = macho::get_architecture_info_table();
            const auto architecture_info_table_size = macho::get_architecture_info_table_size();

            auto index = uint64_t();
            for (; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & ((uint64_t)1 << index))) {
                    continue;
                }

                fprintf(output, "  - archs:%-12s[ %s", "", architecture_info_table[index].name);
                break;
            }

            for (index++; index < architecture_info_table_size; index++) {
                if (!(architecture_overrides & ((uint64_t)1 << index))) {
                    continue;
                }

                fprintf(output, ", %s", architecture_info_table[index].name);
            }

            fputs(" ]\n", output);
        } else {
            fprintf(output, "  - archs:%-12s[ %s ]\n", "", architecture_info->name);
        }

        print_reexports_to_tbd_output(output, reexports);

        print_symbols_to_tbd_output(output, symbols, symbol::type::symbols);
        print_symbols_to_tbd_output(output, symbols, symbol::type::weak_symbols);
        print_symbols_to_tbd_output(output, symbols, symbol::type::objc_classes);
        print_symbols_to_tbd_output(output, symbols, symbol::type::objc_ivars);

        fputs("...\n", output);
        return creation_result::ok;
    }
}
