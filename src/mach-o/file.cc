//
//  src/mach-o/file.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include <cerrno>

#include "../utils/range.h"
#include "file.h"

namespace macho {
    file::open_result file::open(const char *path) noexcept {
        const auto result = stream.open(path, "r");
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return this->load_containers();
    }

    file::open_result file::open(const char *path, const char *mode) noexcept {
        const auto result = stream.open(path, mode);
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return this->load_containers();
    }

    file::open_result file::open(FILE *file) noexcept {
        const auto result = stream.open(file);
        if (result != stream::file::shared::open_result::ok) {
            return open_result::failed_to_open_stream;
        }

        return this->load_containers();
    }

    file::open_result file::open_copy(const file &file, const char *mode) noexcept {
        this->stream.open_copy(file.stream, mode);

        this->magic = file.magic;
        this->containers = file.containers;

        return open_result::ok;
    }

    file::open_result file::load_containers() noexcept {
        const auto stream_size = this->stream.size();
        if (stream_size < sizeof(header)) {
            return open_result::not_a_macho;
        }

        if (!this->stream.read(&this->magic, sizeof(this->magic))) {
            return open_result::stream_read_error;
        }

        const auto magic_is_fat = macho::magic_is_fat(this->magic);
        if (magic_is_fat) {
            auto nfat_arch = uint32_t();
            if (!this->stream.read(&nfat_arch, sizeof(nfat_arch))) {
                return open_result::stream_read_error;
            }

            if (nfat_arch == 0) {
                return open_result::zero_containers;
            }

            const auto magic_is_big_endian = macho::magic_is_big_endian(this->magic);
            if (magic_is_big_endian) {
                ::utils::swap::uint32(nfat_arch);
            }

            const auto magic_is_fat_64 = macho::magic_is_fat_64(this->magic);
            if (magic_is_fat_64) {
                const auto architectures_size = sizeof(architecture_64) * nfat_arch;
                const auto occupied_header_space = sizeof(fat) + architectures_size;

                if (occupied_header_space >= stream_size) {
                    return open_result::too_many_containers;
                }

                const auto architectures = new architecture_64[nfat_arch];
                if (!this->stream.read(architectures, architectures_size)) {
                    delete[] architectures;
                    return open_result::stream_read_error;
                }

                if (magic_is_big_endian) {
                    utils::swap::architectures_64(architectures, nfat_arch);
                }

                const auto architectures_end = &architectures[nfat_arch];
                const auto architectures_back = architectures_end - 1;

                // Validate first architecture outside of for-loop to avoid
                // checking twice more than necessary when in for loop

                if (architectures->offset < occupied_header_space) {
                    delete[] architectures;
                    return open_result::invalid_container;
                }

                if (!::utils::range::is_valid_size_based_range(architectures->offset, architectures->size)) {
                    delete[] architectures;
                    return open_result::invalid_container;
                }

                auto last_range = ::utils::range(architectures->offset, architectures->offset + architectures->size);
                if (last_range.is_or_goes_past_end(stream_size)) {
                    delete[] architectures;
                    return open_result::invalid_container;
                }

                for (auto iter = architectures; iter != architectures_back; iter++) {
                    for (auto jter = iter + 1; jter != architectures_end; jter++) {
                        if (!::utils::range::is_valid_size_based_range(jter->offset, jter->size)) {
                            delete[] architectures;
                            return open_result::invalid_container;
                        }

                        const auto jter_range = ::utils::range(jter->offset, jter->offset + jter->size);

                        if (jter_range.is_or_goes_past_end(stream_size)) {
                            delete[] architectures;
                            return open_result::invalid_container;
                        }

                        if (last_range.overlaps_with_range(jter_range)) {
                            delete[] architectures;
                            return open_result::overlapping_containers;
                        }

                        last_range = jter_range;
                    }
                }

                this->containers.reserve(nfat_arch);

                for (auto iter = architectures; iter != architectures_end; iter++) {
                    auto container = macho::container();
                    switch (container.open(stream, iter->offset, iter->size)) {
                        case container::open_result::ok:
                            break;

                        case container::open_result::stream_seek_error:
                            delete[] architectures;
                            return open_result::stream_seek_error;

                        case container::open_result::stream_read_error:
                            delete[] architectures;
                            return open_result::stream_read_error;

                        case container::open_result::fat_container:
                        case container::open_result::not_a_macho:
                        case container::open_result::invalid_macho:
                            delete[] architectures;
                            return open_result::invalid_container;
                    }

                    this->containers.emplace_back(std::move(container));
                }

                delete[] architectures;
            } else {
                const auto architectures_size = sizeof(architecture) * nfat_arch;
                const auto occupied_header_space = sizeof(fat) + architectures_size;

                if (occupied_header_space >= stream_size) {
                    return open_result::too_many_containers;
                }

                const auto architectures = new architecture[nfat_arch];
                if (!this->stream.read(architectures, architectures_size)) {
                    delete[] architectures;
                    return open_result::stream_read_error;
                }

                if (magic_is_big_endian) {
                    utils::swap::architectures(architectures, nfat_arch);
                }

                const auto architectures_end = &architectures[nfat_arch];
                const auto architectures_back = architectures_end - 1;

                // Validate first architecture outside of for-loop to avoid
                // checking twice more than necessary when in for loop

                if (architectures->offset < occupied_header_space) {
                    delete[] architectures;
                    return open_result::invalid_container;
                }

                if (!::utils::range::is_valid_size_based_range(architectures->offset, architectures->size)) {
                    delete[] architectures;
                    return open_result::invalid_container;
                }

                auto last_range = ::utils::range(architectures->offset, architectures->offset + architectures->size);
                if (last_range.is_or_goes_past_end(stream_size)) {
                    delete[] architectures;
                    return open_result::invalid_container;
                }

                for (auto iter = architectures; iter != architectures_back; iter++) {
                    for (auto jter = iter + 1; jter != architectures_end; jter++) {
                        if (!::utils::range::is_valid_size_based_range(jter->offset, jter->size)) {
                            delete[] architectures;
                            return open_result::invalid_container;
                        }

                        const auto jter_range = ::utils::range(jter->offset, jter->offset + jter->size);

                        if (jter_range.is_or_goes_past_end(stream_size)) {
                            delete[] architectures;
                            return open_result::invalid_container;
                        }

                        if (last_range.overlaps_with_range(jter_range)) {
                            delete[] architectures;
                            return open_result::overlapping_containers;
                        }

                        last_range = jter_range;
                    }
                }

                this->containers.reserve(nfat_arch);

                for (auto iter = architectures; iter != architectures_end; iter++) {
                    auto container = macho::container();
                    switch (container.open(this->stream, iter->offset, iter->size)) {
                        case container::open_result::ok:
                            break;

                        case container::open_result::stream_seek_error:
                            delete[] architectures;
                            return open_result::stream_seek_error;

                        case container::open_result::stream_read_error:
                            delete[] architectures;
                            return open_result::stream_read_error;

                        case container::open_result::fat_container:
                        case container::open_result::not_a_macho:
                        case container::open_result::invalid_macho:
                            delete[] architectures;
                            return open_result::invalid_container;
                    }

                    this->containers.emplace_back(std::move(container));
                }

                delete[] architectures;
            }
        } else {
            const auto magic_is_thin = macho::magic_is_thin(this->magic);
            if (magic_is_thin) {
                auto container = macho::container();
                switch (container.open(this->stream, 0, stream_size)) {
                    case container::open_result::ok:
                        break;

                    case container::open_result::stream_seek_error:
                        return open_result::stream_seek_error;

                    case container::open_result::stream_read_error:
                        return open_result::stream_read_error;

                    case container::open_result::fat_container:
                    case container::open_result::not_a_macho:
                    case container::open_result::invalid_macho:
                        // return invalid_macho as the whole
                        // file is supposed to be a container

                        return open_result::invalid_macho;
                }

                this->containers.emplace_back(std::move(container));
            } else {
                return open_result::not_a_macho;
            }
        }

        return open_result::ok;
    }
}
