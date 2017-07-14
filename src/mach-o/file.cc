//
//  src/macho/file.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/swap.h>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include "file.h"

namespace macho {
    file::file(const std::string &path)
    : file_(fopen(path.data(), "r")) {
        if (!file_) {
            fprintf(stderr, "Unable to open mach-o file at path (%s)\n", path.data());
            exit(1);
        }

        this->validate();
    }

    file::~file() {
        fclose(file_);
    }

    bool file::is_valid_file(const std::string &path) noexcept {
        const auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            return false;
        }

        uint32_t magic;
        read(descriptor, &magic, sizeof(uint32_t));

        auto result = magic == MH_MAGIC || magic == MH_CIGAM || magic == MH_MAGIC_64 || magic == MH_CIGAM_64 || magic == FAT_MAGIC || magic == FAT_CIGAM || magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64;

        close(descriptor);
        return result;
    }
    
    bool file::has_library_command(int descriptor, const struct mach_header &header) noexcept {
        auto should_swap = false;
        
        const auto header_magic_is_64_bit = header.magic == MH_MAGIC_64 || header.magic == MH_CIGAM_64;
        if (header_magic_is_64_bit) {
            lseek(descriptor, sizeof(uint32_t), SEEK_CUR);
        } else {
            const auto header_magic_is_32_bit = header.magic == MH_MAGIC || header.magic == MH_CIGAM;
            if (!header_magic_is_32_bit) {
                close(descriptor);
                return false;
            }
        }
        
        const auto header_magic_is_big_endian = header.magic == MH_CIGAM_64 || header.magic == MH_CIGAM;
        if (header_magic_is_big_endian) {
            should_swap = true;
        }
        
        const auto load_commands = new char[header.sizeofcmds];
        read(descriptor, load_commands, header.sizeofcmds);
        
        auto index = 0;
        auto size_left = header.sizeofcmds;
        
        const auto &ncmds = header.ncmds;
        for (auto i = 0; i < ncmds; i++) {
            const auto load_cmd = (struct load_command *)&load_commands[index];
            if (should_swap) {
                swap_load_command(load_cmd, NX_LittleEndian);
            }
            
            if (load_cmd->cmd == LC_ID_DYLIB) {
                delete[] load_commands;
                close(descriptor);
                
                return true;
            }
            
            auto cmdsize = load_cmd->cmdsize;
            
            const auto cmdsize_is_too_small = cmdsize < sizeof(struct load_command);
            const auto cmdsize_is_larger_than_load_command_space = cmdsize > size_left;
            const auto cmdsize_takes_up_rest_of_load_command_space = (cmdsize == size_left && i != ncmds - 1);
            
            if (cmdsize_is_too_small || cmdsize_is_larger_than_load_command_space || cmdsize_takes_up_rest_of_load_command_space) {
                delete[] load_commands;
                close(descriptor);
                
                return false;
            }
            
            index += cmdsize;
        }
        
        delete[] load_commands;
        close(descriptor);
        
        return false;
    };

    bool file::is_valid_library(const std::string &path) noexcept {
        const auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            return false;
        }

        uint32_t magic;
        read(descriptor, &magic, sizeof(uint32_t));

        if (magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64) {
            uint32_t nfat_arch;
            read(descriptor, &nfat_arch, sizeof(uint32_t));

            const auto magic_is_64_bit = magic == FAT_CIGAM_64;
            if (magic_is_64_bit) {
                swap_value(nfat_arch);
            }

            auto architectures = new struct fat_arch_64[nfat_arch];
            read(descriptor, architectures, sizeof(struct fat_arch_64) * nfat_arch);

            if (magic_is_64_bit) {
                swap_fat_arch_64(architectures, nfat_arch, NX_LittleEndian);
            }

            for (auto i = 0; i < nfat_arch; i++) {
                const auto &architecture = architectures[i];
                struct mach_header header;

                lseek(descriptor, architecture.offset, SEEK_SET);
                read(descriptor, &header, sizeof(struct mach_header));

                if (!has_library_command(architecture.offset + sizeof(struct mach_header), header)) {
                    close(descriptor);
                    return false;
                }
            }

            delete[] architectures;
        } else if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
            uint32_t nfat_arch;
            read(descriptor, &nfat_arch, sizeof(uint32_t));

            swap_value(nfat_arch);

            auto architectures = new struct fat_arch[nfat_arch];
            read(descriptor, architectures, sizeof(struct fat_arch) * nfat_arch);

            const auto magic_is_big_endian = magic == FAT_CIGAM;
            if (magic_is_big_endian) {
                swap_fat_arch(architectures, nfat_arch, NX_LittleEndian);
            }

            for (auto i = 0; i < nfat_arch; i++) {
                const auto &architecture = architectures[i];
                struct mach_header header;

                lseek(descriptor, architecture.offset, SEEK_SET);
                read(descriptor, &header, sizeof(struct mach_header));

                if (!has_library_command(architecture.offset + sizeof(struct mach_header), header)) {
                    close(descriptor);
                    return false;
                }
            }

            delete[] architectures;
        } else {
            const auto magic_is_thin = magic == MH_MAGIC_64 || magic == MH_CIGAM_64 || magic == MH_MAGIC || magic == MH_CIGAM;
            if (magic_is_thin) {
                struct mach_header header;
                header.magic = magic;

                read(descriptor, &header.cputype, sizeof(struct mach_header) - sizeof(uint32_t));
                if (!has_library_command(sizeof(struct mach_header), header)) {
                    close(descriptor);
                    return false;
                }
            } else {
                close(descriptor);
                return false;
            }
        }

        close(descriptor);
        return true;
    }

    void file::validate() {
        fread(&magic_, sizeof(uint32_t), 1, file_);

        const auto magic_is_fat = magic_ == FAT_MAGIC || magic_ == FAT_CIGAM || magic_ == FAT_MAGIC_64 || magic_ == FAT_CIGAM_64;
        if (magic_is_fat) {
            uint32_t nfat_arch;
            fread(&nfat_arch, sizeof(uint32_t), 1, file_);

            if (!nfat_arch) {
                fprintf(stderr, "Mach-o file has 0 architectures");
                exit(1);
            }

            auto should_swap = magic_ == FAT_CIGAM || magic_ == FAT_CIGAM_64;
            if (should_swap) {
                swap_value(nfat_arch);
            }

            containers_.reserve(nfat_arch);
            if (magic_ == FAT_MAGIC_64 || magic_ == FAT_CIGAM_64) {
                const auto architectures = new struct fat_arch_64[nfat_arch];
                fread(architectures, sizeof(struct fat_arch_64) * nfat_arch, 1, file_);

                if (should_swap) {
                    swap_fat_arch_64(architectures, nfat_arch, NX_LittleEndian);
                }

                for (auto i = 0; i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];
                    containers_.emplace_back(file_, 0, architecture);
                }

                delete[] architectures;
            } else {
                const auto architectures = new struct fat_arch[nfat_arch];
                fread(architectures, sizeof(struct fat_arch) * nfat_arch, 1, file_);

                if (should_swap) {
                    swap_fat_arch(architectures, nfat_arch, NX_LittleEndian);
                }

                for (auto i = 0; i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];
                    containers_.emplace_back(file_, 0, architecture);
                }

                delete[] architectures;
            }
        } else {
            const auto magic_is_thin = magic_ == MH_MAGIC || magic_ == MH_CIGAM || magic_ == MH_MAGIC_64 || magic_ == MH_CIGAM_64;
            if (magic_is_thin) {
                containers_.emplace_back(file_, 0);
            } else {
                fputs("Mach-o file is invalid, does not have a valid magic-number", stderr);
                exit(1);
            }
        }
    }
}
