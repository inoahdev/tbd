//
//  src/mach-o/utils/load_commands/data.cc
//  tbd
//
//  Created by inoahdev on 11/23/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "../../../utils/swap.h"
#include "../swap.h"

#include "data.h"

namespace macho::utils::load_commands {
    enum load_commands data::iterator::cmd() const noexcept {
        auto cmd = ptr->cmd;
        if (this->is_big_endian()) {
            swap::load_commands::cmd(cmd);
        }
        
        return cmd;
    }
    
    uint32_t data::iterator::cmdsize() const noexcept {
        auto cmdsize = ptr->cmdsize;
        if (this->is_big_endian()) {
            ::utils::swap::uint32(cmdsize);
        }
        
        return cmdsize;
    }
    
    data::creation_result data::create(const container &container, const options &options) noexcept {
        auto load_commands_count = container.header.ncmds;
        auto load_commands_size = container.header.sizeofcmds;
        
        if (!load_commands_count) {
            return creation_result::no_load_commands;
        }
        
        if (!load_commands_size) {
            return creation_result::load_commands_area_is_too_small;
        }
        
        auto flags = data::flags();
        if (container.is_big_endian()) {
            flags.set_is_big_endian();
            
            ::utils::swap::uint32(load_commands_count);
            ::utils::swap::uint32(load_commands_size);
        }
        
        if (options.convert_to_big_endian()) {
            flags.set_is_big_endian();
        } else if (options.convert_to_little_endian()) {
            flags.set_is_little_endian();
        }
        
        if (load_commands_size < sizeof(load_command)) {
            return creation_result::load_commands_area_is_too_small;
        }
        
        const auto load_commands_minimum_size = sizeof(load_command) * load_commands_count;
        const auto container_available_size = container.size - sizeof(container.header);

        if (load_commands_minimum_size > container_available_size) {
            return creation_result::too_many_load_commands;
        }
        
        if (load_commands_size < load_commands_minimum_size) {
            return creation_result::load_commands_area_is_too_small;
        }
        
        if (load_commands_size > container_available_size) {
            return creation_result::load_commands_area_is_too_large;
        }
        
        auto load_commands_begin = container.base + sizeof(container.header);
        if (container.is_64_bit()) {
            load_commands_begin += sizeof(uint32_t);
            flags.set_is_64_bit();
        }
        
        if (!container.stream.seek(load_commands_begin, stream::file::seek_type::beginning)) {
            return creation_result::stream_seek_error;
        }
        
        const auto load_commands_data = reinterpret_cast<load_command *>(new uint8_t[load_commands_size]);
        if (!container.stream.read(load_commands_data, load_commands_size)) {
            this->_destruct();
            return creation_result::stream_read_error;
        }
        
        const auto begin = iterator(load_commands_data, flags);
        
        // Unfortunately, to maintain a working iterator implementation,
        // we need to manually find the end instead of relying on sizeofcmds,
        // in the rare case where sizeofcmds doesn't align perfectly with the
        // load-commands array, and also if options asks for enforcement
        // against this
        
        // We also need to swap load-commands (if proper options are provided)
        
        const auto load_commands_max_index = load_commands_count - 1;
        
        auto size_taken = uint32_t();
        auto index = uint32_t();
        
        auto iter = load_commands_data;
        for (; index < load_commands_count; ) {
            auto base = *iter;

            if (container.is_big_endian()) {
                utils::swap::load_commands::base(base);

                if (options.convert_to_little_endian()) {
                    this->switch_endian_of_load_command_with_cmd(*iter, base.cmd);
                }
            } else {
                if (options.convert_to_big_endian()) {
                    this->switch_endian_of_load_command_with_cmd(*iter, base.cmd);
                }
            }

            if (base.cmdsize < sizeof(load_command)) {
                this->_destruct();
                return creation_result::invalid_load_command;
            }
            
            if (base.cmdsize > load_commands_size) {
                this->_destruct();
                return creation_result::invalid_load_command;
            }
            
            if (size_taken + base.cmdsize < size_taken) {
                this->_destruct();
                return creation_result::invalid_load_command;
            }
            
            size_taken += base.cmdsize;
            
            if (size_taken > load_commands_size) {
                this->_destruct();
                return creation_result::invalid_load_command;
            }
            
            if (size_taken == load_commands_size) {
                if (index != load_commands_max_index) {
                    this->_destruct();
                    return creation_result::load_commands_area_is_too_small;
                }
            }
         
            iter = reinterpret_cast<load_command *>(reinterpret_cast<uint8_t *>(iter) + base.cmdsize);
            index++;
        }
        
        if (options.enforce_strict_alignment()) {
            if (iter != &load_commands_data[load_commands_size]) {
                this->_destruct();
                return creation_result::misaligned_load_commands;
            }
        }
        
        this->begin = begin;
        this->end = iter;
        
        return creation_result::ok;
    }
    
    void data::_swap_load_command(load_command &load_command) {
        auto cmd = load_command.cmd;
        this->switch_endian_of_load_command_with_cmd(load_command, utils::swap::load_commands::cmd(cmd));
    }
    
    void data::switch_endian_of_load_command_with_cmd(load_command &load_command, macho::load_commands cmd) {
        switch (cmd) {
            case macho::load_commands::segment:
                swap::load_commands::segment_command(static_cast<struct segment_command &>(load_command));
                break;
                
            case macho::load_commands::symbol_table:
                swap::load_commands::symtab_command(static_cast<struct symtab_command &>(load_command));
                break;
            
            case macho::load_commands::symbol_segment:
                break;
                
            case macho::load_commands::thread:
                break;
                
            case macho::load_commands::load_fvmlib:
            case macho::load_commands::identification_fvmlib:
                swap::load_commands::fvmlib_command(static_cast<fvmlib_command &>(load_command));
                break;
                
            case macho::load_commands::identification:
            case macho::load_commands::load_dylib:
            case macho::load_commands::load_weak_dylib:
            case macho::load_commands::load_upward_dylib:
            case macho::load_commands::reexport_dylib:
                swap::load_commands::dylib_command(static_cast<struct dylib_command &>(load_command));
                break;
                
            case macho::load_commands::fvmfile:
                swap::load_commands::fvmfile_command(static_cast<struct fvmfile_command &>(load_command));
                break;
                
            case macho::load_commands::prepage:
                break;
                
            case macho::load_commands::dynamic_symbol_table:
                swap::load_commands::dysymtab_command(static_cast<struct dysymtab_command &>(load_command));
                break;
                
            case macho::load_commands::load_dylinker:
            case macho::load_commands::identification_dylinker:
                swap::load_commands::dylinker_command(static_cast<dylinker_command &>(load_command));
                break;
                
            case macho::load_commands::prebound_dylib:
                swap::load_commands::prebound_dylib_command(static_cast<struct prebound_dylib_command &>(load_command));
                break;
            
            case macho::load_commands::routines:
                swap::load_commands::routines_command(static_cast<struct routines_command &>(load_command));
                break;
                
            case macho::load_commands::sub_framework:
                swap::load_commands::sub_framework_command(static_cast<sub_framework_command &>(load_command));
                break;
                
            case macho::load_commands::sub_umbrella:
                swap::load_commands::sub_umbrella_command(static_cast<sub_umbrella_command &>(load_command));
                break;
                
            case macho::load_commands::sub_client:
                swap::load_commands::sub_client_command(static_cast<sub_client_command &>(load_command));
                break;
                
            case macho::load_commands::sub_library:
                swap::load_commands::sub_library_command(static_cast<sub_library_command &>(load_command));
                break;
                
            case macho::load_commands::twolevel_hints:
                swap::load_commands::twolevel_hints_command(static_cast<struct twolevel_hints_command &>(load_command));
                break;
            
            case macho::load_commands::prebind_checksum:
                swap::load_commands::prebind_cksum_command(static_cast<struct prebind_cksum_command &>(load_command));
                break;
                
            case macho::load_commands::segment_64:
                swap::load_commands::segment_command_64(static_cast<struct segment_command_64 &>(load_command));
                break;
                
            case macho::load_commands::routines_64:
                swap::load_commands::routines_command_64(static_cast<struct routines_command_64 &>(load_command));
                break;
                
            case macho::load_commands::uuid:
                break;
                
            case macho::load_commands::rpath:
                swap::load_commands::rpath_command(static_cast<rpath_command &>(load_command));
                break;
                
            case macho::load_commands::code_signature:
            case macho::load_commands::segment_split_info:
            case macho::load_commands::function_starts:
            case macho::load_commands::data_in_code:
            case macho::load_commands::dylib_code_sign:
            case macho::load_commands::linker_optimization_hint:
                swap::load_commands::linkedit_data_command(static_cast<struct linkedit_data_command &>(load_command));
                break;
                
            case macho::load_commands::encryption_info:
                swap::load_commands::encryption_info_command(static_cast<struct encryption_info_command &>(load_command));
                break;
                
            case macho::load_commands::dyld_info:
            case macho::load_commands::dyld_info_only:
                swap::load_commands::dyld_info_command(static_cast<struct dyld_info_command &>(load_command));
                break;
                
            case macho::load_commands::version_min_macosx:
            case macho::load_commands::version_min_iphoneos:
            case macho::load_commands::version_min_tvos:
            case macho::load_commands::version_min_watchos:
                swap::load_commands::version_min_command(static_cast<version_min_command &>(load_command));
                break;
                
            case macho::load_commands::main:
                swap::load_commands::entry_point_command(static_cast<struct entry_point_command &>(load_command));
                break;
                
            case macho::load_commands::source_version:
                swap::load_commands::source_version_command(static_cast<source_version_command &>(load_command));
                break;
                
            case macho::load_commands::encryption_info_64:
                swap::load_commands::encryption_info_command_64(static_cast<struct encryption_info_command_64 &>(load_command));
                break;
                
            case macho::load_commands::linker_option:
                swap::load_commands::linker_option_command(static_cast<linker_option_command &>(load_command));
                break;
                
            case macho::load_commands::note:
                swap::load_commands::note_command(static_cast<struct note_command &>(load_command));
                break;
                
            case macho::load_commands::build_version:
                swap::load_commands::build_version_command(static_cast<struct build_version_command &>(load_command));
                break;
                
            default:
                break;
        }
    }
    
    uint32_t data::count() const noexcept {
        auto count = uint32_t();
        for (auto iter = this->begin; iter != this->end; iter++) {
            count++;
        }
        
        return count;
    }
    
    void data::_destruct() noexcept {
        if (this->begin.load_command() != nullptr) {
            // We need to cast to const uint8_t * as
            // load-commands buffer was allocated as
            // an array of uint8_t
            
            delete[] reinterpret_cast<const uint8_t *>(this->begin.load_command());
            this->begin = iterator(nullptr);
        }
        
        this->end = nullptr;
    }
}
