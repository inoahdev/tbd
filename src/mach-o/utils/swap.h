//
//  src/mach-o/utils/swap.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include "../headers/architecture.h"
#include "../headers/header.h"
#include "../headers/load_commands.h"
#include "../headers/segments.h"
#include "../headers/symbol_table.h"

namespace macho::utils::swap {
    cputype &cputype(cputype &cputype) noexcept;
    filetype &filetype(filetype &filetype) noexcept;

    header &mach_header(header &header) noexcept;
    fat &fat_header(fat &header) noexcept;

    struct architecture &architecture(struct architecture &arch) noexcept;
    struct architecture_64 &architecture_64(struct architecture_64 &arch) noexcept;

    struct architecture *architectures(struct architecture *archs, uint32_t count) noexcept;
    struct architecture_64 *architectures_64(struct architecture_64 *archs, uint32_t count) noexcept;

    namespace load_commands {
        macho::load_commands &cmd(macho::load_commands &cmd) noexcept;
        load_command &base(load_command &load_cmd) noexcept;

        struct segment_command &segment_command(struct segment_command &segment) noexcept;
        struct segment_command_64 &segment_command_64(struct segment_command_64 &segment) noexcept;

        struct fvmlib_command &fvmlib_command(struct fvmlib_command &fvmlib) noexcept;
        struct dylib_command &dylib_command(struct dylib_command &dylib) noexcept;

        struct sub_framework_command &sub_framework_command(struct sub_framework_command &sub_framework) noexcept;
        struct sub_client_command &sub_client_command(struct sub_client_command &sub_client) noexcept;

        struct sub_umbrella_command &sub_umbrella_command(struct sub_umbrella_command &sub_umbrella) noexcept;
        struct sub_library_command &sub_library_command(struct sub_library_command &sub_library) noexcept;

        struct prebound_dylib_command &prebound_dylib_command(struct prebound_dylib_command &prebound_dylib) noexcept;
        struct dylinker_command &dylinker_command(struct dylinker_command &dylinker) noexcept;

        struct routines_command &routines_command(struct routines_command &routines) noexcept;
        struct routines_command_64 &routines_command_64(struct routines_command_64 &routines) noexcept;

        struct symtab_command &symtab_command(struct symtab_command &symtab) noexcept;
        struct dysymtab_command &dysymtab_command(struct dysymtab_command &dysymtab) noexcept;

        struct twolevel_hints_command &twolevel_hints_command(struct twolevel_hints_command &twolevel_hints) noexcept;
        struct prebind_cksum_command &prebind_cksum_command(struct prebind_cksum_command &prebind_cksum) noexcept;

        struct uuid_command &uuid_command(struct uuid_command &uuid) noexcept;
        struct rpath_command &rpath_command(struct rpath_command &rpath) noexcept;

        struct linkedit_data_command &linkedit_data_command(struct linkedit_data_command &linkedit_data) noexcept;

        struct encryption_info_command &encryption_info_command(struct encryption_info_command &encryption_info) noexcept;
        struct encryption_info_command_64 &encryption_info_command_64(struct encryption_info_command_64 &encryption_info) noexcept;

        struct version_min_command &version_min_command(struct version_min_command &version_min) noexcept;
        struct dyld_info_command &dyld_info_command(struct dyld_info_command &dyld_info) noexcept;

        struct linker_option_command &linker_option_command(struct linker_option_command &linker_option) noexcept;
        struct ident_command &ident_command(struct ident_command &ident) noexcept;

        struct fvmfile_command &fvmfile_command(struct fvmfile_command &fvmfile) noexcept;
        struct entry_point_command &entry_point_command(struct entry_point_command &entry_point) noexcept;

        struct source_version_command &source_version_command(struct source_version_command &source_version) noexcept;

        struct build_version_command &build_version_command(struct build_version_command &build_version) noexcept;
        struct note_command &note_command(struct note_command &note) noexcept;
    }

    namespace segments {
        struct macho::segments::section &section(struct macho::segments::section &section) noexcept;
        struct macho::segments::section_64 &section_64(struct macho::segments::section_64 &section) noexcept;

        struct macho::segments::section *sections(struct macho::segments::section *section, uint32_t count) noexcept;
        struct macho::segments::section_64 *sections_64(struct macho::segments::section_64 *section, uint32_t count) noexcept;
    }

    namespace symbol_table {
        struct macho::symbol_table::entry &entry(struct macho::symbol_table::entry &entry) noexcept;
        struct macho::symbol_table::entry_64 &entry_64(struct macho::symbol_table::entry_64 &entry) noexcept;

        struct macho::symbol_table::entry *entries(struct macho::symbol_table::entry *entries, uint32_t count) noexcept;
        struct macho::symbol_table::entry_64 *entries_64(struct macho::symbol_table::entry_64 *entries, uint32_t count) noexcept;
    }
}
