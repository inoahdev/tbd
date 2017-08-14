//
//  src/macho/file.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>

#include <fcntl.h>
#include <unistd.h>

#include "file.h"

namespace macho {
    file::file(const std::string &path)
    : stream_(fopen(path.data(), "r")) {
        auto &stream = stream_;
        if (!stream) {
            fprintf(stderr, "Failed to open mach-o file at path (%s), failing with error (%s)\n", path.data(), strerror(errno));
            exit(1);
        }

        this->validate();
    }

    file::~file() {
        fclose(stream_);
    }

    bool file::is_valid_file(const std::string &path) noexcept {
        const auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            return false;
        }

        magic magic;

        read(descriptor, &magic, sizeof(magic));
        close(descriptor);

        return magic_is_valid(magic);
    }

    bool file::has_library_command(int descriptor, const struct header &header) noexcept {
        const auto header_magic_is_big_endian = magic_is_big_endian(header.magic);
        const auto header_magic_is_64_bit = magic_is_64_bit(header.magic);

        if (header_magic_is_64_bit) {
            lseek(descriptor, sizeof(uint32_t), SEEK_CUR);
        } else {
            const auto header_magic_is_32_bit = magic_is_32_bit(header.magic);
            if (!header_magic_is_32_bit) {
                return false;
            }
        }

        const auto load_commands_size = header.sizeofcmds;
        const auto load_commands = new char[load_commands_size];

        read(descriptor, load_commands, load_commands_size);

        auto index = 0;
        auto size_left = load_commands_size;

        const auto &ncmds = header.ncmds;
        for (auto i = 0; i < ncmds; i++) {
            const auto load_cmd = (struct load_command *)&load_commands[index];
            if (header_magic_is_big_endian) {
                swap_load_command(load_cmd);
            }

            auto cmdsize = load_cmd->cmdsize;

            const auto cmdsize_is_too_small = cmdsize < sizeof(struct load_command);
            if (cmdsize_is_too_small) {
                goto fail;
            }

            const auto cmdsize_is_larger_than_load_command_space = cmdsize > size_left;
            if (cmdsize_is_larger_than_load_command_space) {
                goto fail;
            }

            const auto cmdsize_takes_up_rest_of_load_command_space = (cmdsize == size_left && i != ncmds - 1);
            if (cmdsize_takes_up_rest_of_load_command_space) {
                goto fail;
            }

            if (load_cmd->cmd == load_commands::identification_dylib) {
                return true;
            }

            index += cmdsize;
        }

    fail:
        delete[] load_commands;
        return false;
    };

    bool file::is_valid_library(const std::string &path) noexcept {
        const auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            return false;
        }

        magic magic;
        read(descriptor, &magic, sizeof(magic));

        const auto magic_is_fat_64 = macho::magic_is_fat_64(magic);
        if (magic_is_fat_64) {
            uint32_t nfat_arch;
            read(descriptor, &nfat_arch, sizeof(uint32_t));

            const auto magic_is_big_endian = magic == magic::fat_64_big_endian;
            if (magic_is_big_endian) {
                swap_value(nfat_arch);
            }

            const auto architectures = new architecture_64[nfat_arch];
            const auto architectures_size = sizeof(architecture_64) * nfat_arch;

            read(descriptor, architectures, architectures_size);

            if (magic_is_big_endian) {
                swap_fat_arch_64(architectures, nfat_arch);
            }

            for (auto i = 0; i < nfat_arch; i++) {
                const auto &architecture = architectures[i];
                header header;

                lseek(descriptor, architecture.offset, SEEK_SET);
                read(descriptor, &header, sizeof(struct header));

                if (!has_library_command(descriptor, header)) {
                    close(descriptor);
                    return false;
                }
            }

            delete[] architectures;
        } else {
            const auto magic_is_fat_32 = macho::magic_is_fat_32(magic);
            if (magic_is_fat_32) {
                uint32_t nfat_arch;
                read(descriptor, &nfat_arch, sizeof(uint32_t));

                const auto magic_is_big_endian = magic == magic::fat_big_endian;
                if (magic_is_big_endian) {
                    swap_value(nfat_arch);
                }

                const auto architectures = new architecture[nfat_arch];
                const auto architectures_size = sizeof(architecture) * nfat_arch;

                read(descriptor, architectures, architectures_size);

                if (magic_is_big_endian) {
                    swap_fat_arch(architectures, nfat_arch);
                }

                for (auto i = 0; i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];
                    header header;

                    lseek(descriptor, architecture.offset, SEEK_SET);
                    read(descriptor, &header, sizeof(header));

                    if (!has_library_command(descriptor, header)) {
                        close(descriptor);
                        return false;
                    }
                }

                delete[] architectures;
            } else {
                const auto magic_is_thin = macho::magic_is_thin(magic);
                if (magic_is_thin) {
                    header header;
                    header.magic = magic;

                    read(descriptor, &header.cputype, sizeof(header) - sizeof(uint32_t));
                    if (!has_library_command(descriptor, header)) {
                        close(descriptor);
                        return false;
                    }
                } else {
                    close(descriptor);
                    return false;
                }
            }
        }

        close(descriptor);
        return true;
    }

    void file::validate() {
        auto &containers = containers_;
        auto &stream = stream_;

        auto &magic = magic_;
        fread(&magic, sizeof(magic), 1, stream);

        const auto magic_is_fat = macho::magic_is_fat(magic);
        if (magic_is_fat) {
            uint32_t nfat_arch;
            fread(&nfat_arch, sizeof(uint32_t), 1, stream);

            if (!nfat_arch) {
                fprintf(stderr, "Mach-o file has 0 architectures");
                exit(1);
            }

            auto magic_is_big_endian = macho::magic_is_big_endian(magic);
            if (magic_is_big_endian) {
                swap_value(nfat_arch);
            }

            containers.reserve(nfat_arch);

            const auto magic_is_fat_64 = macho::magic_is_fat_64(magic);
            if (magic_is_fat_64) {
                const auto architectures = new architecture_64[nfat_arch];
                const auto architectures_size = sizeof(architecture_64) * nfat_arch;

                fread(architectures, architectures_size, 1, stream);

                if (magic_is_big_endian) {
                    swap_fat_arch_64(architectures, nfat_arch);
                }

                for (auto i = 0; i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];
                    containers.emplace_back(stream, architecture.offset, architecture.size);
                }

                delete[] architectures;
            } else {
                const auto architectures = new architecture[nfat_arch];
                const auto architectures_size = sizeof(architecture) * nfat_arch;

                fread(architectures, architectures_size, 1, stream);

                if (magic_is_big_endian) {
                    swap_fat_arch(architectures, nfat_arch);
                }

                for (auto i = 0; i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];
                    containers.emplace_back(stream, architecture.offset, architecture.size);
                }

                delete[] architectures;
            }
        } else {
            const auto magic_is_thin = macho::magic_is_thin(magic);
            if (magic_is_thin) {
                containers.emplace_back(stream, 0);
            } else {
                fputs("Mach-o file is invalid, does not have a valid magic-number", stderr);
                exit(1);
            }
        }
    }
}
