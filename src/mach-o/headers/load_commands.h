//
//  src/mach-o/headers/load_commands.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace macho {
    enum class load_commands : uint32_t {
        segment = 1,
        symbol_table,
        symbol_segment,
        thread,
        unix_thread,
        load_fvmlib,
        identification_fvmlib,
        identification,
        fvmfile,
        prepage,
        dynamic_symbol_table,
        load_dylib,
        identification_dylib,
        load_dylinker,
        identification_dylinker,
        prebound_dylib,
        routines,
        sub_framework,
        sub_umbrella,
        sub_client,
        sub_library,
        twolevel_hints,
        prebind_checksum,

        load_weak_dylib,
        segment_64,
        routines_64,
        uuid,
        rpath = 0x1c | 0x80000000,
        code_signature = 0x1d,
        segment_split_info,
        reexport_dylib = 0x1f | 0x80000000,
        lazy_load_dylib = 0x20,
        encryption_info,
        dyld_info,
        dyld_info_only = 0x22 | 0x80000000,
        load_upward_dylib = 0x23 | 0x80000000,
        version_min_macosx = 0x24,
        version_min_iphoneos,
        function_starts,
        dyld_environment,

        main = 0x28 | 0x80000000,
        data_in_code = 0x29,
        source_version,
        dylib_code_sign,
        encryption_info_64,
        linker_option,
        linker_optimization_hint,
        version_min_tvos,
        version_min_watchos,

        note,
        build_version,
    };

    union lc_str {
        uint32_t offset;
    };

    struct load_command {
        load_commands cmd;
        uint32_t cmdsize;
    };

    struct segment_command : load_command {
        char segname[16];
        uint32_t vmaddr;
        uint32_t vmsize;
        uint32_t fileoff;
        uint32_t filesize;
        int32_t	 maxprot;
        int32_t  initprot;
        uint32_t nsects;
        uint32_t flags;
    };

    struct segment_command_64 : load_command {
        char segname[16];
        uint64_t vmaddr;
        uint64_t vmsize;
        uint64_t fileoff;
        uint64_t filesize;
        int32_t	 maxprot;
        int32_t  initprot;
        uint32_t nsects;
        uint32_t flags;
    };

    struct fvmlib_command : load_command {
        union lc_str name;

        uint32_t minor_version;
        uint32_t header_addr;
    };

    struct dylib_command : load_command {
        union lc_str name;

        uint32_t timestamp;
        uint32_t current_version;
        uint32_t compatibility_version;
    };

    struct sub_framework_command : load_command {
        union lc_str umbrella;
    };

    struct sub_client_command : load_command {
        union lc_str client;
    };

    struct sub_umbrella_command : load_command {
        union lc_str sub_umbrella;
    };

    struct sub_library_command : load_command {
        union lc_str sub_library;
    };

    struct prebound_dylib_command : load_command {
        union lc_str name;

        uint32_t nmodules;
        union lc_str linked_modules;
    };

    struct dylinker_command : load_command {
        union lc_str name;
    };

    struct routines_command : load_command {
        uint32_t init_address;
        uint32_t init_module;

        uint32_t reserved1;
        uint32_t reserved2;
        uint32_t reserved3;
        uint32_t reserved4;
        uint32_t reserved5;
        uint32_t reserved6;
    };

    struct routines_command_64 : load_command {
        uint64_t init_address;
        uint64_t init_module;

        uint64_t reserved1;
        uint64_t reserved2;
        uint64_t reserved3;
        uint64_t reserved4;
        uint64_t reserved5;
        uint64_t reserved6;
    };

    struct symtab_command : load_command {
        uint32_t symoff;
        uint32_t nsyms;
        uint32_t stroff;
        uint32_t strsize;
    };

    struct dysymtab_command : load_command {
        uint32_t ilocalsym;
        uint32_t nlocalsym;

        uint32_t iextdefsym;
        uint32_t nextdefsym;

        uint32_t iundefsym;
        uint32_t nundefsym;

        uint32_t tocoff;
        uint32_t ntoc;

        uint32_t modtaboff;
        uint32_t nmodtab;

        uint32_t extrefsymoff;
        uint32_t nextrefsyms;

        uint32_t indirectsymoff;
        uint32_t nindirectsyms;

        uint32_t extreloff;
        uint32_t nextrel;

        uint32_t locreloff;
        uint32_t nlocrel;
    };

    struct twolevel_hints_command : load_command {
        uint32_t offset;
        uint32_t nhints;
    };

    struct prebind_cksum_command : load_command {
        uint32_t cksum;
    };

    struct uuid_command : load_command {
        uint8_t	uuid[16];
    };

    struct rpath_command : load_command {
        union lc_str path;
    };

    struct linkedit_data_command : load_command {
        uint32_t dataoff;
        uint32_t datasize;
    };

    struct encryption_info_command : load_command {
        uint32_t cryptoff;
        uint32_t cryptsize;
        uint32_t cryptid;
    };

     struct encryption_info_command_64 : load_command {
        uint32_t cryptoff;
        uint32_t cryptsize;
        uint32_t cryptid;

        uint32_t pad;
    };

    struct version_min_command : load_command {
        uint32_t version;
        uint32_t sdk;
    };

    struct dyld_info_command : load_command {
        uint32_t rebase_off;
        uint32_t rebase_size;

        uint32_t bind_off;
        uint32_t bind_size;

        uint32_t weak_bind_off;
        uint32_t weak_bind_size;

        uint32_t lazy_bind_off;
        uint32_t lazy_bind_size;

        uint32_t export_off;
        uint32_t export_size;
    };

    struct linker_option_command {
        uint32_t count;
    };

    struct symseg_command : load_command {
        uint32_t offset;
        uint32_t size;
    };

    struct ident_command : load_command {};

    struct fvmfile_command : load_command {
        union lc_str name;
        uint32_t header_addr;
    };

    struct entry_point_command : load_command {
        uint64_t entryoff;
        uint64_t stacksize;
    };

    struct source_version_command : load_command {
        uint64_t version;
    };

    struct note_command : load_command {
        char data_owner[16];
        uint64_t offset;
        uint64_t size;
    };

    struct build_version_command : load_command {
        uint32_t platform;
        uint32_t minos;
        uint32_t sdk;
        uint32_t ntools;
    };
}
