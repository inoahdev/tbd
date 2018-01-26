//
//  src/mach-o/utilsswap.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "../../utils/swap.h"
#include "swap.h"

namespace macho::utils::swap {
    enum cputype &cputype(enum cputype &cputype) noexcept {
        ::utils::swap::int32(*reinterpret_cast<int32_t *>(&cputype));
        return cputype;
    }
    
    enum filetype &filetype(enum filetype &filetype) noexcept {
        ::utils::swap::uint32(*reinterpret_cast<uint32_t *>(&filetype));
        return filetype;
    }
    
    header &mach_header(header &header) noexcept {
        cputype(header.cputype);
        ::utils::swap::int32(header.cpusubtype);
        filetype(header.filetype);
        ::utils::swap::uint32(header.ncmds);
        ::utils::swap::uint32(header.sizeofcmds);
        ::utils::swap::uint32(header.flags.value);
        
        return header;
    }

    fat &fat_header(fat &header) noexcept {
        ::utils::swap::uint32(header.nfat_arch);
        return header;
    }

    struct architecture &architecture(struct architecture &arch) noexcept {
        ::utils::swap::int32(arch.cputype);
        ::utils::swap::int32(arch.cpusubtype);
        ::utils::swap::uint32(arch.offset);
        ::utils::swap::uint32(arch.size);
        ::utils::swap::uint32(arch.align);
        
        return arch;
    }

    struct architecture_64 &architecture_64(struct architecture_64 &arch) noexcept {
        ::utils::swap::int32(arch.cputype);
        ::utils::swap::int32(arch.cpusubtype);
        ::utils::swap::uint64(arch.offset);
        ::utils::swap::uint64(arch.size);
        ::utils::swap::uint32(arch.align);
        
        return arch;
    }

    struct architecture *architectures(struct architecture *archs, uint32_t count) noexcept {
        auto iter = archs;
        while (count) {
            architecture(*iter);

            iter++;
            count--;
        }
        
        return archs;
    }

    struct architecture_64 *architectures_64(struct architecture_64 *archs, uint32_t count) noexcept {
        auto iter = archs;
        while (count) {
            architecture_64(*iter);

            iter++;
            count--;
        }
        
        return archs;
    }

    namespace load_commands {
        macho::load_commands &cmd(macho::load_commands &cmd) noexcept {
            ::utils::swap::uint32(*reinterpret_cast<uint32_t *>(&cmd));
            return cmd;
        }
        
        load_command &base(load_command &load_cmd) noexcept {
            cmd(load_cmd.cmd);
            ::utils::swap::uint32(load_cmd.cmdsize);
            
            return load_cmd;
        }
        
        struct segment_command &segment_command(struct segment_command &segment) noexcept {
            base(segment);
            
            ::utils::swap::uint32(segment.vmaddr);
            ::utils::swap::uint32(segment.vmsize);
            
            ::utils::swap::uint32(segment.fileoff);
            ::utils::swap::uint32(segment.filesize);
            
            ::utils::swap::int32(segment.maxprot);
            ::utils::swap::int32(segment.initprot);
            
            ::utils::swap::uint32(segment.nsects);
            ::utils::swap::uint32(segment.flags);
            
            return segment;
        }
        
        struct segment_command_64 &segment_command_64(struct segment_command_64 &segment) noexcept {
            base(segment);
            
            ::utils::swap::uint64(segment.vmaddr);
            ::utils::swap::uint64(segment.vmsize);
            
            ::utils::swap::uint64(segment.fileoff);
            ::utils::swap::uint64(segment.filesize);
            
            ::utils::swap::int32(segment.maxprot);
            ::utils::swap::int32(segment.initprot);
            
            ::utils::swap::uint32(segment.nsects);
            ::utils::swap::uint32(segment.flags);
            
            return segment;
        }
        
        struct fvmlib_command &fvmlib_command(struct fvmlib_command &fvmlib) noexcept {
            base(fvmlib);
            ::utils::swap::uint32(fvmlib.name.offset);
            
            return fvmlib;
        }
        
        struct dylib_command &dylib_command(struct dylib_command &dylib) noexcept {
            base(dylib);
            
            ::utils::swap::uint32(dylib.name.offset);
            ::utils::swap::uint32(dylib.current_version);
            ::utils::swap::uint32(dylib.compatibility_version);
            
            return dylib;
        }
        
        struct sub_framework_command &sub_framework_command(struct sub_framework_command &sub_framework) noexcept {
            base(sub_framework);
            ::utils::swap::uint32(sub_framework.umbrella.offset);
            
            return sub_framework;
        }
        
        struct sub_client_command &sub_client_command(struct sub_client_command &sub_client) noexcept {
            base(sub_client);
            ::utils::swap::uint32(sub_client.client.offset);
            
            return sub_client;
        }
        
        struct sub_umbrella_command &sub_umbrella_command(struct sub_umbrella_command &sub_umbrella) noexcept {
            base(sub_umbrella);
            ::utils::swap::uint32(sub_umbrella.sub_umbrella.offset);
            
            return sub_umbrella;
        }
        
        struct sub_library_command &sub_library_command(struct sub_library_command &sub_library) noexcept {
            base(sub_library);
            ::utils::swap::uint32(sub_library.sub_library.offset);
            
            return sub_library;
        }
        
        struct prebound_dylib_command &prebound_dylib_command(struct prebound_dylib_command &prebound_dylib) noexcept {
            base(prebound_dylib);
            
            ::utils::swap::uint32(prebound_dylib.name.offset);
            ::utils::swap::uint32(prebound_dylib.linked_modules.offset);
            ::utils::swap::uint32(prebound_dylib.nmodules);
            
            return prebound_dylib;
        }
        
        struct dylinker_command &dylinker_command(struct dylinker_command &dylinker) noexcept {
            base(dylinker);
            ::utils::swap::uint32(dylinker.name.offset);
            
            return dylinker;
        }
        
        struct routines_command &routines_command(struct routines_command &routines) noexcept {
            base(routines);
            
            ::utils::swap::uint32(routines.init_address);
            ::utils::swap::uint32(routines.init_module);
            
            ::utils::swap::uint32(routines.reserved1);
            ::utils::swap::uint32(routines.reserved2);
            ::utils::swap::uint32(routines.reserved3);
            ::utils::swap::uint32(routines.reserved4);
            ::utils::swap::uint32(routines.reserved5);
            ::utils::swap::uint32(routines.reserved6);
            
            return routines;
        }
        
        struct routines_command_64 &routines_command_64(struct routines_command_64 &routines) noexcept {
            base(routines);
            
            ::utils::swap::uint64(routines.init_address);
            ::utils::swap::uint64(routines.init_module);
            
            ::utils::swap::uint64(routines.reserved1);
            ::utils::swap::uint64(routines.reserved2);
            ::utils::swap::uint64(routines.reserved3);
            ::utils::swap::uint64(routines.reserved4);
            ::utils::swap::uint64(routines.reserved5);
            ::utils::swap::uint64(routines.reserved6);
            
            return routines;
        }
        
        struct symtab_command &symtab_command(struct symtab_command &symtab) noexcept {
            base(symtab);
            
            ::utils::swap::uint32(symtab.symoff);
            ::utils::swap::uint32(symtab.nsyms);
            
            ::utils::swap::uint32(symtab.stroff);
            ::utils::swap::uint32(symtab.strsize);
            
            return symtab;
        }
        
        struct dysymtab_command &dysymtab_command(struct dysymtab_command &dysymtab) noexcept {
            base(dysymtab);
            
            ::utils::swap::uint32(dysymtab.ilocalsym);
            ::utils::swap::uint32(dysymtab.nlocalsym);
            
            ::utils::swap::uint32(dysymtab.tocoff);
            ::utils::swap::uint32(dysymtab.ntoc);
            
            ::utils::swap::uint32(dysymtab.modtaboff);
            ::utils::swap::uint32(dysymtab.nmodtab);
            
            ::utils::swap::uint32(dysymtab.extrefsymoff);
            ::utils::swap::uint32(dysymtab.nextrefsyms);
            
            ::utils::swap::uint32(dysymtab.indirectsymoff);
            ::utils::swap::uint32(dysymtab.nindirectsyms);
            
            ::utils::swap::uint32(dysymtab.extreloff);
            ::utils::swap::uint32(dysymtab.nextrel);
            
            return dysymtab;
        }
        
        struct twolevel_hints_command &twolevel_hints_command(struct twolevel_hints_command &twolevel_hints) noexcept {
            base(twolevel_hints);
            
            ::utils::swap::uint32(twolevel_hints.offset);
            ::utils::swap::uint32(twolevel_hints.nhints);
            
            return twolevel_hints;
        }
        
        struct prebind_cksum_command &prebind_cksum_command(struct prebind_cksum_command &prebind_cksum) noexcept {
            base(prebind_cksum);
            ::utils::swap::uint32(prebind_cksum.cksum);
            
            return prebind_cksum;
        }
        
        struct uuid_command &uuid_command(struct uuid_command &uuid) noexcept {
            base(uuid);
            return uuid;
        }
        
        struct rpath_command &rpath_command(struct rpath_command &rpath) noexcept {
            base(rpath);
            ::utils::swap::uint32(rpath.path.offset);
            
            return rpath;
        }
        
        struct linkedit_data_command &linkedit_data_command(struct linkedit_data_command &linkedit_data) noexcept {
            base(linkedit_data);
            
            ::utils::swap::uint32(linkedit_data.dataoff);
            ::utils::swap::uint32(linkedit_data.datasize);
            
            return linkedit_data;
        }
        
        struct encryption_info_command &encryption_info_command(struct encryption_info_command &encryption_info) noexcept {
            base(encryption_info);
            
            ::utils::swap::uint32(encryption_info.cryptoff);
            ::utils::swap::uint32(encryption_info.cryptsize);
            ::utils::swap::uint32(encryption_info.cryptid);
            
            return encryption_info;
        }
        
        struct encryption_info_command_64 &encryption_info_command_64(struct encryption_info_command_64 &encryption_info) noexcept {
            base(encryption_info);
            
            ::utils::swap::uint32(encryption_info.cryptoff);
            ::utils::swap::uint32(encryption_info.cryptsize);
            ::utils::swap::uint32(encryption_info.cryptid);
            ::utils::swap::uint32(encryption_info.pad);
            
            return encryption_info;
        }
        
        struct version_min_command &version_min_command(struct version_min_command &version_min) noexcept {
            base(version_min);
            ::utils::swap::uint32(version_min.version);
            
            return version_min;
        }
        
        struct dyld_info_command &dyld_info_command(struct dyld_info_command &dyld_info) noexcept {
            base(dyld_info);
            
            ::utils::swap::uint32(dyld_info.rebase_off);
            ::utils::swap::uint32(dyld_info.rebase_size);
            
            ::utils::swap::uint32(dyld_info.bind_off);
            ::utils::swap::uint32(dyld_info.bind_size);
            
            ::utils::swap::uint32(dyld_info.weak_bind_off);
            ::utils::swap::uint32(dyld_info.weak_bind_size);
            
            ::utils::swap::uint32(dyld_info.lazy_bind_off);
            ::utils::swap::uint32(dyld_info.lazy_bind_size);
            
            ::utils::swap::uint32(dyld_info.export_off);
            ::utils::swap::uint32(dyld_info.export_size);
            
            return dyld_info;
        }
        
        struct linker_option_command &linker_option_command(struct linker_option_command &linker_option) noexcept {
            base(linker_option);
            ::utils::swap::uint32(linker_option.count);
        
            return linker_option;
        }
        
        struct ident_command &ident_command(struct ident_command &ident) noexcept {
            base(ident);
            return ident;
        }
        
        struct fvmfile_command &fvmfile_command(struct fvmfile_command &fvmfile) noexcept {
            base(fvmfile);
            
            ::utils::swap::uint32(fvmfile.name.offset);
            ::utils::swap::uint32(fvmfile.header_addr);
            
            return fvmfile;
        }
        
        struct entry_point_command &entry_point_command(struct entry_point_command &entry_point) noexcept {
            base(entry_point);
            
            ::utils::swap::uint64(entry_point.entryoff);
            ::utils::swap::uint64(entry_point.stacksize);
            
            return entry_point;
        }
        
        struct source_version_command &source_version_command(struct source_version_command &source_version) noexcept {
            base(source_version);
            ::utils::swap::uint64(source_version.version);
            
            return source_version;
        }
        
        struct build_version_command &build_version_command(struct build_version_command &build_version) noexcept {
            base(build_version);
            
            ::utils::swap::uint32(build_version.minos);
            ::utils::swap::uint32(build_version.ntools);
            
            return build_version;
        }
        
        struct note_command &note_command(struct note_command &note) noexcept {
            base(note);
            
            ::utils::swap::uint64(note.offset);
            ::utils::swap::uint64(note.size);
            
            return note;
        }
    }
    
    namespace segments {
        struct macho::segments::section &section(struct macho::segments::section &section) noexcept {
            ::utils::swap::uint32(section.addr);
            ::utils::swap::uint32(section.size);
            ::utils::swap::uint32(section.offset);
            ::utils::swap::uint32(section.align);
            ::utils::swap::uint32(section.reloff);
            ::utils::swap::uint32(section.nreloc);
            ::utils::swap::uint32(section.flags);
            ::utils::swap::uint32(section.reserved1);
            ::utils::swap::uint32(section.reserved2);
            
            return section;
        }
        
        struct macho::segments::section_64 &section_64(struct macho::segments::section_64 &section) noexcept {
            ::utils::swap::uint64(section.addr);
            ::utils::swap::uint64(section.size);
            ::utils::swap::uint32(section.offset);
            ::utils::swap::uint32(section.align);
            ::utils::swap::uint32(section.reloff);
            ::utils::swap::uint32(section.nreloc);
            ::utils::swap::uint32(section.flags);
            ::utils::swap::uint32(section.reserved1);
            ::utils::swap::uint32(section.reserved2);
            
            return section;
        }
        
        struct macho::segments::section *sections(struct macho::segments::section *section, uint32_t count) noexcept {
            auto iter = section;
            while (count != 0) {
                segments::section(*iter);
                
                iter++;
                count--;
            }
            
            return section;
        }
        
        struct macho::segments::section_64 *sections_64(struct macho::segments::section_64 *section, uint32_t count) noexcept {
            auto iter = section;
            while (count != 0) {
                section_64(*iter);
                
                iter++;
                count--;
            }
            
            return section;
        }
    }

    namespace symbol_table {
        struct macho::symbol_table::entry &entry(struct macho::symbol_table::entry &entry) noexcept {
            ::utils::swap::uint32(entry.n_un.n_strx);
            
            ::utils::swap::uint16(*reinterpret_cast<uint16_t *>(&entry.n_desc));
            ::utils::swap::uint32(entry.n_value);
            
            return entry;
        }
        
        struct macho::symbol_table::entry_64 &entry_64(struct macho::symbol_table::entry_64 &entry) noexcept {
            ::utils::swap::uint32(entry.n_un.n_strx);
            
            ::utils::swap::uint16(*reinterpret_cast<uint16_t *>(&entry.n_desc));
            ::utils::swap::uint64(entry.n_value);
            
            return entry;
        }
        
        struct macho::symbol_table::entry *entries(struct macho::symbol_table::entry *entries, uint32_t count) noexcept {
            auto iter = entries;
            while (count) {
                entry(*iter);
                
                iter++;
                count--;
            }
            
            return entries;
        }
        
        struct macho::symbol_table::entry_64 *entries_64(struct macho::symbol_table::entry_64 *entries, uint32_t count) noexcept {
            auto iter = entries;
            while (count) {
                entry_64(*iter);
                
                iter++;
                count--;
            }
            
            return iter;
        }
    }
}
