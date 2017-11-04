//
//  src/mach-o/file.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "container.h"

namespace macho {
    class file {
    public:
        explicit file() = default;

        FILE *stream = nullptr;
        magic magic = magic::normal;

        std::vector<container> containers = std::vector<container>();

        explicit file(const file &) = delete;
        explicit file(file &&) noexcept;

        file &operator=(const file &) = delete;
        file &operator=(file &&) noexcept;

        ~file() noexcept;

        enum class open_result {
            ok,

            failed_to_open_stream,
            failed_to_allocate_memory,
            failed_to_retrieve_information,

            stream_seek_error,
            stream_read_error,

            zero_architectures,
            architectures_goes_past_end_of_file,

            invalid_container,
            not_a_macho,

            not_a_library,
            not_a_dynamic_library
        };

        open_result open(const char *path) noexcept;
        inline open_result open(const std::string &path) noexcept {
            return open(path.data());
        }

        open_result open(const char *path, const char *mode) noexcept;
        inline open_result open(const std::string &path, const char *mode) noexcept {
            return open(path.data(), mode);
        }

        open_result open(FILE *file, const char *mode = "r") noexcept;

        open_result open_from_library(const char *path) noexcept;
        inline open_result open_from_library(const std::string &path) noexcept {
            return open_from_library(path.data());
        }

        open_result open_from_library(const char *path, const char *mode) noexcept;
        inline open_result open_from_library(const std::string &path, const char *mode) noexcept {
            return open_from_library(path.data(), mode);
        }

        open_result open_from_library(FILE *file, const char *mode = "r") noexcept;

        open_result open_from_dynamic_library(const char *path) noexcept;
        inline open_result open_from_dynamic_library(const std::string &path) noexcept {
            return open_from_dynamic_library(path.data());
        }

        open_result open_from_dynamic_library(const char *path, const char *mode) noexcept;
        inline open_result open_from_dynamic_library(const std::string &path, const char *mode) noexcept {
            return open_from_dynamic_library(path.data(), mode);
        }

        open_result open_from_dynamic_library(FILE *file, const char *mode = "r") noexcept;

        open_result open_copy(const file &file);
        open_result open_copy(const file &file, const char *mode);

        enum class check_error {
            ok,
            failed_to_allocate_memory,

            failed_to_open_descriptor,
            failed_to_close_descriptor,

            failed_to_seek_descriptor,
            failed_to_read_descriptor
        };

        static bool is_valid_file(const char *path, check_error *error = nullptr) noexcept;
        static bool is_valid_file(const std::string &path, check_error *error = nullptr) noexcept {
            return is_valid_file(path.data());
        }

        static bool is_valid_library(const char *path, check_error *error = nullptr) noexcept;
        static bool is_valid_library(const std::string &path, check_error *error = nullptr) noexcept {
            return is_valid_library(path.data());
        }

        static bool is_valid_dynamic_library(const char *path, check_error *error = nullptr) noexcept;
        static bool is_valid_dynamic_library(const std::string &path, check_error *error = nullptr) noexcept {
            return is_valid_dynamic_library(path.data());
        }

    private:
        const char *mode_ = nullptr; // for copies

        static bool has_library_command(int descriptor, const struct header *header, check_error *error) noexcept;
        static int get_library_file_descriptor(const char *path, check_error *error);

        enum class type : uint64_t {
            none,
            library,
            dynamic_library
        };

        template <type type = type::none>
        static inline bool is_valid_file_of_file_type(const char *path, check_error *error = nullptr) noexcept {
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

                        if constexpr (type == type::library || type == type::dynamic_library) {
                            if constexpr (type == type::library) {
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

                            if constexpr (type == type::dynamic_library) {
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

                        if constexpr (type == type::library || type == type::dynamic_library) {
                            if constexpr (type == type::library) {
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

                            if constexpr (type == type::dynamic_library) {
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

                    if constexpr (type == type::library || type == type::dynamic_library) {
                        if constexpr (type == type::library) {
                            if (!filetype_is_library(header.filetype)) {
                                close(descriptor);
                                return false;
                            }
                        }

                        if constexpr (type == type::dynamic_library) {
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

        template <type type = type::none>
        inline open_result load_containers() noexcept {
            struct stat information;
            if (fstat(stream->_file, &information) != 0) {
                return open_result::failed_to_retrieve_information;
            }

            const auto remaining_size = [&](size_t size) noexcept {
                auto pos = ftell(stream);
                if (pos == -1L) {
                    return -1uL;
                }

                auto remaining_space = information.st_size - pos;
                if (size > remaining_space) {
                    return -1uL;
                }

                return static_cast<unsigned long>(size - remaining_space);
            };

            if (fread(&magic, sizeof(magic), 1, stream) != 1) {
                return open_result::stream_read_error;
            }

            const auto magic_is_fat = macho::magic_is_fat(magic);
            if (magic_is_fat) {
                struct range {
                    uint64_t location;
                    uint64_t size;
                };

                auto ranges = std::vector<range>();
                const auto find_range = [&](uint64_t location, uint64_t size) {
                    for (const auto &range : ranges) {
                        const auto range_end = range.location + range.size;
                        if (location >= range.location && location < range_end) {
                            return true;
                        }

                        auto end = location + size;
                        if (end >= range.location && end < range_end) {
                            return true;
                        }
                    }

                    return false;
                };

                ranges.emplace_back(range({ 0, sizeof(macho::fat_header) }));

                auto nfat_arch = uint32_t();
                if (fread(&nfat_arch, sizeof(nfat_arch), 1, stream) != 1) {
                    return open_result::stream_read_error;
                }

                if (!nfat_arch) {
                    return open_result::zero_architectures;
                }

                const auto magic_is_big_endian = macho::magic_is_big_endian(magic);
                if (magic_is_big_endian) {
                    swap_uint32(nfat_arch);
                }

                containers.reserve(nfat_arch);

                const auto magic_is_fat_64 = macho::magic_is_fat_64(magic);
                if (magic_is_fat_64) {
                    const auto architectures_size = sizeof(architecture_64) * nfat_arch;
                    if (remaining_size(architectures_size) <= 0) {
                        return open_result::architectures_goes_past_end_of_file;
                    }

                    const auto architectures = (architecture_64 *)malloc(architectures_size);
                    if (!architectures) {
                        return open_result::failed_to_allocate_memory;
                    }

                    ranges.emplace_back(range({ sizeof(macho::fat_header), architectures_size }));
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

                        if (remaining_size(architecture.size) <= 0) {
                            return open_result::invalid_container;
                        }

                        if (find_range(architecture.offset, architecture.size)) {
                            return open_result::invalid_container;
                        }

                        if constexpr (type == type::none) {
                            container_open_result = container.open(stream);
                        }

                        if constexpr (type == type::library) {
                            container_open_result = container.open_from_library(stream, architecture.offset, architecture.size);
                        }

                        if constexpr (type == type::dynamic_library) {
                            container_open_result = container.open_from_dynamic_library(stream, architecture.offset, architecture.size);
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
                                if constexpr (type == type::library) {
                                    free(architectures);
                                    return open_result::not_a_library;
                                }

                                break;

                            case container::open_result::not_a_dynamic_library:
                                if constexpr (type == type::dynamic_library) {
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
                    if (remaining_size(architectures_size) <= 0) {
                        return open_result::architectures_goes_past_end_of_file;
                    }

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

                        if (remaining_size(architecture.size) <= 0) {
                            return open_result::invalid_container;
                        }

                        if (find_range(architecture.offset, architecture.size)) {
                            return open_result::invalid_container;
                        }

                        if constexpr (type == type::none) {
                            container_open_result = container.open(stream);
                        }

                        if constexpr (type == type::library) {
                            container_open_result = container.open_from_library(stream, architecture.offset, architecture.size);
                        }

                        if constexpr (type == type::dynamic_library) {
                            container_open_result = container.open_from_dynamic_library(stream, architecture.offset, architecture.size);
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
                                if constexpr (type == type::library) {
                                    free(architectures);
                                    return open_result::not_a_library;
                                }

                                break;

                            case container::open_result::not_a_dynamic_library:
                                if constexpr (type == type::dynamic_library) {
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

                    if constexpr (type == type::none) {
                        container_open_result = container.open(stream);
                    }

                    if constexpr (type == type::library) {
                        container_open_result = container.open_from_library(stream);
                    }

                    if constexpr (type == type::dynamic_library) {
                        container_open_result = container.open_from_dynamic_library(stream);
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
                            if constexpr (type == type::library) {
                                return open_result::not_a_library;
                            }

                            break;

                        case container::open_result::not_a_dynamic_library:
                            if constexpr (type == type::dynamic_library) {
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
    };
}
