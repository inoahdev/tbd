//
//  main.cc
//  tbd
//
//  Created by inoahdev on 4/16/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/arch.h>
#include <mach-o/loader.h>
#include <mach-o/fat.h>
#include <mach-o/swap.h>

#include <sys/stat.h>

#include <algorithm>
#include <cerrno>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>

inline auto &swap_value(uint32_t *value_ptr) noexcept {
    auto &value = *value_ptr;

    value = ((value >> 8) & 0x00ff00ff) | ((value << 8) & 0xff00ff00);
    value = ((value >> 16) & 0x0000ffff) | ((value << 16) & 0xffff0000);

    return value;
}

class macho_container {
public:
    explicit macho_container(FILE *file, const long &macho_base)
    : file_(file), macho_base_(macho_base), is_architecture_(false) {
        const auto position = ftell(file);
        fseek(file, 0, SEEK_END);

        this->size_ = ftell(file);

        fseek(file, position, SEEK_SET);
        this->validate();
    }

    explicit macho_container(FILE *file, const long &macho_base, const struct fat_arch &architecture)
    : file_(file), macho_base_(macho_base), base_(architecture.offset), size_(architecture.size), is_architecture_(true) {
        this->validate();
    }

    explicit macho_container(FILE *file, const long &macho_base, const struct fat_arch_64 &architecture)
    : file_(file), macho_base_(macho_base), base_(architecture.offset), size_(architecture.size), is_architecture_(true) {
        this->validate();
    }

    ~macho_container() {
        if (cached_) {
            delete cached_;
        }

        if (string_table_) {
            delete[] string_table_;
        }
    }

    auto iterate_load_commands(const std::function<bool(const struct load_command *load_cmd)> &callback) noexcept {
        const auto &ncmds = header_.ncmds;
        const auto &sizeofcmds = header_.sizeofcmds;

        if (!cached_) {
            cached_ = new char[header_.sizeofcmds];

            auto base = this->base() + sizeof(struct mach_header);
            if (this->is_64_bit()) {
                base += sizeof(uint32_t);
            }

            const auto position = ftell(file_);

            fseek(file_, base, SEEK_SET);
            fread(cached_, header_.sizeofcmds, 1, file_);

            fseek(file_, position, SEEK_SET);
        }

        auto size_used = 0;
        auto index = 0;

        for (auto i = 0; i < ncmds; i++) {
            auto load_cmd = (struct load_command *)&((char *)cached_)[index];
            if (should_swap_ && !swapped_cache) {
                swap_load_command(load_cmd, NX_LittleEndian);
            }

            const auto &cmdsize = load_cmd->cmdsize;
            if (cmdsize < sizeof(struct load_command)) {
                if (is_architecture_) {
                    fprintf(stderr, "Load-command (at index %d) of architecture is too small to be valid", i);
                } else {
                    fprintf(stderr, "Load-command (at index %d) of mach-o file is too small to be valid", i);
                }

                exit(1);
            }

            if (cmdsize >= sizeofcmds) {
                if (is_architecture_) {
                    fprintf(stderr, "Load-command (at index %d) of architecture is larger than/or equal to entire area allocated for load-commands", i);
                } else {
                    fprintf(stderr, "Load-command (at index %d) of mach-o file is larger than/or equal to entire area allocated for load-commands", i);
                }

                exit(1);
            }

            size_used += cmdsize;
            if (size_used > sizeofcmds) {
                if (is_architecture_) {
                    fprintf(stderr, "Load-command (at index %d) of architecture goes past end of area allocated for load-commands", i);
                } else {
                    fprintf(stderr, "Load-command (at index %d) of mach-o file goes past end of area allocated for load-commands", i);
                }

                exit(1);
            } else if (size_used == sizeofcmds && i != ncmds - 1) {
                if (is_architecture_) {
                    fprintf(stderr, "Load-command (at index %d) of architecture takes up all of the remaining space allocated for load-commands", i);
                } else {
                    fprintf(stderr, "Load-command (at index %d) of mach-o file takes up all of the remaining space allocated for load-commands", i);
                }

                exit(1);
            }

            auto result = callback(load_cmd);
            if (!result) {
                break;
            }

            index += cmdsize;
        }
    }

    void iterate_symbols(const std::function<bool(const struct nlist_64 &, const char *)> &callback) noexcept {
        auto base = this->base();
        auto position = ftell(file_);

        struct symtab_command *symtab_command = nullptr;
        iterate_load_commands([&](const struct load_command *load_cmd) {
            if (load_cmd->cmd != LC_SYMTAB) {
                return true;
            }

            symtab_command = (struct symtab_command *)load_cmd;
            if (should_swap_) {
                swap_symtab_command(symtab_command, NX_LittleEndian);
            }

            return false;
        });

        if (!symtab_command) {
            if (is_architecture_) {
                fputs("Architecture does not have a symbol-table", stderr);
            } else {
                fputs("Mach-o file does not have a symbol-table", stderr);
            }

            exit(1);
        }

        const auto &string_table_offset = symtab_command->stroff;
        if (string_table_offset > size_) {
            if (is_architecture_) {
                fputs("Architecture has a string-table outside of its container", stderr);
            } else {
                fputs("Mach-o file has a string-table outside of its container", stderr);
            }

            exit(1);
        }

        const auto &string_table_size = symtab_command->strsize;
        if (string_table_offset + string_table_size > size_) {
            if (is_architecture_) {
                fputs("Architecture has a string-table that goes outside of its container", stderr);
            } else {
                fputs("Mach-o file has a string-table that goes outside of its container", stderr);
            }

            exit(1);
        }

        const auto &symbol_table_offset = symtab_command->symoff;
        if (symbol_table_offset > size_) {
            if (is_architecture_) {
                fputs("Architecture has a symbol-table that is outside of its container", stderr);
            } else {
                fputs("Mach-o file has a symbol-table that is outside of its container", stderr);
            }

            exit(1);
        }

        if (!string_table_) {
            string_table_ = new char[string_table_size];

            fseek(file_, base + string_table_offset, SEEK_SET);
            fread(string_table_, string_table_size, 1, file_);
        }

        const auto &symbol_table_count = symtab_command->nsyms;
        fseek(file_, base + symbol_table_offset, SEEK_SET);

        if (this->is_64_bit()) {
            const auto symbol_table_size = sizeof(struct nlist_64) * symbol_table_count;
            if (symbol_table_offset + symbol_table_size > size_) {
                if (is_architecture_) {
                    fputs("Architecture (at base 0x%.8lX) has a symbol-table that goes outside of its container", stderr);
                } else {
                    fputs("Mach-o file has a symbol-table that goes outside of its container", stderr);
                }

                exit(1);
            }

            const auto symbol_table = new struct nlist_64[symbol_table_count];
            fread(symbol_table, symbol_table_size, 1, file_);

            for (auto i = 0; i < symbol_table_count; i++) {
                const auto &symbol_table_entry = symbol_table[i];
                const auto &symbol_table_entry_string_table_index = symbol_table_entry.n_un.n_strx;

                if (should_swap_) {
                    swap_nlist_64(symbol_table, symbol_table_count, NX_LittleEndian);
                }

                if (symbol_table_entry_string_table_index > (string_table_size - 1)) {
                    fprintf(stderr, "Symbol-table entry (at index %d) has symbol-string past end of string-table", i);
                    exit(1);
                }

                const auto symbol_table_string_table_string = &string_table_[symbol_table_entry_string_table_index];
                const auto result = callback(symbol_table_entry, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }

            delete[] symbol_table;
        } else {
            const auto symbol_table_size = sizeof(struct nlist) * symbol_table_count;
            if (symbol_table_offset + symbol_table_size > size_) {
                if (is_architecture_) {
                    fputs("Architecture (at base 0x%.8lX) has a symbol-table that goes outside of its container", stderr);
                } else {
                    fputs("Mach-o file has a symbol-table that goes outside of its container", stderr);
                }

                exit(1);
            }

            const auto symbol_table = new struct nlist[symbol_table_count];
            fread(symbol_table, symbol_table_size, 1, file_);

            if (should_swap_) {
                swap_nlist(symbol_table, symbol_table_count, NX_LittleEndian);
            }

            for (auto i = 0; i < symbol_table_count; i++) {
                const auto &symbol_table_entry = symbol_table[i];
                const auto &symbol_table_entry_string_table_index = symbol_table_entry.n_un.n_strx;

                if (symbol_table_entry_string_table_index > (string_table_size - 1)) {
                    fprintf(stderr, "Symbol-table entry (at index %d) has symbol-string past end of string-table", i);
                    exit(1);
                }

                const struct nlist_64 symbol_table_entry_64 = { symbol_table_entry.n_un.n_strx, symbol_table_entry.n_type, symbol_table_entry.n_sect, (uint16_t)symbol_table_entry.n_desc, symbol_table_entry.n_value };

                const auto symbol_table_string_table_string = &string_table_[symbol_table_entry_string_table_index];
                const auto result = callback(symbol_table_entry_64, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }

            delete[] symbol_table;
        }

        fseek(file_, position, SEEK_SET);
    }

    inline uint32_t &swap_value(uint32_t &value) const noexcept {
        if (should_swap_) {
            ::swap_value(&value);
        }

        return value;
    }

    inline const FILE *file() const noexcept { return file_; }
    inline const struct mach_header &header() const noexcept { return header_; }

    inline const bool should_swap() const noexcept { return should_swap_; }

    inline const long base() const noexcept { return this->macho_base_ + this->base_; }
    inline const long size() const noexcept { return this->size_; }

    inline const bool is_32_bit() const noexcept { return header_.magic == MH_MAGIC || header_.magic == MH_CIGAM; }
    inline const bool is_64_bit() const noexcept { return header_.magic == MH_MAGIC_64 || header_.magic == MH_CIGAM_64; }

private:
    FILE *file_;
    long macho_base_;

    long base_ = 0;
    long size_ = 0;

    struct mach_header header_;

    bool is_architecture_;
    bool should_swap_ = false;

    char *cached_ = nullptr;
    bool swapped_cache = false;

    char *string_table_ = nullptr;

    void validate() {
        auto base = this->base();
        auto position = ftell(file_);

        auto &magic = header_.magic;

        fseek(file_, base_, SEEK_SET);
        fread(&magic, sizeof(uint32_t), 1, file_);

        if (magic == MH_MAGIC || magic == MH_CIGAM || magic == MH_MAGIC_64 || magic == MH_CIGAM_64) {
            fread(&header_.cputype, sizeof(struct mach_header) - sizeof(uint32_t), 1, file_);
            if (magic == MH_CIGAM || magic == MH_CIGAM_64) {
                should_swap_ = true;
                swap_mach_header(&header_, NX_LittleEndian);
            }
        } else if (magic == FAT_MAGIC || magic == FAT_CIGAM || magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64) {
            fprintf(stderr, "Architecture at offset (%ld) cannot be a fat mach-o file itself", base);
            exit(1);
        } else {
            fprintf(stderr, "Architecture at offset (%ld) is not a valid mach-o base", base);
            exit(1);
        }

        fseek(file_, position, SEEK_SET);
    }
};

class macho_file {
public:
    explicit macho_file(const std::string &path)
    : file_(fopen(path.data(), "r")) {
        if (!file_) {
            fprintf(stderr, "Unable to open mach-o file at path (%s)\n", path.data());
            exit(1);
        }

        this->validate();
    }

    ~macho_file() {
        fclose(file_);
    }

    static bool is_valid_file(const std::string &path) noexcept {
        auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            return false;
        }

        uint32_t magic;
        read(descriptor, &magic, sizeof(uint32_t));

        auto result = magic == MH_MAGIC || magic == MH_CIGAM || magic == MH_MAGIC_64 || magic == MH_CIGAM_64 || magic == FAT_MAGIC || magic == FAT_CIGAM || magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64;

        close(descriptor);
        return result;
    }

    static bool is_valid_library(const std::string &path) noexcept {
        auto descriptor = open(path.data(), O_RDONLY);
        if (descriptor == -1) {
            return false;
        }

        uint32_t magic;
        read(descriptor, &magic, sizeof(uint32_t));

        auto has_library_command = [&](uint64_t base, const struct mach_header &header) {
            auto should_swap = false;
            if (header.magic == MH_MAGIC_64 || header.magic == MH_CIGAM_64) {
                lseek(descriptor, sizeof(uint32_t), SEEK_CUR);

            }

            if (header.magic == MH_CIGAM_64 || header.magic == MH_CIGAM) {
                should_swap = true;
            }

            const auto load_commands = new char[header.sizeofcmds];
            read(descriptor, load_commands, header.sizeofcmds);

            auto ncmds = header.ncmds;
            auto index = 0;

            auto size_left = header.sizeofcmds;

            for (auto i = 0; i < ncmds; i++) {
                const auto load_cmd = (struct load_command *)&load_commands[index];
                if (should_swap) {
                    swap_load_command(load_cmd, NX_LittleEndian);
                }

                if (load_cmd->cmd == LC_ID_DYLIB) {
                    delete[] load_commands;
                    return true;
                }

                auto cmdsize = load_cmd->cmdsize;
                if (cmdsize < sizeof(struct load_command)) {
                    fprintf(stderr, "Load-command (%d) is too small to be valid", i);
                    exit(1);
                } else if (cmdsize > size_left) {
                    fprintf(stderr, "Load-command (%d) is larger than remaining space left for load commands", i);
                    exit(1);
                } else if (cmdsize == size_left && i != ncmds - 1) {
                    fprintf(stderr, "Load-command (%d) takes up all the remaining space left for load commands", i);
                    exit(1);
                }

                index += cmdsize;
            }

            return false;
        };

        if (magic == FAT_MAGIC_64 || magic == FAT_CIGAM_64) {
            uint32_t nfat_arch;
            read(descriptor, &nfat_arch, sizeof(uint32_t));

            if (magic == FAT_CIGAM_64) {
                swap_value(&nfat_arch);
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
        } else if (magic == FAT_MAGIC || magic == FAT_CIGAM) {
            uint32_t nfat_arch;
            read(descriptor, &nfat_arch, sizeof(uint32_t));

            if (magic == FAT_CIGAM) {
                swap_value(&nfat_arch);
            }

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

    inline const FILE *file() const noexcept { return file_; }
    inline std::vector<macho_container> &containers() noexcept { return containers_; }

    inline const bool is_thin() const noexcept { return magic_ == MH_MAGIC || magic_ == MH_CIGAM || magic_ == MH_MAGIC_64 || magic_ == MH_CIGAM_64; }
    inline const bool is_fat() const noexcept { return magic_ == FAT_MAGIC || magic_ == FAT_CIGAM || magic_ == FAT_MAGIC_64 || magic_ == FAT_CIGAM_64; }

private:
    FILE *file_;
    uint32_t magic_;

    std::vector<macho_container> containers_ = std::vector<macho_container>();

    void validate() {
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
                swap_value(&nfat_arch);
            }

            containers_.reserve(nfat_arch);
            if (magic_ == FAT_MAGIC_64 || magic_ == FAT_CIGAM_64) {
                const auto architectures = new struct fat_arch_64[nfat_arch];
                fread(architectures, sizeof(struct fat_arch) * nfat_arch, 1, file_);

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
};

class symbol {
public:
    explicit symbol(const std::string &string, bool weak) noexcept
    : string_(string), weak_(weak) {}

    void add_architecture_info(const NXArchInfo *architecture_info) noexcept {
        architecture_infos_.emplace_back(architecture_info);
    }

    inline const bool weak() const noexcept { return weak_; }

    inline const std::string &string() const noexcept { return string_; }
    inline const std::vector<const NXArchInfo *> architecture_infos() const noexcept { return architecture_infos_; }

    inline const bool operator==(const std::string &string) const noexcept { return string_ == string; }
    inline const bool operator==(const symbol &symbol) const noexcept { return string_ == symbol.string_; }

private:
    std::string string_;
    std::vector<const NXArchInfo *> architecture_infos_;

    bool weak_;
};

class group {
public:
    explicit group(const std::vector<const NXArchInfo *> &architecture_infos) noexcept
    : architecture_infos_(architecture_infos) {}

    void add_symbol(const symbol &symbol) noexcept {
        auto &symbols = this->symbols_;
        if (std::find(symbols_.begin(), symbols_.end(), symbol) == symbols_.end()) {
            symbols.emplace_back(symbol);
        }
    }

    void add_reexport(const symbol &reexport) noexcept {
        auto &reexports = this->reexports_;
        if (std::find(reexports_.begin(), reexports_.end(), reexport) == reexports_.end()) {
            reexports.emplace_back(reexport);
        }
    }

    inline const std::vector<const NXArchInfo *> &architecture_infos() const noexcept { return architecture_infos_; }

    inline const std::vector<symbol> &symbols() const noexcept { return symbols_; }
    inline const std::vector<symbol> &reexports() const noexcept { return reexports_; }

    inline const bool operator==(const std::vector<const NXArchInfo *> &architecture_infos) const noexcept { return architecture_infos_ == architecture_infos; }

private:
    std::vector<const NXArchInfo *> architecture_infos_;

    std::vector<symbol> symbols_;
    std::vector<symbol> reexports_;
};

class tbd {
public:
    enum platform {
        ios,
        macos,
        watchos,
        tvos
    };

    static const char *platform_to_string(const platform &platform) noexcept {
        switch (platform) {
            case ios:
                return "ios";
                break;

            case macos:
                return "macos";
                break;

            case watchos:
                return "watchos";
                break;

            case tvos:
                return "tvos";
                break;

            default:
                return nullptr;
        }
    }

    static platform string_to_platform(const char *platform) noexcept {
        if (strcmp(platform, "ios") == 0) {
            return platform::ios;
        } else if (strcmp(platform, "macos") == 0) {
            return platform::macos;
        } else if (strcmp(platform, "watchos") == 0) {
            return platform::watchos;
        } else if (strcmp(platform, "tvos") == 0) {
            return platform::tvos;
        }

        return (enum platform)-1;
    }

    enum class version {
        v1 = 1,
        v2
    };

    static version string_to_version(const char *version) noexcept {
        if (strcmp(version, "v1") == 0) {
            return version::v1;
        } else if (strcmp(version, "v2") == 0) {
            return version::v2;
        }

        return (enum version)-1;
    }

    explicit tbd() = default;
    explicit tbd(const std::vector<std::string> &macho_files, const std::vector<std::string> &output_files, const platform &platform, const version &version, const std::vector<const NXArchInfo *> &architecture_overrides = std::vector<const NXArchInfo *>())
    : macho_files_(macho_files), output_files_(output_files), platform_(platform), version_(version), architectures_(architecture_overrides) {
        this->validate();
    }

    void execute() {
        auto output_path_index = 0;
        for (const auto &macho_file_path : macho_files_) {
            auto output_file = stdout;
            if (output_path_index < output_files_.size()) {
                const auto &output_path_string = output_files_.at(output_path_index);
                if (output_path_string != "stdout") {
                    const auto output_path_string_data = output_path_string.data();

                    output_file = fopen(output_path_string_data, "w");
                    if (!output_file) {
                        fprintf(stderr, "Failed to open file at path (%s) for writing, failing with error (%s)\n", output_path_string_data, strerror(errno));
                        exit(1);
                    }
                }
            }

            auto macho_file_object = macho_file(macho_file_path);
            auto macho_file_has_architecture_overrides = architectures_.size() != 0;

            auto current_version = std::string();
            auto compatibility_version = std::string();

            auto symbols = std::vector<symbol>();
            auto reexports = std::vector<symbol>();

            auto installation_name = std::string();
            auto uuids = std::vector<std::string>();

            auto &macho_file_containers = macho_file_object.containers();
            auto macho_container_counter = 0;

            const auto macho_file_containers_size = macho_file_containers.size();
            const auto macho_file_is_fat = macho_file_containers_size != 0;

            for (auto &macho_container : macho_file_containers) {
                const auto &macho_container_header = macho_container.header();
                const auto macho_container_should_swap = macho_container.should_swap();

                const auto macho_container_architecture_info = NXGetArchInfoFromCpuType(macho_container_header.cputype, macho_container_header.cpusubtype);
                if (!macho_container_architecture_info) {
                    if (macho_file_is_fat) {
                        fprintf(stderr, "Architecture (#%d) is not of a recognizable cputype", macho_container_counter);
                    } else {
                        fputs("Mach-o file is not of a recognizable cputype", stderr);
                    }

                    exit(1);
                }

                if (!macho_file_has_architecture_overrides) {
                    architectures_.emplace_back(macho_container_architecture_info);
                }

                auto local_current_version = std::string();
                auto local_compatibility_version = std::string();
                auto local_installation_name = std::string();

                auto added_uuid = false;

                macho_container.iterate_load_commands([&](const struct load_command *load_cmd) {
                    switch (load_cmd->cmd) {
                        case LC_ID_DYLIB: {
                            if (local_installation_name.size() != 0) {
                                if (macho_file_is_fat) {
                                    fprintf(stderr, "Architecture (#%d) has two library-identification load-commands\n", macho_container_counter);
                                } else {
                                    fputs("Mach-o file has two library-identification load-commands\n", stderr);
                                }

                                exit(1);
                            }

                            auto id_dylib_command = (struct dylib_command *)load_cmd;
                            if (macho_container_should_swap) {
                                swap_dylib_command(id_dylib_command, NX_LittleEndian);
                            }

                            const auto &id_dylib = id_dylib_command->dylib;
                            const auto &id_dylib_installation_name_string_index = id_dylib.name.offset;

                            if (id_dylib_installation_name_string_index > load_cmd->cmdsize) {
                                fputs("Library identification load-command has an invalid identification-string position\n", stderr);
                                exit(1);
                            }

                            const auto &id_dylib_installation_name_string = &((char *)load_cmd)[id_dylib_installation_name_string_index];
                            local_installation_name = id_dylib_installation_name_string;

                            char id_dylib_current_version_string_data[33];
                            sprintf(id_dylib_current_version_string_data, "%u.%u.%u", id_dylib.current_version >> 16, (id_dylib.current_version >> 8) & 0xff, id_dylib.current_version & 0xff);

                            char id_dylib_compatibility_version_string_data[33];
                            sprintf(id_dylib_compatibility_version_string_data, "%u.%u.%u", id_dylib.compatibility_version >> 16, (id_dylib.compatibility_version >> 8) & 0xff, id_dylib.compatibility_version & 0xff);

                            local_current_version = id_dylib_current_version_string_data;
                            local_compatibility_version = id_dylib_compatibility_version_string_data;

                            break;
                        }

                        case LC_REEXPORT_DYLIB: {
                            auto reexport_dylib_command = (struct dylib_command *)load_cmd;
                            if (macho_container_should_swap) {
                                swap_dylib_command(reexport_dylib_command, NX_LittleEndian);
                            }

                            const auto &reexport_dylib = reexport_dylib_command->dylib;
                            const auto &reexport_dylib_string_index = reexport_dylib.name.offset;

                            if (reexport_dylib_string_index > load_cmd->cmdsize) {
                                fputs("Re-export dylib load-command has an invalid identification-string position\n", stderr);
                                exit(1);
                            }

                            const auto &reexport_dylib_string = &((char *)load_cmd)[reexport_dylib_string_index];
                            const auto reexports_iter = std::find(reexports.begin(), reexports.end(), reexport_dylib_string);

                            if (reexports_iter != reexports.end()) {
                                reexports_iter->add_architecture_info(macho_container_architecture_info);
                            } else {
                                reexports.emplace_back(reexport_dylib_string, false);
                            }

                            break;
                        }

                        case LC_UUID: {
                            const auto &uuid = ((struct uuid_command *)load_cmd)->uuid;

                            char uuid_string[33] = {};
                            sprintf(uuid_string, "%.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X", uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);

                            const auto uuids_iter = std::find(uuids.begin(), uuids.end(), uuid_string);
                            if (uuids_iter != uuids.end()) {
                                fprintf(stderr, "uuid-string (%s) is found in multiple architectures", uuid_string);
                                exit(1);
                            }

                            uuids.emplace_back(uuid_string);
                            added_uuid = true;

                            break;
                        }
                    }

                    return true;
                });

                if (local_installation_name.empty()) {
                    fputs("Mach-o file is not a library or framework\n", stderr);
                    exit(1);
                }

                if (installation_name.size() && installation_name != local_installation_name) {
                    fprintf(stderr, "Mach-o file has conflicting installation-names (%s vs %s from %s)\n", installation_name.data(), local_installation_name.data(), macho_container_architecture_info->name);
                    exit(1);
                } else if (installation_name.empty()) {
                    installation_name = local_installation_name;
                }

                if (current_version.size() && current_version != local_current_version) {
                    fprintf(stderr, "Mach-o file has conflicting current_version (%s vs %s from %s)\n", current_version.data(), local_current_version.data(), macho_container_architecture_info->name);
                    exit(1);
                } else if (current_version.empty()) {
                    current_version = local_current_version;
                }

                if (compatibility_version.size() && compatibility_version != local_compatibility_version) {
                    fprintf(stderr, "Mach-o file has conflicting compatibility-version (%s vs %s from %s)\n", compatibility_version.data(), local_compatibility_version.data(), macho_container_architecture_info->name);
                    exit(1);
                } else if (compatibility_version.empty()) {
                    compatibility_version = local_compatibility_version;
                }

                if (!added_uuid && version_ == version::v2) {
                    fprintf(stderr, "Macho-file with architecture (%s) does not have a uuid command\n", macho_container_architecture_info->name);
                    exit(1);
                }

                macho_container.iterate_symbols([&](const struct nlist_64 &symbol_table_entry, const char *symbol_string) {
                    const auto &symbol_table_entry_type = symbol_table_entry.n_type;
                    if ((symbol_table_entry_type & N_TYPE) != N_SECT || (symbol_table_entry_type & N_EXT) != N_EXT) {
                        return true;
                    }

                    const auto symbols_iter = std::find(symbols.begin(), symbols.end(), symbol_string);
                    if (symbols_iter != symbols.end()) {
                        symbols_iter->add_architecture_info(macho_container_architecture_info);
                    } else {
                        symbols.emplace_back(symbol_string, symbol_table_entry.n_desc & N_WEAK_DEF);
                        symbols.back().add_architecture_info(macho_container_architecture_info);
                    }

                    return true;
                });

                macho_container_counter++;
            }

            auto groups = std::vector<group>();
            if (macho_file_has_architecture_overrides) {
                groups.emplace_back(architectures_);

                auto &group = groups.front();
                for (const auto &reexport : reexports) {
                    group.add_reexport(reexport);
                }

                for (const auto &symbol : symbols) {
                    group.add_symbol(symbol);
                }
            } else {
                for (const auto &reexport : reexports) {
                    const auto &reexport_architecture_infos = reexport.architecture_infos();
                    const auto group_iter = std::find(groups.begin(), groups.end(), reexport_architecture_infos);

                    if (group_iter != groups.end()) {
                        group_iter->add_reexport(reexport);
                    } else {
                        groups.emplace_back(reexport_architecture_infos);
                        groups.back().add_reexport(reexport);
                    }
                }

                for (const auto &symbol : symbols) {
                    const auto &symbol_architecture_infos = symbol.architecture_infos();
                    const auto group_iter = std::find(groups.begin(), groups.end(), symbol_architecture_infos);

                    if (group_iter != groups.end()) {
                        group_iter->add_symbol(symbol);
                    } else {
                        groups.emplace_back(symbol_architecture_infos);
                        groups.back().add_symbol(symbol);
                    }
                }
            }

            std::sort(groups.begin(), groups.end(), [](const group &lhs, const group &rhs) {
                const auto &lhs_symbols = lhs.symbols();
                const auto lhs_symbols_size = lhs_symbols.size();

                const auto &rhs_symbols = rhs.symbols();
                const auto rhs_symbols_size = rhs_symbols.size();

                return lhs_symbols_size < rhs_symbols_size;
            });

            fputs("---", output_file);
            if (version_ == version::v2) {
                fputs(" !tapi-tbd-v2", output_file);
            }

            fprintf(output_file, "\narchs:%-17s[ ", "");

            auto architectures_begin = architectures_.begin();
            fprintf(output_file, "%s", (*architectures_begin)->name);

            for (architectures_begin++; architectures_begin != architectures_.end(); architectures_begin++) {
                fprintf(output_file, ", %s", (*architectures_begin)->name);
            }

            fputs(" ]\n", output_file);

            if (version_ == version::v2) {
                fprintf(output_file, "uuids:%-17s[ ", "");

                auto counter = 1;
                auto uuids_begin = uuids.begin();

                for (auto architectures_begin = architectures_.begin(); architectures_begin < architectures_.end(); architectures_begin++, uuids_begin++) {
                    fprintf(output_file, "'%s: %s'", (*architectures_begin)->name, uuids_begin->data());

                    if (architectures_begin != architectures_.end() - 1) {
                        fputs(", ", output_file);

                        if (counter % 2 == 0) {
                            fprintf(output_file, "%-27s", "\n");
                        }

                        counter++;
                    }
                }

                fputs(" ]\n", output_file);
            }

            fprintf(output_file, "platform:%-14s%s\n", "", platform_to_string(platform_));
            fprintf(output_file, "install-name:%-10s%s\n", "", installation_name.data());

            fprintf(output_file, "current-version:%-7s%s\n", "", current_version.data());
            fprintf(output_file, "compatibility-version: %s\n", compatibility_version.data());

            if (version_ == version::v2) {
                fprintf(output_file, "objc-constraint:%-7snone\n", "");
            }

            fputs("exports:\n", output_file);
            for (auto &group : groups) {
                auto ordinary_symbols = std::vector<std::string>();
                auto weak_symbols = std::vector<std::string>();
                auto objc_classes = std::vector<std::string>();
                auto objc_ivars = std::vector<std::string>();
                auto reexports = std::vector<std::string>();

                for (const auto &reexport : group.reexports()) {
                    reexports.emplace_back(reexport.string());
                }

                for (const auto &symbol : group.symbols()) {
                    auto &symbol_string = const_cast<std::string &>(symbol.string());
                    auto symbol_string_begin_pos = 0;

                    if (symbol_string.compare(0, 3, "$ld") == 0) {
                        symbol_string.insert(symbol_string.begin(), '\'');
                        symbol_string.append(1, '\'');

                        symbol_string_begin_pos += 4;
                    }

                    if (symbol_string.compare(symbol_string_begin_pos, 13, "_OBJC_CLASS_$") == 0) {
                        symbol_string.erase(0, 13);
                        objc_classes.emplace_back(symbol_string);
                    } else if (symbol_string.compare(symbol_string_begin_pos, 17, "_OBJC_METACLASS_$") == 0) {
                        symbol_string.erase(0, 17);
                        objc_classes.emplace_back(symbol_string);
                    } else if (symbol_string.compare(symbol_string_begin_pos, 12, "_OBJC_IVAR_$") == 0) {
                        symbol_string.erase(0, 12);
                        objc_ivars.emplace_back(symbol_string);
                    } else if (symbol.weak()) {
                        weak_symbols.emplace_back(symbol_string);
                    } else {
                        ordinary_symbols.emplace_back(symbol_string);
                    }
                }

                std::sort(ordinary_symbols.begin(), ordinary_symbols.end());
                std::sort(weak_symbols.begin(), weak_symbols.end());
                std::sort(objc_classes.begin(), objc_classes.end());
                std::sort(objc_ivars.begin(), objc_ivars.end());
                std::sort(reexports.begin(), reexports.end());

                ordinary_symbols.erase(std::unique(ordinary_symbols.begin(), ordinary_symbols.end()), ordinary_symbols.end());
                weak_symbols.erase(std::unique(weak_symbols.begin(), weak_symbols.end()), weak_symbols.end());
                objc_classes.erase(std::unique(objc_classes.begin(), objc_classes.end()), objc_classes.end());
                objc_ivars.erase(std::unique(objc_ivars.begin(), objc_ivars.end()), objc_ivars.end());
                reexports.erase(std::unique(reexports.begin(), reexports.end()), reexports.end());

                const auto &group_architecture_infos = group.architecture_infos();
                auto group_architecture_infos_begin = group_architecture_infos.begin();

                fprintf(output_file, "  - archs:%-12s[ %s", "", (*group_architecture_infos_begin)->name);
                for (group_architecture_infos_begin++; group_architecture_infos_begin < group_architecture_infos.end(); group_architecture_infos_begin++) {
                    fprintf(output_file, ", %s", (*group_architecture_infos_begin)->name);
                }

                fputs(" ]\n", output_file);

                const auto line_length_max = 85;
                const auto key_spacing_length_max = 18;

                auto character_count = 0;
                const auto print_symbols = [&](const std::vector<std::string> &symbols, const char *key) {
                    if (symbols.empty()) {
                        return;
                    }

                    fprintf(output_file, "%-4s%s:", "", key);

                    auto symbols_begin = symbols.begin();
                    auto symbols_end = symbols.end();

                    for (auto i = strlen(key) + 1; i < key_spacing_length_max; i++) {
                        fputs(" ", output_file);
                    }

                    fprintf(output_file, "[ %s", symbols_begin->data());
                    character_count += symbols_begin->length();

                    for (symbols_begin++; symbols_begin != symbols_end; symbols_begin++) {
                        const auto &symbol = *symbols_begin;
                        auto line_length = symbol.length() + 1;

                        if (character_count >= line_length_max || (character_count != 0 && character_count + line_length > line_length_max)) {
                            fprintf(output_file, ",\n%-24s", "");
                            character_count = 0;
                        } else {
                            fputs(", ", output_file);
                            line_length++;
                        }

                        fputs(symbol.data(), output_file);
                        character_count += line_length;
                    }

                    fprintf(output_file, " ]\n");
                    character_count = 0;
                };

                print_symbols(reexports, "re-exports");
                print_symbols(ordinary_symbols, "symbols");
                print_symbols(weak_symbols, "weak-def-symbols");
                print_symbols(objc_classes, "objc-classes");
                print_symbols(objc_ivars, "objc-ivars");
            }

            fputs("...\n", output_file);

            output_path_index++;
            if (output_file != stdout) {
                fclose(output_file);
            }
         }
    }

    inline const std::vector<std::string> &macho_files() const noexcept { return macho_files_; }
    inline const std::vector<std::string> &output_files() const noexcept { return output_files_; }

    inline void add_macho_file(const std::string &macho_file) noexcept {
        macho_files_.emplace_back(macho_file);
    }

    inline void add_output_file(const std::string &output_files) noexcept {
        output_files_.emplace_back(output_files);
    }

    inline void set_architectures(const std::vector<const NXArchInfo *> &architectures) noexcept {
        architectures_ = architectures;
    }

    inline void set_platform(const platform &platform) noexcept {
        platform_ = platform;
    }

    inline void set_version(const version &version) noexcept {
        version_ = version;
    }

    inline const platform &platform() const noexcept { return platform_; }
    inline const version &version() const noexcept { return version_; }

private:
    std::vector<std::string> macho_files_;
    std::vector<std::string> output_files_;

    enum platform platform_;
    enum version version_;

    std::vector<const NXArchInfo *> architectures_;

    void validate() const {
        if (version_ == version::v2 && architectures_.size() != 0) {
            fputs("Cannot use custom architectures for tbd version v2. Specify version v1 to be able to do so\n", stderr);
            exit(1);
        }
    }
};

void print_usage() {
    fputs("Usage: tbd [-p file-paths] [-v/--version v2] [-a/--archs architectures] [-o/-output output-paths-or-stdout]\n", stdout);
    fputs("Main options:\n", stdout);
    fputs("    -a, --archs,    Specify Architecture(s) to use, instead of the ones in the provieded mach-o file(s)\n", stdout);
    fputs("    -h, --help,     Print this message\n", stdout);
    fputs("    -o, --output,   Path(s) to output file(s) to write converted .tbd. If provided file(s) already exists, contents will get overrided. Can also provide \"stdout\" to print to stdout\n", stdout);
    fputs("    -p, --path,     Path(s) to mach-o file(s) to convert to a .tbd\n", stdout);
    fputs("    -u, --usage,    Print this message\n", stdout);
    fputs("    -v, --version,  Set version of tbd to convert to (default is v2)\n", stdout);

    fputs("\n", stdout);
    fputs("Extra options:\n", stdout);
    fputs("        --platform, Specify platform for all mach-o files provided\n", stdout);
    fputs("    -r, --recurse,  Specify directory to recurse and find mach-o files in. Use in conjunction with -p (ex. -p -r /path/to/directory)\n", stdout);
    fputs("        --versions, Print a list of all valid tbd-versions\n", stdout);

    exit(0);
}

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fputs("Please run -h or -u to see a list of options\n", stderr);
        return 1;
    }

    auto architectures = std::vector<const NXArchInfo *>();
    auto tbds = std::vector<tbd>();

    auto platform = std::string();
    auto tbd_version = 2;

    const char *current_directory = nullptr;

    auto parse_architectures = [&](std::vector<const NXArchInfo *> &architectures, int &index) {
        while (index < argc) {
            const auto &architecture_string = argv[index];
            if (*architecture_string == '-') {
                break;
            }

            const auto architecture = NXGetArchInfoFromName(architecture_string);
            if (!architecture) {
                fprintf(stderr, "Architecture (%s) is invalid\n", architecture_string);
                exit(1);
            }

            architectures.emplace_back(architecture);
            index++;
        }
    };

    auto output_paths_index = 0;
    for (auto i = 1; i < argc; i++) {
        const auto &argument = argv[i];
        if (*argument != '-') {
            fprintf(stderr, "Unrecognized argument (%s)\n", argument);
            return 1;
        }

        auto option = &argument[1];
        if (*option == '-') {
            option++;
        }

        const auto is_last_argument = i == argc - 1;
        if (strcmp(option, "a") == 0 || strcmp(option, "archs") == 0) {
            if (is_last_argument) {
                fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                return 1;
            }

            parse_architectures(architectures, i);
            i--;
        } else if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
            if (i != 1 || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_usage();
        } else if (strcmp(option, "o") == 0 || strcmp(option, "output") == 0) {
            if (is_last_argument) {
                fputs("Please provide path(s) to output files\n", stderr);
                return 1;
            }

            for (i++; i < argc; i++) {
                auto path = std::string(argv[i]);
                const auto &path_front = path.front();

                if (path_front == '-') {
                    break;
                }

                if (output_paths_index >= tbds.size()) {
                    fprintf(stderr, "No coresponding mach-o files for output-path (%s)\n", path.data());
                    return 1;
                }

                auto &tbd = tbds.at(output_paths_index);
                auto &macho_files = tbd.macho_files();

                if (path_front != '/' && path != "stdout") {
                    if (!current_directory) {
                        current_directory = getcwd(nullptr, 0);

                        if (!current_directory) {
                            fputs("Failed to get current-working-directory\n", stderr);
                            return 1;
                        }
                    }

                    path.insert(0, current_directory);
                } else if (path == "stdout" && macho_files.size() > 1) {
                    fputs("Can't output multiple mach-o files to stdout\n", stderr);
                    return 1;
                }

                struct stat sbuf;
                if (stat(path.data(), &sbuf) == 0) {
                    if (S_ISDIR(sbuf.st_mode)) {
                        const auto &macho_files = tbd.macho_files();
                        if (macho_files.size() < 2) {
                            fprintf(stderr, "Cannot output tbd-file to a directory at path (%s), please provide a full path to a file to output to\n", path.data());
                            return 1;
                        } else if (path == "stdout") {
                            fputs("Cannot output recursive mach-o files to stdout. Please provide a directory to output to", stderr);
                            return 1;
                        }

                        for (const auto &macho_file : tbd.macho_files()) {
                            const auto macho_file_iter = macho_file.find_last_of('/');
                            auto macho_file_output_path = macho_file.substr(macho_file_iter);

                            macho_file_output_path.insert(0, path);
                            macho_file_output_path.append(".tbd");

                            tbd.add_output_file(macho_file_output_path);
                        }
                    } else if (S_ISREG(sbuf.st_mode)) {
                        const auto &macho_files = tbd.macho_files();
                        if (macho_files.size() > 1) {
                            fprintf(stderr, "Can't output multiple mach-o files and output to file at path (%s)\n", path.data());
                            return 1;
                        }

                        tbd.add_output_file(path);
                    }
                } else {
                    const auto &macho_files = tbd.macho_files();
                    if (macho_files.size() > 1) {
                        fprintf(stderr, "Directory at path (%s) does not exist\n", path.data());
                        return 1;
                    }

                    tbd.add_output_file(path);
                }

                output_paths_index++;
            }

            i--;
        } else if (strcmp(option, "p") == 0 || strcmp(option, "path") == 0) {
            if (is_last_argument) {
                fputs("Please provide path(s) to mach-o files\n", stderr);
                return 1;
            }

            auto local_architectures = std::vector<const NXArchInfo *>();

            auto local_platform = std::string();
            auto local_tbd_version = 0;

            auto is_recursive = false;
            auto should_continue = false;

            for (i++; i < argc; i++) {
                const auto &argument = argv[i];
                if (*argument == '-') {
                    const char *option = &argument[1];
                    if (*option == '-') {
                        option = &option[1];
                    }

                    const auto is_last_argument = i == argc - 1;
                    if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                        is_recursive = true;
                    } else if (strcmp(option, "a") == 0 || strcmp(option, "archs") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                            return 1;
                        }

                        i++;
                        parse_architectures(local_architectures, i);

                        i--;
                        break;
                    } else if (strcmp(option, "platform") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a platform-string (ios, macos, tvos, watchos)", stderr);
                            return 1;
                        }

                        i++;

                        const auto &platform_string = argv[i];
                        if (strcmp(platform_string, "ios") != 0 && strcmp(platform_string, "macos") != 0 && strcmp(platform_string, "watchos") != 0 && strcmp(platform_string, "tvos") != 0) {
                            fprintf(stderr, "Platform-string (%s) is invalid\n", platform_string);
                            return 1;
                        }

                        platform = platform_string;
                    } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
                        if (is_last_argument) {
                            fputs("Please provide a tbd-version\n", stderr);
                            return 1;
                        }

                        i++;
                        local_tbd_version = (int)tbd::string_to_version(argv[i]);

                        if (!(int)local_tbd_version) {
                            fprintf(stderr, "(%s) is not a valid tbd-version\n", argv[i]);
                            return 1;
                        }
                    } else {
                        should_continue = true;
                        break;
                    }

                    continue;
                }

                if (should_continue) {
                    continue;
                }

                auto path = std::string(argument);
                if (path.front() != '/') {
                    if (!current_directory) {
                        current_directory = getcwd(nullptr, 0);

                        if (!current_directory) {
                            fputs("Failed to get current-working-directory\n", stderr);
                            return 1;
                        }
                    }

                    path.insert(0, current_directory);
                }

                struct stat path_sbuf;
                if (stat(path.data(), &path_sbuf) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object at path (%s), failing with error (%s)\n", path.data(), strerror(errno));
                    return 1;
                }

                auto tbd = ::tbd();

                if (S_ISDIR(path_sbuf.st_mode)) {
                    if (!is_recursive) {
                        fprintf(stderr, "Cannot open directory at path (%s) as a macho-file, use -r to recurse the directory\n", path.data());
                        return 1;
                    }

                    const auto directory = opendir(path.data());
                    if (!directory) {
                        fprintf(stderr, "Failed to open directory at path (%s), failing with error (%s)\n", path.data(), strerror(errno));
                        return 1;
                    }

                    std::function<void(DIR *, const std::string &)> loop_directory = [&](DIR *directory, const std::string &directory_path) {
                        auto directory_entry = readdir(directory);
                        while (directory_entry != nullptr) {
                            if (directory_entry->d_type == DT_DIR) {
                                if (strncmp(directory_entry->d_name, ".", directory_entry->d_namlen) == 0 ||
                                    strncmp(directory_entry->d_name, "..", directory_entry->d_namlen) == 0) {
                                    directory_entry = readdir(directory);
                                    continue;
                                }

                                auto sub_directory_path = directory_path;

                                sub_directory_path.append(directory_entry->d_name);
                                sub_directory_path.append(1, '/');

                                const auto sub_directory = opendir(sub_directory_path.data());
                                if (!sub_directory) {
                                    fprintf(stderr, "Failed to open sub-directory at path (%s), failing with error (%s)\n", sub_directory_path.data(), strerror(errno));
                                    exit(1);
                                }

                                loop_directory(sub_directory, sub_directory_path);
                                closedir(sub_directory);
                            } else if (directory_entry->d_type == DT_REG) {
                                const auto &directory_entry_path = directory_path + directory_entry->d_name;

                                if (macho_file::is_valid_library(directory_entry_path)) {
                                    tbd.add_macho_file(directory_entry_path);
                                }
                            }

                            directory_entry = readdir(directory);
                        }
                    };

                    loop_directory(directory, path);
                    closedir(directory);
                } else if (S_ISREG(path_sbuf.st_mode)) {
                    if (is_recursive) {
                        fprintf(stderr, "Cannot recurse file at path (%s)\n", path.data());
                        return 1;
                    }

                    if (!macho_file::is_valid_library(path)) {
                        fprintf(stderr, "File at path (%s) is not a valid mach-o library\n", path.data());
                    }

                    tbd.add_macho_file(path);
                } else {
                    fprintf(stderr, "Object at path (%s) is not a regular file\n", path.data());
                    return 1;
                }

                auto tbd_architectures = &local_architectures;
                if (tbd_architectures->empty()) {
                    tbd_architectures = &architectures;
                }

                auto tbd_platform = &local_platform;
                if (tbd_platform->empty()) {
                    tbd_platform = &platform;
                    while (platform.empty() || (platform != "ios" && platform != "macos" && platform != "watchos" && platform != "tvos")) {
                        if (S_ISDIR(path_sbuf.st_mode)) {
                            fprintf(stdout, "Please provide a platform for files in directory at path (%s) (ios, macos, watchos, or tvos): ", path.data());
                        } else {
                            fprintf(stdout, "Please provide a platform for file at path (%s) (ios, macos, watchos, or tvos): ", path.data());
                        }

                        getline(std::cin, platform);
                    }
                }

                auto version_tbd = &local_tbd_version;
                if (!*version_tbd) {
                    version_tbd = &tbd_version;
                }

                tbd.set_architectures(*tbd_architectures);
                tbd.set_platform(::tbd::string_to_platform(tbd_platform->data()));
                tbd.set_version(*(enum tbd::version *)version_tbd);

                tbds.emplace_back(tbd);
                is_recursive = false;

                local_architectures.clear();
                local_platform.clear();

                local_tbd_version = 0;
            }

            if (is_recursive || local_architectures.size() != 0 || local_platform.size() != 0 || local_tbd_version != 0) {
                fputs("Please provide a path to a directory to recurse through\n", stderr);
                return 1;
            }

            i--;
        } else if (strcmp(option, "platform") == 0) {
            if (is_last_argument) {
                fputs("Please provide a platform-string (ios, macos, tvos, watchos)", stderr);
                return 1;
            }

            i++;

            const auto &platform_string = argv[i];
            if (strcmp(platform_string, "ios") != 0 && strcmp(platform_string, "macos") != 0 && strcmp(platform_string, "watchos") != 0 && strcmp(platform_string, "tvos") != 0) {
                fprintf(stderr, "Platform-string (%s) is invalid\n", platform_string);
                return 1;
            }

            platform = platform_string;
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (i != 1 || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            print_usage();
        } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
            if (is_last_argument) {
                fputs("Please provide a tbd-version\n", stderr);
                return 1;
            }

            i++;
            const auto &version = argv[i];

            if (*version == '-') {
                fputs("Please provide a tbd-version\n", stderr);
                return 1;
            }

            if (strcmp(version, "v1") == 0) {
                tbd_version = 1;
            } else if (strcmp(version, "v2") != 0) {
                fprintf(stderr, "tbd-version (%s) is invalid\n", version);
                return 1;
            }
        } else if (strcmp(option, "versions") == 0) {
            if (i != 1 || !is_last_argument) {
                fprintf(stderr, "Option (%s) should be run by itself\n", argument);
                return 1;
            }

            fputs("v1\nv2 (default)\n", stdout);
            return 0;
        } else {
            fprintf(stderr, "Unrecognized argument: %s\n", argument);
            return 1;
        }
    }

    if (!tbds.size()) {
        fputs("No mach-o files have been provided", stderr);
        return 1;
    }

    for (auto &tbd : tbds) {
        auto &output_files = tbd.output_files();
        if (output_files.size() != 0) {
            continue;
        }

        const auto &macho_files = tbd.macho_files();
        if (macho_files.size() > 1) {
            for (const auto &macho_file : macho_files) {
                tbd.add_output_file(macho_file + ".tbd");
            }
        } else {
            tbd.add_output_file("stdout");
        }
    }

    for (auto &tbd : tbds) {
        tbd.execute();
        printf("");
    }
}
