//
//  src/utils/tbd-macho.cc
//  tbd
//
//  Created by inoahdev on 3/17/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "../mach-o/headers/build.h"
#include "../mach-o/utils/segments/segment_with_sections_iterator.h"
#include "../mach-o/utils/strings/table/data.h"

#include "../objc/image_info.h"

#include "range.h"
#include "tbd.h"

namespace utils {
    enum tbd::creation_from_macho_result
    tbd::parse_macho_container_information(const macho::container &container,
                                           const creation_options &options,
                                           const version &version,
                                           struct macho_core_information &info_out) noexcept
    {
        auto cputype = container.header.cputype;
        auto cpusubtype = container.header.cpusubtype;
        
        if (container.header.magic.is_big_endian()) {
            macho::utils::swap::cputype(cputype);
            ::utils::swap::int32(cpusubtype);
        }
        
        const auto subtype = subtype_from_cputype(cputype, cpusubtype);
        if (subtype == macho::subtype::none) {
            return creation_from_macho_result::invalid_container_header_subtype;
        }
        
        auto architecture_info_index = macho::architecture_info_index_from_cputype(cputype, subtype);
        if (architecture_info_index == -1) {
            return creation_from_macho_result::unrecognized_container_cputype_subtype_pair;
        }
        
        if (!options.ignore_architectures) {
            const auto architecture_info_index_bit = (1ull << architecture_info_index);
            if (this->architectures & architecture_info_index_bit) {
                return creation_from_macho_result::multiple_containers_for_same_architecture;
            }
            
            this->architectures |= architecture_info_index_bit;
        }
    
        if (!options.ignore_unnecessary_fields_for_version) {
            if (!options.ignore_flags) {
                if (version == version::v2 || options.parse_unsupported_fields_for_version) {
                    struct tbd::flags flags;
                    
                    flags.flat_namespace = container.header.flags.two_level_namespaces;
                    flags.not_app_extension_safe = !container.header.flags.app_extension_safe;
                    
                    info_out.flags = flags;
                }
            }
        }
        
        auto symtab = macho::symtab_command();
        auto dysymtab = macho::dysymtab_command();
        
        enum creation_from_macho_result parse_load_command_result =
            this->parse_macho_load_command_information(container,
                                                       options,
                                                       version,
                                                       architecture_info_index,
                                                       info_out,
                                                       symtab,
                                                       dysymtab);
        
        if (parse_load_command_result != creation_from_macho_result::ok) {
            return parse_load_command_result;
        }
        
        if (!options.ignore_exports) {
            enum creation_from_macho_result parse_symtab_result =
                this->parse_macho_symbol_table(container,
                                               symtab,
                                               dysymtab,
                                               options,
                                               architecture_info_index,
                                               info_out);
            
            if (parse_symtab_result != creation_from_macho_result::ok) {
                return parse_symtab_result;
            }
        }
        
        return creation_from_macho_result::ok;
    }
    
    tbd::creation_from_macho_result
    tbd::parse_macho_load_command_information(const macho::container &container,
                                              const creation_options &options,
                                              const version &version,
                                              uint64_t architecture_info_index,
                                              struct macho_core_information &info_out,
                                              struct macho::symtab_command &symtab_out,
                                              struct macho::dysymtab_command &dysymtab_out) noexcept
    {
        auto load_commands = macho::utils::load_commands::data();
        auto load_commands_options = macho::utils::load_commands::data::options();
        
        switch (load_commands.create(container, load_commands_options)) {
            case macho::utils::load_commands::data::creation_result::ok:
                break;
                
            default:
                return creation_from_macho_result::failed_to_retrieve_load_cammands;
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
        auto found_container_uuid = false;
        
        auto symtab_command = static_cast<const struct macho::symtab_command *>(nullptr);
        auto dysymtab_command = static_cast<const struct macho::dysymtab_command *>(nullptr);

        for (auto iter = load_commands.begin; iter != load_commands.end; iter++) {
            const auto cmd = iter.cmd();
            switch (cmd) {
                case macho::load_commands::build_version: {
                    if (options.ignore_platform) {
                        break;
                    }
                    
                    const auto build_version_command = static_cast<const struct macho::build_version_command *>(iter.load_command());
                    auto build_version_platform = build_version_command->platform;
                    
                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(build_version_platform);
                    }
                    
                    // XXX: Maybe we shouldn't ignore when build_version
                    // command is larger than it should be
                    
                    const auto cmdsize = iter.cmdsize();
                    if (cmdsize < sizeof(struct macho::build_version_command)) {
                        return creation_from_macho_result::invalid_build_version_command;
                    }
                    
                    auto parsed_platform = platform::none;
                    switch (macho::build_version::platform(build_version_platform)) {
                        case macho::build_version::platform::macos:
                            parsed_platform = platform::macosx;
                            break;
                            
                        case macho::build_version::platform::ios:
                            parsed_platform = platform::ios;
                            break;
                            
                        case macho::build_version::platform::watchos:
                            parsed_platform = platform::watchos;
                            break;
                            
                        case macho::build_version::platform::tvos:
                            parsed_platform = platform::tvos;
                            break;
                            
                        case macho::build_version::platform::bridgeos:
                            return creation_from_macho_result::unsupported_platform;
                            
                        default:
                            return creation_from_macho_result::unrecognized_platform;
                    }
                    
                    if (info_out.platform != parsed_platform) {
                        if (found_container_platform) {
                            return creation_from_macho_result::multiple_platforms;
                        }
                    }
                    
                    info_out.platform = parsed_platform;
                    found_container_platform = true;
                    
                    break;
                }
                    
                case macho::load_commands::identification_dylib: {
                    if (options.ignore_current_version && options.ignore_compatibility_version && options.ignore_install_name) {
                        break;
                    }
                    
                    const auto dylib_command = static_cast<const struct macho::dylib_command *>(iter.load_command());
                    
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
                            if (info_out.current_version.value != dylib_command_current_version) {
                                if (found_container_identification) {
                                    return creation_from_macho_result::multiple_current_versions;
                                }
                            }
                            
                            info_out.current_version.value = dylib_command_current_version;
                        }
                        
                        if (!options.ignore_compatibility_version) {
                            if (info_out.compatibility_version.value != dylib_command_compatibility_version) {
                                if (found_container_identification) {
                                    return creation_from_macho_result::multiple_compatibility_versions;
                                }
                            }
                            
                            info_out.compatibility_version.value = dylib_command_compatibility_version;
                        }
                    }
                    
                    // Check for when dylib_command_name_location
                    // overlaps with dylib-command load-command
                    
                    if (dylib_command_name_location < sizeof(struct macho::dylib_command)) {
                        return creation_from_macho_result::invalid_dylib_command;
                    }
                    
                    const auto dylib_command_cmdsize = iter.cmdsize();
                    const auto dylib_command_name_estimated_length = dylib_command_cmdsize - dylib_command_name_location;
                    
                    if (dylib_command_cmdsize < sizeof(struct macho::dylib_command)) {
                        return creation_from_macho_result::invalid_dylib_command;
                    }
                    
                    // Check for when dylib_command_name_location
                    // is past end or goes past end of dylib_command
                    
                    if (dylib_command_name_estimated_length >= dylib_command_cmdsize || dylib_command_name_estimated_length == 0) {
                        return creation_from_macho_result::invalid_dylib_command;
                    }
                    
                    const auto dylib_command_name = &reinterpret_cast<const char *>(dylib_command)[dylib_command_name_location];
                    const auto dylib_command_name_length = strnlen(dylib_command_name, dylib_command_name_estimated_length);
                    const auto dylib_command_end = &dylib_command_name[dylib_command_name_length];
                    
                    if (std::all_of(dylib_command_name, dylib_command_end, isspace)) {
                        return creation_from_macho_result::empty_install_name;
                    }
                    
                    if (!options.ignore_install_name) {
                        if (info_out.install_name.size() != 0) {
                            if (info_out.install_name.length() != dylib_command_name_length) {
                                if (found_container_identification) {
                                    return creation_from_macho_result::multiple_install_names;
                                }
                            }
                            
                            if (strncmp(info_out.install_name.c_str(), dylib_command_name, dylib_command_name_length) != 0) {
                                if (found_container_identification) {
                                    return creation_from_macho_result::multiple_install_names;
                                }
                            }
                        }
                        
                        info_out.install_name.assign(dylib_command_name, dylib_command_name_length);
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
                    
                    const auto dylib_command = static_cast<const struct macho::dylib_command *>(iter.load_command());
                    auto dylib_command_name_location = dylib_command->name.offset;
                    
                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(dylib_command_name_location);
                    }
                    
                    // Check for when dylib_command_name_location
                    // overlaps with dylib-command load-command
                    
                    if (dylib_command_name_location < sizeof(struct macho::dylib_command)) {
                        return creation_from_macho_result::invalid_dylib_command;
                    }
                    
                    const auto dylib_command_cmdsize = iter.cmdsize();
                    const auto dylib_command_name_estimated_length = dylib_command_cmdsize - dylib_command_name_location;
                    
                    if (dylib_command_cmdsize < sizeof(struct macho::dylib_command)) {
                        return creation_from_macho_result::invalid_dylib_command;
                    }
                    
                    // Check for when dylib_command_name_location overlaps with
                    // load-command, is past end or goes past end of dylib_command
                    
                    if (dylib_command_name_estimated_length >= dylib_command_cmdsize || dylib_command_name_estimated_length == 0) {
                        return creation_from_macho_result::invalid_dylib_command;
                    }
                    
                    const auto dylib_command_name = &reinterpret_cast<const char *>(dylib_command)[dylib_command_name_location];
                    
                    const auto dylib_command_name_length = strnlen(dylib_command_name, dylib_command_name_estimated_length);
                    const auto dylib_command_end = &dylib_command_name[dylib_command_name_length];
                    
                    if (std::all_of(dylib_command_name, dylib_command_end, isspace)) {
                        continue;
                    }
                    
                    // Assume dylib_command_name is
                    // not null-terminated
                    
                    auto reexports_iter = info_out.reexports->begin();
                    const auto reexports_end = info_out.reexports->end();
                    
                    for (; reexports_iter != reexports_end; reexports_iter++) {
                        if (reexports_iter->string.length() != dylib_command_name_length) {
                            continue;
                        }
                        
                        if (strncmp(reexports_iter->string.c_str(), dylib_command_name, dylib_command_name_length) != 0) {
                            continue;
                        }
                        
                        if (!options.ignore_architectures) {
                            reexports_iter->add_architecture(architecture_info_index);
                        }
                        
                        break;
                    }
                    
                    if (reexports_iter == reexports_end) {
                        auto reexport_string = std::string(dylib_command_name, dylib_command_name_length);
                        auto &reexport = info_out.reexports->emplace_back(1ull << architecture_info_index, std::move(reexport_string));
                        
                        if (options.ignore_architectures) {
                            reexport.set_all_architectures();
                        } else {
                            reexport.add_architecture(architecture_info_index);
                        }
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
                    
                    const auto segment_command = static_cast<const struct macho::segment_command *>(iter.load_command());
                    
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
                    
                    const auto segment = macho::utils::segments::command(segment_command, container.is_big_endian());
                    if (segment.sections.begin == nullptr) {
                        return creation_from_macho_result::invalid_segment_command;
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
                            return creation_from_macho_result::invalid_objc_imageinfo_segment_section;
                        }
                        
                        if (!::utils::range::is_valid_size_based_range(section_location, section_size)) {
                            return creation_from_macho_result::invalid_objc_imageinfo_segment_section;
                        }
                        
                        // XXX: We should validate section_data_range doesn't
                        // overlap container's header and load-commands buffer
                        
                        const auto section_data_range = ::utils::range(section_location, section_location + section_size);
                        if (section_data_range.is_or_goes_past_end(container.size)) {
                            return creation_from_macho_result::invalid_objc_imageinfo_segment_section;
                        }
                        
                        if (!container.stream.seek(container.base + section_location, stream::file::seek_type::beginning)) {
                            return creation_from_macho_result::stream_seek_error;
                        }
                        
                        auto objc_image_info = objc::image_info();
                        if (!container.stream.read(&objc_image_info, sizeof(objc_image_info))) {
                            return creation_from_macho_result::stream_read_error;
                        }
                        
                        enum creation_from_macho_result parse_objc_imageinfo_result =
                            this->parse_macho_objc_imageinfo_section(container,
                                                               objc_image_info,
                                                               options,
                                                               found_container_objc_information,
                                                               info_out);
                        
                        if (parse_objc_imageinfo_result != creation_from_macho_result::ok) {
                            return parse_objc_imageinfo_result;
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
                    
                    const auto segment_command = static_cast<const struct macho::segment_command_64 *>(iter.load_command());
                    
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
                    
                    const auto segment = macho::utils::segments::command_64(segment_command, container.is_big_endian());
                    if (segment.sections.begin == nullptr) {
                        return creation_from_macho_result::invalid_segment_command_64;
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
                            return creation_from_macho_result::invalid_objc_imageinfo_segment_section;
                        }
                        
                        // XXX: We should validate section_data_range doesn't
                        // overlap container's header and load-commands buffer
                        
                        const auto section_range = ::utils::range(section_location, section_location + section_size);
                        if (section_range.is_or_goes_past_end(container.size)) {
                            return creation_from_macho_result::invalid_objc_imageinfo_segment_section;
                        }
                        
                        if (!container.stream.seek(container.base + section_location, stream::file::seek_type::beginning)) {
                            return creation_from_macho_result::stream_seek_error;
                        }
                        
                        auto objc_image_info = objc::image_info();
                        if (!container.stream.read(&objc_image_info, sizeof(objc_image_info))) {
                            return creation_from_macho_result::stream_read_error;
                        }
                        
                        enum creation_from_macho_result parse_objc_imageinfo_result =
                            this->parse_macho_objc_imageinfo_section(container,
                                                               objc_image_info,
                                                               options,
                                                               found_container_objc_information,
                                                               info_out);
                        
                        if (parse_objc_imageinfo_result != creation_from_macho_result::ok) {
                            return parse_objc_imageinfo_result;
                        }
                        
                        found_container_objc_information = true;
                        break;
                    }
                    
                    break;
                }
                    
                case macho::load_commands::sub_client: {
                    if (options.ignore_exports) {
                        break;
                    }
                    
                    if (options.ignore_clients) {
                        break;
                    }
                    
                    if (options.ignore_unnecessary_fields_for_version) {
                        break;
                    }
                    
                    const auto sub_client_command = static_cast<const struct macho::sub_client_command *>(iter.load_command());
                    auto sub_client_command_name_location = sub_client_command->client.offset;
                    
                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(sub_client_command_name_location);
                    }
                    
                    // Check for when sub_client_command_name_location
                    // overlaps with sub-client load-command
                    
                    if (sub_client_command_name_location < sizeof(struct macho::sub_client_command)) {
                        return creation_from_macho_result::invalid_sub_client_command;
                    }
                    
                    const auto sub_client_command_cmdsize = iter.cmdsize();
                    const auto sub_client_command_name_estimated_length = sub_client_command_cmdsize - sub_client_command_name_location;
                    
                    if (sub_client_command_cmdsize < sizeof(struct macho::sub_client_command)) {
                        return creation_from_macho_result::invalid_sub_client_command;
                    }
                    
                    // Check for when sub_client_command_name_location
                    // is past end or goes past end of dylib-command
                    
                    if (sub_client_command_name_estimated_length == 0 || sub_client_command_name_estimated_length >= sub_client_command_cmdsize) {
                        return creation_from_macho_result::invalid_sub_client_command;
                    }
                    
                    const auto sub_client_command_name = &reinterpret_cast<const char *>(sub_client_command)[sub_client_command_name_location];
                    
                    const auto sub_client_command_name_length = strnlen(sub_client_command_name, sub_client_command_name_estimated_length);
                    const auto sub_client_command_end = &sub_client_command_name[sub_client_command_name_length];
                    
                    if (std::all_of(sub_client_command_name, sub_client_command_end, isspace)) {
                        continue;
                    }
                    
                    // Assume sub_client_command_name
                    // is not null-terminated
                    
                    auto clients_iter = info_out.clients->begin();
                    const auto clients_end = info_out.clients->end();
                    
                    for (; clients_iter != clients_end; clients_iter++) {
                        if (clients_iter->string.length() != sub_client_command_name_length) {
                            continue;
                        }
                        
                        if (strncmp(clients_iter->string.c_str(), sub_client_command_name, sub_client_command_name_length) != 0) {
                            continue;
                        }
                        
                        if (!options.ignore_architectures) {
                            clients_iter->add_architecture(architecture_info_index);
                        }
                        
                        break;
                    }
                    
                    // In case a sub-client
                    // was not found
                    
                    if (clients_iter == clients_end) {
                        auto client_string = std::string(sub_client_command_name, sub_client_command_name_length);
                        auto &client = info_out.clients->emplace_back(1ull << architecture_info_index, std::move(client_string));
                        
                        if (options.ignore_architectures) {
                            client.set_all_architectures();
                        } else {
                            client.add_architecture(architecture_info_index);
                        }
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
                        if (version != version::v2) {
                            break;
                        }
                    }
                    
                    if (options.ignore_unnecessary_fields_for_version) {
                        break;
                    }
                    
                    const auto sub_umbrella_command = static_cast<const struct macho::sub_umbrella_command *>(iter.load_command());
                    auto sub_umbrella_command_name_location = sub_umbrella_command->sub_umbrella.offset;
                    
                    if (container.is_big_endian()) {
                        ::utils::swap::uint32(sub_umbrella_command_name_location);
                    }
                    
                    // Check for when sub_umbrella_command_name_location
                    // overlaps with sub-client load-command
                    
                    if (sub_umbrella_command_name_location < sizeof(struct macho::sub_umbrella_command)) {
                        return creation_from_macho_result::invalid_sub_umbrella_command;
                    }
                    
                    const auto sub_umbrella_command_cmdsize = iter.cmdsize();
                    const auto sub_umbrella_command_name_estimated_length = sub_umbrella_command_cmdsize - sub_umbrella_command_name_location;
                    
                    if (sub_umbrella_command_cmdsize < sizeof(struct macho::sub_umbrella_command)) {
                        return creation_from_macho_result::invalid_sub_umbrella_command;
                    }
                    
                    // Check for when sub_umbrella_command_name_location overlaps with
                    // load-command, is past end or goes past end of sub_umbrella_command
                    
                    if (sub_umbrella_command_name_estimated_length >= sub_umbrella_command_cmdsize || sub_umbrella_command_name_estimated_length == 0) {
                        return creation_from_macho_result::invalid_sub_umbrella_command;
                    }
                    
                    const auto sub_umbrella_command_name = &reinterpret_cast<const char *>(sub_umbrella_command)[sub_umbrella_command_name_location];
                    
                    const auto sub_umbrella_command_name_length = strnlen(sub_umbrella_command_name, sub_umbrella_command_name_estimated_length);
                    const auto sub_umbrella_command_end = &sub_umbrella_command_name[sub_umbrella_command_name_length];
                    
                    if (std::all_of(sub_umbrella_command_name, sub_umbrella_command_end, isspace)) {
                        return creation_from_macho_result::empty_parent_umbrella;
                    }
                    
                    if (info_out.parent_umbrella.length() != sub_umbrella_command_name_length) {
                        if (found_container_parent_umbrella) {
                            return creation_from_macho_result::multiple_parent_umbrella;
                        }
                    }
                    
                    if (strncmp(info_out.parent_umbrella.c_str(), sub_umbrella_command_name, sub_umbrella_command_name_length) != 0) {
                        if (found_container_parent_umbrella) {
                            return creation_from_macho_result::multiple_parent_umbrella;
                        }
                    }
                    
                    info_out.parent_umbrella.assign(sub_umbrella_command_name, sub_umbrella_command_name_length);
                    break;
                }
                    
                case macho::load_commands::dynamic_symbol_table: {
                    if (options.ignore_exports) {
                        break;
                    }
                    
                    // XXX: We may want to return multiple_symbol_tables
                    // in the scenario where another symbol-table has
                    // been found, instead of returning invalid_symtab_command
                    
                    const auto cmdsize = iter.cmdsize();
                    if (cmdsize != sizeof(struct macho::dysymtab_command)) {
                        return creation_from_macho_result::invalid_dysymtab_command;
                    }
                    
                    if (dysymtab_command != nullptr) {
                        // Ignore multiple symbol-tables
                        // if they both match up
                        
                        if (memcmp(dysymtab_command, iter.load_command(), sizeof(struct macho::dysymtab_command)) == 0) {
                            break;
                        }
                        
                        return creation_from_macho_result::multiple_symbol_tables;
                    }
                    
                    dysymtab_command = static_cast<const struct macho::dysymtab_command *>(iter.load_command());
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
                    if (cmdsize != sizeof(struct macho::symtab_command)) {
                        return creation_from_macho_result::invalid_symtab_command;
                    }
                    
                    if (symtab_command != nullptr) {
                        // Ignore multiple symbol-tables
                        // if they both match up
                        
                        if (memcmp(symtab_command, iter.load_command(), sizeof(struct macho::symtab_command)) == 0) {
                            break;
                        }
                        
                        return creation_from_macho_result::multiple_symbol_tables;
                    }
                    
                    symtab_command = static_cast<const struct macho::symtab_command *>(iter.load_command());
                    break;
                }
                    
                case macho::load_commands::uuid: {
                    if (options.ignore_uuids) {
                        break;
                    }
                    
                    // tbd field uuids is only
                    // available on tbd-version v2
                    
                    if (!options.parse_unsupported_fields_for_version) {
                        if (version != version::v2) {
                            break;
                        }
                    }
                    
                    if (options.ignore_unnecessary_fields_for_version) {
                        break;
                    }
                    
                    const auto uuid_command = static_cast<const struct macho::uuid_command *>(iter.load_command());
                    auto uuid = uuid_pair();
                    
                    // Make a copy of uuid so as the container's
                    // load-command data storage is only temporary
                    
                    uuid.architecture = macho::architecture_info_from_index(architecture_info_index);
                    uuid.set_uuid(uuid_command->uuid);
                    
                    info_out.uuid = std::move(uuid);
                    found_container_uuid = true;
                    
                    break;
                }
                    
                case macho::load_commands::version_min_macosx:
                    if (options.ignore_platform) {
                        break;
                    }
                    
                    if (info_out.platform != platform::macosx) {
                        if (found_container_platform) {
                            return creation_from_macho_result::multiple_platforms;
                        }
                    }
                    
                    info_out.platform = platform::macosx;
                    found_container_platform = true;
                    
                    break;
                    
                case macho::load_commands::version_min_iphoneos:
                    if (options.ignore_platform) {
                        break;
                    }
                    
                    if (info_out.platform != platform::ios) {
                        if (found_container_platform) {
                            return creation_from_macho_result::multiple_platforms;
                        }
                    }
                    
                    info_out.platform = platform::ios;
                    found_container_platform = true;
                    
                    break;
                    
                case macho::load_commands::version_min_watchos:
                    if (options.ignore_platform) {
                        break;
                    }
                    
                    if (info_out.platform != platform::watchos) {
                        if (found_container_platform) {
                            return creation_from_macho_result::multiple_platforms;
                        }
                    }
                    
                    info_out.platform = platform::watchos;
                    found_container_platform = true;
                    
                    break;
                    
                case macho::load_commands::version_min_tvos:
                    if (options.ignore_platform) {
                        break;
                    }
                    
                    if (info_out.platform != platform::tvos) {
                        if (found_container_platform) {
                            return creation_from_macho_result::multiple_platforms;
                        }
                    }
                    
                    info_out.platform = platform::tvos;
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
                    return creation_from_macho_result::container_is_missing_dynamic_library_identification;
                }
            }
        }
        
        if (!options.ignore_platform && !options.ignore_missing_platform) {
            if (!found_container_platform) {
                return creation_from_macho_result::container_is_missing_platform;
            }
        }
        
        if (version == version::v2 || options.parse_unsupported_fields_for_version) {
            if (!options.ignore_unnecessary_fields_for_version) {
                if (!options.ignore_uuids && !options.ignore_missing_uuids) {
                    if (!found_container_uuid) {
                        return creation_from_macho_result::container_is_missing_uuid;
                    }
                }
            }
        }
        
        if (!options.ignore_exports) {
            if (symtab_command == nullptr) {
                return creation_from_macho_result::container_is_missing_symbol_table;
            }
            
            if (dysymtab_command == nullptr) {
                return creation_from_macho_result::container_is_missing_dynamic_symbol_table;
            }
            
            symtab_out = *symtab_command;
            dysymtab_out = *dysymtab_command;
        }
        
        return creation_from_macho_result::ok;
    }
    
    tbd::creation_from_macho_result
    tbd::parse_macho_objc_imageinfo_section(const macho::container &container,
                                            const objc::image_info &image_info,
                                            const creation_options &options,
                                            bool found_container_objc_information,
                                            struct macho_core_information &info_out) noexcept
    {
        struct objc::image_info::flags objc_image_info_flags(image_info.flags);
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
            
            if (info_out.objc_constraint != objc_constraint) {
                if (found_container_objc_information) {
                    return creation_from_macho_result::multiple_objc_constraint;
                }
            }
            
            info_out.objc_constraint = objc_constraint;
        }
        
        if (!options.ignore_swift_version) {
            if (info_out.swift_version != objc_image_info_flags.swift_version) {
                if (found_container_objc_information) {
                    return creation_from_macho_result::multiple_swift_version;
                }
            }
            
            // A check here for when swift_version is
            // zero is not needed as info_out.swift_version
            // by default is zero
            
            info_out.swift_version = objc_image_info_flags.swift_version;
        }
        
        return creation_from_macho_result::ok;
    }
    
    tbd::creation_from_macho_result
    tbd::parse_macho_symbol_table(const macho::container &container,
                                  const macho::symtab_command &symtab,
                                  const macho::dysymtab_command &dysymtab,
                                  const creation_options &options,
                                  uint64_t architecture_info_index,
                                  struct macho_core_information &info_out) noexcept
    {
        auto string_table = macho::utils::strings::table::data();
        auto string_table_options = macho::utils::strings::table::data::options();
        
        switch (string_table.create(container, symtab, string_table_options)) {
            case macho::utils::strings::table::data::creation_result::ok:
                break;
                
            default:
                return creation_from_macho_result::failed_to_retrieve_string_table;
        }
        
        if (container.is_64_bit()) {
            auto symbol_table = macho::utils::symbols::table_64::data();
            auto symbol_table_options = macho::utils::symbols::table_64::data::options();
            
            symbol_table_options.convert_to_little_endian = true;
            
            switch (symbol_table.create(container, symtab, symbol_table_options)) {
                case macho::utils::symbols::table_64::data::creation_result::ok:
                    break;
                    
                default:
                    return creation_from_macho_result::failed_to_retrieve_symbol_table;
            }
            
            for (auto iter = symbol_table.begin; iter != symbol_table.end; iter++) {
                auto symbol_table_entry = *iter;
                if (symbol_table_entry.n_type.type != macho::symbol_table::type::indirect &&
                    symbol_table_entry.n_type.type != macho::symbol_table::type::section) {
                    continue;
                }
                
                enum creation_from_macho_result parse_symbol_table_entry_result =
                    this->parse_macho_symbol_table_entry(string_table,
                                                         iter->n_un.n_strx,
                                                         iter->n_desc,
                                                         iter->n_type,
                                                         options,
                                                         architecture_info_index,
                                                         info_out);
                
                if (parse_symbol_table_entry_result != creation_from_macho_result::ok) {
                    return parse_symbol_table_entry_result;
                }
            }
        } else {
            auto symbol_table = macho::utils::symbols::table::data();
            auto symbol_table_options = macho::utils::symbols::table::data::options();
            
            symbol_table_options.convert_to_little_endian = true;
            
            switch (symbol_table.create(container, symtab, symbol_table_options)) {
                case macho::utils::symbols::table::data::creation_result::ok:
                    break;
                    
                default:
                    return creation_from_macho_result::failed_to_retrieve_symbol_table;
            }
         
            for (auto iter = symbol_table.begin; iter != symbol_table.end; iter++) {
                auto symbol_table_entry = *iter;
                if (symbol_table_entry.n_type.type != macho::symbol_table::type::indirect &&
                    symbol_table_entry.n_type.type != macho::symbol_table::type::section) {
                    continue;
                }
                
                enum creation_from_macho_result parse_symbol_table_entry_result =
                    this->parse_macho_symbol_table_entry(string_table,
                                                         iter->n_un.n_strx,
                                                         iter->n_desc,
                                                         iter->n_type,
                                                         options,
                                                         architecture_info_index,
                                                         info_out);
                
                if (parse_symbol_table_entry_result != creation_from_macho_result::ok) {
                    return parse_symbol_table_entry_result;
                }
            }
        }
        
        return creation_from_macho_result::ok;
    }
    
    tbd::creation_from_macho_result
    tbd::parse_macho_symbol_table_entry(const macho::utils::strings::table::data &string_table,
                                        uint32_t index,
                                        macho::symbol_table::desc desc,
                                        macho::symbol_table::n_type type,
                                        const creation_options &options,
                                        uint64_t architecture_info_index,
                                        struct macho_core_information &info_out) noexcept
    {
        auto string_table_size = string_table.size();
        if (index >= string_table_size) {
            return creation_from_macho_result::ok;
        }
        
        // Skip empty strings
        
        auto string = &string_table[index];
        if (*string == '\0') {
            return creation_from_macho_result::ok;
        }
        
        // Assume that the string isn't null-terminated to
        // avoid resulting in undefined behavior when copying
        
        auto string_index = size_t();
        auto string_length = strnlen(string, string_table.size() - index);
        
        if (std::all_of(string, &string[string_length], isspace)) {
            return creation_from_macho_result::ok;
        }
        
        const auto symbol_type =
            symbol::type_from_symbol_string(string, desc, &string_index);
        
        switch (symbol_type) {
            case symbol::type::normal:
                if (!options.allow_private_normal_symbols) {
                    if (!type.external) {
                        return creation_from_macho_result::ok;
                    }
                }
                
                break;
                
            case symbol::type::weak:
                if (!options.allow_private_weak_symbols) {
                    if (!type.external) {
                        return creation_from_macho_result::ok;
                    }
                }
                
                break;
                
            case symbol::type::objc_class:
                if (!options.allow_private_objc_class_symbols) {
                    if (!type.external) {
                        return creation_from_macho_result::ok;
                    }
                }
                
                break;
                
            case symbol::type::objc_ivar:
                if (!options.allow_private_objc_ivar_symbols) {
                    if (!type.external) {
                        return creation_from_macho_result::ok;
                    }
                }
                
                break;
        }
        
        string = &string[string_index];
        string_length -= string_index;
        
        const auto symbols_end = info_out.symbols->end();
        
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
        
        auto symbols_iter = info_out.symbols->begin();
        for (; symbols_iter != symbols_end; symbols_iter++) {
            // As we separate out symbol-prefixes identifying the
            // type of a symbol, the types have to be checked as well
            
            // XXX: We may not want to allow multiple symbols
            // where one is weak, as weak-symbols are not
            // identified through their string, and as such
            // can result in multiple symbols of the exact
            // same raw-string present in the symbols-vector
            
            if (symbols_iter->type != symbol_type) {
                continue;
            }
            
            if (symbols_iter->string.length() != string_length) {
                continue;
            }
            
            if (strncmp(symbols_iter->string.c_str(), string, string_length) != 0) {
                continue;
            }
            
            if (!options.ignore_architectures) {
                symbols_iter->add_architecture(architecture_info_index);
            }
            
            break;
        }
        
        if (symbols_iter == symbols_end) {
            auto symbol_string = std::string(string, string_length);
            auto &symbol = info_out.symbols->emplace_back(1ull << architecture_info_index, std::move(symbol_string), symbol_type);
            
            if (options.ignore_architectures) {
                symbol.set_all_architectures();
            } else {
                symbol.add_architecture(architecture_info_index);
            }
        }
        
        return creation_from_macho_result::ok;
    }
}
