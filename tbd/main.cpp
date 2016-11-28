//
//  main.cpp
//  tbd
//
//  Created by Programming on 6/21/16.
//  Copyright © 2016 Programming. All rights reserved.
//

#include <mach-o/arch.h>
#include <mach-o/loader.h>

#include <mach-o/fat.h>
#include <mach-o/nlist.h>

#include <sys/stat.h>

#include <iostream>
#include <future>
#include <fstream>

#include <dispatch/dispatch.h>

#include <map>
#include <unistd.h>
#include <vector>

#include "String.hpp"

#define error(str, ...) fprintf(stderr, "\x1B[31mError:\x1B[0m " str "\n", ##__VA_ARGS__); return -1
#define Error(str, ...) fprintf(stderr, "\x1B[31mError:\x1B[0m " str "\n", ##__VA_ARGS__); exit(-1)

static inline uint16_t swap_(uint16_t value) {
    return ((value >> 8) & 0x00ff) | ((value << 8) & 0xff00);
}

static inline uint32_t swap_(uint32_t value) {
    value = ((value >> 8) & 0x00ff00ff) | ((value << 8) & 0xff00ff00);
    value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);
    
    return value;
}

static inline uint64_t swap_(uint64_t value) {
    value = (value & 0x00000000ffffffff) << 32 | (value & 0xffffffff00000000) >> 32;
    value = (value & 0x0000ffff0000ffff) << 16 | (value & 0xffff0000ffff0000) >> 16;
    value = (value & 0x00ff00ff00ff00ff) << 8  | (value & 0xff00ff00ff00ff00) >> 8;

    
    return value;
}

bool requiresSwap = false;

static inline uint16_t swap(uint16_t value) {
    return requiresSwap ? swap_(value) : value;
}

static inline uint32_t swap(uint32_t value) {
    return requiresSwap ? swap_(value) : value;
}

static inline uint64_t swap(uint64_t value) {
    return requiresSwap ? swap_(value) : value;
}

static inline int16_t swap(int16_t value) {
    return swap(static_cast<uint16_t>(value));
}

static inline int32_t swap(int32_t value) {
    return swap(static_cast<uint32_t>(value));
}

static inline int64_t swap(int64_t value) {
    return swap(static_cast<uint64_t>(value));
}

static inline uint16_t swap(uint32_t magic, uint16_t value) {
    switch (magic) {
        case MH_CIGAM_64:
        case FAT_MAGIC_64:
        case FAT_CIGAM_64:
            return swap_(value);
            break;
            
        default:
            return value;
            break;
    }
}

static inline uint32_t swap(uint32_t magic, uint32_t value) {
    switch (magic) {
        case MH_CIGAM_64:
        case FAT_MAGIC_64:
        case FAT_CIGAM_64:
            return swap_(value);
            break;
            
        default:
            return value;
            break;
    }
}

static inline uint64_t swap(uint32_t magic, uint64_t value) {
    switch (magic) {
        case MH_CIGAM_64:
        case FAT_MAGIC_64:
        case FAT_CIGAM_64:
            return swap_(value);
            break;
            
        default:
            return value;
            break;
    }
}

__attribute__ ((unused)) static inline int16_t swap(uint32_t magic, int16_t value) {
    return static_cast<uint16_t>(swap(magic, static_cast<uint16_t>(value)));
}

static inline int32_t swap(uint32_t magic, int32_t value) {
    return static_cast<uint32_t>(swap(magic, static_cast<uint32_t>(value)));
}

__attribute__ ((unused)) static inline int64_t swap(uint32_t magic, int64_t value) {
    return static_cast<uint64_t>(swap(magic, static_cast<uint64_t>(value)));
}

class Symbol {
public:
    Symbol(struct nlist *nl, const char *strtab, uint32_t magic, std::vector<const NXArchInfo *> architectures) {
        name_ = &strtab[swap(magic, nl->n_un.n_strx)];
        
        this->architectures_ = architectures;
        
        this->type_ = swap(magic, static_cast<uint32_t>(nl->n_type));
        this->sect_ = swap(magic, static_cast<uint32_t>(nl->n_sect));
        this->desc_ = swap(magic, static_cast<uint16_t>(nl->n_desc));
        this->value_ = static_cast<uint64_t>(swap(magic, nl->n_value));
        this->index_ = swap(magic, nl->n_un.n_strx);
    }
    
    Symbol(struct nlist_64 *nl, const char *strtab, uint32_t magic, std::vector<const NXArchInfo *> architectures) {
        name_ = &strtab[swap(magic, nl->n_un.n_strx)];
        
        this->architectures_ = architectures;
        
        this->type_ = swap(magic, static_cast<uint32_t>(nl->n_type));
        this->sect_ = swap(magic, static_cast<uint32_t>(nl->n_sect));
        this->desc_ = swap(magic, static_cast<uint16_t>(nl->n_desc));
        this->value_ = swap(magic, nl->n_value);
        this->index_ = swap(magic, nl->n_un.n_strx);
    }
    
    inline void addArchitecture(const NXArchInfo *archInfo) noexcept {
        if (std::find(architectures_.begin(), architectures_.end(), archInfo) == architectures_.end())
            architectures_.push_back(archInfo);
    }
   
    inline String name() const noexcept {
        return name_;
    }
    
    inline uint32_t type() const noexcept {
        return type_;
    }
    
    inline uint32_t sect() const noexcept {
        return sect_;
    }
    
    inline uint16_t desc() const noexcept {
        return desc_;
    }
    
    inline uint32_t index() const noexcept {
        return index_;
    }
    
    inline uint64_t value() const noexcept {
        return value_;
    }
    
    inline std::vector<const NXArchInfo *> architectures() const noexcept {
        return architectures_;
    }
    
    inline operator String() const noexcept {
        return name_;
    }
    
    inline operator const char *() const noexcept {
        return name_.c_str();
    }
    
    inline operator std::vector<const NXArchInfo *>() const noexcept {
        return architectures_;
    }
    
    inline bool operator==(const Symbol& lhs) const noexcept {
        //we really only have to compare symbol names, as we (hopefully) can't have 2 symbols with the same name, and different nlist fields
        return lhs.name_ == name_;
    }
    
    inline bool operator!=(const Symbol& lhs) const noexcept {
        return !(lhs == *this);
    }
private:
    String name_;
    
    uint8_t type_;
    uint8_t sect_;
    
    uint16_t desc_;
    
    uint32_t index_;
    uint64_t value_;
    
    std::vector<const NXArchInfo *> architectures_;
};

//use a class instead of std::map<String, std::vector<Symbol>> as it is probably slightly faster (and results in more organized code)
class Group {
public:
    Group(std::vector<const NXArchInfo *> archInfos) {
        name_ = archInfos[0]->name;
        archInfos.erase(archInfos.begin());
        
        for (std::vector<const NXArchInfo *>::const_iterator it = archInfos.begin(); it != archInfos.end(); it++) {
            name_ += String(", ") + (*it)->name;
        }
    }
    
    inline void addSymbol(Symbol symbol) noexcept {
        if (std::find(symbols_.begin(), symbols_.end(), symbol) == symbols_.end())
            symbols_.push_back(symbol);
    }
    
    inline operator const char *() const noexcept {
        return name_.c_str();
    }
    
    inline operator String() const noexcept {
        return name_;
    }
    
    inline operator std::vector<Symbol>() const noexcept {
        return symbols_;
    }
    
    inline bool operator==(const Group& lhs) const noexcept {
        return lhs.name_ == name_;
    }
    
    inline bool operator!=(const Group& lhs) const noexcept {
        return !(lhs  == *this);
    }
    
    inline bool operator==(std::vector<const NXArchInfo *> archInfos) const noexcept {
        String name = archInfos[0]->name;
        for (std::vector<const NXArchInfo *>::const_iterator it = archInfos.begin(); it != archInfos.end(); it++) {
            name += (name + ", ") + (*it)->name;
        }
        
        return name_ == name;
    }
    
    inline bool operator!=(std::vector<const NXArchInfo *> archInfos) const noexcept {
        String name = archInfos[0]->name;
        for (std::vector<const NXArchInfo *>::const_iterator it = archInfos.begin(); it != archInfos.end(); it++) {
            name += (name + ", ") + (*it)->name;
        }
        
        return name_ == name;
    }
    
    void Print(FILE *stream, std::vector<String> reexports) const noexcept {
        fprintf(stream, "  - archs:           [ %s ]\n", name_.data());
        
        std::vector<String> symbols;
        std::vector<String> objc_classes;
        std::vector<String> objc_ivars;
        
        for (std::vector<Symbol>::const_iterator it = symbols_.begin(); it != symbols_.end(); it++) {
            String name = it->name();
            if (name.hasPrefix("$ld")) {
                name.insert(0, '\'');
                name.append(1, '\'');
            }
            
            if (name.hasPrefix("_OBJC_CLASS_$")) {
                name.substrInPlace(sizeof("_OBJC_CLASS_$") - 1);
                objc_classes.push_back(name);
                
                continue;
            }
            
            if (name.hasPrefix("_OBJC_METACLASS_$")) {
                name.substrInPlace(sizeof("_OBJC_METACLASS_$") - 1);
                objc_classes.push_back(name);
                
                continue;
            }
            
            if (name.hasPrefix("_OBJC_IVAR_$")) {
                name.substrInPlace(sizeof("_OBJC_IVAR_$") - 1);
                objc_ivars.push_back(name);
                
                continue;
            }
            
            symbols.push_back(name);
        }
        
        if (!symbols.empty()) {
            fprintf(stream, "    symbols:         [ %s", symbols.begin()->data());

            String::Size lineLength = symbols.begin()->length();
            symbols.erase(symbols.begin());

            for (std::vector<String>::const_iterator it = symbols.begin(); it != symbols.end(); it++) {
                if (lineLength > 85) {
                    fprintf(stream, ",\n                       ");
                    lineLength = 0;
                } else {
                    fprintf(stream, ", ");
                }

                fprintf(stream, "%s", it->data());
                lineLength += it->length();
            }
            
            fprintf(stream, " ]\n");
        }
        
        if (!reexports.empty()) {
            fprintf(stream, "    reexports:       [ %s", reexports.begin()->data());
            
            String::Size lineLength = 0;
            reexports.erase(reexports.begin());
            
            for (std::vector<String>::const_iterator it = reexports.begin(); it != reexports.end(); it++) {
                if (lineLength > 85) {
                    fprintf(stream, ",\n                       ");
                    lineLength = 0;
                } else {
                    fprintf(stream, ", ");
                }

                fprintf(stream, "%s", it->data());
                lineLength += it->length();
            }
            
            fprintf(stream, " ]\n");
        }
        
        if (!objc_classes.empty()) {
            fprintf(stream, "    objc-classes:    [ %s", objc_classes.begin()->data());
            
            String::Size lineLength = 0;
            objc_classes.erase(objc_classes.begin());
            
            for (std::vector<String>::const_iterator it = objc_classes.begin(); it != objc_classes.end(); it++) {
                if (lineLength > 85) {
                    fprintf(stream, ",\n                       ");
                    lineLength = 0;
                } else {
                    fprintf(stream, ", ");
                }
                
                fprintf(stream, "%s", it->data());
                lineLength += it->length();
            }
            
            fprintf(stream, " ]\n");
        }
        
        if (!objc_ivars.empty()) {
            fprintf(stream, "    objc-ivars:      [ %s", objc_ivars.begin()->data());
            
            String::Size lineLength = 0;
            objc_ivars.erase(objc_ivars.begin());
            
            for (std::vector<String>::const_iterator it = objc_ivars.begin(); it != objc_ivars.end(); it++) {
                if (lineLength > 85) {
                    fprintf(stream, ",\n                       ");
                    lineLength = 0;
                } else {
                    fprintf(stream, ", ");
                }

                fprintf(stream, "%s", it->data());
                lineLength += it->length();
            }

            fprintf(stream, " ]\n");
        }
    }
    
private:
    String name_;
    std::vector<Symbol> symbols_;
};

static inline bool isfat(uint32_t magic) noexcept {
    return magic == FAT_MAGIC || magic == FAT_MAGIC_64 || magic == FAT_CIGAM || magic == FAT_CIGAM_64;
}

static inline bool is64(uint32_t magic) noexcept {
    return magic == MH_MAGIC_64 || magic == MH_CIGAM_64 || magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64;
}

String getdylibname(FILE *file, struct mach_header_64 *header, off_t offset) noexcept {
    const char *name = new char[4096];
    bzero((void *)name, 4096);
    
    fseek(file, offset, SEEK_SET);
    
    int i = 0;
    while (i < 4096) {
        if (!fread((void *)&name[i], 4, 1, file)) break;
        i += 4;
        
        bool found = false;
        for (int j = i - 4; j < i; j++) {
            if (name[j] != '\0') continue;
            
            found = true;
            break;
        }
        
        if (found) break;
    }
    
    return name;
}

void getloadcommands(FILE *file, off_t offset, struct mach_header_64 *header, struct symtab_command *symtab, struct dylib_command *id_dylib_cmd, struct uuid_command *uuid_cmd, bool architecture, std::map<String, std::vector<const NXArchInfo *>>& reexports, off_t& nameOffset) noexcept { //architecture boolean for fancy error message
    
    bzero(symtab, sizeof(struct symtab_command));
    bzero(uuid_cmd, sizeof(struct uuid_command));
    bzero(id_dylib_cmd, sizeof(struct dylib_command));
    
    offset += sizeof(struct mach_header);
    if (is64(header->magic))
        offset += sizeof(uint32_t);
    
    uint32_t ncmds = swap(header->magic, header->ncmds);
    for (uint32_t i = 0; i < ncmds; i++) {
        struct load_command *load_cmd = new struct load_command;
        fread(load_cmd, sizeof(struct load_command), 1, file);
        
        uint32_t cmd = swap(header->magic, load_cmd->cmd);
        switch (cmd) {
            case LC_SYMTAB:
                memcpy(symtab, load_cmd, sizeof(struct load_command));
                fread(&symtab->symoff, sizeof(struct symtab_command) - sizeof(struct load_command), 1, file); //instead of seeking back to start of cmd, read from &symoff
                break;
            case LC_ID_DYLIB:
                memcpy(id_dylib_cmd, load_cmd, sizeof(struct load_command));
                fread(&id_dylib_cmd->dylib, sizeof(struct dylib_command) - sizeof(struct load_command), 1, file);
                
                nameOffset = offset + swap(header->magic, id_dylib_cmd->dylib.name.offset);
                fseek(file, swap(header->magic, load_cmd->cmdsize) - sizeof(struct dylib_command), SEEK_CUR);
                
                break;
            case LC_REEXPORT_DYLIB: {
                struct dylib_command *dylib_cmd = new struct dylib_command;
                memcpy(dylib_cmd, load_cmd, sizeof(struct load_command));
                
                fread(&dylib_cmd->dylib, sizeof(struct dylib_command) - sizeof(struct load_command), 1, file);
                
                std::vector<const NXArchInfo *> archInfo = { NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype)) } ;
                reexports.insert(std::pair<String, std::vector<const NXArchInfo *>>(getdylibname(file, header, offset + swap(header->magic, dylib_cmd->dylib.name.offset)), archInfo));
                
                fseek(file, offset + swap(header->magic, load_cmd->cmdsize), SEEK_SET);
                break;
            }
            case LC_UUID:
                memcpy(uuid_cmd, load_cmd, sizeof(struct load_command));
                fread((void *)uuid_cmd->uuid, sizeof(struct uuid_command) - sizeof(struct load_command), 1, file);
                break;
            default:
                fseek(file, swap(header->magic, load_cmd->cmdsize) - sizeof(struct load_command), SEEK_CUR); //instead of finding offset and seeking to cmdisze, seek past the rest of cmd
                break;
        }
        
        offset += swap(header->magic, load_cmd->cmdsize);
        if (symtab->cmd && uuid_cmd->cmd && id_dylib_cmd->cmd)
            break;
    }
    
    if (!id_dylib_cmd->cmd) {
        if (architecture) {
            const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype));
            Error("Architecture %s is not a framework (could not find struct dylib_command (LC_ID_DYLIB) within architecture %s)", archInfo->name, archInfo->name);
        } else Error("File is not a framework (could not find struct dylib_command (LC_ID_DYLIB) in executable)");
    }
    
    if (!symtab->cmd) {
        if (architecture) {
            Error("Unable to retrieve symbols (could not find struct symtab_command within architecture %s)", NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype))->name);
        } else Error("Unable to retrieve symbols (could not find struct symtab_command in executable)");
    }
    
    if (!uuid_cmd->cmd) {
        if (architecture) {
            Error("Unable to retrieve uuid (could not find struct uuid_command within architecture %s)", NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype))->name);
        } else Error("Unable to retrieve uuid (could not find struct uuid_command in executable)");
    }
}

void getdylibversions(struct mach_header_64 *header, struct dylib_command *dylib_cmd, String& currentVersion, String& compatabilityVersion) noexcept {
    currentVersion = String("%u.%u.%u", (swap(header->magic, dylib_cmd->dylib.current_version) >> 16), ((swap(header->magic, dylib_cmd->dylib.current_version) >> 8) & 0xff), (swap(header->magic, dylib_cmd->dylib.current_version) & 0xff));
    compatabilityVersion = String("%u.%u.%u", (swap(header->magic, dylib_cmd->dylib.compatibility_version) >> 16), ((swap(header->magic, dylib_cmd->dylib.compatibility_version) >> 8) & 0xff), (swap(header->magic, dylib_cmd->dylib.compatibility_version) & 0xff));
}

String getuuid(uint8_t *uuid) noexcept {
    String uuidString = String("%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
    return String("%s-%s-%s-%s-Ts", uuidString.substr(0, 8).data(), uuidString.substr(8, 4).data(), uuidString.substr(12, 4).data(), uuidString.substr(16, 4).data(), uuidString.substr(20, 12).data());
}

std::vector<Symbol> retrievesymbols(FILE *file, off_t offset, struct mach_header_64 *header, struct symtab_command *symtab, bool architecture) noexcept { //architecture boolean just for fancy error message
    std::vector<Symbol> symbols;
    
    uint32_t strsize = swap(header->magic, symtab->strsize);
    const char *strtab = new char[strsize];
    
    fseek(file, offset + swap(header->magic, symtab->stroff), SEEK_SET);
    fread((void *)strtab, strsize, 1, file);
    
    if (is64(header->magic)) {
        uint32_t nsyms = swap(header->magic, symtab->nsyms);
        struct nlist_64 *nlists = new struct nlist_64[nsyms];
        
        fseek(file, offset + swap(header->magic, symtab->symoff), SEEK_SET);
        fread(nlists, sizeof(struct nlist_64) * nsyms, 1, file);
        
        for (uint32_t i = 0; i < nsyms; i++) {
            struct nlist_64 nl = nlists[i];
            
            uint32_t n_type = swap(header->magic, nl.n_type);
            if ((n_type & N_TYPE) != N_SECT || (n_type & N_EXT) != N_EXT) {
                continue;
            }
            
            std::vector<const NXArchInfo *> architectures = { NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype)) };
            symbols.emplace_back(&nl, strtab, header->magic, architectures);
        }
    } else {
        uint32_t nsyms = swap(header->magic, symtab->nsyms);
        struct nlist *nlists = new struct nlist[nsyms];
        
        fseek(file, offset + swap(header->magic, symtab->symoff), SEEK_SET);
        fread(nlists, sizeof(struct nlist) * nsyms, 1, file);
        
        for (uint32_t i = 0; i < nsyms; i++) {
            struct nlist nl = nlists[i];
            
            uint32_t n_type = swap(header->magic, nl.n_type);
            if ((n_type & N_TYPE) != N_SECT || (n_type & N_EXT) != N_EXT) {
                continue;
            }
            
            std::vector<const NXArchInfo *> architectures = { NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype)) };
            symbols.emplace_back(&nl, strtab, header->magic, architectures);
        }
    }
    
    return symbols;
}

std::vector<const NXArchInfo *> parsearchitectures(String str) {
    if (str.empty()) {
        Error("Please provide architecture(s)");
    }
    
    std::vector<const NXArchInfo *> archInfos;
    for (String::Size i = 0; i < str.length(); i++) {
        if (isspace(str[i]) || str[i] == ',') {
            continue;
        }
        
        String architecture;
        
        for (String::Size j = i; j < str.length(); j++) {
            if (isspace(str[j]) || str[j] == ',') {
                break;
            }
            
            architecture.append(1, str[j]);
            i = j;
        }
        
        const NXArchInfo *archInfo = NXGetArchInfoFromName(architecture.data());
        if (!archInfo) {
            Error("Unrecognized Architecture: %s", architecture.data());
        }
        
        archInfos.push_back(archInfo);
    }
    
    return archInfos;
}

//TODO: Add native '_String<std::allocator<char>>' support, instead of using proprietary std::string
String Ask(String question) noexcept {
    std::string input;
    std::cout << question << ": ";
    
    std::cin >> input;
    return input;
}

__dead2 void print_usage() noexcept {
    std::cout << "Usage: tbd [path-to-file] [(-v/--version)=v2] [-o/-output path-to-output]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "    -a, --archs,   Specify Architecture(s) to use, instead of the ones in the binary. Seperate architectures with \",\". (ex. -archs=\"armv7, arm64\")" << std::endl;
    std::cout << "    -o, --output,  Output converted .tbd to a file" << std::endl;
    std::cout << "    -u, --usage,   Print this message" << std::endl;
    std::cout << "    -v, --version, Set version of tbd to convert to. Run --versions to see a list of available versions. (ex. -v=v1) \"v2\" is the default version" << std::endl;
    
    exit(0);
}

std::vector<String> versions = { "v1", "v2" };
__dead2 void print_versions() noexcept {
    std::cout << "tbd-versions: " << std::endl;
    for (String version : versions) {
        std::cout << versions[0] << std::endl;
    }
    
    exit(0);
}

int main(int argc, const char * argv[]) noexcept {
    if (argc < 2) {
        print_usage();
    }
    
    String arg1 = argv[1];
    if (arg1.caseInsensitiveCompare("-u") == 0) {
        if (argc > 2) {
            error("Too many arguments provided");
        }
        
        print_usage();
    } else if (arg1.caseInsensitiveCompare("--versions") == 0) {
        if (argc > 2) {
            error("Too many arguments provided");
        }
        
        print_versions();
    }
    
    if (arg1[0] != '/') {
        //use current directory + arg1 to make a path object
        char *buf = new char[4096];
        if (!getcwd(buf, 4096)) {
            error("Unable to get the current working directory. Is the directory deleted? (getcwd(buf, 4096) returned null)");
        }
        
        String::Size len = strlen(buf);
        if (buf[len - 1] != '/') {
            arg1.insert(0, 1, '/');
        }
        
        arg1.insert(0, buf);
    }
    
    //check to see if file exists and provide a more informative error if so, instead of just giving the user an error about being unable to read the file
    if (access(arg1.data(), R_OK) != 0) {
        if (access(arg1.data(), F_OK) != 0) {
            error("File at path does not exist. (access(\"%s\", F_OK) returned negative)", arg1.data());
        } else {
            error("Unable to read file at path. Try changing permissions of the file, and/or running tbdconverter as root. (access(\"%s\", R_OK) returned negative)", arg1.data());
        }
    }
    
    struct stat *sbuf = new struct stat;
    if (stat(arg1.data(), sbuf) < 0) {
        error("Unable to gather information on file provided. (stat() failed)");
    }
    
    if (!S_ISREG(sbuf->st_mode)) {
        if (S_ISDIR(sbuf->st_mode)) {
            error("Directories are not supported");
        } else {
            error("Only Mach-O Files are supported"); //Should probably give a more accurate error of 'Unknown Type'
        }
    }
    
    FILE *file = fopen(arg1.data(), "r");
    if (!file) {
        error("Unable to open file. (fopen(\"%s\", \"r\") returned null)", arg1.data());
    }
    
    FILE *output = stdout;
    
    std::vector<const NXArchInfo *> archInfos;
    String version = "v2";
    
    //now check the rest of the arguments since we have validated the path object
    for (int i = 2; i < argc; i++) { //start from 2 to skip the path that was already parsed
        String arg2 = argv[i];
        if (arg2.caseInsensitiveCompare("-o") == 0 || arg2.caseInsensitiveCompare("--output") == 0) {
            if (i == argc - 1) {
                error("Please provide an output file");
            }
            
            i++;
            String path = argv[i];
            
            if (path[0] != '/') {
                //use current directory + arg1 to make a path object
                char *buf = new char[4096];
                if (!getcwd(buf, 4096)) {
                    error("Unable to get the current working directory. Is the directory deleted? (getcwd(buf, 4096) returned null)");
                }
                
                String::Size len = strlen(buf);
                if (buf[len - 1] != '/') {
                    path.insert(0, 1, '/');
                }
                
                path.insert(0, buf);
            }
            
            output = fopen(path.data(), "w+");
            if (!output) {
                error("Unable to create/open output file. (fopen(\"%s\", \"w\") return null)", path.data());
            }
        } else if (arg2.caseInsensitiveCompare("-v", 2) == 0 || arg2.caseInsensitiveCompare("--version", 9) == 0) {
            String::Size f = arg2.find('=');
            if (f == String::NoPosition || f == arg2.length() - 1) {
                error("Please provide a tbd version");
            }
            
            version = arg2.substr(f + 1);
            if (std::find(versions.begin(), versions.end(), version) == versions.end()) {
                error("Version \"%s\" is not valid", version.data());
            }
        } else if (arg2.caseInsensitiveCompare("-a", 2) == 0 || arg2.caseInsensitiveCompare("-archs", 6) == 0) {
            String::Size f = arg2.find('=');
            if (f == String::NoPosition || f == arg2.length() - 1) {
                error("Please provide architecture(s)");
            }
            
            archInfos = parsearchitectures(arg2.substr(f + 1));
        } else {
            error("Unrecognized Argument %s", arg2.data());
        }
    }

    struct fat_header *fat = new struct fat_header;

    fseek(file, 0x0, SEEK_SET);
    fread(fat, sizeof(struct fat_header), 1, file);
    
    requiresSwap = fat->magic == MH_CIGAM || fat->magic == MH_CIGAM_64 || fat->magic == FAT_CIGAM || fat->magic == FAT_CIGAM_64;
    
    std::vector<Symbol> symbols;
    std::map<const NXArchInfo *, String> uuids;
    
    struct symtab_command *symtab = new struct symtab_command;
    struct dylib_command *dylib_cmd = new struct dylib_command;
    struct uuid_command *uuid_cmd = new struct uuid_command;
    
    std::map<String, std::vector<const NXArchInfo *>> reexports;
    off_t nameOffset = 0x0;
    
    String installName;
    String currentVersion;
    String compatibilityVersion;
    
    if (isfat(fat->magic)) {
        if (is64(fat->magic)) {
            uint32_t nfat_arch = swap(fat->nfat_arch);
            
            //store all the architectures instead of reading over and over
            struct fat_arch_64 *archs = new struct fat_arch_64[swap(nfat_arch)];
            fread(archs, sizeof(struct fat_arch_64) * swap(nfat_arch), 1, file);
            
            for (uint32_t i = 0; i < swap(fat->nfat_arch); i++) {
                struct fat_arch_64 arch = archs[i];
                struct mach_header_64 *header = new struct mach_header_64; //use mach_header_64 for safety
                
                uint64_t offset = swap(arch.offset);
                
                fseek(file, offset, SEEK_SET);
                fread(header, sizeof(struct mach_header), 1, file);
                
                if (header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
                    fread(&header->reserved, sizeof(uint32_t), 1, file); //don't really need this, but let's advance the file pointer for accuracy
                }
                
                std::map<String, std::vector<const NXArchInfo *>> reexports_;
                
                getloadcommands(file, offset, header, symtab, dylib_cmd, uuid_cmd, true, reexports, nameOffset);
                getdylibversions(header, dylib_cmd, currentVersion, compatibilityVersion);
                
                installName = getdylibname(file, header, nameOffset);
                uuids.insert(std::pair<const NXArchInfo *, String>(NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype)), getuuid(uuid_cmd->uuid)));
                
                for (std::map<String, std::vector<const NXArchInfo *>>::iterator it = reexports_.begin(); it != reexports_.end(); it++) {
                    std::map<String, std::vector<const NXArchInfo *>>::iterator jt = std::find(reexports.begin(), reexports.end(), *it);
                    if (jt != reexports.end()) {
                        jt->second.push_back(NXGetArchInfoFromCpuType(swap(header->cputype), swap(header->cpusubtype)));
                    } else {
                        reexports.insert(*it);
                    }
                }
                
                std::vector<Symbol> symbols_ = retrievesymbols(file, offset, header, symtab, true);
                if (symbols_.size() > 10000) {
                    const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype));
                    fprintf(stderr, "Symbol Processing for architecture \"%s\" will take a while (tbdconverter has to process %lu symbols)", archInfo->name, symbols_.size()); //Show message while tbdconverter processes symbols
                }
                
                static unsigned int counter = 0;
                static unsigned int dotCounter = 0;
                
                const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype));
                for (std::vector<Symbol>::iterator it = symbols_.begin(); it != symbols_.end(); it++) {
                    if (symbols_.size() > 15000 && counter % 9750 == 0 && dotCounter < 3) {
                        fprintf(stderr, ".");
                        dotCounter++;
                    }
                    
                    std::vector<Symbol>::iterator jt = std::find(symbols.begin(), symbols.end(), *it);
                    if (jt != symbols.end()) {
                        jt->addArchitecture(archInfo);
                    } else {
                        symbols.push_back(*it);
                    }
                    
                    counter++;
                }
            }
        } else {
            uint32_t nfat_arch = swap(fat->nfat_arch);
            
            //store all the architectures instead of reading over and over
            struct fat_arch *archs = new struct fat_arch[nfat_arch];
            fread(archs, sizeof(struct fat_arch) * nfat_arch, 1, file);
            
            for (uint32_t i = 0; i < swap(fat->nfat_arch); i++) {
                struct fat_arch arch = archs[i];
                struct mach_header_64 *header = new struct mach_header_64; //use mach_header_64 for safety
                
                uint32_t offset = swap(arch.offset);
                
                fseek(file, offset, SEEK_SET);
                fread(header, sizeof(struct mach_header), 1, file);
                
                if (header->magic == MH_MAGIC_64 || header->magic == MH_CIGAM_64) {
                    fread(&header->reserved, sizeof(uint32_t), 1, file); //don't really need this, but let's advance the file pointer for accuracy
                }
                
                std::map<String, std::vector<const NXArchInfo *>> reexports_;
                
                getloadcommands(file, offset, header, symtab, dylib_cmd, uuid_cmd, true, reexports, nameOffset);
                getdylibversions(header, dylib_cmd, currentVersion, compatibilityVersion);
                
                installName = getdylibname(file, header, nameOffset);
                uuids.insert(std::pair<const NXArchInfo *, String>(NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype)), getuuid(uuid_cmd->uuid)));
                
                for (std::map<String, std::vector<const NXArchInfo *>>::iterator it = reexports_.begin(); it != reexports_.end(); it++) {
                    std::map<String, std::vector<const NXArchInfo *>>::iterator jt = std::find(reexports.begin(), reexports.end(), *it);
                    if (jt != reexports.end()) {
                        jt->second.push_back(NXGetArchInfoFromCpuType(swap(header->cputype), swap(header->cpusubtype)));
                    } else {
                        reexports.insert(*it);
                    }
                }
                
                std::vector<Symbol> symbols_ = retrievesymbols(file, offset, header, symtab, true);
                if (symbols_.size() > 15000) {
                    const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype));
                    fprintf(stderr, "Symbol Processing for architecture \"%s\" will take a while (tbdconverter has to process %lu symbols)", archInfo->name, symbols_.size()); //Show message while tbdconverter processes symbols
                }
                
                static unsigned int counter = 0;
                static unsigned int dotCounter = 0;
                
                const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype));
                for (std::vector<Symbol>::iterator it = symbols_.begin(); it != symbols_.end(); it++) {
                    if (symbols_.size() > 15000 && counter % 9750 == 0 && dotCounter < 3) {
                        fprintf(stderr, ".");
                        dotCounter++;
                    }
                    
                    std::vector<Symbol>::iterator jt = std::find(symbols.begin(), symbols.end(), *it);
                    if (jt != symbols.end()) {
                        jt->addArchitecture(archInfo);
                    } else {
                        symbols.push_back(*it);
                    }
                    
                    counter++;
                }
            }
        }
    } else if (fat->magic == MH_MAGIC || fat->magic == MH_CIGAM || is64(fat->magic)) {
        struct mach_header_64 *header = new struct mach_header_64;
        memcpy(header, fat, sizeof(struct fat_header));
        
        //after memcpy the first 2 uint32_t fields of fat into mach_header, read the rest of mach_header
        fread(&header->cpusubtype, sizeof(struct mach_header) - sizeof(struct fat_header), 1, file);
        if (is64(header->magic)) {
            fread(&header->reserved, sizeof(uint32_t), 1, file);
        }
        
        std::map<String, std::vector<const NXArchInfo *>> reexports_;
        
        getloadcommands(file, 0x0, header, symtab, dylib_cmd, uuid_cmd, false, reexports, nameOffset);
        getdylibversions(header, dylib_cmd, currentVersion, compatibilityVersion);
        
        installName = getdylibname(file, header, nameOffset);
        uuids.insert(std::pair<const NXArchInfo *, String>(NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype)), getuuid(uuid_cmd->uuid)));
        
        const NXArchInfo *archInfo = NXGetArchInfoFromCpuType(swap(header->magic, header->cputype), swap(header->magic, header->cpusubtype));
        for (std::map<String, std::vector<const NXArchInfo *>>::iterator it = reexports_.begin(); it != reexports_.end(); it++) {
            std::map<String, std::vector<const NXArchInfo *>>::iterator jt = std::find(reexports.begin(), reexports.end(), *it);
            if (jt != reexports.end()) {
                jt->second.push_back(archInfo);
            } else {
                reexports.insert(*it);
            }
        }
        
        std::vector<Symbol> symbols_ = retrievesymbols(file, 0x0, header, symtab, false);
        if (symbols_.size() > 10000) {
            fprintf(stderr, "Symbol Processing will take a while (tbdconverter has to process %lu symbols)", symbols_.size()); //Show message while tbdconverter processes symbols
        }
        
        static unsigned int counter = 0;
        static unsigned int dotCounter = 0;
        
        for (std::vector<Symbol>::iterator it = symbols_.begin(); it != symbols_.end(); it++) {
            if (symbols_.size() > 15000 && counter % 9750 == 0 && dotCounter < 3) {
                fprintf(stderr, ".");
                dotCounter++;
            }
            
            std::vector<Symbol>::iterator jt = std::find(symbols.begin(), symbols.end(), *it);
            if (jt != symbols.end()) {
                jt->addArchitecture(archInfo);
            } else {
                symbols.push_back(*it);
            }
            
            counter++;
        }
        
        archInfos.push_back(archInfo);
    } else {
        error("Only Mach-O Files are supported");
    }
    
    //organize symbols into 'groups', based on each symbols supported architectures.
    
    std::vector<Group> symbolGroups;
    
    if (archInfos.empty()) {
        for (std::vector<Symbol>::const_iterator it = symbols.begin(); it != symbols.end(); it++) {
            std::vector<Group>::iterator jt = std::find(symbolGroups.begin(), symbolGroups.end(), Group(it->architectures()));
            if (jt != symbolGroups.end()) {
                jt->addSymbol(*it);
            } else {
                symbolGroups.push_back(Group(it->architectures()));
            }
        }
    } else {
        symbolGroups.emplace_back(archInfos);
        
        for (std::vector<Symbol>::const_iterator it = symbols.begin(); it != symbols.end(); it++) {
            symbolGroups.begin()->addSymbol(*it);
        }
    }
    
    if (symbols.size() > 15000) {
        fprintf(stderr, "\n"); //create newline since we finished symbol processing
    }
    
    String platform = "";
    while (platform.caseInsensitiveCompare("ios") != 0 && platform.caseInsensitiveCompare("macosx") != 0) {
        platform = Ask("Please enter the platform (ios/macosx)");
    }
    
    platform = platform.toLower(); //since we don't enforce case when requesting platform, we need to lower the case of `platform` to make it acceptable
    
    //now start making the tbd file, goal is to replicate an apple-made tbd file
    fprintf(output, "---%s\n", (version == "v2") ? " !tapi-tbd-v2" : "");
    
    if (archInfos.empty()) {
        fprintf(output, "archs:                 [ %s", uuids.begin()->first->name);
        
        std::map<const NXArchInfo *, String> uuidsCopy = uuids;
        uuidsCopy.erase(uuidsCopy.begin());
        
        for (std::map<const NXArchInfo *, String>::const_iterator it = uuidsCopy.begin()++; it != uuidsCopy.end(); it++) {
            fprintf(output, ", %s", it->first->name);
        }
        
        fprintf(output, " ]\n");
        if (version == "v2") {
            uuidsCopy = uuids;
            
            fprintf(output, "uuids:                 [ '%s: %s'", uuidsCopy.begin()->first->name, uuidsCopy.begin()->second.data());
            
            String::Size len = strlen(String(uuidsCopy.begin()->first->name) + ": " + uuidsCopy.begin()->second + "'");
            uuidsCopy.erase(uuidsCopy.begin());
            
            for (std::map<const NXArchInfo *, String>::const_iterator it = uuidsCopy.begin(); it != uuidsCopy.end(); it++) {
                len += strlen(String(it->first->name) + ": " + it->second + "'");
                fprintf(output, ", '%s: %s'", it->first->name, it->second.data());
                
                if (len >= 87) {
                    fprintf(output, "\n                   ");
                }
            }
            
            fprintf(output, " ]\n");
        }
    } else {
        std::vector<const NXArchInfo *> archInfos_ = archInfos;
        
        String name = archInfos_[0]->name;
        archInfos_.erase(archInfos_.begin());
        
        for (std::vector<const NXArchInfo *>::const_iterator it = archInfos_.begin(); it != archInfos_.end(); it++) {
            name += String(", ") + (*it)->name;
        }
        
        fprintf(output, "archs:                 [ %s ]\n", name.data());
    }
    
    fprintf(output, "platform:              %s\n", platform.data());
    fprintf(output, "install-name:          %s\n", installName.data());
    fprintf(output, "current-version:       %s\n", currentVersion.data());
    fprintf(output, "compatibility-version: %s\n", compatibilityVersion.data());
    
    if (version == "v2") fprintf(output, "objc-constraint:       none\n");
    fprintf(output, "exports:\n");
    
    for (std::vector<Group>::const_iterator it = symbolGroups.begin(); it != symbolGroups.end(); it++) {
        std::vector<String> reexports_ = {};
        for (std::map<String, std::vector<const NXArchInfo *>>::iterator jt = reexports.begin(); jt != reexports.end(); jt++) {
            if (Group(jt->second) == *it) {
                reexports_.push_back(jt->first);
            }
        }
        
        it->Print(output, reexports_);
    }
    
    fprintf(output, "...\n");
    if (output != stdout) { //print a Success message
        fprintf(stdout, "Successfully wrote to output file!\n");
    }
    
    return 0;
}
