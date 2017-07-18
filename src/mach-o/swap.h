//
//  src/macho/swap.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <mach-o/nlist.h>

namespace macho {
    void swap_uint16(uint16_t *uint16);
    void swap_uint32(uint32_t *uint32);
    void swap_uint64(uint64_t *uint64);

    void swap_lc_str(lc_str *str);

    void swap_mach_header(struct mach_header *header);
    void swap_fat_header(struct fat_header *header);

    void swap_fat_arch(struct fat_arch *archs, uint32_t count);
    void swap_fat_arch_64(struct fat_arch_64 *archs, uint32_t count);

    void swap_load_command(struct load_command *load_cmd);

    void swap_segment_command(struct segment_command *segment);
    void swap_segment_command_64(struct segment_command_64 *segment);

    void swap_section(struct section *section, uint32_t count);
    void swap_section_64(struct section_64 *section, uint32_t count);

    void swap_fvmlib_command(struct fvmlib_command *fvmlib);
    void swap_dylib_command(struct dylib_command *dylib);

    void swap_sub_framework_command(struct sub_framework_command *sub_framework);
    void swap_sub_client_command(struct sub_client_command *sub_client);

    void swap_sub_umbrella_command(struct sub_umbrella_command *sub_umbrella);
    void swap_sub_library_command(struct sub_library_command *sub_library);

    void swap_prebound_dylib_command(struct prebound_dylib_command *prebound_dylib);
    void swap_dylinker_command(struct dylinker_command *dylinker);

    void swap_routines_command(struct routines_command *routines);
    void swap_routines_command_64(struct routines_command_64 *routines);

    void swap_symtab_command(struct symtab_command *symtab);
    void swap_dysymtab_command(struct dysymtab_command *dysymtab);

    void swap_nlist(struct nlist *nlist, uint32_t count);
    void swap_nlist_64(struct nlist_64 *nlist, uint32_t count);

    void swap_twolevel_hints_command(struct twolevel_hints_command *twolevel_hints);
    void swap_prebind_cksum_command(struct prebind_cksum_command *prebind_cksum);

    void swap_uuid_command(struct uuid_command *uuid);
    void swap_rpath_command(struct rpath_command *rpath);

    void swap_linkedit_data_command(struct linkedit_data_command *linkedit_data);

    void swap_encryption_info_command(struct encryption_info_command *encryption_info);
    void swap_encryption_info_command_64(struct encryption_info_command_64 *encryption_info);

    void swap_version_min_command(struct version_min_command *version_min);
    void swap_dyld_info_command(struct dyld_info_command *dyld_info);

    void swap_linker_option_command(struct linker_option_command *linker_option);
    void swap_ident_command(struct ident_command *ident);

    void swap_fvmfile_command(struct fvmfile_command *fvmfile);
    void swap_entry_point_command(struct entry_point_command *entry_point);

    void swap_source_version_command(struct source_version_command *source_version);
}
