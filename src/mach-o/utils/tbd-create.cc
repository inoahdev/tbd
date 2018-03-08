//
//  src/mach-o/utils/tbd-create.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "../../utils/range.h"
#include "../../objc/image_info.h"

#include "../headers/build.h"
#include "../headers/flags.h"

#include "segments/segment_with_sections_iterator.h"
#include "tbd.h"

namespace macho::utils {
    tbd::creation_result tbd::create(const macho::file &file, const creation_options &options) noexcept {
        auto container_index = uint32_t();
        for (const auto &container : file.containers) {
            auto container_cputype = container.header.cputype;
            auto container_cpusubtype = container.header.cpusubtype;

            if (container.is_big_endian()) {
                swap::cputype(container_cputype);
                ::utils::swap::int32(container_cpusubtype);
            }

            const auto container_subtype = subtype_from_cputype(macho::cputype(container_cputype), container_cpusubtype);
            if (container_subtype == subtype::none) {
                return creation_result::invalid_container_header_subtype;
            }

            const auto architecture_info_index = macho::architecture_info_index_from_cputype(macho::cputype(container_cputype), container_subtype);
            if (architecture_info_index == -1) {
                return creation_result::unrecognized_container_cputype_subtype_pair;
            }

            const auto architecture_info_index_bit = (1ull << architecture_info_index);
            if (this->architectures & architecture_info_index_bit) {
                return creation_result::multiple_containers_for_same_architecture;
            }
            
            this->architectures |= architecture_info_index_bit;

            if (!options.ignore_unnecessary_fields_for_version) {
                if (!options.ignore_flags) {
                    if (this->version == version::v2 || options.parse_unsupported_fields_for_version) {
                        struct tbd::flags flags;
                        
                        flags.flat_namespace = container.header.flags.two_level_namespaces;
                        flags.not_app_extension_safe = !container.header.flags.app_extension_safe;
                        
                        if (container_index != 0) {
                            if (this->flags != flags) {
                                return creation_result::flags_mismatch;
                            }
                        }
                        
                        this->flags = flags;
                    }
                }
            }

            auto load_commands = load_commands::data();
            switch (load_commands.create(container, load_commands::data::options())) {
                case load_commands::data::creation_result::ok:
                    break;

                default:
                    return creation_result::failed_to_retrieve_load_cammands;
            }

            // The first container and its load-commands are loaded into
            // the tbd information fields, and the subsequent containers
            // and their load-commands are compared against the first

            // As required load-commands might not be present, We need to
            // track whether or not all required load-commands were found

            auto found_container_identification = false;
            auto found_container_platform = false;
            auto found_container_objc_information = false;
            auto found_container_parent_umbrella = false;

            auto symtab_command = static_cast<const struct symtab_command *>(nullptr);

            for (auto iter = load_commands.begin; iter != load_commands.end; iter++) {
                const auto cmd = iter.cmd();
                switch (cmd) {
                    case macho::load_commands::build_version: {
                        if (options.ignore_platform) {
                            break;
                        }

                        const auto build_version_command = static_cast<const struct build_version_command *>(iter.load_command());
                        auto build_version_platform = build_version_command->platform;

                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(build_version_platform);
                        }

                        // XXX: Maybe we shouldn't ignore when build_version
                        // command is larger than it should be

                        const auto cmdsize = iter.cmdsize();
                        if (cmdsize < sizeof(struct build_version_command)) {
                            return creation_result::invalid_build_version_command;
                        }

                        auto parsed_platform = platform::none;
                        switch (build_version::platform(build_version_platform)) {
                            case build_version::platform::macos:
                                parsed_platform = platform::macosx;
                                break;

                            case build_version::platform::ios:
                                parsed_platform = platform::ios;
                                break;

                            case build_version::platform::watchos:
                                parsed_platform = platform::watchos;
                                break;

                            case build_version::platform::tvos:
                                parsed_platform = platform::tvos;
                                break;

                            case build_version::platform::bridgeos:
                                return creation_result::unsupported_platform;

                            default:
                                return creation_result::unrecognized_platform;
                        }

                        if (this->platform != parsed_platform) {
                            if (found_container_platform) {
                                return creation_result::multiple_platforms;
                            }

                            if (container_index != 0) {
                                return creation_result::platforms_mismatch;
                            }
                        }

                        this->platform = parsed_platform;
                        found_container_platform = true;

                        break;
                    }

                    case macho::load_commands::identification_dylib: {
                        if (options.ignore_current_version && options.ignore_compatibility_version && options.ignore_install_name) {
                            break;
                        }

                        const auto dylib_command = static_cast<const struct dylib_command *>(iter.load_command());

                        auto dylib_command_name_location = dylib_command->name.offset;
                        auto dylib_command_current_version = dylib_command->current_version;
                        auto dylib_command_compatibility_version = dylib_command->compatibility_version;

                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(dylib_command_name_location);
                            ::utils::swap::uint32(dylib_command_current_version);
                            ::utils::swap::uint32(dylib_command_compatibility_version);
                        }

                        if (!options.ignore_unnecessary_fields_for_version) {
                            if (!options.ignore_current_version) {
                                if (this->current_version.value != dylib_command_current_version) {
                                    if (found_container_identification) {
                                        return creation_result::multiple_current_versions;
                                    }
                                    
                                    if (container_index != 0) {
                                        return creation_result::current_versions_mismatch;
                                    }
                                }
                                
                                this->current_version.value = dylib_command_current_version;
                            }
                            
                            if (!options.ignore_compatibility_version) {
                                if (this->compatibility_version.value != dylib_command_compatibility_version) {
                                    if (found_container_identification) {
                                        return creation_result::multiple_compatibility_versions;
                                    }
                                    
                                    if (container_index != 0) {
                                        return creation_result::compatibility_versions_mismatch;
                                    }
                                }

                                this->compatibility_version.value = dylib_command_compatibility_version;
                            }
                        }

                        // Check for when dylib_command_name_location
                        // overlaps with dylib-command load-command

                        if (dylib_command_name_location < sizeof(struct dylib_command)) {
                            return creation_result::invalid_dylib_command;
                        }

                        const auto dylib_command_cmdsize = iter.cmdsize();
                        const auto dylib_command_name_estimated_length = dylib_command_cmdsize - dylib_command_name_location;

                        if (dylib_command_cmdsize < sizeof(struct dylib_command)) {
                            return creation_result::invalid_dylib_command;
                        }

                        // Check for when dylib_command_name_location
                        // is past end or goes past end of dylib_command

                        if (dylib_command_name_estimated_length >= dylib_command_cmdsize || dylib_command_name_estimated_length == 0) {
                            return creation_result::invalid_dylib_command;
                        }

                        const auto dylib_command_name = &reinterpret_cast<const char *>(dylib_command)[dylib_command_name_location];
                        const auto dylib_command_name_length = strnlen(dylib_command_name, dylib_command_name_estimated_length);
                        const auto dylib_command_end = &dylib_command_name[dylib_command_name_length];

                        if (std::all_of(dylib_command_name, dylib_command_end, isspace)) {
                            return creation_result::empty_install_name;
                        }

                        if (!options.ignore_install_name) {
                            if (this->install_name.size() != 0) {
                                if (this->install_name.length() != dylib_command_name_length) {
                                    if (found_container_identification) {
                                        return creation_result::multiple_install_names;
                                    }

                                    if (container_index != 0) {
                                        return creation_result::install_name_mismatch;
                                    }
                                }

                                if (strncmp(this->install_name.c_str(), dylib_command_name, dylib_command_name_length) != 0) {
                                    if (found_container_identification) {
                                        return creation_result::multiple_install_names;
                                    }

                                    if (container_index != 0) {
                                        return creation_result::install_name_mismatch;
                                    }
                                }
                            }
                            
                            this->install_name.assign(dylib_command_name, dylib_command_name_length);
                        }
                        
                        found_container_identification = true;
                        break;
                    }

                    case macho::load_commands::reexport_dylib: {
                        if (options.ignore_reexports) {
                            break;
                        }
                        
                        if (options.ignore_exports) {
                            break;
                        }

                        const auto dylib_command = static_cast<const struct dylib_command *>(iter.load_command());
                        auto dylib_command_name_location = dylib_command->name.offset;

                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(dylib_command_name_location);
                        }

                        // Check for when dylib_command_name_location
                        // overlaps with dylib-command load-command

                        if (dylib_command_name_location < sizeof(struct dylib_command)) {
                            return creation_result::invalid_dylib_command;
                        }

                        const auto dylib_command_cmdsize = iter.cmdsize();
                        const auto dylib_command_name_estimated_length = dylib_command_cmdsize - dylib_command_name_location;

                        if (dylib_command_cmdsize < sizeof(struct dylib_command)) {
                            return creation_result::invalid_dylib_command;
                        }

                        // Check for when dylib_command_name_location overlaps with
                        // load-command, is past end or goes past end of dylib_command

                        if (dylib_command_name_estimated_length >= dylib_command_cmdsize || dylib_command_name_estimated_length == 0) {
                            return creation_result::invalid_dylib_command;
                        }

                        const auto dylib_command_name = &reinterpret_cast<const char *>(dylib_command)[dylib_command_name_location];

                        const auto dylib_command_name_length = strnlen(dylib_command_name, dylib_command_name_estimated_length);
                        const auto dylib_command_end = &dylib_command_name[dylib_command_name_length];

                        if (std::all_of(dylib_command_name, dylib_command_end, isspace)) {
                            continue;
                        }

                        // Assume dylib_command_name is
                        // not null-terminated

                        auto reexports_iter = this->reexports.begin();
                        const auto reexports_end = this->reexports.end();

                        for (; reexports_iter != reexports_end; reexports_iter++) {
                            if (reexports_iter->string.length() != dylib_command_name_length) {
                                continue;
                            }

                            if (strncmp(reexports_iter->string.c_str(), dylib_command_name, dylib_command_name_length) != 0) {
                                continue;
                            }

                            reexports_iter->add_architecture(architecture_info_index);
                            break;
                        }

                        if (reexports_iter == reexports_end) {
                            auto reexport_string = std::string(dylib_command_name, dylib_command_name_length);
                            auto &reexport = this->reexports.emplace_back(1ull << architecture_info_index, std::move(reexport_string));

                            reexport.add_architecture(architecture_info_index);
                        }

                        break;
                    }

                    case macho::load_commands::segment: {
                        // Ignore 32-bit containers
                        // on 64-bit containers

                        if (container.is_64_bit()) {
                            continue;
                        }
                        
                        if (options.ignore_objc_constraint && options.ignore_swift_version) {
                            break;
                        }
                        
                        if (options.ignore_unnecessary_fields_for_version) {
                            break;
                        }

                        const auto segment_command = static_cast<const struct segment_command *>(iter.load_command());

                        // A check for whether cmdsize is valid is not needed
                        // as utils::segments::command below does the check
                        // for us

                        // The segments below are where the objc
                        // runtime searches for the objc image-info

                        if (strncmp(segment_command->segname, "__DATA", sizeof(segment_command->segname)) != 0 &&
                            strncmp(segment_command->segname, "__DATA_CONST", sizeof(segment_command->segname)) != 0 &&
                            strncmp(segment_command->segname, "__DATA_DIRTY", sizeof(segment_command->segname)) != 0 &&
                            strncmp(segment_command->segname, "__OBJC", sizeof(segment_command->segname)) != 0) {
                            continue;
                        }

                        const auto segment = segments::command(segment_command, container.is_big_endian());
                        if (segment.sections.begin == nullptr) {
                            return creation_result::invalid_segment_command;
                        }

                        for (auto iter = segment.sections.begin; iter != segment.sections.end; iter++) {
                            // XXX: utils::segment is nice, but we still
                            // need to validate section information ourselves

                            if (strncmp(iter->sectname, "__objc_imageinfo", sizeof(iter->sectname)) != 0 &&
                                strncmp(iter->sectname, "__image_info", sizeof(iter->sectname)) != 0) {
                                continue;
                            }

                            auto section_location = iter->offset;
                            auto section_size = iter->size;

                            if (container.is_big_endian()) {
                                ::utils::swap::uint32(section_location);
                                ::utils::swap::uint32(section_size);
                            }

                            if (section_size < sizeof(objc::image_info)) {
                                return creation_result::invalid_objc_imageinfo_segment_section;
                            }

                            if (!::utils::range::is_valid_size_based_range(section_location, section_size)) {
                                return creation_result::invalid_objc_imageinfo_segment_section;
                            }

                            // XXX: We should validate section_data_range doesn't
                            // overlap container's header and load-commands buffer

                            const auto section_data_range = ::utils::range(section_location, section_location + section_size);
                            if (section_data_range.is_or_goes_past_end(container.size)) {
                                return creation_result::invalid_objc_imageinfo_segment_section;
                            }

                            if (!container.stream.seek(container.base + section_location, stream::file::seek_type::beginning)) {
                                return creation_result::stream_seek_error;
                            }

                            auto objc_image_info = objc::image_info();
                            if (!container.stream.read(&objc_image_info, sizeof(objc_image_info))) {
                                return creation_result::stream_read_error;
                            }

                            struct objc::image_info::flags objc_image_info_flags(objc_image_info.flags);
                            if (container.is_big_endian()) {
                                ::utils::swap::uint32(objc_image_info_flags.value);
                            }

                            if (!options.ignore_objc_constraint) {
                                auto objc_constraint = objc_constraint::retain_release;
                                if (objc_image_info_flags.requires_gc) {
                                    objc_constraint = objc_constraint::gc;
                                } else if (objc_image_info_flags.supports_gc) {
                                    objc_constraint = objc_constraint::retain_release_or_gc;
                                } else if (objc_image_info_flags.is_simulated) {
                                    objc_constraint = objc_constraint::retain_release_for_simulator;
                                }

                                if (this->objc_constraint != objc_constraint) {
                                    if (found_container_objc_information) {
                                        return creation_result::multiple_objc_constraint;
                                    }

                                    if (container_index != 0) {
                                        return creation_result::objc_constraint_mismatch;
                                    }
                                }

                                this->objc_constraint = objc_constraint;
                            }

                            if (!options.ignore_swift_version) {
                                if (this->swift_version != objc_image_info_flags.swift_version) {
                                    if (found_container_objc_information) {
                                        return creation_result::multiple_swift_version;
                                    }

                                    if (container_index != 0) {
                                        return creation_result::swift_version_mismatch;
                                    }
                                }

                                // A check here for when swift_version is
                                // zero is not needed as this->swift_version
                                // by default is zero

                                this->swift_version = objc_image_info_flags.swift_version;
                            }

                            found_container_objc_information = true;
                            break;
                        }

                        break;
                    }

                    case macho::load_commands::segment_64: {
                        // Ignore 64-bit containers
                        // on 32-bit containers

                        if (container.is_32_bit()) {
                            continue;
                        }
                        
                        if (options.ignore_objc_constraint && options.ignore_swift_version) {
                            break;
                        }
                        
                        if (options.ignore_unnecessary_fields_for_version) {
                            break;
                        }

                        const auto segment_command = static_cast<const struct segment_command_64 *>(iter.load_command());

                        // A check for whether cmdsize is valid is not needed
                        // as utils::segments::command_64 below does the check
                        // for us

                        // The segments below are where the objc
                        // runtime searches for the objc image-info

                        if (strncmp(segment_command->segname, "__DATA", sizeof(segment_command->segname)) != 0 &&
                            strncmp(segment_command->segname, "__DATA_CONST", sizeof(segment_command->segname)) != 0 &&
                            strncmp(segment_command->segname, "__DATA_DIRTY", sizeof(segment_command->segname)) != 0 &&
                            strncmp(segment_command->segname, "__OBJC", sizeof(segment_command->segname)) != 0) {
                            continue;
                        }

                        const auto segment = segments::command_64(segment_command, container.is_big_endian());
                        if (segment.sections.begin == nullptr) {
                            return creation_result::invalid_segment_command_64;
                        }

                        for (auto iter = segment.sections.begin; iter != segment.sections.end; iter++) {
                            // XXX: utils::segment is nice, but we still need to validate section
                            // information ourselves

                            if (strncmp(iter->sectname, "__objc_imageinfo", sizeof(iter->sectname)) != 0 &&
                                strncmp(iter->sectname, "__image_info", sizeof(iter->sectname)) != 0) {
                                continue;
                            }

                            auto section_location = iter->offset;
                            auto section_size = iter->size;

                            if (container.is_big_endian()) {
                                ::utils::swap::uint32(section_location);
                                ::utils::swap::uint64(section_size);
                            }

                            if (!::utils::range::is_valid_size_based_range(section_location, section_size)) {
                                return creation_result::invalid_objc_imageinfo_segment_section;
                            }

                            // XXX: We should validate section_data_range doesn't
                            // overlap container's header and load-commands buffer

                            const auto section_range = ::utils::range(section_location, section_location + section_size);
                            if (section_range.is_or_goes_past_end(container.size)) {
                                return creation_result::invalid_objc_imageinfo_segment_section;
                            }

                            if (!container.stream.seek(container.base + section_location, stream::file::seek_type::beginning)) {
                                return creation_result::stream_seek_error;
                            }

                            auto objc_image_info = objc::image_info();
                            if (!container.stream.read(&objc_image_info, sizeof(objc_image_info))) {
                                return creation_result::stream_read_error;
                            }

                            struct objc::image_info::flags objc_image_info_flags(objc_image_info.flags);
                            if (container.is_big_endian()) {
                                ::utils::swap::uint32(objc_image_info_flags.value);
                            }

                            if (!options.ignore_objc_constraint) {
                                auto objc_constraint = objc_constraint::retain_release;
                                if (objc_image_info_flags.requires_gc) {
                                    objc_constraint = objc_constraint::gc;
                                } else if (objc_image_info_flags.supports_gc) {
                                    objc_constraint = objc_constraint::retain_release_or_gc;
                                } else if (objc_image_info_flags.is_simulated) {
                                    objc_constraint = objc_constraint::retain_release_for_simulator;
                                }

                                if (this->objc_constraint != objc_constraint) {
                                    if (found_container_objc_information) {
                                        return creation_result::multiple_objc_constraint;
                                    }

                                    if (container_index != 0) {
                                        return creation_result::objc_constraint_mismatch;
                                    }
                                }

                                this->objc_constraint = objc_constraint;
                            }

                            if (!options.ignore_swift_version) {
                                if (this->swift_version != objc_image_info_flags.swift_version) {
                                    if (found_container_objc_information) {
                                        return creation_result::multiple_swift_version;
                                    }

                                    if (container_index != 0) {
                                        return creation_result::swift_version_mismatch;
                                    }
                                }

                                // A check here for when swift_version is
                                // zero is not needed as this->swift_version
                                // by default is zero

                                this->swift_version = objc_image_info_flags.swift_version;
                            }

                            found_container_objc_information = true;
                            break;
                        }

                        break;
                    }

                    case macho::load_commands::sub_client: {
                        if (options.ignore_allowable_clients) {
                            break;
                        }
                        
                        if (options.ignore_unnecessary_fields_for_version) {
                            break;
                        }

                        const auto sub_client_command = static_cast<const struct sub_client_command *>(iter.load_command());
                        auto sub_client_command_name_location = sub_client_command->client.offset;

                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(sub_client_command_name_location);
                        }

                        // Check for when sub_client_command_name_location
                        // overlaps with sub-client load-command

                        if (sub_client_command_name_location < sizeof(struct sub_client_command)) {
                            return creation_result::invalid_sub_client_command;
                        }

                        const auto sub_client_command_cmdsize = iter.cmdsize();
                        const auto sub_client_command_name_estimated_length = sub_client_command_cmdsize - sub_client_command_name_location;

                        if (sub_client_command_cmdsize < sizeof(struct sub_client_command)) {
                            return creation_result::invalid_sub_client_command;
                        }

                        // Check for when sub_client_command_name_location
                        // is past end or goes past end of dylib-command

                        if (sub_client_command_name_estimated_length == 0 || sub_client_command_name_estimated_length >= sub_client_command_cmdsize) {
                            return creation_result::invalid_sub_client_command;
                        }

                        const auto sub_client_command_name = &reinterpret_cast<const char *>(sub_client_command)[sub_client_command_name_location];

                        const auto sub_client_command_name_length = strnlen(sub_client_command_name, sub_client_command_name_estimated_length);
                        const auto sub_client_command_end = &sub_client_command_name[sub_client_command_name_length];

                        if (std::all_of(sub_client_command_name, sub_client_command_end, isspace)) {
                            continue;
                        }

                        // Assume sub_client_command_name
                        // is not null-terminated

                        auto sub_clients_iter = this->sub_clients.begin();
                        const auto sub_clients_end = this->sub_clients.end();

                        for (; sub_clients_iter != sub_clients_end; sub_clients_iter++) {
                            if (sub_clients_iter->client.length() != sub_client_command_name_length) {
                                continue;
                            }

                            if (strncmp(sub_clients_iter->client.c_str(), sub_client_command_name, sub_client_command_name_length) != 0) {
                                continue;
                            }

                            sub_clients_iter->add_architecture(architecture_info_index);
                        }

                        // In case a sub-client
                        // was not found

                        if (sub_clients_iter == sub_clients_end) {
                            auto sub_client_string = std::string(sub_client_command_name, sub_client_command_name_length);
                            auto &sub_client = sub_clients.emplace_back(1ull << architecture_info_index, std::move(sub_client_string));

                            sub_client.add_architecture(architecture_info_index);
                        }

                        break;
                    }

                    case macho::load_commands::sub_umbrella: {
                        if (options.ignore_parent_umbrella) {
                            break;
                        }
                        
                        // tbd field parent-umbrella is
                        // only available on tbd-version v2

                        if (!options.parse_unsupported_fields_for_version) {
                            if (this->version != version::v2) {
                                break;
                            }
                        }
                        
                        if (options.ignore_unnecessary_fields_for_version) {
                            break;
                        }

                        const auto sub_umbrella_command = static_cast<const struct sub_umbrella_command *>(iter.load_command());
                        auto sub_umbrella_command_name_location = sub_umbrella_command->sub_umbrella.offset;

                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(sub_umbrella_command_name_location);
                        }

                        // Check for when sub_umbrella_command_name_location
                        // overlaps with sub-client load-command

                        if (sub_umbrella_command_name_location < sizeof(struct sub_umbrella_command)) {
                            return creation_result::invalid_sub_umbrella_command;
                        }

                        const auto sub_umbrella_command_cmdsize = iter.cmdsize();
                        const auto sub_umbrella_command_name_estimated_length = sub_umbrella_command_cmdsize - sub_umbrella_command_name_location;

                        if (sub_umbrella_command_cmdsize < sizeof(struct sub_umbrella_command)) {
                            return creation_result::invalid_sub_umbrella_command;
                        }

                        // Check for when sub_umbrella_command_name_location overlaps with
                        // load-command, is past end or goes past end of sub_umbrella_command

                        if (sub_umbrella_command_name_estimated_length >= sub_umbrella_command_cmdsize || sub_umbrella_command_name_estimated_length == 0) {
                            return creation_result::invalid_sub_umbrella_command;
                        }

                        const auto sub_umbrella_command_name = &reinterpret_cast<const char *>(sub_umbrella_command)[sub_umbrella_command_name_location];

                        const auto sub_umbrella_command_name_length = strnlen(sub_umbrella_command_name, sub_umbrella_command_name_estimated_length);
                        const auto sub_umbrella_command_end = &sub_umbrella_command_name[sub_umbrella_command_name_length];

                        if (std::all_of(sub_umbrella_command_name, sub_umbrella_command_end, isspace)) {
                            return creation_result::empty_parent_umbrella;
                        }

                        if (this->parent_umbrella.length() != sub_umbrella_command_name_length) {
                            if (found_container_parent_umbrella) {
                                return creation_result::multiple_parent_umbrella;
                            }

                            if (container_index != 0) {
                                return creation_result::parent_umbrella_mismatch;
                            }
                        }

                        if (strncmp(this->parent_umbrella.c_str(), sub_umbrella_command_name, sub_umbrella_command_name_length) != 0) {
                            if (found_container_parent_umbrella) {
                                return creation_result::multiple_parent_umbrella;
                            }

                            if (container_index != 0) {
                                return creation_result::parent_umbrella_mismatch;
                            }
                        }

                        this->parent_umbrella.assign(sub_umbrella_command_name, sub_umbrella_command_name_length);
                        break;
                    }

                    case macho::load_commands::symbol_table: {
                        if (options.ignore_exports) {
                            break;
                        }

                        // XXX: We may want to return multiple_symbol_tables
                        // in the scenario where another symbol-table has
                        // been found, instead of returning invalid_symtab_command

                        const auto cmdsize = iter.cmdsize();
                        if (cmdsize != sizeof(struct symtab_command)) {
                            return creation_result::invalid_symtab_command;
                        }

                        if (symtab_command != nullptr) {
                            // Ignore multiple symbol-tables
                            // if they both match up

                            if (memcmp(symtab_command, iter.load_command(), sizeof(struct symtab_command)) == 0) {
                                break;
                            }

                            return creation_result::multiple_symbol_tables;
                        }

                        symtab_command = static_cast<const struct symtab_command *>(iter.load_command());
                        break;
                    }

                    case macho::load_commands::uuid: {
                        if (options.ignore_uuids) {
                            break;
                        }

                        // tbd field uuids is only
                        // available on tbd-version v2

                        if (!options.parse_unsupported_fields_for_version) {
                            if (this->version != version::v2) {
                                break;
                            }
                        }
                        
                        if (options.ignore_unnecessary_fields_for_version) {
                            break;
                        }

                        const auto uuid_command = static_cast<const struct uuid_command *>(iter.load_command());
                        const auto uuids_end = this->uuids.cend();

                        if (this->uuids.size() != container_index) {
                            // Ignore if multiple uuid-commands
                            // are found with matching uuids

                            const auto &uuid = this->uuids.back();
                            if (memcmp(uuid.uuid(), uuid_command->uuid, sizeof(uuid_command->uuid)) != 0) {
                                return creation_result::multiple_uuids;
                            }

                            break;
                        }

                        if (!options.ignore_non_unique_uuids) {
                            // Search for same uuids across other containers
                            // in order to guarantee uniqueness

                            auto uuids_iter = this->uuids.cbegin();
                            for (; uuids_iter != uuids_end; uuids_iter++) {
                                if (memcmp(uuids_iter->uuid(), uuid_command->uuid, sizeof(uuid_command->uuid)) != 0) {
                                    continue;
                                }

                                break;
                            }

                            if (uuids_iter != uuids_end) {
                                return creation_result::non_unique_uuid;
                            }
                        }

                        auto uuid = uuid_pair();

                        // Make a copy of uuid so as the container's
                        // load-command data storage is only temporary

                        uuid.architecture = architecture_info_from_index(architecture_info_index);
                        uuid.set_uuid(uuid_command->uuid);

                        this->uuids.emplace_back(std::move(uuid));
                        break;
                    }

                    case macho::load_commands::version_min_macosx:
                        if (options.ignore_platform) {
                            break;
                        }

                        if (this->platform != platform::macosx) {
                            if (found_container_platform) {
                                return creation_result::multiple_platforms;
                            }

                            if (container_index != 0) {
                                return creation_result::platforms_mismatch;
                            }
                        }

                        this->platform = platform::macosx;
                        found_container_platform = true;

                        break;

                    case macho::load_commands::version_min_iphoneos:
                        if (options.ignore_platform) {
                            break;
                        }

                        if (this->platform != platform::ios) {
                            if (found_container_platform) {
                                return creation_result::multiple_platforms;
                            }

                            if (container_index != 0) {
                                return creation_result::platforms_mismatch;
                            }
                        }

                        this->platform = platform::ios;
                        found_container_platform = true;

                        break;

                    case macho::load_commands::version_min_watchos:
                        if (options.ignore_platform) {
                            break;
                        }

                        if (this->platform != platform::watchos) {
                            if (found_container_platform) {
                                return creation_result::multiple_platforms;
                            }

                            if (container_index != 0) {
                                return creation_result::platforms_mismatch;
                            }
                        }

                        this->platform = platform::watchos;
                        found_container_platform = true;

                        break;

                    case macho::load_commands::version_min_tvos:
                        if (options.ignore_platform) {
                            break;
                        }

                        if (this->platform != platform::tvos) {
                            if (found_container_platform) {
                                return creation_result::multiple_platforms;
                            }

                            if (container_index != 0) {
                                return creation_result::platforms_mismatch;
                            }
                        }

                        this->platform = platform::tvos;
                        found_container_platform = true;

                        break;

                    default:
                        continue;
                }
            }

            // Check for the required fields, needed whether
            // or not the first container has them, but ignore
            // if provided options demand so

            if (!options.ignore_missing_identification) {
                if (!options.ignore_current_version || !options.ignore_compatibility_version || !options.ignore_install_name) {
                    if (!found_container_identification) {
                        return creation_result::container_is_missing_dynamic_library_identification;
                    }
                }
            }

            if (!options.ignore_platform && !options.ignore_missing_platform) {
                if (!found_container_platform) {
                    return creation_result::container_is_missing_platform;
                }
            }

            if (!options.ignore_unnecessary_fields_for_version) {
                if (!options.ignore_uuids && !options.ignore_missing_uuids) {
                    if (this->uuids.size() != (container_index + 1)) {
                        if (this->version == version::v2 || options.parse_unsupported_fields_for_version) {
                            return creation_result::container_is_missing_uuid;
                        }
                    }
                }
            }

            // Checks here are to make sure the same information
            // was found between containers.

            // This has to be done here as load-command iteration
            // can't be certain every load-command present in the
            // first container was found in later containers

            if (container_index != 0) {
                // A check for swift-version isn't needed as
                // they are both found in the same load-command

                if ((this->objc_constraint == objc_constraint::none) == found_container_objc_information) {
                    return creation_result::objc_constraint_mismatch;
                }

                if (this->parent_umbrella.empty() == found_container_parent_umbrella) {
                    return creation_result::parent_umbrella_mismatch;
                }
            }

            if (!options.ignore_exports) {
                if (!symtab_command) {
                    if (!options.ignore_missing_symbol_table) {
                        return creation_result::container_is_missing_symbol_table;
                    }

                    container_index++;
                    continue;
                }

                auto string_table = strings::table::data();
                switch (string_table.create(container, *symtab_command, strings::table::data::options())) {
                    case strings::table::data::creation_result::ok:
                        break;

                    default:
                        return creation_result::failed_to_retrieve_string_table;
                }

                if (container.is_64_bit()) {
                    auto symbol_table = symbols::table_64::data();
                    auto symbol_table_options = symbols::table_64::data::options();

                    symbol_table_options.convert_to_little_endian = true;

                    switch (symbol_table.create(container, *symtab_command, symbol_table_options)) {
                        case symbols::table_64::data::creation_result::ok:
                            break;

                        default:
                            return creation_result::failed_to_retrieve_symbol_table;
                    }

                    const auto string_table_size = string_table.size();
                    for (auto iter = symbol_table.begin; iter != symbol_table.end; iter++) {
                        if (iter->n_type.type != symbol_table::type::section) {
                            continue;
                        }

                        auto n_desc = iter->n_desc;
                        auto index = iter->n_un.n_strx;

                        // XXX: We might want to not ignore symbols
                        // whose strings are past end of string-table

                        if (index >= string_table_size) {
                            continue;
                        }

                        // Skip size 0 strings

                        auto string = &string_table[index];
                        if (*string == '\0') {
                            continue;
                        }

                        // Skip empty strings

                        // Assume that the string isn't null-terminated to
                        // avoid resulting in undefined behavior when copying

                        const auto string_length = strnlen(string, string_table_size - index);
                        if (std::all_of(string, &string[string_length], isspace)) {
                            continue;
                        }

                        const auto type = symbol::type_from_symbol_string(string, n_desc);
                        switch (type) {
                            case symbol::type::normal:
                                if (!options.allow_private_normal_symbols) {
                                    if (!iter->n_type.external) {
                                        continue;
                                    }
                                }

                                break;

                            case symbol::type::weak:
                                if (!options.allow_private_weak_symbols) {
                                    if (!iter->n_type.external) {
                                        continue;
                                    }
                                }

                                break;

                            case symbol::type::objc_class:
                                if (!options.allow_private_objc_class_symbols) {
                                    if (!iter->n_type.external) {
                                        continue;
                                    }
                                }

                                break;

                            case symbol::type::objc_ivar:
                                if (!options.allow_private_objc_ivar_symbols) {
                                    if (!iter->n_type.external) {
                                        continue;
                                    }
                                }

                                break;
                        }

                        const auto symbols_end = this->symbols.end();

                        // An optimization can be made here to remove
                        // searching for the first container, but that
                        // can also result in multiple symbols with the
                        // raw-string (and hence the same symbol-type)
                        // to be in the symbols-count

                        // That can be averted however by null-terminating
                        // the first byte of every string we come across leading
                        // to those symbols with strings to be skipped, but if we
                        // in the future are to allow the caller to provide the
                        // string-buffer that cannot happen

                        auto symbols_iter = this->symbols.begin();
                        for (; symbols_iter != symbols_end; symbols_iter++) {
                            // As we separate out symbol-prefixes identifying the
                            // type of a symbol, the types have to be checked as well

                            // XXX: We may not want to allow multiple symbols where
                            // one is weak, as weak-symbols are notidentified through
                            // their string

                            if (symbols_iter->type != type) {
                                continue;
                            }

                            if (symbols_iter->string.length() != string_length) {
                                continue;
                            }

                            if (strncmp(symbols_iter->string.c_str(), string, string_length) != 0) {
                                continue;
                            }

                            symbols_iter->add_architecture(architecture_info_index);
                            break;
                        }

                        if (symbols_iter == symbols_end) {
                            auto symbol_string = std::string(string, string_length);
                            auto &symbol = this->symbols.emplace_back(1ull << architecture_info_index, std::move(symbol_string), type);

                            symbol.add_architecture(architecture_info_index);
                        }
                    }
                } else {
                    auto symbol_table = symbols::table::data();
                    auto symbol_table_options = symbols::table::data::options();

                    symbol_table_options.convert_to_little_endian = true;

                    switch (symbol_table.create(container, *symtab_command, symbol_table_options)) {
                        case symbols::table::data::creation_result::ok:
                            break;

                        default:
                            return creation_result::failed_to_retrieve_symbol_table;
                    }

                    const auto string_table_size = string_table.size();
                    for (auto iter = symbol_table.begin; iter != symbol_table.end; iter++) {
                        if (iter->n_type.type != symbol_table::type::section) {
                            continue;
                        }

                        auto n_desc = iter->n_desc;
                        auto index = iter->n_un.n_strx;

                        // XXX: We might want to not ignore symbols
                        // whose strings are past end of string-table

                        if (index >= string_table_size) {
                            continue;
                        }

                        // Skip empty strings

                        auto string = &string_table[index];
                        if (*string == '\0') {
                            continue;
                        }

                        const auto type = symbol::type_from_symbol_string(string, n_desc);
                        switch (type) {
                            case symbol::type::normal:
                                if (!options.allow_private_normal_symbols) {
                                    if (!iter->n_type.external) {
                                        continue;
                                    }
                                }

                                break;

                            case symbol::type::weak:
                                if (!options.allow_private_weak_symbols) {
                                    if (!iter->n_type.external) {
                                        continue;
                                    }
                                }

                                break;

                            case symbol::type::objc_class:
                                if (!options.allow_private_objc_class_symbols) {
                                    if (!iter->n_type.external) {
                                        continue;
                                    }
                                }

                                break;

                            case symbol::type::objc_ivar:
                                if (!options.allow_private_objc_ivar_symbols) {
                                    if (!iter->n_type.external) {
                                        continue;
                                    }
                                }

                                break;
                        }

                        // Assume that the string isn't null-terminated to
                        // avoid resulting in undefined behavior when copying

                        const auto string_length = strnlen(string, string_table_size - index);
                        const auto symbols_end = this->symbols.end();

                        // An optimization can be made here to remove
                        // searching for the first container, but that
                        // can also result in multiple symbols with the
                        // raw-string (and hence the same symbol-type),
                        // and present in the same container, to be in
                        // the symbols vector

                        // That can be averted however, by null-terminating
                        // the first byte of every string we come across, leading
                        // to those symbols with strings to be skipped. But if we
                        // in the future are to allow the caller to provide the
                        // string-buffer, that cannot happen

                        auto symbols_iter = this->symbols.begin();
                        for (; symbols_iter != symbols_end; symbols_iter++) {
                            // As we separate out symbol-prefixes identifying the
                            // type of a symbol, the types have to be checked as well

                            // XXX: We may not want to allow multiple symbols
                            // where one is weak, as weak-symbols are not
                            // identified through their string, and as such
                            // can result in multiple symbols of the exact
                            // same raw-string present in the symbols-vector

                            if (symbols_iter->type != type) {
                                continue;
                            }

                            if (symbols_iter->string.length() != string_length) {
                                continue;
                            }

                            if (strncmp(symbols_iter->string.c_str(), string, string_length) != 0) {
                                continue;
                            }

                            symbols_iter->add_architecture(architecture_info_index);
                            break;
                        }

                        if (symbols_iter == symbols_end) {
                            auto symbol_string = std::string(string, string_length);
                            auto &symbol = this->symbols.emplace_back(1ull << architecture_info_index, std::move(symbol_string), type);

                            symbol.add_architecture(architecture_info_index);
                        }
                    }
                }
            }

            container_index++;
        }

        std::sort(this->reexports.begin(), this->reexports.end(), [](const reexport &lhs, const reexport &rhs) {
            return lhs.string.compare(rhs.string) < 0;
        });

        std::sort(this->symbols.begin(), this->symbols.end(), [](const symbol &lhs, const symbol &rhs) {
            return lhs.string.compare(rhs.string) < 0;
        });

        return creation_result::ok;
    }

    tbd::creation_result tbd::create(const container &container, load_commands::data &load_commands, symbols::table::data &symbol_table, strings::table::data &string_table, const creation_options &options) noexcept {
        auto container_cputype = container.header.cputype;
        auto container_cpusubtype = container.header.cpusubtype;

        if (container.is_big_endian()) {
            swap::cputype(container_cputype);
            ::utils::swap::int32(container_cpusubtype);
        }

        const auto container_subtype = subtype_from_cputype(macho::cputype(container_cputype), container_cpusubtype);
        if (container_subtype == subtype::none) {
            return creation_result::invalid_container_header_subtype;
        }

        const auto architecture_info_index = macho::architecture_info_index_from_cputype(macho::cputype(container_cputype), container_subtype);
        if (architecture_info_index == -1) {
            return creation_result::unrecognized_container_cputype_subtype_pair;
        }

        const auto architecture_info_index_bit = (1ull << architecture_info_index);
        if (this->architectures & architecture_info_index_bit) {
            return creation_result::multiple_containers_for_same_architecture;
        }
        
        this->architectures |= architecture_info_index_bit;
        
        if (!options.ignore_unnecessary_fields_for_version) {
            if (!options.ignore_flags) {
                if (this->version == version::v2 || options.parse_unsupported_fields_for_version) {
                    struct tbd::flags flags;
                    
                    flags.flat_namespace = container.header.flags.two_level_namespaces;
                    flags.not_app_extension_safe = !container.header.flags.app_extension_safe;
                    
                    this->flags = flags;
                }
            }
        }

        for (auto iter = load_commands.begin; iter != load_commands.end; iter++) {
            const auto cmd = iter.cmd();
            switch (cmd) {
                case macho::load_commands::build_version: {
                    if (options.ignore_platform) {
                        break;
                    }

                    const auto build_version_command = static_cast<const struct build_version_command *>(iter.load_command());
                    auto build_version_platform = build_version_command->platform;

                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(build_version_platform);
                    }

                    // XXX: Maybe we shouldn't ignore when build_version
                    // command is larger than it should be

                    const auto cmdsize = iter.cmdsize();
                    if (cmdsize < sizeof(struct build_version_command)) {
                        return creation_result::invalid_build_version_command;
                    }

                    auto parsed_platform = platform::none;
                    switch (build_version::platform(build_version_platform)) {
                        case build_version::platform::macos:
                            parsed_platform = platform::macosx;
                            break;

                        case build_version::platform::ios:
                            parsed_platform = platform::ios;
                            break;

                        case build_version::platform::watchos:
                            parsed_platform = platform::watchos;
                            break;

                        case build_version::platform::tvos:
                            parsed_platform = platform::tvos;
                            break;

                        case build_version::platform::bridgeos:
                            return creation_result::unsupported_platform;

                        default:
                            return creation_result::unrecognized_platform;
                    }

                    if (this->platform != platform::none) {
                        if (this->platform != parsed_platform) {
                            return creation_result::multiple_platforms;
                        }
                    }

                    this->platform = parsed_platform;
                    break;
                }

                case macho::load_commands::identification_dylib: {
                    if (options.ignore_current_version && options.ignore_compatibility_version && options.ignore_install_name) {
                        break;
                    }

                    const auto dylib_command = static_cast<const struct dylib_command *>(iter.load_command());

                    auto dylib_command_name_location = dylib_command->name.offset;
                    auto dylib_command_current_version = dylib_command->current_version;
                    auto dylib_command_compatibility_version = dylib_command->compatibility_version;

                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(dylib_command_name_location);
                        ::utils::swap::uint32(dylib_command_current_version);
                        ::utils::swap::uint32(dylib_command_compatibility_version);
                    }

                    if (!options.ignore_unnecessary_fields_for_version) {
                        if (!options.ignore_current_version) {
                            if (this->current_version.value != dylib_command_current_version) {
                                // Use install-name to indicate whether
                                // or not we've parsed identification in
                                // the past, as there is no "no-value"
                                // current-version
                                
                                if (this->install_name.length() != 0) {
                                    return creation_result::multiple_current_versions;
                                }
                            }
                        }
                        
                        if (!options.ignore_compatibility_version) {
                            if (this->compatibility_version.value != dylib_command_compatibility_version) {
                                // Use install-name to indicate whether
                                // or not we've parsed identification in
                                // the past, as there is no "no-value"
                                // compatibility-version
                                
                                if (this->install_name.length() != 0) {
                                    return creation_result::multiple_compatibility_versions;
                                }
                            }
                        }
                        
                        this->current_version.value = dylib_command_current_version;
                        this->compatibility_version.value = dylib_command_compatibility_version;
                    }

                    // Check for when dylib_command_name_location
                    // overlaps with dylib-command load-command

                    if (dylib_command_name_location < sizeof(struct dylib_command)) {
                        return creation_result::invalid_dylib_command;
                    }

                    const auto dylib_command_cmdsize = iter.cmdsize();
                    const auto dylib_command_name_estimated_length = dylib_command_cmdsize - dylib_command_name_location;

                    if (dylib_command_cmdsize < sizeof(struct dylib_command)) {
                        return creation_result::invalid_dylib_command;
                    }

                    // Check for when dylib_command_name_location
                    // is past end or goes past end of dylib_command

                    if (dylib_command_name_estimated_length >= dylib_command_cmdsize || dylib_command_name_estimated_length == 0) {
                        return creation_result::invalid_dylib_command;
                    }

                    const auto dylib_command_name = &reinterpret_cast<const char *>(dylib_command)[dylib_command_name_location];

                    const auto dylib_command_name_length = strnlen(dylib_command_name, dylib_command_name_estimated_length);
                    const auto dylib_command_end = &dylib_command_name[dylib_command_name_length];

                    if (std::all_of(dylib_command_name, dylib_command_end, isspace)) {
                        return creation_result::empty_install_name;
                    }

                    if (!options.ignore_install_name) {
                        const auto install_name_length = this->install_name.length();
                        if (install_name_length != 0) {
                            if (install_name_length != dylib_command_name_length) {
                                return creation_result::multiple_install_names;
                            }

                            if (strncmp(this->install_name.c_str(), dylib_command_name, dylib_command_name_length) != 0) {
                                return creation_result::multiple_install_names;
                            }
                        }
                    
                        this->install_name.assign(dylib_command_name, dylib_command_name_length);
                    }

                    break;
                }

                case macho::load_commands::reexport_dylib: {
                    if (options.ignore_reexports) {
                        break;
                    }
                    
                    if (options.ignore_exports) {
                        break;
                    }

                    const auto dylib_command = static_cast<const struct dylib_command *>(iter.load_command());
                    auto dylib_command_name_location = dylib_command->name.offset;

                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(dylib_command_name_location);
                    }

                    // Check for when dylib_command_name_location
                    // overlaps with dylib-command load-command

                    if (dylib_command_name_location < sizeof(struct dylib_command)) {
                        return creation_result::invalid_dylib_command;
                    }

                    const auto dylib_command_cmdsize = iter.cmdsize();
                    const auto dylib_command_name_estimated_length = dylib_command_cmdsize - dylib_command_name_location;

                    if (dylib_command_cmdsize < sizeof(struct dylib_command)) {
                        return creation_result::invalid_dylib_command;
                    }

                    // Check for when dylib_command_name_location overlaps with
                    // load-command, is past end or goes past end of dylib_command

                    if (dylib_command_name_estimated_length >= dylib_command_cmdsize || dylib_command_name_estimated_length == 0) {
                        return creation_result::invalid_dylib_command;
                    }

                    const auto dylib_command_name = &reinterpret_cast<const char *>(dylib_command)[dylib_command_name_location];

                    const auto dylib_command_name_length = strnlen(dylib_command_name, dylib_command_name_estimated_length);
                    const auto dylib_command_end = &dylib_command_name[dylib_command_name_length];

                    if (std::all_of(dylib_command_name, dylib_command_end, isspace)) {
                        continue;
                    }

                    // Assume dylib_command_name is
                    // not null-terminated

                    this->reexports.emplace_back(1ull << architecture_info_index, std::string(dylib_command_name, dylib_command_name_length));
                    break;
                }

                case macho::load_commands::segment: {
                    // Ignore 32-bit containers
                    // on 64-bit containers

                    if (container.is_64_bit()) {
                        continue;
                    }

                    if (options.ignore_objc_constraint && options.ignore_swift_version) {
                        break;
                    }

                    // tbd fields objc-constraint and swift-version
                    // are only found on tbd-version v2

                    if (!options.parse_unsupported_fields_for_version) {
                        if (this->version != version::v2) {
                            break;
                        }
                    }

                    if (options.ignore_unnecessary_fields_for_version) {
                        break;
                    }
                    
                    const auto segment_command = static_cast<const struct segment_command *>(iter.load_command());

                    // A check for whether cmdsize is valid is not needed
                    // as utils::segments::command below does the check
                    // for us

                    // The segments below are where the objc
                    // runtime searches for the objc image-info

                    if (strncmp(segment_command->segname, "__DATA", sizeof(segment_command->segname)) != 0 &&
                        strncmp(segment_command->segname, "__DATA_CONST", sizeof(segment_command->segname)) != 0 &&
                        strncmp(segment_command->segname, "__DATA_DIRTY", sizeof(segment_command->segname)) != 0 &&
                        strncmp(segment_command->segname, "__OBJC", sizeof(segment_command->segname)) != 0) {
                        continue;
                    }

                    const auto segment = segments::command(segment_command, container.is_big_endian());
                    if (segment.sections.begin == nullptr) {
                        return creation_result::invalid_segment_command;
                    }

                    for (auto iter = segment.sections.begin; iter != segment.sections.end; iter++) {
                        // XXX: utils::segment is nice, but we still
                        // need to validate section information ourselves

                        if (strncmp(iter->sectname, "__objc_imageinfo", sizeof(iter->sectname)) != 0 &&
                            strncmp(iter->sectname, "__image_info", sizeof(iter->sectname)) != 0) {
                            continue;
                        }

                        auto section_location = iter->offset;
                        auto section_size = iter->size;

                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(section_location);
                            ::utils::swap::uint32(section_size);
                        }

                        if (section_size < sizeof(objc::image_info)) {
                            return creation_result::invalid_objc_imageinfo_segment_section;
                        }

                        if (!::utils::range::is_valid_size_based_range(section_location, section_size)) {
                            return creation_result::invalid_objc_imageinfo_segment_section;
                        }

                        // XXX: We should validate section_data_range doesn't
                        // overlap container's header and load-commands buffer

                        const auto section_data_range = ::utils::range(section_location, section_location + section_size);
                        if (section_data_range.is_or_goes_past_end(container.size)) {
                            return creation_result::invalid_objc_imageinfo_segment_section;
                        }

                        if (!container.stream.seek(container.base + section_location, stream::file::seek_type::beginning)) {
                            return creation_result::stream_seek_error;
                        }

                        auto objc_image_info = objc::image_info();
                        if (!container.stream.read(&objc_image_info, sizeof(objc_image_info))) {
                            return creation_result::stream_read_error;
                        }

                        struct objc::image_info::flags objc_image_info_flags(objc_image_info.flags);
                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(objc_image_info_flags.value);
                        }

                        if (!options.ignore_objc_constraint) {
                            auto objc_constraint = objc_constraint::retain_release;
                            if (objc_image_info_flags.requires_gc) {
                                objc_constraint = objc_constraint::gc;
                            } else if (objc_image_info_flags.supports_gc) {
                                objc_constraint = objc_constraint::retain_release_or_gc;
                            } else if (objc_image_info_flags.is_simulated) {
                                objc_constraint = objc_constraint::retain_release_for_simulator;
                            }

                            if (this->objc_constraint != objc_constraint::none) {
                                if (this->objc_constraint != objc_constraint) {
                                    return creation_result::multiple_objc_constraint;
                                }
                            }

                            this->objc_constraint = objc_constraint;
                        }

                        if (!options.ignore_swift_version) {
                            if (this->swift_version != 0) {
                                if (this->swift_version != objc_image_info_flags.swift_version) {
                                    return creation_result::multiple_swift_version;
                                }
                            }

                            // A check here for when swift_version is
                            // zero is not needed as this->swift_version
                            // by default is zero

                            this->swift_version = objc_image_info_flags.swift_version;
                        }

                        break;
                    }

                    break;
                }

                case macho::load_commands::segment_64: {
                    // Ignore 64-bit containers
                    // on 32-bit containers

                    if (container.is_32_bit()) {
                        continue;
                    }

                    if (options.ignore_objc_constraint && options.ignore_swift_version) {
                        break;
                    }
                    
                    if (!options.ignore_unnecessary_fields_for_version) {
                        break;
                    }

                    const auto segment_command = static_cast<const struct segment_command_64 *>(iter.load_command());

                    // A check for whether cmdsize is valid is not needed
                    // as utils::segments::command_64 below does the check
                    // for us

                    // The segments below are where the objc
                    // runtime searches for the objc image-info

                    if (strncmp(segment_command->segname, "__DATA", sizeof(segment_command->segname)) != 0 &&
                        strncmp(segment_command->segname, "__DATA_CONST", sizeof(segment_command->segname)) != 0 &&
                        strncmp(segment_command->segname, "__DATA_DIRTY", sizeof(segment_command->segname)) != 0 &&
                        strncmp(segment_command->segname, "__OBJC", sizeof(segment_command->segname)) != 0) {
                        continue;
                    }

                    const auto segment = segments::command_64(segment_command, container.is_big_endian());
                    if (segment.sections.begin == nullptr) {
                        return creation_result::invalid_segment_command_64;
                    }

                    for (auto iter = segment.sections.begin; iter != segment.sections.end; iter++) {
                        // XXX: utils::segment is nice, but we still need to validate section
                        // information ourselves

                        if (strncmp(iter->sectname, "__objc_imageinfo", sizeof(iter->sectname)) != 0 &&
                            strncmp(iter->sectname, "__image_info", sizeof(iter->sectname)) != 0) {
                            continue;
                        }

                        auto section_location = iter->offset;
                        auto section_size = iter->size;

                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(section_location);
                            ::utils::swap::uint64(section_size);
                        }

                        if (!::utils::range::is_valid_size_based_range(section_location, section_size)) {
                            return creation_result::invalid_objc_imageinfo_segment_section;
                        }

                        // XXX: We should validate section_data_range doesn't
                        // overlap container's header and load-commands buffer

                        const auto section_range = ::utils::range(section_location, section_location + section_size);
                        if (section_range.is_or_goes_past_end(container.size)) {
                            return creation_result::invalid_objc_imageinfo_segment_section;
                        }

                        if (!container.stream.seek(container.base + section_location, stream::file::seek_type::beginning)) {
                            return creation_result::stream_seek_error;
                        }

                        auto objc_image_info = objc::image_info();
                        if (!container.stream.read(&objc_image_info, sizeof(objc_image_info))) {
                            return creation_result::stream_read_error;
                        }

                        struct objc::image_info::flags objc_image_info_flags(objc_image_info.flags);
                        if (container.is_big_endian()) {
                            ::utils::swap::uint32(objc_image_info_flags.value);
                        }

                        if (!options.ignore_objc_constraint) {
                            auto objc_constraint = objc_constraint::retain_release;
                            if (objc_image_info_flags.requires_gc) {
                                objc_constraint = objc_constraint::gc;
                            } else if (objc_image_info_flags.supports_gc) {
                                objc_constraint = objc_constraint::retain_release_or_gc;
                            } else if (objc_image_info_flags.is_simulated) {
                                objc_constraint = objc_constraint::retain_release_for_simulator;
                            }

                            if (this->objc_constraint != objc_constraint::none) {
                                if (this->objc_constraint != objc_constraint) {
                                    return creation_result::multiple_objc_constraint;
                                }
                            }

                            this->objc_constraint = objc_constraint;
                        }

                        if (!options.ignore_swift_version) {
                            if (this->swift_version != 0) {
                                if (this->swift_version != objc_image_info_flags.swift_version) {
                                    return creation_result::multiple_swift_version;
                                }
                            }

                            // A check here for when swift_version is
                            // zero is not needed as this->swift_version
                            // by default is zero

                            this->swift_version = objc_image_info_flags.swift_version;
                        }

                        break;
                    }

                    break;
                }

                case macho::load_commands::sub_client: {
                    if (options.ignore_allowable_clients) {
                        break;
                    }
                    
                    if (options.ignore_unnecessary_fields_for_version) {
                        break;
                    }

                    const auto sub_client_command = static_cast<const struct sub_client_command *>(iter.load_command());
                    auto sub_client_command_name_location = sub_client_command->client.offset;

                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(sub_client_command_name_location);
                    }

                    // Check for when sub_client_command_name_location
                    // overlaps with sub-client load-command

                    if (sub_client_command_name_location < sizeof(struct sub_client_command)) {
                        return creation_result::invalid_sub_client_command;
                    }

                    const auto sub_client_command_cmdsize = iter.cmdsize();
                    const auto sub_client_command_name_estimated_length = sub_client_command_cmdsize - sub_client_command_name_location;

                    if (sub_client_command_cmdsize < sizeof(struct sub_client_command)) {
                        return creation_result::invalid_sub_client_command;
                    }

                    // Check for when sub_client_command_name_location
                    // is past end or goes past end of dylib-command

                    if (sub_client_command_name_estimated_length >= sub_client_command_cmdsize || sub_client_command_name_estimated_length == 0) {
                        return creation_result::invalid_sub_client_command;
                    }

                    const auto sub_client_command_name = &reinterpret_cast<const char *>(sub_client_command)[sub_client_command_name_location];

                    const auto sub_client_command_name_length = strnlen(sub_client_command_name, sub_client_command_name_estimated_length);
                    const auto sub_client_command_end = &sub_client_command_name[sub_client_command_name_length];

                    if (std::all_of(sub_client_command_name, sub_client_command_end, isspace)) {
                        continue;
                    }

                    this->sub_clients.emplace_back(1ull << architecture_info_index, std::string(sub_client_command_name, sub_client_command_name_length));
                    break;
                }

                case macho::load_commands::sub_umbrella: {
                    if (options.ignore_parent_umbrella) {
                        break;
                    }

                    // tbd field parent-umbrella is
                    // only available on tbd-version v2

                    if (!options.parse_unsupported_fields_for_version) {
                        if (this->version != version::v2) {
                            break;
                        }
                    }
                    
                    if (options.ignore_unnecessary_fields_for_version) {
                        break;
                    }

                    const auto sub_umbrella_command = static_cast<const struct sub_umbrella_command *>(iter.load_command());
                    auto sub_umbrella_command_name_location = sub_umbrella_command->sub_umbrella.offset;

                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(sub_umbrella_command_name_location);
                    }

                    // Check for when sub_umbrella_command_name_location
                    // overlaps with sub-client load-command

                    if (sub_umbrella_command_name_location < sizeof(struct sub_umbrella_command)) {
                        return creation_result::invalid_sub_umbrella_command;
                    }

                    const auto sub_umbrella_command_cmdsize = iter.cmdsize();
                    const auto sub_umbrella_command_name_estimated_length = sub_umbrella_command_cmdsize - sub_umbrella_command_name_location;

                    if (sub_umbrella_command_cmdsize < sizeof(struct sub_umbrella_command)) {
                        return creation_result::invalid_sub_umbrella_command;
                    }

                    // Check for when sub_umbrella_command_name_location overlaps with
                    // load-command, is past end or goes past end of sub_umbrella_command

                    if (sub_umbrella_command_name_estimated_length >= sub_umbrella_command_cmdsize || sub_umbrella_command_name_estimated_length == 0) {
                        return creation_result::invalid_sub_umbrella_command;
                    }

                    const auto sub_umbrella_command_name = &reinterpret_cast<const char *>(sub_umbrella_command)[sub_umbrella_command_name_location];

                    const auto sub_umbrella_command_name_length = strnlen(sub_umbrella_command_name, sub_umbrella_command_name_estimated_length);
                    const auto sub_umbrella_command_end = &sub_umbrella_command_name[sub_umbrella_command_name_length];

                    if (std::all_of(sub_umbrella_command_name, sub_umbrella_command_end, isspace)) {
                        return creation_result::empty_parent_umbrella;
                    }

                    if (this->parent_umbrella.length() != 0) {
                        if (this->parent_umbrella.length() != sub_umbrella_command_name_length) {
                            return creation_result::multiple_parent_umbrella;
                        }

                        if (strncmp(this->parent_umbrella.c_str(), sub_umbrella_command_name, sub_umbrella_command_name_length) != 0) {
                            return creation_result::multiple_parent_umbrella;
                        }
                    }

                    this->parent_umbrella.assign(sub_umbrella_command_name, sub_umbrella_command_name_length);
                    break;
                }

                case macho::load_commands::uuid: {
                    if (options.ignore_uuids) {
                        break;
                    }

                    // tbd field uuids is only
                    // available on tbd-version v2

                    if (!options.parse_unsupported_fields_for_version) {
                        if (this->version != version::v2) {
                            break;
                        }
                    }
                    
                    if (options.ignore_unnecessary_fields_for_version) {
                        break;
                    }

                    const auto uuid_command = static_cast<const struct uuid_command *>(iter.load_command());
                    const auto uuids_end = this->uuids.cend();

                    if (this->uuids.size() != 0) {
                        // Ignore if multiple uuid-commands
                        // are found with matching uuids

                        const auto &uuid = this->uuids.back();
                        if (memcmp(uuid.uuid(), uuid_command->uuid, sizeof(uuid_command->uuid)) != 0) {
                            return creation_result::multiple_uuids;
                        }
                    }

                    if (!options.ignore_non_unique_uuids) {
                        // Search for same uuids across other containers
                        // in order to guarantee uniqueness

                        auto uuids_iter = this->uuids.cbegin();
                        for (; uuids_iter != uuids_end; uuids_iter++) {
                            if (memcmp(uuids_iter->uuid(), uuid_command->uuid, sizeof(uuid_command->uuid)) != 0) {
                                continue;
                            }

                            break;
                        }

                        if (uuids_iter != uuids_end) {
                            return creation_result::non_unique_uuid;
                        }
                    }

                    auto uuid = uuid_pair();

                    // Make a copy of uuid so as the container's
                    // load-command data storage is only temporary

                    uuid.architecture = architecture_info_from_index(architecture_info_index);
                    uuid.set_uuid(uuid_command->uuid);

                    this->uuids.emplace_back(std::move(uuid));
                    break;
                }

                case macho::load_commands::version_min_macosx:
                    if (options.ignore_platform) {
                        break;
                    }

                    if (this->platform != platform::none) {
                        if (this->platform != platform::macosx) {
                            return creation_result::multiple_platforms;
                        }
                    }

                    this->platform = platform::macosx;
                    break;

                case macho::load_commands::version_min_iphoneos:
                    if (options.ignore_platform) {
                        break;
                    }

                    if (this->platform != platform::none) {
                        if (this->platform != platform::ios) {
                            return creation_result::multiple_platforms;
                        }
                    }

                    this->platform = platform::ios;
                    break;

                case macho::load_commands::version_min_watchos:
                    if (options.ignore_platform) {
                        break;
                    }

                    if (this->platform != platform::none) {
                        if (this->platform != platform::watchos) {
                            return creation_result::multiple_platforms;
                        }
                    }

                    this->platform = platform::watchos;
                    break;

                case macho::load_commands::version_min_tvos:
                    if (options.ignore_platform) {
                        break;
                    }

                    if (this->platform != platform::none) {
                        if (this->platform != platform::tvos) {
                            return creation_result::multiple_platforms;
                        }
                    }

                    this->platform = platform::tvos;
                    break;

                default:
                    continue;
            }
        }

        // Check for the required fields, needed whether
        // or not the first container has them, but ignore
        // if provided options demand so

        if (!options.ignore_missing_identification) {
            if (!options.ignore_current_version || !options.ignore_compatibility_version || !options.ignore_install_name) {
                if (this->install_name.empty()) {
                    return creation_result::container_is_missing_dynamic_library_identification;
                }
            }
        }

        if (!options.ignore_platform && !options.ignore_missing_platform) {
            if (this->platform == platform::none) {
                return creation_result::container_is_missing_platform;
            }
        }

        if (!options.ignore_unnecessary_fields_for_version) {
            if (!options.ignore_uuids && !options.ignore_missing_uuids) {
                if (this->uuids.size() != 1) {
                    if (this->version == version::v2 || options.parse_unsupported_fields_for_version) {
                        return creation_result::container_is_missing_uuid;
                    }
                }
            }
        }

        if (!options.ignore_exports) {
            const auto string_table_size = string_table.size();
            const auto symbol_table_size = symbol_table.size();

            this->symbols.reserve(symbol_table_size);

            for (auto iter = symbol_table.begin; iter != symbol_table.end; iter++) {
                if (iter->n_type.type != symbol_table::type::section) {
                    continue;
                }

                auto desc = iter->n_desc;
                auto index = iter->n_un.n_strx;

                if (container.is_big_endian()) {
                    ::utils::swap::uint16(desc.value);
                    ::utils::swap::uint32(index);
                }

                // XXX: We might want to not ignore symbols
                // whose strings are past end of string-table

                if (index >= string_table_size) {
                    continue;
                }

                // Skip size 0 strings

                auto string = &string_table[index];
                if (*string == '\0') {
                    continue;
                }

                // Skip empty strings

                // Assume that the string isn't null-terminated to
                // avoid resulting in undefined behavior when copying

                const auto string_length = strnlen(string, string_table_size - index);
                if (std::all_of(string, &string[string_length], isspace)) {
                    continue;
                }

                const auto type = symbol::type_from_symbol_string(string, desc);
                switch (type) {
                    case symbol::type::normal:
                        if (!options.allow_private_normal_symbols) {
                            if (!iter->n_type.external) {
                                continue;
                            }
                        }

                        break;

                    case symbol::type::weak:
                        if (!options.allow_private_weak_symbols) {
                            if (!iter->n_type.external) {
                                continue;
                            }
                        }

                        break;

                    case symbol::type::objc_class:
                        if (!options.allow_private_objc_class_symbols) {
                            if (!iter->n_type.external) {
                                continue;
                            }
                        }

                        break;

                    case symbol::type::objc_ivar:
                        if (!options.allow_private_objc_ivar_symbols) {
                            if (!iter->n_type.external) {
                                continue;
                            }
                        }

                        break;
                }

                this->symbols.emplace_back(1ull << architecture_info_index, std::string(string, string_length), type);
            }

            std::sort(this->reexports.begin(), this->reexports.end(), [](const reexport &lhs, const reexport &rhs) {
                return lhs.string.compare(rhs.string) < 0;
            });
            
            std::sort(this->symbols.begin(), this->symbols.end(), [](const symbol &lhs, const symbol &rhs) {
                return lhs.string.compare(rhs.string) < 0;
            });
        }

        return creation_result::ok;
    }

    void tbd::clear() noexcept {
        this->architectures = 0;
        this->current_version.value = 0;
        this->compatibility_version.value = 0;
        this->flags.clear();
        this->platform = platform::none;
        this->install_name.clear();
        this->parent_umbrella.clear();
        this->sub_clients.clear();
        this->reexports.clear();
        this->symbols.clear();
        this->swift_version = 0;
        this->objc_constraint = objc_constraint::none;
        this->version = version::none;
        this->uuids.clear();
    }
}
