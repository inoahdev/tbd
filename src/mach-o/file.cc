//
//  src/mach-o/file.cc
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
    file::creation_result file::create(file *file, const std::string &path) noexcept {
        file->stream = fopen(path.data(), "r");
        if (!file->stream) {
            return file::creation_result::failed_to_open_stream;
        }

        return file->validate();
    }

    file::~file() noexcept {
        if (stream != nullptr) {
            fclose(stream);
        }
    }

    file::creation_result file::validate() noexcept {
        if (fread(&magic, sizeof(magic), 1, stream) != 1) {
            return file::creation_result::stream_read_error;
        }

        const auto magic_is_fat = macho::magic_is_fat(magic);
        if (magic_is_fat) {
            auto nfat_arch = uint32_t();
            if (fread(&nfat_arch, sizeof(nfat_arch), 1, stream) != 1) {
                return file::creation_result::stream_read_error;
            }

            if (!nfat_arch) {
                return file::creation_result::zero_architectures;
            }

            auto magic_is_big_endian = macho::magic_is_big_endian(magic);
            if (magic_is_big_endian) {
                swap_value(nfat_arch);
            }

            containers.reserve(nfat_arch);

            const auto magic_is_fat_64 = macho::magic_is_fat_64(magic);
            if (magic_is_fat_64) {
                const auto architectures = std::make_unique<architecture_64[]>(nfat_arch);
                const auto architectures_size = sizeof(architecture_64) * nfat_arch;

                if (fread(architectures.get(), architectures_size, 1, stream) != 1) {
                    return file::creation_result::stream_read_error;
                }

                if (magic_is_big_endian) {
                    swap_fat_arch_64(architectures.get(), nfat_arch);
                }

                for (auto i = 0; i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];

                    auto container = macho::container();
                    auto container_creation_result = container::create(&container, stream, architecture.offset, architecture.size);

                    switch (container_creation_result) {
                        case container::creation_result::ok:
                            break;

                        case container::creation_result::stream_seek_error:
                            return file::creation_result::stream_seek_error;

                        case container::creation_result::stream_read_error:
                            return file::creation_result::stream_read_error;

                        case container::creation_result::fat_container:
                        case container::creation_result::not_a_macho:
                            return file::creation_result::invalid_container;
                    }

                    containers.push_back(std::move(container));
                }
            } else {
                const auto architectures = std::make_unique<architecture[]>(nfat_arch);
                const auto architectures_size = sizeof(architecture) * nfat_arch;

                if (fread(architectures.get(), architectures_size, 1, stream) != 1) {
                    return file::creation_result::stream_read_error;
                }

                if (magic_is_big_endian) {
                    swap_fat_arch(architectures.get(), nfat_arch);
                }

                for (auto i = 0; i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];

                    auto container = macho::container();
                    auto container_creation_result = container::create(&container, stream, architecture.offset, architecture.size);

                    switch (container_creation_result) {
                        case container::creation_result::ok:
                            break;

                        case container::creation_result::stream_seek_error:
                            return file::creation_result::stream_seek_error;

                        case container::creation_result::stream_read_error:
                            return file::creation_result::stream_read_error;

                        case container::creation_result::fat_container:
                        case container::creation_result::not_a_macho:
                            return file::creation_result::invalid_container;
                    }

                    containers.push_back(std::move(container));
                }
            }
        } else {
            const auto magic_is_thin = macho::magic_is_thin(magic);
            if (magic_is_thin) {
                auto container = macho::container();
                auto container_creation_result = container::create(&container, stream, 0);

                switch (container_creation_result) {
                    case container::creation_result::ok:
                        break;

                    case container::creation_result::stream_seek_error:
                        return file::creation_result::stream_seek_error;

                    case container::creation_result::stream_read_error:
                        return file::creation_result::stream_read_error;

                    case container::creation_result::fat_container:
                    case container::creation_result::not_a_macho:
                        return file::creation_result::not_a_macho;
                }

                containers.push_back(std::move(container));
            } else {
                return creation_result::not_a_macho;
            }
        }

        return file::creation_result::ok;
    }

    bool file::is_valid_file(const std::string &path, check_error *error) noexcept {
        const auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_open_descriptor;
            }

            return false;
        }

        enum magic magic;

        if (read(descriptor, &magic, sizeof(magic)) == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_read_descriptor;
            }

            return false;
        }

        if (close(descriptor) != 0) {
            if (error != nullptr) {
                *error = check_error::failed_to_close_descriptor;
            }
        }

        return magic_is_valid(magic);
    }

    bool file::has_library_command(int descriptor, const struct header *header, check_error *error) noexcept {
        const auto header_magic_is_big_endian = magic_is_big_endian(header->magic);
        const auto header_magic_is_64_bit = magic_is_64_bit(header->magic);

        if (header_magic_is_64_bit) {
            if (lseek(descriptor, sizeof(magic), SEEK_CUR) == -1) {
                if (error != nullptr) {
                    *error = check_error::failed_to_seek_descriptor;
                }

                return false;
            }
        } else {
            const auto header_magic_is_32_bit = magic_is_32_bit(header->magic);
            if (!header_magic_is_32_bit) {
                return false;
            }
        }

        const auto load_commands_size = header->sizeofcmds;
        const auto load_commands = std::make_unique<char[]>(load_commands_size);

        if (read(descriptor, load_commands.get(), load_commands_size) == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_read_descriptor;
            }

            return false;
        }

        auto index = 0;
        auto size_left = load_commands_size;

        const auto &ncmds = header->ncmds;
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
        return false;
    }

    bool file::is_valid_library(const std::string &path, check_error *error) noexcept {
        const auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_open_descriptor;
            }

            return false;
        }

        auto magic = ::macho::magic();
        if (read(descriptor, &magic, sizeof(magic)) == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_read_descriptor;
            }

            if (close(descriptor) != 0) {
                if (error != nullptr) {
                    *error = check_error::failed_to_close_descriptor;
                }
            }

            return false;
        }

        const auto magic_is_fat_64 = macho::magic_is_fat_64(magic);
        if (magic_is_fat_64) {
            auto nfat_arch = uint32_t();
            if (read(descriptor, &nfat_arch, sizeof(nfat_arch)) == -1) {
                if (error != nullptr) {
                    *error = check_error::failed_to_read_descriptor;
                }

                goto fail;
            }

            const auto magic_is_big_endian = magic == magic::fat_64_big_endian;
            if (magic_is_big_endian) {
                swap_value(nfat_arch);
            }

            const auto architectures = std::make_unique<architecture_64[]>(nfat_arch);
            const auto architectures_size = sizeof(architecture_64) * nfat_arch;

            if (read(descriptor, architectures.get(), architectures_size) == -1) {
                if (error != nullptr) {
                    *error = check_error::failed_to_read_descriptor;
                }

                goto fail;
            }

            if (magic_is_big_endian) {
                swap_fat_arch_64(architectures.get(), nfat_arch);
            }

            for (auto i = 0; i < nfat_arch; i++) {
                const auto &architecture = architectures[i];
                header header;

                if (lseek(descriptor, architecture.offset, SEEK_SET) == -1) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_seek_descriptor;
                    }

                    goto fail;
                }

                if (read(descriptor, &header, sizeof(header)) == -1) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_read_descriptor;
                    }

                    goto fail;
                }

                if (!has_library_command(descriptor, &header, error)) {
                    goto fail;
                }
            }
        } else {
            const auto magic_is_fat_32 = macho::magic_is_fat_32(magic);
            if (magic_is_fat_32) {
                auto nfat_arch = uint32_t();
                if (read(descriptor, &nfat_arch, sizeof(nfat_arch)) == -1) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_read_descriptor;
                    }

                    goto fail;
                }

                const auto magic_is_big_endian = magic == magic::fat_big_endian;
                if (magic_is_big_endian) {
                    swap_value(nfat_arch);
                }

                const auto architectures = std::make_unique<architecture[]>(nfat_arch);
                const auto architectures_size = sizeof(architecture) * nfat_arch;

                if (read(descriptor, architectures.get(), architectures_size) == -1) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_read_descriptor;
                    }

                    goto fail;
                }

                if (magic_is_big_endian) {
                    swap_fat_arch(architectures.get(), nfat_arch);
                }

                for (auto i = 0; i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];
                    header header;

                    if (lseek(descriptor, architecture.offset, SEEK_SET) == -1) {
                        if (error != nullptr) {
                            *error = check_error::failed_to_seek_descriptor;
                        }

                        goto fail;
                    }

                    if (read(descriptor, &header, sizeof(header)) == -1) {
                        if (error != nullptr) {
                            *error = check_error::failed_to_read_descriptor;
                        }

                        goto fail;
                    }

                    if (!has_library_command(descriptor, &header, error)) {
                        goto fail;
                    }
                }
            } else {
                const auto magic_is_thin = macho::magic_is_thin(magic);
                if (magic_is_thin) {
                    header header;
                    header.magic = magic;

                    if (read(descriptor, &header.cputype, sizeof(header) - sizeof(header.magic)) == -1) {
                        if (error != nullptr) {
                            *error = check_error::failed_to_read_descriptor;
                        }

                        goto fail;
                    }

                    if (!has_library_command(descriptor, &header, error)) {
                        goto fail;
                    }
                } else {
                    goto fail;
                }
            }
        }

        if (close(descriptor) != 0) {
            if (error != nullptr) {
                *error = check_error::failed_to_close_descriptor;
            }
        }

        return true;

    fail:
        if (close(descriptor) != 0) {
            if (error != nullptr) {
                *error = check_error::failed_to_close_descriptor;
            }
        }

        return false;
    }
}
