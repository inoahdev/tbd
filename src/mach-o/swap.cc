//
//  src/mach-o/swap.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include "swap.h"

namespace macho {
    void swap_uint16(uint16_t *uint16) {
        *uint16 = ((*uint16 >>  8) & 0x00ff) | ((*uint16 <<  8) & 0xff00);
    }

    void swap_uint32(uint32_t *uint32) {
         *uint32 = ((*uint32 >>  8) & 0x00ff00ff) | ((*uint32 <<  8) & 0xff00ff00);
         *uint32 = ((*uint32 >> 16) & 0x0000ffff) | ((*uint32 << 16) & 0xffff0000);
    }

    void swap_uint64(uint64_t *uint64) {
        *uint64 = (*uint64 & 0x00000000ffffffff) << 32 | (*uint64 & 0xffffffff00000000) >> 32;
        *uint64 = (*uint64 & 0x0000ffff0000ffff) << 16 | (*uint64 & 0xffff0000ffff0000) >> 16;
        *uint64 = (*uint64 & 0x00ff00ff00ff00ff) << 8  | (*uint64 & 0xff00ff00ff00ff00) >> 8;
    }

    void swap_lc_str(lc_str *str) {
        swap_uint32(&str->offset);
    }

    void swap_mach_header(header *header) {
        swap_uint32((uint32_t *)&header->cputype);
        swap_uint32((uint32_t *)&header->cpusubtype);
        swap_uint32((uint32_t *)&header->filetype);
        swap_uint32(&header->ncmds);
        swap_uint32(&header->sizeofcmds);
        swap_uint32(&header->flags);
    }

    void swap_fat_header(fat *header) {
        swap_uint32(&header->nfat_arch);
    }

    void swap_fat_arch(architecture *archs, uint32_t count) {
        while (count) {
            swap_uint32((uint32_t *)&archs->cputype);
            swap_uint32((uint32_t *)&archs->cpusubtype);
            swap_uint32(&archs->offset);
            swap_uint32(&archs->size);
            swap_uint32(&archs->align);

            archs++;
            count--;
        }
    }

    void swap_fat_arch_64(architecture_64 *archs, uint32_t count) {
        while (count) {
            swap_uint32((uint32_t *)&archs->cputype);
            swap_uint32((uint32_t *)&archs->cpusubtype);
            swap_uint64(&archs->offset);
            swap_uint64(&archs->size);
            swap_uint32(&archs->align);

            archs++;
            count--;
        }
    }

    void swap_load_command(load_command *load_cmd) {
        swap_uint32((uint32_t *)&load_cmd->cmd);
        swap_uint32(&load_cmd->cmdsize);
    }

    void swap_segment_command(segment_command *segment) {
        swap_load_command((struct load_command *)segment);

        swap_uint32(&segment->vmaddr);
        swap_uint32(&segment->vmsize);

        swap_uint32(&segment->fileoff);
        swap_uint32(&segment->filesize);

        swap_uint32((uint32_t *)&segment->maxprot);
        swap_uint32((uint32_t *)&segment->initprot);

        swap_uint32(&segment->nsects);
        swap_uint32(&segment->flags);
    }

    void swap_segment_command_64(segment_command_64 *segment) {
        swap_load_command((struct load_command *)segment);

        swap_uint64(&segment->vmaddr);
        swap_uint64(&segment->vmsize);

        swap_uint64(&segment->fileoff);
        swap_uint64(&segment->filesize);

        swap_uint32((uint32_t *)&segment->maxprot);
        swap_uint32((uint32_t *)&segment->initprot);

        swap_uint32(&segment->nsects);
        swap_uint32(&segment->flags);
    }

    void swap_fvmlib_command(fvmlib_command *fvmlib) {
        swap_load_command((struct load_command *)fvmlib);
        swap_uint32(&fvmlib->name.offset);
    }

    void swap_dylib_command(dylib_command *dylib) {
        swap_load_command((struct load_command *)dylib);

        swap_uint32(&dylib->name.offset);
        swap_uint32(&dylib->current_version);
        swap_uint32(&dylib->compatibility_version);
    }

    void swap_sub_framework_command(sub_framework_command *sub_framework) {
        swap_load_command((struct load_command *)sub_framework);
        swap_lc_str(&sub_framework->umbrella);
    }

    void swap_sub_client_command(sub_client_command *sub_client) {
        swap_load_command((struct load_command *)sub_client);
        swap_lc_str(&sub_client->client);
    }

    void swap_sub_umbrella_command(sub_umbrella_command *sub_umbrella) {
        swap_load_command((struct load_command *)sub_umbrella);
        swap_lc_str(&sub_umbrella->sub_umbrella);;
    }

    void swap_sub_library_command(sub_library_command *sub_library) {
        swap_load_command((struct load_command *)sub_library);
        swap_lc_str(&sub_library->sub_library);
    }

    void swap_prebound_dylib_command(prebound_dylib_command *prebound_dylib) {
        swap_load_command((struct load_command *)prebound_dylib);

        swap_lc_str(&prebound_dylib->name);
        swap_lc_str(&prebound_dylib->linked_modules);

        swap_uint32(&prebound_dylib->nmodules);
    }

    void swap_dylinker_command(dylinker_command *dylinker) {
        swap_load_command((struct load_command *)dylinker);
        swap_lc_str(&dylinker->name);
    }

    void swap_routines_command(routines_command *routines) {
        swap_load_command((struct load_command *)routines);

        swap_uint32(&routines->init_address);
        swap_uint32(&routines->init_module);

        swap_uint32(&routines->reserved1);
        swap_uint32(&routines->reserved2);
        swap_uint32(&routines->reserved3);
        swap_uint32(&routines->reserved4);
        swap_uint32(&routines->reserved5);
        swap_uint32(&routines->reserved6);
    }

    void swap_routines_command_64(routines_command_64 *routines) {
        swap_load_command((struct load_command *)routines);

        swap_uint64(&routines->init_address);
        swap_uint64(&routines->init_module);

        swap_uint64(&routines->reserved1);
        swap_uint64(&routines->reserved2);
        swap_uint64(&routines->reserved3);
        swap_uint64(&routines->reserved4);
        swap_uint64(&routines->reserved5);
        swap_uint64(&routines->reserved6);
    }

    void swap_symtab_command(symtab_command *symtab) {
        swap_load_command((struct load_command *)symtab);

        swap_uint32(&symtab->symoff);
        swap_uint32(&symtab->nsyms);
        swap_uint32(&symtab->stroff);
        swap_uint32(&symtab->strsize);
    }

    void swap_dysymtab_command(dysymtab_command *dysymtab) {
        swap_load_command((struct load_command *)dysymtab);

        swap_uint32(&dysymtab->ilocalsym);
        swap_uint32(&dysymtab->nlocalsym);

        swap_uint32(&dysymtab->tocoff);
        swap_uint32(&dysymtab->ntoc);

        swap_uint32(&dysymtab->modtaboff);
        swap_uint32(&dysymtab->nmodtab);

        swap_uint32(&dysymtab->extrefsymoff);
        swap_uint32(&dysymtab->nextrefsyms);

        swap_uint32(&dysymtab->indirectsymoff);
        swap_uint32(&dysymtab->nindirectsyms);

        swap_uint32(&dysymtab->extreloff);
        swap_uint32(&dysymtab->nextrel);
    }

    void swap_nlist(nlist *nlist, uint32_t count) {
        while (count) {
            swap_uint32(&nlist->n_un.n_strx);

            swap_uint16((uint16_t *)&nlist->n_desc);
            swap_uint32(&nlist->n_value);

            nlist++;
            count--;
        }
    }

    void swap_nlist_64(nlist_64 *nlist, uint32_t count) {
        while (count) {
            swap_uint32(&nlist->n_un.n_strx);

            swap_uint16((uint16_t *)&nlist->n_desc);
            swap_uint64(&nlist->n_value);

            nlist++;
            count--;
        }
    }

    void swap_twolevel_hints_command(twolevel_hints_command *twolevel_hints) {
        swap_load_command((struct load_command *)twolevel_hints);

        swap_uint32(&twolevel_hints->offset);
        swap_uint32(&twolevel_hints->nhints);
    }

    void swap_prebind_cksum_command(prebind_cksum_command *prebind_cksum) {
        swap_load_command((struct load_command *)prebind_cksum);
        swap_uint32(&prebind_cksum->cksum);
    }

    void swap_uuid_command(uuid_command *uuid) {
        swap_load_command((struct load_command *)uuid);
    }

    void swap_rpath_command(rpath_command *rpath) {
        swap_load_command((struct load_command *)rpath);
        swap_lc_str(&rpath->path);
    }

    void swap_linkedit_data_command(linkedit_data_command *linkedit_data) {
        swap_load_command((struct load_command *)linkedit_data);

        swap_uint32(&linkedit_data->dataoff);
        swap_uint32(&linkedit_data->datasize);
    }

    void swap_encryption_info_command(encryption_info_command *encryption_info) {
        swap_load_command((struct load_command *)encryption_info);

        swap_uint32(&encryption_info->cryptoff);
        swap_uint32(&encryption_info->cryptsize);
        swap_uint32(&encryption_info->cryptid);
    }

    void swap_encryption_info_command_64(encryption_info_command_64 *encryption_info) {
        swap_load_command((struct load_command *)encryption_info);

        swap_uint32(&encryption_info->cryptoff);
        swap_uint32(&encryption_info->cryptsize);
        swap_uint32(&encryption_info->cryptid);
        swap_uint32(&encryption_info->pad);
    }

    void swap_version_min_command(version_min_command *version_min) {
        swap_load_command((struct load_command *)version_min);
        swap_uint32(&version_min->version);
    }

    void swap_dyld_info_command(dyld_info_command *dyld_info) {
        swap_load_command((struct load_command *)dyld_info);

        swap_uint32(&dyld_info->rebase_off);
        swap_uint32(&dyld_info->rebase_size);

        swap_uint32(&dyld_info->bind_off);
        swap_uint32(&dyld_info->bind_size);

        swap_uint32(&dyld_info->weak_bind_off);
        swap_uint32(&dyld_info->weak_bind_size);

        swap_uint32(&dyld_info->lazy_bind_off);
        swap_uint32(&dyld_info->lazy_bind_size);

        swap_uint32(&dyld_info->export_off);
        swap_uint32(&dyld_info->export_size);
    }

    void swap_linker_option_command(linker_option_command *linker_option) {
        swap_load_command((struct load_command *)linker_option);
        swap_uint32(&linker_option->count);
    }

    void swap_ident_command(ident_command *ident) {
        swap_load_command((struct load_command *)ident);
    }

    void swap_fvmfile_command(fvmfile_command *fvmfile) {
        swap_load_command((struct load_command *)fvmfile);

        swap_lc_str(&fvmfile->name);
        swap_uint32(&fvmfile->header_addr);
    }

    void swap_entry_point_command(entry_point_command *entry_point) {
        swap_load_command((struct load_command *)entry_point);

        swap_uint64(&entry_point->entryoff);
        swap_uint64(&entry_point->stacksize);
    }

    void swap_source_version_command(source_version_command *source_version) {
        swap_load_command((struct load_command *)source_version);
        swap_uint64(&source_version->version);
    }
}
