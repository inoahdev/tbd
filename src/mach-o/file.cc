//
//  src/macho/file.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/swap.h>

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

    bool file::is_valid_library(const std::string &path) noexcept {
        const auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            return false;
        }

        uint32_t magic;
        read(descriptor, &magic, sizeof(uint32_t));

        auto has_library_command = [&](const uint64_t &base, const struct mach_header &header) {
            auto should_swap = false;
            if (header.magic == MH_MAGIC_64 || header.magic == MH_CIGAM_64) {
                lseek(descriptor, sizeof(uint32_t), SEEK_CUR);
            } else if (header.magic != MH_MAGIC && header.magic != MH_CIGAM) {
                close(descriptor);
                return false;
            }

            if (header.magic == MH_CIGAM_64 || header.magic == MH_CIGAM) {
                should_swap = true;
            }

            const auto load_commands = new char[header.sizeofcmds];
            read(descriptor, load_commands, header.sizeofcmds);

            const auto &ncmds = header.ncmds;
            auto index = 0;

            auto size_left = header.sizeofcmds;
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
                if (cmdsize < sizeof(struct load_command) || cmdsize > size_left || (cmdsize == size_left && i != ncmds - 1)) {
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

        if (magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64) {
            uint32_t nfat_arch;
            read(descriptor, &nfat_arch, sizeof(uint32_t));

            if (magic == FAT_CIGAM_64) {
                swap_value(nfat_arch);
            }

            auto architectures = new struct fat_arch_64[nfat_arch];
            read(descriptor, architectures, sizeof(struct fat_arch_64) * nfat_arch);

            if (magic == FAT_CIGAM_64) {
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

            if (magic == FAT_CIGAM) {
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
        } else if (magic == MH_MAGIC_64 || magic == MH_CIGAM_64 || magic == MH_MAGIC || magic == MH_CIGAM) {
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

        close(descriptor);
        return true;
    }

    void file::validate() {
        fread(&magic_, sizeof(uint32_t), 1, file_);

        if (magic_ == FAT_MAGIC || magic_ == FAT_CIGAM || magic_ == FAT_MAGIC_64 || magic_ == FAT_CIGAM_64) {
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
        } else if (magic_ == MH_MAGIC || magic_ == MH_CIGAM || magic_ == MH_MAGIC_64 || magic_ == MH_CIGAM_64) {
            containers_.emplace_back(file_, 0);
        } else {
            fputs("Mach-o file is invalid, does not have a valid magic-number", stderr);
            exit(1);
        }
    }
}
