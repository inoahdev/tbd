//
//  src/tbd/tbd.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/nlist.h>
#include <mach-o/swap.h>

#include <cerrno>
#include <cstdlib>

#include "group.h"
#include "tbd.h"

const char *tbd::platform_to_string(const enum tbd::platform &platform) noexcept {
    switch (platform) {
        case ios:
            return "ios";
            break;

        case macos:
            return "macos";
            break;

        case watchos:
            return "watchos";
            break;

        case tvos:
            return "tvos";
            break;

        default:
            return nullptr;
    }
}

enum tbd::platform tbd::string_to_platform(const char *platform) noexcept {
    if (strcmp(platform, "ios") == 0) {
        return platform::ios;
    } else if (strcmp(platform, "macos") == 0) {
        return platform::macos;
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

tbd::tbd(const std::vector<std::string> &macho_files, const std::vector<std::string> &output_files, const enum tbd::platform &platform, const enum tbd::version &version, const std::vector<const NXArchInfo *> &architecture_overrides)
: macho_files_(macho_files), output_files_(output_files), platform_(platform), version_(version), architectures_(architecture_overrides) {
    this->validate();
}

void tbd::run() {
    auto output_path_index = 0;
    for (const auto &macho_file_path : macho_files_) {
        auto output_file = stdout;
        if (output_path_index < output_files_.size()) {
            const auto &output_path_string = output_files_.at(output_path_index);
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
        auto macho_file_has_architecture_overrides = architectures_.size() != 0;

        auto current_version = std::string();
        auto compatibility_version = std::string();

        auto symbols = std::vector<symbol>();
        auto reexports = std::vector<symbol>();

        auto installation_name = std::string();
        auto uuids = std::vector<std::string>();

        auto &macho_file_containers = macho_file.containers();
        auto macho_container_counter = 0;

        const auto macho_file_containers_size = macho_file_containers.size();
        const auto macho_file_is_fat = macho_file_containers_size != 0;

        for (auto &macho_container : macho_file_containers) {
            const auto &macho_container_header = macho_container.header();
            const auto macho_container_should_swap = macho_container.should_swap();

            const auto macho_container_architecture_info = NXGetArchInfoFromCpuType(macho_container_header.cputype, macho_container_header.cpusubtype);
            if (!macho_container_architecture_info) {
                if (macho_file_is_fat) {
                    fprintf(stderr, "Architecture (#%d) is not of a recognizable cputype", macho_container_counter);
                } else {
                    fputs("Mach-o file is not of a recognizable cputype", stderr);
                }

                exit(1);
            }

            if (!macho_file_has_architecture_overrides) {
                architectures_.emplace_back(macho_container_architecture_info);
            }

            auto local_current_version = std::string();
            auto local_compatibility_version = std::string();
            auto local_installation_name = std::string();

            auto added_uuid = false;

            macho_container.iterate_load_commands([&](const struct load_command *load_cmd) {
                switch (load_cmd->cmd) {
                    case LC_ID_DYLIB: {
                        if (local_installation_name.size() != 0) {
                            if (macho_file_is_fat) {
                                fprintf(stderr, "Architecture (#%d) has two library-identification load-commands\n", macho_container_counter);
                            } else {
                                fputs("Mach-o file has two library-identification load-commands\n", stderr);
                            }

                            exit(1);
                        }

                        auto id_dylib_command = (struct dylib_command *)load_cmd;
                        if (macho_container_should_swap) {
                            swap_dylib_command(id_dylib_command, NX_LittleEndian);
                        }

                        const auto &id_dylib = id_dylib_command->dylib;
                        const auto &id_dylib_installation_name_string_index = id_dylib.name.offset;

                        if (id_dylib_installation_name_string_index > load_cmd->cmdsize) {
                            fputs("Library identification load-command has an invalid identification-string position\n", stderr);
                            exit(1);
                        }

                        const auto &id_dylib_installation_name_string = &((char *)load_cmd)[id_dylib_installation_name_string_index];
                        local_installation_name = id_dylib_installation_name_string;

                        char id_dylib_current_version_string_data[33];
                        sprintf(id_dylib_current_version_string_data, "%u.%u.%u", id_dylib.current_version >> 16, (id_dylib.current_version >> 8) & 0xff, id_dylib.current_version & 0xff);

                        char id_dylib_compatibility_version_string_data[33];
                        sprintf(id_dylib_compatibility_version_string_data, "%u.%u.%u", id_dylib.compatibility_version >> 16, (id_dylib.compatibility_version >> 8) & 0xff, id_dylib.compatibility_version & 0xff);

                        local_current_version = id_dylib_current_version_string_data;
                        local_compatibility_version = id_dylib_compatibility_version_string_data;

                        break;
                    }

                    case LC_REEXPORT_DYLIB: {
                        auto reexport_dylib_command = (struct dylib_command *)load_cmd;
                        if (macho_container_should_swap) {
                            swap_dylib_command(reexport_dylib_command, NX_LittleEndian);
                        }

                        const auto &reexport_dylib = reexport_dylib_command->dylib;
                        const auto &reexport_dylib_string_index = reexport_dylib.name.offset;

                        if (reexport_dylib_string_index > load_cmd->cmdsize) {
                            fputs("Re-export dylib load-command has an invalid identification-string position\n", stderr);
                            exit(1);
                        }

                        const auto &reexport_dylib_string = &((char *)load_cmd)[reexport_dylib_string_index];
                        const auto reexports_iter = std::find(reexports.begin(), reexports.end(), reexport_dylib_string);

                        if (reexports_iter != reexports.end()) {
                            reexports_iter->add_architecture_info(macho_container_architecture_info);
                        } else {
                            reexports.emplace_back(reexport_dylib_string, false);
                        }

                        break;
                    }

                    case LC_UUID: {
                        const auto &uuid = ((struct uuid_command *)load_cmd)->uuid;

                        char uuid_string[33] = {};
                        sprintf(uuid_string, "%.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);

                        const auto uuids_iter = std::find(uuids.begin(), uuids.end(), uuid_string);
                        if (uuids_iter != uuids.end()) {
                            fprintf(stderr, "uuid-string (%s) is found in multiple architectures", uuid_string);
                            exit(1);
                        }

                        uuids.emplace_back(uuid_string);
                        added_uuid = true;

                        break;
                    }
                }

                return true;
            });

            if (local_installation_name.empty()) {
                fputs("Mach-o file is not a library or framework\n", stderr);
                exit(1);
            }

            if (installation_name.size() && installation_name != local_installation_name) {
                fprintf(stderr, "Mach-o file has conflicting installation-names (%s vs %s from %s)\n", installation_name.data(), local_installation_name.data(), macho_container_architecture_info->name);
                exit(1);
            } else if (installation_name.empty()) {
                installation_name = local_installation_name;
            }

            if (current_version.size() && current_version != local_current_version) {
                fprintf(stderr, "Mach-o file has conflicting current_version (%s vs %s from %s)\n", current_version.data(), local_current_version.data(), macho_container_architecture_info->name);
                exit(1);
            } else if (current_version.empty()) {
                current_version = local_current_version;
            }

            if (compatibility_version.size() && compatibility_version != local_compatibility_version) {
                fprintf(stderr, "Mach-o file has conflicting compatibility-version (%s vs %s from %s)\n", compatibility_version.data(), local_compatibility_version.data(), macho_container_architecture_info->name);
                exit(1);
            } else if (compatibility_version.empty()) {
                compatibility_version = local_compatibility_version;
            }

            if (!added_uuid && version_ == version::v2) {
                fprintf(stderr, "Macho-file with architecture (%s) does not have a uuid command\n", macho_container_architecture_info->name);
                exit(1);
            }

            macho_container.iterate_symbols([&](const struct nlist_64 &symbol_table_entry, const char *symbol_string) {
                const auto &symbol_table_entry_type = symbol_table_entry.n_type;
                if ((symbol_table_entry_type & N_TYPE) != N_SECT || (symbol_table_entry_type & N_EXT) != N_EXT) {
                    return true;
                }

                const auto symbols_iter = std::find(symbols.begin(), symbols.end(), symbol_string);
                if (symbols_iter != symbols.end()) {
                    symbols_iter->add_architecture_info(macho_container_architecture_info);
                } else {
                    symbols.emplace_back(symbol_string, symbol_table_entry.n_desc & N_WEAK_DEF);
                    symbols.back().add_architecture_info(macho_container_architecture_info);
                }

                return true;
            });

            macho_container_counter++;
        }

        auto groups = std::vector<group>();
        if (macho_file_has_architecture_overrides) {
            groups.emplace_back(architectures_);

            auto &group = groups.front();
            for (const auto &reexport : reexports) {
                group.add_reexport(reexport);
            }

            for (const auto &symbol : symbols) {
                group.add_symbol(symbol);
            }
        } else {
            for (const auto &reexport : reexports) {
                const auto &reexport_architecture_infos = reexport.architecture_infos();
                const auto group_iter = std::find(groups.begin(), groups.end(), reexport_architecture_infos);

                if (group_iter != groups.end()) {
                    group_iter->add_reexport(reexport);
                } else {
                    groups.emplace_back(reexport_architecture_infos);
                    groups.back().add_reexport(reexport);
                }
            }

            for (const auto &symbol : symbols) {
                const auto &symbol_architecture_infos = symbol.architecture_infos();
                const auto group_iter = std::find(groups.begin(), groups.end(), symbol_architecture_infos);

                if (group_iter != groups.end()) {
                    group_iter->add_symbol(symbol);
                } else {
                    groups.emplace_back(symbol_architecture_infos);
                    groups.back().add_symbol(symbol);
                }
            }
        }

        std::sort(groups.begin(), groups.end(), [](const group &lhs, const group &rhs) {
            const auto &lhs_symbols = lhs.symbols();
            const auto lhs_symbols_size = lhs_symbols.size();

            const auto &rhs_symbols = rhs.symbols();
            const auto rhs_symbols_size = rhs_symbols.size();

            return lhs_symbols_size < rhs_symbols_size;
        });

        fputs("---", output_file);
        if (version_ == version::v2) {
            fputs(" !tapi-tbd-v2", output_file);
        }

        fprintf(output_file, "\narchs:%-17s[ ", "");

        auto architectures_begin = architectures_.begin();
        fprintf(output_file, "%s", (*architectures_begin)->name);

        for (architectures_begin++; architectures_begin != architectures_.end(); architectures_begin++) {
            fprintf(output_file, ", %s", (*architectures_begin)->name);
        }

        fputs(" ]\n", output_file);

        if (version_ == version::v2) {
            fprintf(output_file, "uuids:%-17s[ ", "");

            auto counter = 1;
            auto uuids_begin = uuids.begin();

            for (auto architectures_begin = architectures_.begin(); architectures_begin < architectures_.end(); architectures_begin++, uuids_begin++) {
                fprintf(output_file, "'%s: %s'", (*architectures_begin)->name, uuids_begin->data());

                if (architectures_begin != architectures_.end() - 1) {
                    fputs(", ", output_file);

                    if (counter % 2 == 0) {
                        fprintf(output_file, "%-27s", "\n");
                    }

                    counter++;
                }
            }

            fputs(" ]\n", output_file);
        }

        fprintf(output_file, "platform:%-14s%s\n", "", platform_to_string(platform_));
        fprintf(output_file, "install-name:%-10s%s\n", "", installation_name.data());

        fprintf(output_file, "current-version:%-7s%s\n", "", current_version.data());
        fprintf(output_file, "compatibility-version: %s\n", compatibility_version.data());

        if (version_ == version::v2) {
            fprintf(output_file, "objc-constraint:%-7snone\n", "");
        }

        fputs("exports:\n", output_file);
        for (auto &group : groups) {
            auto ordinary_symbols = std::vector<std::string>();
            auto weak_symbols = std::vector<std::string>();
            auto objc_classes = std::vector<std::string>();
            auto objc_ivars = std::vector<std::string>();
            auto reexports = std::vector<std::string>();

            for (const auto &reexport : group.reexports()) {
                reexports.emplace_back(reexport.string());
            }

            for (const auto &symbol : group.symbols()) {
                auto &symbol_string = const_cast<std::string &>(symbol.string());
                auto symbol_string_begin_pos = 0;

                if (symbol_string.compare(0, 3, "$ld") == 0) {
                    symbol_string.insert(symbol_string.begin(), '\'');
                    symbol_string.append(1, '\'');

                    symbol_string_begin_pos += 4;
                }

                if (symbol_string.compare(symbol_string_begin_pos, 13, "_OBJC_CLASS_$") == 0) {
                    symbol_string.erase(0, 13);
                    objc_classes.emplace_back(symbol_string);
                } else if (symbol_string.compare(symbol_string_begin_pos, 17, "_OBJC_METACLASS_$") == 0) {
                    symbol_string.erase(0, 17);
                    objc_classes.emplace_back(symbol_string);
                } else if (symbol_string.compare(symbol_string_begin_pos, 12, "_OBJC_IVAR_$") == 0) {
                    symbol_string.erase(0, 12);
                    objc_ivars.emplace_back(symbol_string);
                } else if (symbol.weak()) {
                    weak_symbols.emplace_back(symbol_string);
                } else {
                    ordinary_symbols.emplace_back(symbol_string);
                }
            }

            std::sort(ordinary_symbols.begin(), ordinary_symbols.end());
            std::sort(weak_symbols.begin(), weak_symbols.end());
            std::sort(objc_classes.begin(), objc_classes.end());
            std::sort(objc_ivars.begin(), objc_ivars.end());
            std::sort(reexports.begin(), reexports.end());

            ordinary_symbols.erase(std::unique(ordinary_symbols.begin(), ordinary_symbols.end()), ordinary_symbols.end());
            weak_symbols.erase(std::unique(weak_symbols.begin(), weak_symbols.end()), weak_symbols.end());
            objc_classes.erase(std::unique(objc_classes.begin(), objc_classes.end()), objc_classes.end());
            objc_ivars.erase(std::unique(objc_ivars.begin(), objc_ivars.end()), objc_ivars.end());
            reexports.erase(std::unique(reexports.begin(), reexports.end()), reexports.end());

            const auto &group_architecture_infos = group.architecture_infos();
            auto group_architecture_infos_begin = group_architecture_infos.begin();

            fprintf(output_file, "  - archs:%-12s[ %s", "", (*group_architecture_infos_begin)->name);
            for (group_architecture_infos_begin++; group_architecture_infos_begin < group_architecture_infos.end(); group_architecture_infos_begin++) {
                fprintf(output_file, ", %s", (*group_architecture_infos_begin)->name);
            }

            fputs(" ]\n", output_file);

            const auto line_length_max = 85;
            const auto key_spacing_length_max = 18;

            auto character_count = 0;
            const auto print_symbols = [&](const std::vector<std::string> &symbols, const char *key) {
                if (symbols.empty()) {
                    return;
                }

                fprintf(output_file, "%-4s%s:", "", key);

                auto symbols_begin = symbols.begin();
                auto symbols_end = symbols.end();

                for (auto i = strlen(key) + 1; i < key_spacing_length_max; i++) {
                    fputs(" ", output_file);
                }

                fprintf(output_file, "[ %s", symbols_begin->data());
                character_count += symbols_begin->length();

                for (symbols_begin++; symbols_begin != symbols_end; symbols_begin++) {
                    const auto &symbol = *symbols_begin;
                    auto line_length = symbol.length() + 1;

                    if (character_count >= line_length_max || (character_count != 0 && character_count + line_length > line_length_max)) {
                        fprintf(output_file, ",\n%-24s", "");
                        character_count = 0;
                    } else {
                        fputs(", ", output_file);
                        line_length++;
                    }

                    fputs(symbol.data(), output_file);
                    character_count += line_length;
                }

                fprintf(output_file, " ]\n");
                character_count = 0;
            };

            print_symbols(reexports, "re-exports");
            print_symbols(ordinary_symbols, "symbols");
            print_symbols(weak_symbols, "weak-def-symbols");
            print_symbols(objc_classes, "objc-classes");
            print_symbols(objc_ivars, "objc-ivars");
        }

        fputs("...\n", output_file);

        output_path_index++;
        if (output_file != stdout) {
            fclose(output_file);
        }
     }
}
