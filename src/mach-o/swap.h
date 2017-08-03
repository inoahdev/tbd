//
//  src/macho/swap.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include "headers/architecture.h"
#include "headers/header.h"
#include "headers/load_commands.h"
#include "headers/symbol_table.h"

namespace macho {
    void swap_uint16(uint16_t *uint16);
    void swap_uint32(uint32_t *uint32);
    void swap_uint64(uint64_t *uint64);

    void swap_lc_str(lc_str *str);

    void swap_mach_header(header *header);
    void swap_fat_header(fat *header);

    void swap_fat_arch(architecture *archs, uint32_t count);
    void swap_fat_arch_64(architecture_64 *archs, uint32_t count);

    void swap_load_command(load_command *load_cmd);

    void swap_segment_command(segment_command *segment);
    void swap_segment_command_64(segment_command_64 *segment);

    void swap_fvmlib_command(fvmlib_command *fvmlib);
    void swap_dylib_command(dylib_command *dylib);

    void swap_sub_framework_command(sub_framework_command *sub_framework);
    void swap_sub_client_command(sub_client_command *sub_client);

    void swap_sub_umbrella_command(sub_umbrella_command *sub_umbrella);
    void swap_sub_library_command(sub_library_command *sub_library);

    void swap_prebound_dylib_command(prebound_dylib_command *prebound_dylib);
    void swap_dylinker_command(dylinker_command *dylinker);

    void swap_routines_command(routines_command *routines);
    void swap_routines_command_64(routines_command_64 *routines);

    void swap_symtab_command(symtab_command *symtab);
    void swap_dysymtab_command(dysymtab_command *dysymtab);

    void swap_nlist(nlist *nlist, uint32_t count);
    void swap_nlist_64(nlist_64 *nlist, uint32_t count);

    void swap_twolevel_hints_command(twolevel_hints_command *twolevel_hints);
    void swap_prebind_cksum_command(prebind_cksum_command *prebind_cksum);

    void swap_uuid_command(uuid_command *uuid);
    void swap_rpath_command(rpath_command *rpath);

    void swap_linkedit_data_command(linkedit_data_command *linkedit_data);

    void swap_encryption_info_command(encryption_info_command *encryption_info);
    void swap_encryption_info_command_64(encryption_info_command_64 *encryption_info);

    void swap_version_min_command(version_min_command *version_min);
    void swap_dyld_info_command(dyld_info_command *dyld_info);

    void swap_linker_option_command(linker_option_command *linker_option);
    void swap_ident_command(ident_command *ident);

    void swap_fvmfile_command(fvmfile_command *fvmfile);
    void swap_entry_point_command(entry_point_command *entry_point);

    void swap_source_version_command(source_version_command *source_version);
}
