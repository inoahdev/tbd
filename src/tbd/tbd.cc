//
//  src/tbd/tbd.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>

#include "group.h"
#include "tbd.h"

const char *tbd::platform_to_string(const enum tbd::platform &platform) noexcept {
    switch (platform) {
        case ios:
            return "ios";

        case macosx:
            return "macosx";

        case watchos:
            return "watchos";

        case tvos:
            return "tvos";

        default:
            return nullptr;
    }
}

enum tbd::platform tbd::string_to_platform(const char *platform) noexcept {
    if (strcmp(platform, "ios") == 0) {
        return platform::ios;
    } else if (strcmp(platform, "macosx") == 0) {
        return platform::macosx;
    } else if (strcmp(platform, "watchos") == 0) {
        return platform::watchos;
    } else if (strcmp(platform, "tvos") == 0) {
        return platform::tvos;
    }

    return (enum platform)-1;
}

enum tbd::version tbd::string_to_version(const char *version) noexcept {
    if (strcmp(version, "v1") == 0) {
        return tbd::version::v1;
    } else if (strcmp(version, "v2") == 0) {
        return tbd::version::v2;
    }

    return (enum tbd::version)-1;
}

tbd::tbd(const std::vector<std::string> &macho_files, const std::vector<std::string> &output_files, const enum tbd::platform &platform, const enum tbd::version &version, const std::vector<const macho::architecture_info *> &architecture_overrides)
: macho_files_(macho_files), output_files_(output_files), platform_(platform), version_(version), architectures_(architecture_overrides) {}

void tbd::print_symbols(FILE *output, const flags &flags, std::vector<symbol> &symbols, enum symbol::type type) const noexcept {
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
        if (symbol.type() != type) {
            continue;
        }

        if (should_check_flags) {
            const auto &symbol_flags = symbol.flags();
            if (symbol_flags != flags) {
                continue;
            }
        }

        break;
    }

    // If no valid symbols were found, print_symbols should
    // return immedietly.

    if (symbols_begin == symbols_end) {
        return;
    }

    const auto parse_symbol_string = [](const char *symbol_string, enum symbol::type type) {
        switch (type) {
            case symbol::type::reexports:
            case symbol::type::symbols:
                return symbol_string;

            case symbol::type::objc_classes: {
                if (strncmp(symbol_string, "_OBJC_CLASS_$", 13) == 0) {
                    return &symbol_string[13];
                }

                if (strncmp(symbol_string, ".objc_class_name", 16) == 0) {
                    return &symbol_string[16];
                }

                if (strncmp(symbol_string, "_OBJC_METACLASS_$", 17) == 0) {
                    return &symbol_string[17];
                }

                return symbol_string;
            }

            case symbol::type::objc_ivars:
                return &symbol_string[12];

            default:
                break;
        }

        return symbol_string;
    };

    switch (type) {
        case symbol::type::reexports:
            fprintf(output, "%-4sre-exports:%7s", "", "");
            break;

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

    auto symbols_begin_string = symbols_begin->string();
    auto parsed_symbols_begin_string = parse_symbol_string(symbols_begin_string, type);
    
    fputs("[ ", output);
    
    const auto symbol_string_needs_quotes = strncmp(symbols_begin_string, "$ld", 3) == 0;
    if (symbol_string_needs_quotes) {
        fputc('\'', output);
    }
    
    fputs(parsed_symbols_begin_string, output);
    if (symbol_string_needs_quotes) {
        fputc('\'', output);
    }
    
    auto current_line_length = strlen(parsed_symbols_begin_string);
    for (symbols_begin++; symbols_begin < symbols_end; symbols_begin++) {
        const auto &symbol = *symbols_begin;
        if (symbol.type() != type) {
            continue;
        }

        if (should_check_flags) {
            const auto &symbol_flags = symbol.flags();
            if (symbol_flags != flags) {
                continue;
            }
        }

        // If a symbol-string has a dollar sign, quotes must be
        // added around the string.

        const auto symbol_string = symbol.string();
        const auto symbol_string_needs_quotes = strncmp(symbol_string, "$ld", 3) == 0;

        auto symbol_string_to_print = parse_symbol_string(symbol_string, type);
        const auto symbol_string_to_print_length = strlen(symbol_string_to_print);

        auto new_line_length = symbol_string_to_print_length + 2;
        if (symbol_string_needs_quotes) {
            new_line_length += 2;
        }

        auto new_current_line_length = current_line_length + new_line_length;

        // A line that is printed is allowed to go upto a line_length_max. When
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

        fputs(symbol_string_to_print, output);

        if (symbol_string_needs_quotes) {
            fputc('\'', output);
        }

        current_line_length = new_current_line_length;
    }

    fputs(" ]\n", output);
}

void tbd::run(macho::file &macho_file, FILE *output) {
    const auto &platform = platform_;
    const auto &version = version_;

    auto &architectures = architectures_;
    auto macho_file_has_architecture_overrides = architectures.size() != 0;

    // To create a tbd-file, current and compatibility versions of the
    // mach-o library file need to be found.

    uint32_t current_version = -1;
    uint32_t compatibility_version = -1;

    const char *installation_name = nullptr;

    auto symbols = std::vector<symbol>();
    auto reexports = std::vector<symbol>();

    auto uuids = std::vector<uint8_t *>();

    auto &macho_file_containers = macho_file.containers();
    auto macho_container_index = 0;

    const auto macho_file_containers_size = macho_file_containers.size();
    const auto macho_file_is_fat = macho_file_containers_size != 1;

    uuids.reserve(macho_file_containers_size);

    for (auto &macho_container : macho_file_containers) {
        const auto &macho_container_header = macho_container.header();

        const auto &macho_container_header_cputype = macho::cputype(macho_container_header.cputype);
        const auto &macho_container_header_cpusubtype = macho::subtype_from_cputype(macho_container_header_cputype, macho_container_header.cpusubtype);

        const auto macho_container_architecture_info = macho::architecture_info_from_cputype(macho_container_header_cputype, macho_container_header_cpusubtype);
        if (!macho_container_architecture_info) {
            if (macho_file_is_fat) {
                fprintf(stderr, "Architecture (#%d) is not of a recognizable cputype", macho_container_index);
            } else {
                fputs("Mach-o file is not of a recognizable cputype\n", stderr);
            }

            exit(1);
        }

        if (!macho_file_has_architecture_overrides) {
            // As custom architectures are not being stored, Use
            // architectures to store the architecture-information
            // of the architectures of the mach-o file

            architectures.emplace_back(macho_container_architecture_info);
        }

        const auto &macho_container_base = macho_container.base();
        const auto macho_container_is_big_endian = macho_container.is_big_endian();

        // Use local variables of current_version and compatibility_version and
        // and installation_name to make sure details are the same across containers

        uint32_t local_current_version = -1;
        uint32_t local_compatibility_version = -1;

        const char *local_installation_name = nullptr;
        const auto needs_uuid = version == version::v2;

        auto added_uuid = false;

        macho_container.iterate_load_commands([&](const macho::load_command *load_cmd) {
            switch (load_cmd->cmd) {
                case macho::load_commands::identification_dylib: {
                    if (local_installation_name != nullptr) {
                        if (macho_file_is_fat) {
                            fprintf(stderr, "Architecture (#%d) has multiple library-identification load-commands\n", macho_container_index);
                        } else {
                            fputs("Mach-o file has multiple library-identification load-commands\n", stderr);
                        }

                        exit(1);
                    }

                    auto id_dylib_command = (macho::dylib_command *)load_cmd;
                    if (macho_container_is_big_endian) {
                        macho::swap_dylib_command(id_dylib_command);
                    }

                    const auto &id_dylib_installation_name_string_index = id_dylib_command->name.offset;

                    if (id_dylib_installation_name_string_index >= load_cmd->cmdsize) {
                        fputs("Library identification load-command has an invalid identification-string position\n", stderr);
                        exit(1);
                    }

                    const auto &id_dylib_installation_name_string = &((char *)load_cmd)[id_dylib_installation_name_string_index];

                    local_installation_name = id_dylib_installation_name_string;
                    local_current_version = id_dylib_command->current_version;
                    local_compatibility_version = id_dylib_command->compatibility_version;

                    break;
                }

                case macho::load_commands::reexport_dylib: {
                    auto reexport_dylib_command = (macho::dylib_command *)load_cmd;
                    if (macho_container_is_big_endian) {
                        macho::swap_dylib_command(reexport_dylib_command);
                    }

                    const auto &reexport_dylib_string_index = reexport_dylib_command->name.offset;
                    if (reexport_dylib_string_index >= load_cmd->cmdsize) {
                        fputs("Re-export dylib load-command has an invalid identification-string position\n", stderr);
                        exit(1);
                    }

                    const auto &reexport_dylib_string = &((char *)load_cmd)[reexport_dylib_string_index];
                    const auto reexports_iter = std::find(reexports.begin(), reexports.end(), reexport_dylib_string);

                    if (reexports_iter != reexports.end()) {
                        reexports_iter->add_architecture(macho_container_index);
                    } else {
                        reexports.emplace_back(reexport_dylib_string, false, macho_file_containers_size, symbol::type::reexports);
                        reexports.back().add_architecture(macho_container_index);
                    }

                    break;
                }

                case macho::load_commands::uuid: {
                    if (!needs_uuid) {
                        break;
                    }

                    if (added_uuid) {
                        if (macho_file_is_fat) {
                            fprintf(stderr, "Architecture (#%d) has multiple uuid load-commands\n", macho_container_index);
                        } else {
                            fputs("Mach-o file has multiple uuid load-commands\n", stderr);
                        }

                        exit(1);
                    }

                    const auto &uuid = ((macho::uuid_command *)load_cmd)->uuid;
                    const auto uuids_iter = std::find_if(uuids.begin(), uuids.end(), [&](uint8_t *rhs) {
                        return memcmp(&uuid, rhs, 16) == 0;
                    });

                    const auto &uuids_end = uuids.end();
                    if (uuids_iter != uuids_end) {
                        fprintf(stderr, "uuid-string (%.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X) is found in multiple architectures", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
                        exit(1);
                    }

                    uuids.emplace_back((uint8_t *)&uuid);
                    added_uuid = true;

                    break;
                }

                default:
                    break;
            }

            return true;
        });

        if (local_current_version == -1 || local_compatibility_version == -1 || !local_installation_name) {
            fputs("Mach-o file is not a library or framework\n", stderr);
            exit(1);
        }

        if (installation_name != nullptr && strcmp(installation_name, local_installation_name) != 0) {
            fprintf(stderr, "Mach-o file has conflicting installation-names (%s vs %s from container at base (0x%.8lX))\n", installation_name, local_installation_name, macho_container_base);
            exit(1);
        } else if (!installation_name) {
            installation_name = local_installation_name;
        }

        if (current_version != -1 && current_version != local_current_version) {
            fprintf(stderr, "Mach-o file has conflicting current_version (%d vs %d from container at base (0x%.8lX))\n", current_version, local_current_version, macho_container_base);
            exit(1);
        } else if (current_version == -1) {
            current_version = local_current_version;
        }

        if (compatibility_version != -1 && compatibility_version != local_compatibility_version) {
            fprintf(stderr, "Mach-o file has conflicting compatibility-version (%d vs %d from container at base (0x%.8lX))\n", compatibility_version, local_compatibility_version, macho_container_base);
            exit(1);
        } else if (compatibility_version == -1) {
            compatibility_version = local_compatibility_version;
        }

        if (!added_uuid && version == version::v2) {
            fprintf(stderr, "Macho-file with architecture (%s) does not have a uuid command\n", macho_container_architecture_info->name);
            exit(1);
        }
        
        const auto get_parsed_symbol_string = [](const char *string, bool is_weak, enum symbol::type *type) {
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
                *type = symbol::type::objc_classes;
                return &string[12];
            }
            
            *type = symbol::type::symbols;
            return string;
        };

        macho_container.iterate_symbols([&](const macho::nlist_64 &symbol_table_entry, const char *symbol_string) {
            const auto &symbol_table_entry_type = symbol_table_entry.n_type;
            if ((symbol_table_entry_type & macho::symbol_table::flags::type) != macho::symbol_table::type::section || (symbol_table_entry_type & macho::symbol_table::flags::external) != macho::symbol_table::flags::external) {
                return true;
            }
            
            enum symbol::type symbol_type;
            
            const auto symbol_is_weak = symbol_table_entry.n_desc & macho::symbol_table::description::weak_definition;
            const auto parsed_symbol_string = get_parsed_symbol_string(symbol_string, symbol_is_weak, &symbol_type);
            
            const auto symbols_iter = std::find(symbols.begin(), symbols.end(), parsed_symbol_string);
            
            if (symbols_iter != symbols.end()) {
                symbols_iter->add_architecture(macho_container_index);
            } else {
                symbols.emplace_back(parsed_symbol_string, symbol_is_weak, macho_file_containers_size, symbol_type);
                symbols.back().add_architecture(macho_container_index);
            }

            return true;
        });

        macho_container_index++;
    }

    std::sort(reexports.begin(), reexports.end(), [](const symbol &lhs, const symbol &rhs) {
        const auto lhs_string = lhs.string();
        const auto rhs_string = rhs.string();

        return strcmp(lhs_string, rhs_string) < 0;
    });

    std::sort(symbols.begin(), symbols.end(), [](const symbol &lhs, const symbol &rhs) {
        const auto lhs_string = lhs.string();
        const auto rhs_string = rhs.string();

        return strcmp(lhs_string, rhs_string) < 0;
    });

    auto groups = std::vector<group>();
    if (macho_file_has_architecture_overrides) {
        groups.emplace_back();
    } else {
        for (const auto &reexport : reexports) {
            const auto &reexport_flags = reexport.flags();
            const auto group_iter = std::find(groups.begin(), groups.end(), reexport_flags);

            if (group_iter != groups.end()) {
                group_iter->increment_reexport_count();
            } else {
                groups.emplace_back(reexport_flags);
                groups.back().increment_reexport_count();
            }
        }

        for (const auto &symbol : symbols) {
            const auto &symbol_flags = symbol.flags();
            const auto group_iter = std::find(groups.begin(), groups.end(), symbol_flags);
                            
            
            if (group_iter != groups.end()) {
                group_iter->increment_symbol_count();
            } else {
                groups.emplace_back(symbol_flags);
                groups.back().increment_symbol_count();
            }
        }

        std::sort(groups.begin(), groups.end(), [](const group &lhs, const group &rhs) {
            const auto lhs_symbols_count = lhs.symbols_count();
            const auto rhs_symbols_count = rhs.symbols_count();

            return lhs_symbols_count < rhs_symbols_count;
        });
    }

    fputs("---", output);
    if (version == version::v2) {
        fputs(" !tapi-tbd-v2", output);
    }

    fprintf(output, "\narchs:%-17s[ ", "");

    auto architectures_begin = architectures.begin();
    auto architectures_begin_arch_info = *architectures_begin;

    fprintf(output, "%s", architectures_begin_arch_info->name);

    const auto architectures_end = architectures.end();
    for (architectures_begin++; architectures_begin != architectures_end; architectures_begin++) {
        auto architecture_arch_info = *architectures_begin;
        auto architecture_arch_info_name = architecture_arch_info->name;

        fprintf(output, ", %s", architecture_arch_info_name);
    }

    fputs(" ]\n", output);

    if (version == version::v2) {
        fprintf(output, "uuids:%-17s[ ", "");

        auto uuid_counter = 1;
        auto uuids_begin = uuids.begin();

        const auto &architectures_end = architectures.end();
        const auto &architectures_back = architectures_end - 1;
        
        for (auto architectures_begin = architectures.begin(); architectures_begin < architectures_end; architectures_begin++, uuids_begin++) {
            const auto &architecture_arch_info = *architectures_begin;
            const auto &uuid = *uuids_begin;

            fprintf(output, "'%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X'", architecture_arch_info->name, uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);

            if (architectures_begin != architectures_back) {
                fputs(", ", output);

                if (uuid_counter % 2 == 0) {
                    fprintf(output, "%-26s", "\n");
                }

                uuid_counter++;
            }
        }

        fputs(" ]\n", output);
    }

    fprintf(output, "platform:%-14s%s\n", "", platform_to_string(platform));
    fprintf(output, "install-name:%-10s%s\n", "", installation_name);

    fprintf(output, "current-version:%-7s%u.%u.%u\n", "", current_version >> 16, (current_version >> 8) & 0xff, current_version & 0xff);
    fprintf(output, "compatibility-version: %u.%u.%u\n", compatibility_version >> 16, (compatibility_version >> 8) & 0xff, compatibility_version & 0xff);

    if (version == version::v2) {
        fprintf(output, "objc-constraint:%-7snone\n", "");
    }

    fputs("exports:\n", output);

    if (macho_file_has_architecture_overrides) {
        const auto architectures_begin = architectures.begin();
        const auto architectures_begin_arch_info = *architectures_begin;

        fprintf(output, "  - archs:%-12s[ %s", "", architectures_begin_arch_info->name);

        const auto architectures_end = architectures.end();
        for (auto architectures_iter = architectures_begin + 1; architectures_iter < architectures_end; architectures_iter++) {
            auto architectures_iter_arch_info = *architectures_iter;
            auto architectures_iter_arch_info_name = architectures_iter_arch_info->name;

            fprintf(output, ", %s", architectures_iter_arch_info_name);
        }

        fputs(" ]\n", output);

        const auto &group = groups.front();
        const auto &group_flags = group.flags();

        print_symbols(output, group_flags, reexports, symbol::type::reexports);
        print_symbols(output, group_flags, symbols, symbol::type::symbols);
        print_symbols(output, group_flags, symbols, symbol::type::weak_symbols);
        print_symbols(output, group_flags, symbols, symbol::type::objc_classes);
        print_symbols(output, group_flags, symbols, symbol::type::objc_ivars);

    } else {
        for (auto &group : groups) {
            const auto &group_flags = group.flags();
            const auto architectures_size = architectures.size();

            auto architectures_index = architectures_size - 1;
            auto has_printed_first_architecture = false;

            for (; architectures_index < architectures_size; architectures_index--) {
                const auto architecture = group_flags.at_index(architectures_index);
                if (!architecture) {
                    continue;
                }

                const auto &architecture_info = architectures.at(architectures_index);

                if (!has_printed_first_architecture) {
                    fprintf(output, "  - archs:%-12s[ %s", "", architecture_info->name);
                    has_printed_first_architecture = true;
                } else {
                    fprintf(output, ", %s", architecture_info->name);
                }
            }

            fputs(" ]\n", output);

            print_symbols(output, group_flags, reexports, symbol::type::reexports);
            print_symbols(output, group_flags, symbols, symbol::type::symbols);
            print_symbols(output, group_flags, symbols, symbol::type::weak_symbols);
            print_symbols(output, group_flags, symbols, symbol::type::objc_classes);
            print_symbols(output, group_flags, symbols, symbol::type::objc_ivars);
        }
    }

    fputs("...\n", output);

    // Clear internal architectures vector that was used
    // to store architecture-information for file-containers
    // if no custom architectures were provided

    if (!macho_file_has_architecture_overrides) {
        architectures.clear();
    }
}

void tbd::run() {
    const auto &macho_files = macho_files_;
    const auto &output_files = output_files_;

    auto output_file_index = 0;
    const auto output_files_size = output_files.size();

    for (const auto &macho_file_path : macho_files) {
        // The output-file by default is set to stdout, this is
        // for the user-expected behavior when a output file is
        // not provided.

        auto output_file = stdout;
        if (output_file_index < output_files_size) {
            const auto &output_path_string = output_files.at(output_file_index);
            if (output_path_string != "stdout") {
                const auto output_path_string_data = output_path_string.data();

                output_file = fopen(output_path_string_data, "w");
                if (!output_file) {
                    fprintf(stderr, "Failed to open file at path (%s) for writing, failing with error (%s)\n", output_path_string_data, strerror(errno));
                    exit(1);
                }
            }
        }

        auto macho_file = macho::file(macho_file_path);
        run(macho_file, output_file);

        output_file_index++;

        // Close the output file that was opened for
        // writing earlier

        if (output_file != stdout) {
            fclose(output_file);
        }
    }
}
