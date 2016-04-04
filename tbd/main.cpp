//
//  main.cpp
//  tbd
//
//  Created by Anonymous on 4/1/16.
//  Copyright © 2016 iNoahDev. All rights reserved.
//

#include <algorithm>
#include <string.h>
#include <iostream>
#include <string>
#include <locale>
#include <vector>
#include <map>
#include <ostream>
#include <sstream>      
#include <fstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <mach-o/loader.h>
#include <mach-o/arch.h>
#include <mach-o/fat.h>
#include <mach-o/nlist.h>

#define error(err) "\x1B[31mError:\x1B[0m " << err << std::endl
#define error_(err) "\x1B[31mError:\x1B[0m " << err

uint32_t magic;

extern "C" void NSINoahDev();

bool bits64 = false;
bool fatBits = false;
bool bigEndian = false;

extern "C" void macho_get_segment_by_name_64();

uint32_t swap(uint32_t value) {
    if (bigEndian) {
        value = ((value >>  8) & 0x00ff00ff) | ((value <<  8) & 0xff00ff00);
        value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);
    }
    
    return value;
}

uint32_t swap_(uint32_t magic, uint32_t value) {
    bool isBigEndian = false;
    switch (magic) {
        case MH_CIGAM:
        case MH_CIGAM_64:
        case FAT_CIGAM:
            isBigEndian = true;
            break;
            
        default:
            break;
    }
    
    if (isBigEndian) {
        value = ((value >>  8) & 0x00ff00ff) | ((value <<  8) & 0xff00ff00);
        value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);
    }
    
    return value;
}

struct Symbol {
    Symbol(struct nlist *nl, const char *strtab, uint32_t magic) {
        name = &strtab[swap_(magic, nl->n_un.n_strx)];
        
        this->type = swap_(magic, nl->n_type);
        this->sect = swap_(magic, nl->n_sect);
        this->desc = swap_(magic, nl->n_desc);
        this->value = swap_(magic, nl->n_value);
        this->index = swap_(magic, nl->n_un.n_strx);
    }
    
    Symbol(struct nlist_64 *nl, const char *strtab, uint32_t magic) {
        name = &strtab[swap_(magic, nl->n_un.n_strx)];
        
        this->type = swap_(magic, nl->n_type);
        this->sect = swap_(magic, nl->n_sect);
        this->desc = swap_(magic, nl->n_desc);
        this->value = swap_(magic, nl->n_value);
        this->index = swap_(magic, nl->n_un.n_strx);
    }
    
    std::string name;
    uint8_t type;        /* type flag, see below */
    uint8_t sect;        /* section number or NO_SECT */
    uint16_t desc;       /* see <mach-o/stab.h> */
    uint32_t index;
    uint64_t value;      /* value of this symbol (or stab offset) */
};

bool operator==(const Symbol& lhs, const Symbol& rhs) {
    return lhs.name == rhs.name;
}

bool operator<(const Symbol& lhs, const Symbol& rhs){
    return lhs.index < rhs.index;
}

typedef struct Symbol Symbol;

typedef struct  {
    std::vector<const NXArchInfo *> archs;
    std::vector<Symbol *> symbols;
    std::vector<Symbol *> classes;
    std::vector<Symbol *> ivars;
    std::vector<std::string> reexports;
    std::vector<Symbol *> weak;
} ArchSection;

bool operator==(const ArchSection& lhs, const ArchSection& rhs) {
    return lhs.archs == rhs.archs;
}

void print_usage() {
    std::cout << "Usage: tbd /path/to/library_or_framework [-o /path/to/output]" << std::endl;
    exit(0);
}

int print_yaml(void *base, std::string platform, std::ostream& stream, std::vector<const NXArchInfo *> archInfos, std::map<const NXArchInfo *, struct dylib_command *> dylibs, std::map<const NXArchInfo *, struct symtab_command *> symtabs, std::map<Symbol *, std::vector<const NXArchInfo *>> symbols, std::map<Symbol *, std::vector<const NXArchInfo *>> classes, std::map<Symbol *, std::vector<const NXArchInfo *>> ivars, std::map<Symbol *, std::vector<const NXArchInfo *>> weak, std::map<const NXArchInfo *, std::string> reexports) {
    stream << "---\narchs:           [ " << archInfos[0]->name;
    
    for (auto i = 1; i < archInfos.size(); i++) {
        stream << ", " << archInfos[i]->name;
    }
    
    std::locale loc;
    for (auto i = 0; i < platform.length(); i++) {
        platform[i] = std::tolower(platform[i], loc);
    }
    
    struct dylib_command *dylib = dylibs.begin()->second;
    
    const char *install_name = (const char *)((uintptr_t)dylibs.begin()->second + dylibs.begin()->second->dylib.name.offset);
   
    std::ostringstream current_vers;
    current_vers << (swap(dylib->dylib.current_version) >> 16) << "." << ((swap(dylib->dylib.current_version) >> 8) & 0xff << swap(dylib->dylib.current_version) & 0xff);
    
    for (std::map<const NXArchInfo *, struct dylib_command *>::iterator iterator = dylibs.begin(); iterator != dylibs.end(); iterator++) {
        const char *installname = (const char *)((uintptr_t)iterator->second + iterator->second->dylib.name.offset);
        struct dylib_command *dylib = iterator->second;
        
        std::ostringstream currentvers;
        currentvers << (swap(dylib->dylib.current_version) >> 16) << "." << ((swap(dylib->dylib.current_version) >> 8) & 0xff << swap(dylib->dylib.current_version) & 0xff);
        
        if (strcmp(install_name, installname) != 0) {
            std::cout << error("Architecture " << installname << " does not have the same install-name as Architecture " << archInfos[0]->name);
            return -1;
        } else if (strcmp(current_vers.str().c_str(), currentvers.str().c_str()) != 0) {
            std::cout << error("Architecture " << installname << " does not have the same version as Architecture " << archInfos[0]->name);
            return -1;
        }
    }
    
    stream << " ]" << std::endl << "platform:        " << platform << std::endl;
    
    stream << "install-name:    " << install_name << std::endl;
    stream << "current-version: " << current_vers.str().c_str() << std::endl;
    stream << "exports:" << std::endl;
    
    std::map<std::vector<std::string>, std::vector<std::vector<std::string>>> arch_sections;
    
    if (archInfos.size() == 1) { //non-fat
        stream << "  - archs:           [ " << archInfos[0]->name << " ]" << std::endl;
        
        if (!reexports.empty()) {
            stream << "    re-exports:      [ " << reexports[0];
            for (auto const& reexport : reexports) {
                if (reexport.second == reexports[0]) continue;
                stream << ", " << reexport.second << std::endl;
            }
        }
        
        if (!symbols.empty()) {
            stream << "    symbols:         [ " << symbols.begin()->first->name;
            
            size_t currLength = symbols.begin()->first->name.length();
            const size_t maxLength = 80;

            for (auto const &it : symbols) {
                if (it == *symbols.begin()) continue;
                
                if ((currLength + it.first->name.length()) >= maxLength) {
                    stream << ", " << std::endl << "                       ";
                    currLength = 0;
                } else {
                    stream << ", ";
                    currLength += strlen(", ");
                }
                
                currLength += (it.first->name.length());
            }
            
            stream << " ]" << std::endl;
        }
        
        if (!classes.empty()) {
            stream << "    objc-classes:    [ " << classes.begin()->first->name;
            
            size_t currLength = classes.begin()->first->name.length();
            const size_t maxLength = 80;
            
            for (auto const &it : classes) {
                if (it == *classes.begin()) continue;

                if ((currLength + it.first->name.length()) >= maxLength) {
                    stream << ", " << std::endl << "                     ";
                    currLength = 0;
                } else {
                    stream << ", ";
                    currLength += strlen(", ");
                }
                
                stream << it.first->name.substr(strlen("_OBJC_CLASS_$"), it.first->name.length());
                currLength += it.first->name.length();
            }
            
            stream << " ]" << std::endl;
        }
        
        if (!ivars.empty()) {
            stream << "    objc-ivars:      [ " << ivars.begin()->first->name;
            
            size_t currLength = ivars.begin()->first->name.length();
            const size_t maxLength = 80;
            
            for (auto const &it : ivars) {
                if (it == *ivars.begin()) continue;
                
                if ((currLength + it.first->name.length()) >= maxLength) {
                    stream << ", " << std::endl << "                       ";
                    currLength = 0;
                } else {
                    stream << ", ";
                    currLength += strlen(", ");
                }

                stream << it.first->name.substr(strlen("_OBJC_IVAR_$"), it.first->name.length());
                currLength += it.first->name.length();
            }
            
            stream << " ]" << std::endl;
        }
    } else {
        std::vector<ArchSection *> arch_sections;
        for (auto &it : symbols) {
            Symbol *symbol = it.first;
            std::vector<const NXArchInfo *> archInfos = it.second;
            
            bool found = false;
            for (ArchSection *sect : arch_sections) {
                if (sect->archs != archInfos) continue;
                
                sect->symbols.push_back(symbol);
                found = true;
                
                break;
            }
            
            if (!found) {
                ArchSection *section = new ArchSection();
                
                section->archs = archInfos;
                section->symbols.push_back(symbol);
                
                arch_sections.push_back(section);
            }
        }
        
        for (auto &it : classes) {
            Symbol *symbol = it.first;
            std::vector<const NXArchInfo *> archInfos = it.second;
            
            bool found = false;
            for (ArchSection *sect : arch_sections) {
                if (sect->archs != archInfos) continue;
                
                sect->classes.push_back(symbol);
                found = true;
 
                break;
            }
            
            if (!found) {
                ArchSection *section = new ArchSection();
                
                section->archs = it.second;
                section->classes.push_back(symbol);
                
                arch_sections.push_back(section);
            }
        }
        
        for (auto &it : ivars) {
            Symbol *symbol = it.first;
            std::vector<const NXArchInfo *> archInfos = it.second;
            
            bool found = false;
            for (ArchSection *sect : arch_sections) {
                if (sect->archs != archInfos) continue;
                
                sect->ivars.push_back(symbol);
                found = true;
                
                break;
            }
            
            if (!found) {
                ArchSection *section = new ArchSection();
                
                section->archs = it.second;
                section->ivars.push_back(symbol);
                
                arch_sections.push_back(section);
            }
        }
        
        for (auto &it : weak) {
            Symbol *symbol = it.first;
            std::vector<const NXArchInfo *> archInfos = it.second;
            
            bool found = false;
            for (ArchSection *sect : arch_sections) {
                if (sect->archs != archInfos) continue;
                
                sect->weak.push_back(symbol);
                found = true;
                
                break;
            }
            
            if (!found) {
                ArchSection *section = new ArchSection();
                
                section->archs = it.second;
                section->weak.push_back(symbol);
                
                arch_sections.push_back(section);
            }
        }
        
        for (ArchSection *section : arch_sections) {
            //fill in reexports
            for (const NXArchInfo *archInfo : section->archs) {
                for (auto const& it : reexports) {
                    if (it.first != archInfo) continue;
                    section->reexports.push_back(it.second);
                }
            }
            
            stream << "  - archs:           [ " << section->archs[0]->name;
            for (auto i = 1; i < section->archs.size(); i++) {
                stream << ", " << section->archs[i]->name;
            }
            
            stream << " ]" << std::endl;
            
            if (!section->reexports.empty()) {
                stream << "    re-exports:    [ " << reexports.begin()->second;
                for (auto const& reexport : reexports) {
                    if (reexport.second == reexports.begin()->second) continue;
                    stream << ", " << reexport.second;
                }
                
                stream << " ]" <<std::endl;
            }
            
            if (!section->symbols.empty()) {
                stream << "    symbols:        [ " << section->symbols[0]->name;
                
                size_t currLength = section->symbols[0]->name.length();
                const size_t maxLength = 80;
                
                for (auto const &it : section->symbols) {
                    if ((currLength + it->name.length()) >= maxLength) {
                        stream << ", " << std::endl << "                       ";
                        currLength = 0;
                    } else {
                        stream << ", ";
                        currLength += strlen(", ");
                    }
                    
                    std::string name = it->name;
                    if (name.find("$ld$") == 0) { //prefix
                        name = name.insert(0, "\'");
                        name = name.insert(name.length(), "\'");
                    }
                    
                    stream << name;
                    currLength += (name.length());
                }
                
                stream << " ]" << std::endl;
            }
            
            if (!section->classes.empty()) {
                stream << "    objc-classes:    [ " << section->classes[0]->name.substr(strlen("_OBJC_CLASS_$"), section->classes[0]->name.length());
                
                size_t currLength = classes.begin()->first->name.length();
                const size_t maxLength = 80;
                
                for (auto const &it : section->classes) {
                    if ((currLength + it->name.length()) >= maxLength) {
                        stream << ", " << std::endl << "                     ";
                        currLength = 0;
                    } else {
                        stream << ", ";
                        currLength += strlen(", ");
                    }
                    
                    stream << it->name.substr(strlen("_OBJC_CLASS_$"), it->name.length());
                    currLength += it->name.length();
                }
                
                stream << " ]" << std::endl;
            }
            
            if (!section->ivars.empty()) {
                stream << "    objc-ivars:      [ " << section->ivars[0]->name.substr(strlen("_OBJC_IVAR_$"), section->ivars[0]->name.length());;
                
                size_t currLength = section->ivars[0]->name.length();
                const size_t maxLength = 80;
                
                for (auto const &it : section->ivars) {
                    if ((currLength + it->name.length()) >= maxLength) {
                        stream << ", " << std::endl << "                       ";
                        currLength = 0;
                    } else {
                        stream << ", ";
                        currLength += strlen(", ");
                    }
                    
                    stream << it->name.substr(strlen("_OBJC_IVAR_$"), it->name.length());
                    currLength += it->name.length();
                }
                
                stream << " ]" << std::endl;
            }
        }
    }
    
    stream << "..." << std::endl;
    return 0;
}

int main(int argc, const char * argv[]) {
    // insert code here...
    int fd = -1;
    void *base = nullptr;
    
    std::streambuf *buf = std::cout.rdbuf();
    std::ofstream of;

    if (argc < 2) {
        print_usage();
    } else if (access(argv[1], R_OK) != 0) {
        if (access(argv[1], F_OK) != 0) {
            std::cout << error("File path not valid");
        } else {
            std::cout << error_("Unable to read file at path, Please change permissions of the file");
            if (geteuid() != 0) {
                std::cout << " or run tbd as root" << std::endl;
            }
        }
        
        return -1;
    } else if (argc < 5 && argc > 2) {
        std::string flag = argv[2];
        if (strcasecmp(flag.c_str(), "-o") != 0) {
            std::cout << error("Unrecognized Flag: " << flag);
            return -1;
        } else if (argc < 4) {
            std::cout << error("Please provide an output file.");
            return -1;
        }
        
        if (access(argv[3], W_OK) != 0) {
            if (access(argv[3], F_OK) != 0) {
                std::cout << error("Output file does not exist");
            } else {
                std::cout << error_("Unable to access write permissions for output file Please change permissions of the file");
                if (geteuid() != 0) {
                    std::cout << " or run tbd as root" << std::endl;
                }
            }
            
            return -1;
        }
        
        of.open(argv[3]);
        buf = of.rdbuf();
    }
    
    fd = open(argv[1], R_OK);
    if (!fd) {
        std::cout << error_("Unable to open file at Path, Please change file permissions") << std::endl;
        if (geteuid() != 0) {
            std::cout << " or run tbd as root" << std::endl;
        }
        
        return -1;
    }

    std::ostream outfile(buf);

    struct stat *sbuf = new struct stat;
    fstat(fd, sbuf);
    
    if (S_ISDIR(sbuf->st_mode)) {
        std::cout << error("Directories are not supported");
        return -1;
    } else if (!S_ISREG(sbuf->st_mode)) {
        std::cout << error("Please choose a file");
        return -1;
    }
    
    base = mmap((caddr_t)0, sbuf->st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (!base || base == MAP_FAILED) {
        std::cout << error("Unable to map file to memory");
        return -1;
    }
    
    magic = *(uint32_t *)base;
    switch (magic) {
        case MH_MAGIC:
            break;
        case MH_CIGAM:
            bigEndian = true;
            break;
        case MH_MAGIC_64:
            bits64 = true;
            break;
        case MH_CIGAM_64:
            bits64 = true;
            bigEndian = true;
            break;
        case FAT_MAGIC:
            fatBits = true;
            break;
        case FAT_CIGAM:
            fatBits = true;
            bigEndian = true;
            break;
            
        default:
            std::cout << error("Input File is not Mach-O");
            return -1;
            
            break;
    }
    
    std::vector<const NXArchInfo *> archInfos;
    
    std::map<const NXArchInfo *, struct dylib_command *> dylibs;
    std::map<const NXArchInfo *, struct symtab_command *> symtabs;
    std::map<const NXArchInfo *, std::string> reexports;

    if (fatBits) {
        struct fat_header *fat = (struct fat_header *)base;
        struct fat_arch *arch = (struct fat_arch *)((uintptr_t)fat + sizeof(struct fat_header));
        
        for (int i = 0; i < swap(fat->nfat_arch); i++) {
            //check header load commands
            const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(arch->cputype), swap(arch->cpusubtype));
            if (!archInfo) {
                std::cout << error("Corrupted Architecture detected at offset " << std::hex << swap(arch->offset));
                return -1;
            }
            
            struct mach_header *header = (struct mach_header *)((uintptr_t)base + swap(arch->offset));
            
            struct load_command *loadcmd = (struct load_command *)((uintptr_t)header + sizeof(struct mach_header));
            if (header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
                loadcmd = (struct load_command *)((uintptr_t)loadcmd + sizeof(uint32_t));
            }
            
            bool foundDylib = false;
            bool foundSymtab = false;
            
            for (int j = 0; j < swap_(header->magic, header->ncmds); j++) {
                uint32_t cmd = swap_(header->magic, loadcmd->cmd);
                switch (cmd) {
                    case LC_ID_DYLIB:
                        dylibs.insert(dylibs.end(), std::pair<const NXArchInfo *, struct dylib_command *>(archInfo, (struct dylib_command *)loadcmd));
                        foundDylib = true;
                        break;
                    case LC_REEXPORT_DYLIB: {
                        struct dylib_command *dylib_cmd = (struct dylib_command *)loadcmd;
                        const char *name = (const char *)((uintptr_t)dylib_cmd + swap_(header->magic, dylib_cmd->dylib.name.offset));
                        
                        reexports.insert(reexports.end(), std::pair<const NXArchInfo *, std::string>(archInfo, name));
                        break;
                    }
                    case LC_SYMTAB:
                        symtabs.insert(symtabs.end(), std::pair<const NXArchInfo *, struct symtab_command *>(archInfo, (struct symtab_command *)loadcmd));
                        foundSymtab = true;
                        break;
                    default:
                        break;
                }
                
                loadcmd = (struct load_command *)((uintptr_t)loadcmd + swap_(header->magic, loadcmd->cmdsize));
            }
            
            if (!foundDylib) {
                std::cout << error("Architecture " << archInfo->name << " is not part of a library or a framework");
                return -1;
            } else if (!foundSymtab) {
                std::cout << error("Corrupted Architecture " << archInfo->name << " does not have LC_SYMTAB");
                return -1;
            }
            
            archInfos.push_back(archInfo);
            arch = (struct fat_arch *)((uintptr_t)arch + sizeof(struct fat_arch));
        }
    } else {
        struct mach_header *header = (struct mach_header *)base;
        const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(header->cputype), swap(header->cpusubtype));
        
        struct load_command *loadcmd = (struct load_command *)((uintptr_t)header + sizeof(struct mach_header));
        if (header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
            loadcmd = (struct load_command *)((uintptr_t)loadcmd + sizeof(uint32_t));
        }
        
        for (int j = 0; j < swap(header->ncmds); j++) {
            uint32_t cmd = swap(loadcmd->cmd);
            switch (cmd) {
                case LC_ID_DYLIB:
                    dylibs.insert(dylibs.end(), std::pair<const NXArchInfo *, struct dylib_command *>(archInfo, (struct dylib_command *)loadcmd));
                    break;
                case LC_REEXPORT_DYLIB: {
                    struct dylib_command *dylib_cmd = (struct dylib_command *)loadcmd;
                    const char *name = (const char *)((uintptr_t)dylib_cmd + dylib_cmd->dylib.name.offset);
                    
                    reexports.insert(reexports.end(), std::pair<const NXArchInfo *, std::string>(archInfo, name));
                    break;
                }
                case LC_SYMTAB:
                    symtabs.insert(symtabs.end(), std::pair<const NXArchInfo *, struct symtab_command *>(archInfo, (struct symtab_command *)loadcmd));
                    break;
                default:
                    break;
            }
            
            loadcmd = (struct load_command *)((uintptr_t)loadcmd + swap_(header->magic, loadcmd->cmdsize));
        }
        
        if (dylibs.empty()) {
            std::cout << error("Input File is not a library or a framework executable");
            return -1;
        } else if (symtabs.empty()) {
            std::cout << error("Corrupted Mach-O File does not have LC_SYMTAB");
            return -1;
        }
        
        archInfos.push_back(archInfo);
    }
    
    std::string platform = "";
    
    while (strcasecmp(platform.c_str(), "ios") != 0 && strcasecmp(platform.c_str(), "macosx") != 0) {
        std::cout << "Please choose a platform (ios or macosx): ";
        std::cin >> platform;
    }
    
    std::map<Symbol *, std::vector<const NXArchInfo *>> symbols;
    std::map<Symbol *, std::vector<const NXArchInfo *>> classes;
    std::map<Symbol *, std::vector<const NXArchInfo *>> ivars;
    std::map<Symbol *, std::vector<const NXArchInfo *>> weak;
    
    const char *strtab;
    
    if (fatBits) {
        struct fat_header *fat = (struct fat_header *)base;
        struct fat_arch *archs = (struct fat_arch *)((uintptr_t)base + sizeof(struct fat_header));
        
        for (int i = 0; i < swap(fat->nfat_arch); i++) {
            struct mach_header *header = (struct mach_header *)((uintptr_t)base + swap(archs[i].offset));
            const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(archs[i].cputype), swap(archs[i].cpusubtype));
            
            auto it = symtabs.find(archInfo);
            struct symtab_command *symtab = it->second;
            
            strtab = (const char *)((uintptr_t)header + swap_(header->magic, symtab->stroff));
            
            if (header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
                struct nlist_64 *nlists = (struct nlist_64 *)((uintptr_t)header + swap_(header->magic, symtab->symoff));
                
                for (int j = 0; j < swap_(header->magic, symtab->nsyms); j++) {
                    struct nlist_64 nl = nlists[j];
                    Symbol *symbol = new Symbol(&nl, strtab, header->magic);
                    
                    if ((nl.n_type & N_TYPE)  != N_SECT || (nl.n_type & N_EXT) != N_EXT) {
                        continue;
                    }
                    
                    std::string name = &strtab[swap_(header->magic, nl.n_un.n_strx)];
                    if (strstr(name.c_str(), "_OBJC_CLASS_$") == name.c_str()) {
                        if (classes.find(symbol) != classes.end()) {
                            classes.insert(classes.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[i]}));
                        } else {
                            classes[symbol].push_back(it->first);
                        }
                    } else if (strstr(name.c_str(), "_OBJC_IVAR_$") == name.c_str()) {
                        if (ivars.find(symbol) != classes.end()) {
                            ivars.insert(ivars.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[i]}));
                        } else {
                            ivars[symbol].push_back(it->first);
                        }
                    } else if (strstr(name.c_str(), "_OBJC_METACLASS_$") == name.c_str()) {
                        
                    } else {
                        if (nl.n_desc & N_WEAK_REF) {
                            if (weak.find(symbol) != weak.end()) {
                                weak.insert(weak.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[i]}));
                            } else {
                                ivars[symbol].push_back(it->first);
                            }
                        } else {
                            if (symbols.find(symbol) != symbols.end()) {
                                symbols.insert(symbols.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[i]}));
                            } else {
                                symbols[symbol].push_back(it->first);
                            }
                        }
                    }
                }
            } else {
                struct nlist *nlists = (struct nlist *)((uintptr_t)header + swap_(header->magic, symtab->symoff));
                
                for (int j = 0; j < swap_(header->magic, symtab->nsyms); j++) {
                    struct nlist nl = nlists[j];
                    Symbol *symbol = new Symbol(&nl, strtab, header->magic);
                    
                    if ((nl.n_type & N_TYPE) != N_SECT || (nl.n_type & N_EXT) != N_EXT) {
                        continue;
                    }
                    
                    std::string name = &strtab[swap_(header->magic, nl.n_un.n_strx)];
                    if (strstr(name.c_str(), "_OBJC_CLASS_$") == name.c_str()) {
                        if (classes.find(symbol) != classes.end()) {
                            classes.insert(classes.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[i]}));
                        } else {
                            classes[symbol].push_back(it->first);
                        }
                    } else if (strstr(name.c_str(), "_OBJC_IVAR_$") == name.c_str()) {
                        if (ivars.find(symbol) != ivars.end()) {
                            ivars.insert(ivars.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[i]}));
                        } else {
                            ivars[symbol].push_back(it->first);
                        }
                    } else if (strstr(name.c_str(), "_OBJC_METACLASS_$") == name.c_str()) {
                        
                    } else {
                        if (nl.n_desc & N_WEAK_REF) {
                            if (weak.find(symbol) != weak.end()) {
                                weak.insert(weak.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[i]}));
                            } else {
                                ivars[symbol].push_back(it->first);
                            }
                        } else {
                            if (symbols.find(symbol) != symbols.end()) {
                                symbols.insert(symbols.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[i]}));
                            } else {
                                symbols[symbol].push_back(it->first);
                            }
                        }
                    }
                    
                    
                }
            }
        }
    } else {
        struct mach_header *header = (struct mach_header *)base;
        
        std::map<const NXArchInfo *, struct symtab_command *>::iterator it = symtabs.begin();
        struct symtab_command *symtab = it->second;
        
        strtab = (const char *)((uintptr_t)header + swap_(header->magic, symtab->stroff));
        
        if (header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
            struct nlist_64 *nlists = (struct nlist_64 *)((uintptr_t)header + swap(symtab->symoff));
            
            for (int j = 0; j < swap(symtab->nsyms); j++) {
                struct nlist_64 nl = nlists[j];
                Symbol *symbol = new Symbol(&nl, strtab, magic);
                
                if ((nl.n_type & N_TYPE)  != N_SECT || (nl.n_type & N_EXT) != N_EXT) {
                    continue;
                }
                
                std::string name = &strtab[swap(nl.n_un.n_strx)];
                if (strstr(name.c_str(), "_OBJC_CLASS_$") == name.c_str()) {
                    if (classes.find(symbol) != classes.end()) {
                        classes.insert(classes.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[0]}));
                    } else {
                        classes[symbol].push_back(archInfos[0]);
                    }
                } else if (strstr(name.c_str(), "_OBJC_IVAR_$") == name.c_str()) {
                    if (ivars.find(symbol) != classes.end()) {
                        ivars.insert(ivars.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[0]}));
                    } else {
                        ivars[symbol].push_back(archInfos[0]);
                    }
                } else if (strstr(name.c_str(), "_OBJC_METACLASS_$") == name.c_str()) {
                    
                } else {
                    if (nl.n_desc & N_WEAK_REF) {
                        if (weak.find(symbol) != weak.end()) {
                            weak.insert(weak.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[0]}));
                        } else {
                            ivars[symbol].push_back(archInfos[0]);
                        }
                    } else {
                        if (symbols.find(symbol) != symbols.end()) {
                            symbols.insert(symbols.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[0]}));
                        } else {
                            symbols[symbol].push_back(archInfos[0]);
                        }
                    }
                }
            }
        } else {
            struct nlist *nlists = (struct nlist *)((uintptr_t)header + swap(symtab->symoff));
            
            for (int j = 0; j < swap(symtab->nsyms); j++) {
                struct nlist nl = nlists[j];
                Symbol *symbol = new Symbol(&nl, strtab, magic);
                
                if ((nl.n_type & N_TYPE)  != N_SECT || (nl.n_type & N_EXT) != N_EXT) {
                    continue;
                }
                
                std::string name = symbol->name;
                if (name == "<redacted>") {
                    continue;
                }
                
                if (strstr(name.c_str(), "_OBJC_CLASS_$") == name.c_str()) {
                    if (classes.find(symbol) == classes.end()) {
                        classes.insert(classes.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[0]}));
                    } else {
                        classes[symbol].push_back(it->first);
                    }
                } else if (strstr(name.c_str(), "_OBJC_IVAR_$") == name.c_str()) {
                    if (ivars.find(symbol) == classes.end()) {
                        ivars.insert(ivars.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[0]}));
                    } else {
                        ivars[symbol].push_back(it->first);
                    }
                } else if (strstr(name.c_str(), "_OBJC_METACLASS_$") == name.c_str()) {
                    
                } else {
                    if (nl.n_desc == 0) {
                        continue;
                    }
                    
                    if (nl.n_desc & N_WEAK_REF) {
                        if (weak.find(symbol) == weak.end()) {
                            weak.insert(weak.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[0]}));
                        } else {
                            ivars[symbol].push_back(it->first);
                        }
                    } else {
                        if (symbols.find(symbol) == symbols.end()) {
                            symbols.insert(symbols.end(), std::pair<Symbol *, std::vector<const NXArchInfo *>>(symbol, {archInfos[0]}));
                        } else {
                            symbols[symbol].push_back(it->first);
                        }
                    }
                }
            }
        }
    }
    
    //NSINoahDev();
    
    return print_yaml(base, platform, outfile, archInfos, dylibs, symtabs, symbols, classes, ivars, weak, reexports);
}
