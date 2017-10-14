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
    file::file(file &&file) noexcept :
    stream(file.stream), magic(file.magic), containers(std::move(file.containers)) {
        file.stream = nullptr;
        file.magic = magic::normal;
    }

    file::open_result file::open(const char *path) noexcept {
        mode_ = "r";
        stream = fopen(path, mode_);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers();
    }

    file::open_result file::open(const char *path, const char *mode) noexcept {
        mode_ = mode;
        stream = fopen(path, mode);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers();
    }

    file::open_result file::open(FILE *file, const char *mode) noexcept {
        mode_ = mode;
        stream = file;

        return load_containers();
    }

    file::open_result file::open_from_library(const char *path) noexcept {
        mode_ = "r";
        stream = fopen(path, mode_);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers(file_type::library);
    }

    file::open_result file::open_from_library(const char *path, const char *mode) noexcept {
        mode_ = mode;
        stream = fopen(path, mode);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers(file_type::library);
    }

    file::open_result file::open_from_library(FILE *file, const char *mode) noexcept {
        mode_ = mode;
        stream = file;

        return load_containers(file_type::library);
    }

    file::open_result file::open_from_dynamic_library(const char *path) noexcept {
        mode_ = "r";
        stream = fopen(path, mode_);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers(file_type::dynamic_library);
    }

    file::open_result file::open_from_dynamic_library(const char *path, const char *mode) noexcept {
        mode_ = mode;
        stream = fopen(path, mode);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        return load_containers(file_type::dynamic_library);
    }

    file::open_result file::open_from_dynamic_library(FILE *file, const char *mode) noexcept {
        mode_ = mode;
        stream = file;

        return load_containers(file_type::dynamic_library);
    }

    file::open_result file::open_copy(const file &file) {
        if (!file.stream) {
            if (stream != nullptr) {
                fclose(stream);
                stream = nullptr;
            }

            magic = magic::normal;
            containers.clear();

            // Copied an empty file object
            return open_result::ok;
        }

        mode_ = file.mode_;
        stream = freopen(nullptr, mode_, file.stream);

        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        magic = file.magic;
        containers.reserve(file.containers.size());

        for (auto &container : file.containers) {
            auto new_container = macho::container();
            auto new_container_open_result = new_container.open_copy(container);

            switch (new_container_open_result) {
                case container::open_result::ok:
                    break;

                default:
                    return open_result::invalid_container;
            }

            new_container.stream = stream;
            containers.emplace_back(std::move(new_container));
        }

        return open_result::ok;
    }

    file::open_result file::open_copy(const file &file, const char *mode) {
        if (!file.stream) {
            if (stream != nullptr) {
                fclose(stream);
                stream = nullptr;
            }

            magic = magic::normal;
            containers.clear();

            // Copied an empty file object
            return open_result::ok;
        }

        stream = freopen(nullptr, mode, file.stream);
        if (!stream) {
            return open_result::failed_to_open_stream;
        }

        magic = file.magic;
        containers.reserve(file.containers.size());

        for (auto &container : file.containers) {
            auto new_container = macho::container();
            auto new_container_open_result = new_container.open_copy(container);

            switch (new_container_open_result) {
                case container::open_result::ok:
                    break;

                default:
                    return open_result::invalid_container;
            }

            new_container.stream = stream;
            containers.emplace_back(std::move(new_container));
        }

        return open_result::ok;
    }

    file &file::operator=(file &&file) noexcept {
        stream = file.stream;
        magic = file.magic;

        containers = std::move(file.containers);

        stream = nullptr;
        magic = file.magic;

        return *this;
    }

    file::~file() noexcept {
        if (stream != nullptr) {
            fclose(stream);
        }

        containers.clear();
    }

    file::open_result file::load_containers(file::file_type type) noexcept {
        if (fread(&magic, sizeof(magic), 1, stream) != 1) {
            return open_result::stream_read_error;
        }

        const auto magic_is_fat = macho::magic_is_fat(magic);
        if (magic_is_fat) {
            auto nfat_arch = uint32_t();
            if (fread(&nfat_arch, sizeof(nfat_arch), 1, stream) != 1) {
                return open_result::stream_read_error;
            }

            if (!nfat_arch) {
                return open_result::zero_architectures;
            }

            auto magic_is_big_endian = macho::magic_is_big_endian(magic);
            if (magic_is_big_endian) {
                swap_uint32(nfat_arch);
            }

            containers.reserve(nfat_arch);

            const auto magic_is_fat_64 = macho::magic_is_fat_64(magic);
            if (magic_is_fat_64) {
                const auto architectures_size = sizeof(architecture_64) * nfat_arch;
                const auto architectures = (architecture_64 *)malloc(architectures_size);

                if (!architectures) {
                    return open_result::failed_to_allocate_memory;
                }

                if (fread(architectures, architectures_size, 1, stream) != 1) {
                    free(architectures);
                    return open_result::stream_read_error;
                }

                if (magic_is_big_endian) {
                    swap_fat_archs_64(architectures, nfat_arch);
                }

                for (auto i = uint32_t(); i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];

                    auto container = macho::container();
                    auto container_open_result = container::open_result::ok;

                    switch (type) {
                        case file_type::none:
                            container_open_result = container.open(stream);
                            break;

                        case file_type::library:
                            container_open_result = container.open_from_library(stream, architecture.offset, architecture.size);
                            break;

                        case file_type::dynamic_library:
                            container_open_result = container.open_from_dynamic_library(stream, architecture.offset, architecture.size);
                            break;
                    }

                    switch (container_open_result) {
                        case container::open_result::ok:
                            break;

                        case container::open_result::stream_seek_error:
                            free(architectures);
                            return open_result::stream_seek_error;

                        case container::open_result::stream_read_error:
                            free(architectures);
                            return open_result::stream_read_error;

                        case container::open_result::fat_container:
                        case container::open_result::not_a_macho:
                        case container::open_result::invalid_macho:
                        case container::open_result::invalid_range:
                            free(architectures);
                            return open_result::invalid_container;

                        case container::open_result::not_a_library:
                            if (type == file_type::library) {
                                free(architectures);
                                return open_result::not_a_library;
                            }

                            break;

                        case container::open_result::not_a_dynamic_library:
                            if (type == file_type::dynamic_library) {
                                free(architectures);
                                return open_result::not_a_dynamic_library;
                            }

                            break;
                    }

                    containers.emplace_back(std::move(container));
                }

                free(architectures);
            } else {
                const auto architectures_size = sizeof(architecture) * nfat_arch;
                const auto architectures = (architecture *)malloc(architectures_size);

                if (!architectures) {
                    return open_result::failed_to_allocate_memory;
                }

                if (fread(architectures, architectures_size, 1, stream) != 1) {
                    free(architectures);
                    return open_result::stream_read_error;
                }

                if (magic_is_big_endian) {
                    swap_fat_archs(architectures, nfat_arch);
                }

                for (auto i = uint32_t(); i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];

                    auto container = macho::container();
                    auto container_open_result = container::open_result::ok;

                    switch (type) {
                        case file_type::none:
                            container_open_result = container.open(stream);
                            break;

                        case file_type::library:
                            container_open_result = container.open_from_library(stream, architecture.offset, architecture.size);
                            break;

                        case file_type::dynamic_library:
                            container_open_result = container.open_from_dynamic_library(stream, architecture.offset, architecture.size);
                            break;
                    }

                    switch (container_open_result) {
                        case container::open_result::ok:
                            break;

                        case container::open_result::stream_seek_error:
                            free(architectures);
                            return open_result::stream_seek_error;

                        case container::open_result::stream_read_error:
                            free(architectures);
                            return open_result::stream_read_error;

                        case container::open_result::fat_container:
                        case container::open_result::not_a_macho:
                        case container::open_result::invalid_macho:
                        case container::open_result::invalid_range:
                            free(architectures);
                            return open_result::invalid_container;

                        case container::open_result::not_a_library:
                            if (type == file_type::library) {
                                free(architectures);
                                return open_result::not_a_library;
                            }

                            break;

                        case container::open_result::not_a_dynamic_library:
                            if (type == file_type::dynamic_library) {
                                free(architectures);
                                return open_result::not_a_dynamic_library;
                            }

                            break;
                    }

                    containers.emplace_back(std::move(container));
                }

                free(architectures);
            }
        } else {
            const auto magic_is_thin = macho::magic_is_thin(magic);
            if (magic_is_thin) {
                auto container = macho::container();
                auto container_open_result = container::open_result::ok;

                switch (type) {
                    case file_type::none:
                        container_open_result = container.open(stream);
                        break;

                    case file_type::library:
                        container_open_result = container.open_from_library(stream);
                        break;

                    case file_type::dynamic_library:
                        container_open_result = container.open_from_dynamic_library(stream);
                        break;
                }

                switch (container_open_result) {
                    case container::open_result::ok:
                        break;

                    case container::open_result::stream_seek_error:
                        return open_result::stream_seek_error;

                    case container::open_result::stream_read_error:
                        return open_result::stream_read_error;

                    case container::open_result::fat_container:
                    case container::open_result::not_a_macho:
                    case container::open_result::invalid_macho:
                    case container::open_result::invalid_range:
                        return open_result::not_a_macho;

                    case container::open_result::not_a_library:
                        if (type == file_type::library) {
                            return open_result::not_a_library;
                        }

                        break;

                    case container::open_result::not_a_dynamic_library:
                        if (type == file_type::dynamic_library) {
                            return open_result::not_a_dynamic_library;
                        }

                        break;
                }

                containers.emplace_back(std::move(container));
            } else {
                return open_result::not_a_macho;
            }
        }

        return open_result::ok;
    }

    bool file::is_valid_file(const char *path, check_error *error) noexcept {
        return is_valid_file_of_file_type(path, file_type::none, error);
    }

    bool file::is_valid_library(const char *path, check_error *error) noexcept {
        return is_valid_file_of_file_type(path, file_type::library, error);
    }

    bool file::is_valid_dynamic_library(const char *path, check_error *error) noexcept {
        return is_valid_file_of_file_type(path, file_type::dynamic_library, error);
    }

    bool file::has_library_command(int descriptor, const struct header *header, check_error *error) noexcept {
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
        if (!load_commands_size) {
            return false;
        }

        const auto load_commands = (uint8_t *)malloc(load_commands_size);
        if (!load_commands) {
            if (error != nullptr) {
                *error = check_error::failed_to_allocate_memory;
            }

            return false;
        }

        if (read(descriptor, load_commands, load_commands_size) == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_read_descriptor;
            }

            return false;
        }

        auto index = uint32_t();
        auto size_left = load_commands_size;

        const auto header_magic_is_big_endian = header->magic == magic::big_endian || header->magic == magic::bits64_big_endian;
        const auto &ncmds = header->ncmds;

        for (auto i = uint32_t(); i < ncmds; i++) {
            auto &load_cmd = *(struct load_command *)&load_commands[index];
            if (header_magic_is_big_endian) {
                swap_load_command(load_cmd);
            }

            auto cmdsize = load_cmd.cmdsize;
            if (cmdsize < sizeof(struct load_command)) {
                return false;
            }

            if (cmdsize > size_left || (cmdsize == size_left && i != ncmds - 1)) {
                return false;
            }

            if (load_cmd.cmd == load_commands::identification_dylib) {
                if (load_cmd.cmdsize < sizeof(dylib_command)) {
                    // Make sure load-command is valid
                    return false;
                }

                return true;
            }

            index += cmdsize;
        }

        return false;
    }
    
    bool file::is_valid_file_of_file_type(const char *path, file::file_type type, file::check_error *error) noexcept {
        const auto descriptor = ::open(path, O_RDONLY);
        if (descriptor == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_open_descriptor;
            }
            
            return false;
        }
        
        auto magic = macho::magic();
        if (read(descriptor, &magic, sizeof(magic)) == -1) {
            if (error != nullptr) {
                *error = check_error::failed_to_read_descriptor;
            }
            
            close(descriptor);
            return false;
        }
        
        const auto magic_is_fat = macho::magic_is_fat(magic);
        if (magic_is_fat) {
            auto nfat_arch = uint32_t();
            if (read(descriptor, &nfat_arch, sizeof(nfat_arch)) == -1) {
                if (error != nullptr) {
                    *error = check_error::failed_to_read_descriptor;
                }
                
                close(descriptor);
                return false;
            }
            
            const auto magic_is_big_endian = magic == magic::fat_big_endian || magic == magic::fat_64_big_endian;
            if (magic_is_big_endian) {
                swap_uint32(nfat_arch);
            }
            
            if (!nfat_arch) {
                close(descriptor);
                return false;
            }
            
            const auto magic_is_fat_64 = magic == magic::fat_64 || magic == magic::fat_64_big_endian;
            if (magic_is_fat_64) {
                const auto architectures_size = sizeof(architecture_64) * nfat_arch;
                const auto architectures = (architecture_64 *)malloc(architectures_size);
                
                if (!architectures) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_allocate_memory;
                    }
                    
                    close(descriptor);
                    return false;
                }
                
                if (read(descriptor, architectures, architectures_size) == -1) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_read_descriptor;
                    }
                    
                    free(architectures);
                    close(descriptor);
                    
                    return false;
                }
                
                if (magic_is_big_endian) {
                    swap_fat_archs_64(architectures, nfat_arch);
                }
                
                for (auto i = uint32_t(); i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];
                    header header;
                    
                    if (lseek(descriptor, architecture.offset, SEEK_SET) == -1) {
                        if (error != nullptr) {
                            *error = check_error::failed_to_seek_descriptor;
                        }
                        
                        free(architectures);
                        close(descriptor);
                        
                        return false;
                    }
                    
                    if (read(descriptor, &header, sizeof(header)) == -1) {
                        if (error != nullptr) {
                            *error = check_error::failed_to_read_descriptor;
                        }
                        
                        free(architectures);
                        close(descriptor);
                        
                        return false;
                    }
                    
                    if (magic_is_big_endian) {
                        swap_mach_header(header);
                    }
                    
                    if (type == file_type::library || type == file_type::dynamic_library) {
                        if (type == file_type::library) {
                            if (!filetype_is_library(header.filetype)) {
                                free(architectures);
                                
                                if (close(descriptor) != 0) {
                                    if (error != nullptr) {
                                        if (*error == check_error::ok) {
                                            *error = check_error::failed_to_close_descriptor;
                                        }
                                    }
                                }
                                
                                return false;
                            }
                        }
                        
                        if (type == file_type::dynamic_library) {
                            if (!filetype_is_dynamic_library(header.filetype)) {
                                free(architectures);
                                
                                if (close(descriptor) != 0) {
                                    if (error != nullptr) {
                                        if (*error == check_error::ok) {
                                            *error = check_error::failed_to_close_descriptor;
                                        }
                                    }
                                }
                                
                                return false;
                            }
                        }
                        
                        if (!has_library_command(descriptor, &header, error)) {
                            free(architectures);
                            
                            if (close(descriptor) != 0) {
                                if (error != nullptr) {
                                    if (*error == check_error::ok) {
                                        *error = check_error::failed_to_close_descriptor;
                                    }
                                }
                            }
                            
                            return false;
                        }
                    }
                }
                
                free(architectures);
            } else {
                const auto architectures_size = sizeof(architecture) * nfat_arch;
                const auto architectures = (architecture *)malloc(architectures_size);
                
                if (!architectures) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_allocate_memory;
                    }
                    
                    close(descriptor);
                    return false;
                }
                
                if (read(descriptor, architectures, architectures_size) == -1) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_read_descriptor;
                    }
                    
                    free(architectures);
                    close(descriptor);
                    
                    return false;
                }
                
                if (magic_is_big_endian) {
                    swap_fat_archs(architectures, nfat_arch);
                }
                
                for (auto i = uint32_t(); i < nfat_arch; i++) {
                    const auto &architecture = architectures[i];
                    header header;
                    
                    if (lseek(descriptor, architecture.offset, SEEK_SET) == -1) {
                        if (error != nullptr) {
                            *error = check_error::failed_to_seek_descriptor;
                        }
                        
                        free(architectures);
                        close(descriptor);
                        
                        return false;
                    }
                    
                    if (read(descriptor, &header, sizeof(header)) == -1) {
                        if (error != nullptr) {
                            *error = check_error::failed_to_read_descriptor;
                        }
                        
                        free(architectures);
                        close(descriptor);
                        
                        return false;
                    }
                    
                    if (magic_is_big_endian) {
                        swap_mach_header(header);
                    }
                    
                    if (type == file_type::library || type == file_type::dynamic_library) {
                        if (type == file_type::library) {
                            if (!filetype_is_library(header.filetype)) {
                                free(architectures);
                                
                                if (close(descriptor) != 0) {
                                    if (error != nullptr) {
                                        if (*error == check_error::ok) {
                                            *error = check_error::failed_to_close_descriptor;
                                        }
                                    }
                                }
                                
                                return false;
                            }
                        }
                        
                        if (type == file_type::dynamic_library) {
                            if (!filetype_is_dynamic_library(header.filetype)) {
                                free(architectures);
                                
                                if (close(descriptor) != 0) {
                                    if (error != nullptr) {
                                        if (*error == check_error::ok) {
                                            *error = check_error::failed_to_close_descriptor;
                                        }
                                    }
                                }
                                
                                return false;
                            }
                        }
                        
                        if (!has_library_command(descriptor, &header, error)) {
                            free(architectures);
                            
                            if (close(descriptor) != 0) {
                                if (error != nullptr) {
                                    if (*error == check_error::ok) {
                                        *error = check_error::failed_to_close_descriptor;
                                    }
                                }
                            }
                            
                            return false;
                        }
                    }
                }
                
                free(architectures);
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
                    
                    close(descriptor);
                    return false;
                }
                
                auto magic_is_big_endian = magic == magic::big_endian || magic == magic::bits64_big_endian;
                if (magic_is_big_endian) {
                    swap_mach_header(header);
                }

                if (type == file_type::library || type == file_type::dynamic_library) {
                    if (type == file_type::library) {
                        if (!filetype_is_library(header.filetype)) {
                            close(descriptor);
                            return false;
                        }
                    }
                    
                    if (type == file_type::dynamic_library) {
                        if (!filetype_is_dynamic_library(header.filetype)) {
                            close(descriptor);
                            return false;
                        }
                    }
                
                    if (!has_library_command(descriptor, &header, error)) {
                        if (close(descriptor) != 0) {
                            if (error != nullptr) {
                                if (*error == check_error::ok) {
                                    *error = check_error::failed_to_close_descriptor;
                                }
                            }
                        }
                        
                        return false;
                    }
                }
            } else {
                if (close(descriptor) != 0) {
                    if (error != nullptr) {
                        *error = check_error::failed_to_close_descriptor;
                    }
                }
                
                return false;
            }
        }
        
        if (close(descriptor) != 0) {
            if (error != nullptr) {
                *error = check_error::failed_to_close_descriptor;
            }
        }
        
        return true;
    }
}
