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

#include <unistd.h>

inline void swap_value(uint32_t *value) noexcept {
    *value = ((*value >> 8) & 0x00ff00ff) | ((*value << 8) & 0xff00ff00);
    *value = ((*value >> 16) & 0x0000ffff) | ((*value << 16) & 0xffff0000);
}

class macho_container {
public:
    explicit macho_container(FILE *file, const long &macho_base)
    : file_(file), macho_base_(macho_base) {
        const auto position = ftell(file);
        fseek(file, 0, SEEK_END);

        this->size_ = ftell(file);

        fseek(file, position, SEEK_SET);
        this->validate();
    }

    explicit macho_container(FILE *file, const long &macho_base, const struct fat_arch &architecture)
    : file_(file), macho_base_(macho_base), base_(architecture.offset), size_(architecture.size) {
        this->validate();
    }

    explicit macho_container(FILE *file, const long &macho_base, const struct fat_arch_64 &architecture)
    : file_(file), macho_base_(macho_base), base_(architecture.offset), size_(architecture.size) {
        this->validate();
    }

    ~macho_container() {
        for (struct load_command *load_command : cached_) {
            delete load_command;
        }
    }

    auto iterate_load_commands(const std::function<bool(const struct load_command *load_cmd)> &callback) noexcept {
        auto base = this->base() + sizeof(struct mach_header);
        const auto position = ftell(file_);

        if (this->is_64_bit()) {
            base += sizeof(uint32_t);
        }

        auto ncmds = header_.ncmds;
        const auto &sizeofcmds = header_.sizeofcmds;

        auto finished = false;
        if (cached_.size() != 0) {
            for (struct load_command *load_command : cached_) {
                auto result = callback(load_command);

                base += load_command->cmdsize;
                ncmds--;

                if (!result) {
                    finished = true;
                    break;
                }
            }
        } else {
            cached_.reserve(ncmds);
        }

        if (!finished) {
            if (ncmds != 0) {
                fseek(file_, base, SEEK_SET);
            }

            auto size_used = 0;
            for (auto i = 0; i < ncmds; i++) {
                struct load_command load_cmd;
                fread(&load_cmd, sizeof(struct load_command), 1, file_);

                if (should_swap_) {
                    swap_load_command(&load_cmd, NX_LittleEndian);
                }

                const auto &cmdsize = load_cmd.cmdsize;
                if (cmdsize < sizeof(struct load_command)) {
                    if (base != 0) {
                        fprintf(stderr, "Load-command (at index %d) of architecture (at base 0x%.8lX) is too small to be valid", i, base);
                    } else {
                        fprintf(stderr, "Load-command (at index %d) of mach-o file is too small to be valid", i);
                    }

                    exit(1);
                }

                if (cmdsize >= sizeofcmds) {
                    if (base != 0) {
                        fprintf(stderr, "Load-command (at index %d) of architecture (at base 0x%.8lX) is larger than/or equal to entire area allocated for load-commands", i, base);
                    } else {
                        fprintf(stderr, "Load-command (at index %d) of mach-o file is larger than/or equal to entire area allocated for load-commands", i);
                    }

                    exit(1);
                }

                size_used += cmdsize;
                if (size_used > sizeofcmds) {
                    if (base != 0) {
                        fprintf(stderr, "Load-command (at index %d) of architecture (at base 0x%.8lX) goes past end of area allocated for load-commands", i, base);
                    } else {
                        fprintf(stderr, "Load-command (at index %d) of mach-o file goes past end of area allocated for load-commands", i);
                    }

                    exit(1);
                } else if (size_used == sizeofcmds && i != ncmds - 1) {
                    if (base != 0) {
                        fprintf(stderr, "Load-command (at index %d) of architecture (at base 0x%.8lX) takes up all of the remaining space allocated for load-commands", i, base);
                    } else {
                        fprintf(stderr, "Load-command (at index %d) of mach-o file takes up all of the remaining space allocated for load-commands", i);
                    }

                    exit(1);
                }

                auto load_command = (struct load_command *)new char[cmdsize];

                load_command->cmd = load_cmd.cmd;
                load_command->cmdsize = cmdsize;

                if (cmdsize > sizeof(struct load_command)) {
                    fread(&((char *)load_command)[sizeof(struct load_command)], cmdsize - sizeof(struct load_command), 1, file_);
                }

                cached_.emplace_back(load_command);

                auto result = callback(load_command);
                if (result) {
                    break;
                }
            }
        }

        fseek(file_, position, SEEK_SET);
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
            if (base != 0) {
                fputs("Architecture (at base 0x%.8lX) does not have a symbol-table", stderr);
            } else {
                fputs("Mach-o file does not have a symbol-table", stderr);
            }

            exit(1);
        }

        const auto &string_table_offset = symtab_command->stroff;
        if (string_table_offset > size_) {
            if (base != 0) {
                fputs("Architecture (at base 0x%.8lX) has a string-table outside of its container", stderr);
            } else {
                fputs("Mach-o file has a string-table outside of its container", stderr);
            }

            exit(1);
        }

        const auto &string_table_size = symtab_command->strsize;
        if (string_table_offset + string_table_size > size_) {
            if (base != 0) {
                fputs("Architecture (at base 0x%.8lX) has a string-table that goes outside of its container", stderr);
            } else {
                fputs("Mach-o file has a string-table that goes outside of its container", stderr);
            }

            exit(1);
        }

        const auto &symbol_table_offset = symtab_command->symoff;
        if (symbol_table_offset > size_) {
            if (base != 0) {
                fputs("Architecture (at base 0x%.8lX) has a symbol-table that is outside of its container", stderr);
            } else {
                fputs("Mach-o file has a symbol-table that is outside of its container", stderr);
            }

            exit(1);
        }

        const auto string_table = new char[string_table_size];
        const auto &symbol_table_count = symtab_command->nsyms;

        fseek(file_, base + string_table_offset, SEEK_SET);
        fread(string_table, string_table_size, 1, file_);

        fseek(file_, base + symbol_table_offset, SEEK_SET);

        if (this->is_64_bit()) {
            const auto symbol_table_size = sizeof(struct nlist_64) * symbol_table_count;
            if (symbol_table_offset + symbol_table_size > size_) {
                if (base != 0) {
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

                const auto symbol_table_string_table_string = &string_table[symbol_table_entry_string_table_index];
                const auto result = callback(symbol_table_entry, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }

            delete[] symbol_table;
        } else {
            const auto symbol_table_size = sizeof(struct nlist) * symbol_table_count;
            if (symbol_table_offset + symbol_table_size > size_) {
                if (base != 0) {
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

                const auto symbol_table_string_table_string = &string_table[symbol_table_entry_string_table_index];
                const auto result = callback(symbol_table_entry_64, symbol_table_string_table_string);

                if (!result) {
                    break;
                }
            }

            delete[] symbol_table;
        }

        fseek(file_, position, SEEK_SET);
        delete[] string_table;
    }

    inline const FILE *file() const noexcept { return file_; }
    inline const struct mach_header &header() const noexcept { return header_; }

    inline bool should_swap() const noexcept { return should_swap_; }

    inline long base() const noexcept { return this->macho_base_ + this->base_; }
    inline long size() const noexcept { return this->size_; }

    inline bool is_32_bit() const noexcept { return header_.magic == MH_MAGIC || header_.magic == MH_CIGAM; }
    inline bool is_64_bit() const noexcept { return header_.magic == MH_MAGIC_64 || header_.magic == MH_CIGAM_64; }

private:
    FILE *file_;
    long macho_base_;

    long base_ = 0;
    long size_ = 0;

    struct mach_header header_;
    bool should_swap_ = false;

    std::vector<struct load_command *> cached_;

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

    inline const FILE *file() const noexcept { return file_; }
    inline std::vector<macho_container> &containers() noexcept { return containers_; }

    inline bool is_thin() const noexcept { return magic_ == MH_MAGIC || magic_ == MH_CIGAM || magic_ == MH_MAGIC_64 || magic_ == MH_CIGAM_64; }
    inline bool is_fat() const noexcept { return magic_ == FAT_MAGIC || magic_ == FAT_CIGAM || magic_ == FAT_MAGIC_64 || magic_ == FAT_CIGAM_64; }

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

            if (magic_ == FAT_MAGIC_64 || magic_ == FAT_CIGAM_64) {
                const auto architectures = new struct fat_arch_64[nfat_arch];
                fread(architectures, sizeof(struct fat_arch) * nfat_arch, 1, file_);

                if (should_swap) {
                    swap_fat_arch_64(architectures, nfat_arch, NX_LittleEndian);
                }

                containers_.reserve(nfat_arch);
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

                containers_.reserve(nfat_arch);
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
    explicit symbol(const std::string &string, bool weak = false) noexcept
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
        if (std::find(symbols_.begin(), symbols_.end(), symbol) == symbols_.end()) {
            symbols_.emplace_back(symbol);
        }
    }

    void add_reexport(const symbol &reexport) noexcept {
        if (std::find(reexports_.begin(), reexports_.end(), reexport) == reexports_.end()) {
            reexports_.emplace_back(reexport);
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

void print_usage() noexcept {
    fprintf(stdout, "Usage: tbd [-p file-paths] [-v/--version v2] [-a/--archs architectures] [-o/-output output-paths-or-stdout]\n");
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "    -a, --archs,   Specify Architecture(s) to use, instead of the ones in the provieded mach-o file(s)\n");
    fprintf(stdout, "    -h, --help,    Print this message\n");
    fprintf(stdout, "    -o, --output,  Path(s) for output file(s) to write converted .tbd. If provided file(s) already exists, contents will get overrided\n");
    fprintf(stdout, "    -p, --path,    Path(s) to mach-o file(s) to convert to a .tbd\n");
    fprintf(stdout, "    -u, --usage,   Print this message\n");
    fprintf(stdout, "    -v, --version, Set version of tbd to convert to. Run --versions to see a list of available versions. (ex. -v v1), v2 is the default version\n");

    exit(0);
}

int main(int argc, const char *argv[]) noexcept {
    if (argc < 2) {
        fputs("Please run -h or -u to see a list of options", stderr);
        return 1;
    }

    auto architecture_infos = std::vector<const NXArchInfo *>();
    auto macho_paths = std::vector<std::string>();
    auto output_paths = std::vector<std::string>();

    auto tbd_version = 2;

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

            for (i++; i < argc; i++){
                const auto &argument = argv[i];
                if (*argument == '-') {
                    break;
                }

                const auto architecture_info = NXGetArchInfoFromName(argument);
                if (!architecture_info) {
                    fprintf(stderr, "Architecture (%s) is of an unrecognized type\n", argument);
                    return 1;
                }

                architecture_infos.emplace_back(architecture_info);
            }

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
                const auto &path = argv[i];
                if (*path == '-') {
                    break;
                }

                if (*path != '/' && strcmp(path, "stdout") != 0) {
                    auto full_path = std::string(getcwd(nullptr, 0));
                    if (full_path.back() != '/') {
                        full_path.append(1, '/');
                    }

                    full_path.append(path);

                    const auto full_path_data = full_path.data();
                    struct stat output_sbuf;

                    if (stat(full_path_data, &output_sbuf) != 0) {
                        if (!S_ISREG(output_sbuf.st_mode)) {
                            fprintf(stderr, "Output file at path(%s) is not a regular file\n", full_path_data);
                            return 1;
                        }
                    }

                    output_paths.emplace_back(full_path);
                } else {
                    output_paths.emplace_back(path);
                }
            }

            i--;
        } else if (strcmp(option, "p") == 0 || strcmp(option, "path") == 0) {
            if (is_last_argument) {
                fputs("Please provide path(s) to mach-o files\n", stderr);
                return 1;
            }

            for (i++; i < argc; i++) {
                const auto &path = argv[i];
                if (*path == '-') {
                    break;
                }

                if (*path != '/') {
                    auto full_path = std::string(getcwd(nullptr, 0));
                    if (full_path.back() != '/') {
                        full_path.append(1, '/');
                    }

                    full_path.append(path);

                    const auto full_path_data = full_path.data();
                    if (access(full_path_data, F_OK) != 0) {
                        fprintf(stderr, "Mach-o file at path (%s) does not exist\n", full_path.data());
                        return 1;
                    }

                    struct stat macho_sbuf;
                    if (stat(full_path_data, &macho_sbuf) != 0) {
                        fprintf(stderr, "Unable to get information on mach-o file at path (%s)\n", full_path_data);
                        return 1;
                    }

                    if (!S_ISREG(macho_sbuf.st_mode)) {
                        fprintf(stderr, "Mach-o file at path (%s) is not a regular file\n", full_path_data);
                        return 1;
                    }

                    macho_paths.emplace_back(full_path);
                } else {
                    macho_paths.emplace_back(path);
                }
            }

            i--;
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usagae") == 0) {
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

            fputs("v1\nv2\n", stdout);
            return 0;
        }
    }

    if (macho_paths.empty()) {
        fputs("No mach-o files have been provided\n", stderr);
        return 1;
    }

    if (tbd_version == 2 && architecture_infos.size()) {
        fputs("Cannot use custom architectures for tbd version v2. Specify version v1 to be able to do so\n", stderr);
        return 1;
    }

    auto output_paths_index = 0;
    for (const auto &macho_file_path : macho_paths) {
        const auto macho_file_path_data = macho_file_path.data();
        auto macho_file_object = macho_file(macho_file_path);

        auto output_file = stdout;
        if (output_paths_index < output_paths.size()) {
            const auto &output_file_path = output_paths.at(output_paths_index);
            const auto output_file_path_data = output_file_path.data();

            if (strcmp(output_file_path_data, "stdout") != 0) {
                struct stat output_sbuf;
                if (stat(output_file_path_data, &output_sbuf) == 0) {
                    if (!S_ISREG(output_sbuf.st_mode)) {
                        fprintf(stderr, "Path to output-file (%s) is not a regular file\n", output_file_path_data);
                        return 1;
                    }
                }

                output_file = fopen(output_file_path_data, "w");
                if (!output_file) {
                    fprintf(stderr, "Failed to open/create file at path (%s) for writing (%s)\n", macho_file_path_data, strerror(errno));
                    return 1;
                }
            }
        }

        auto macho_architectures = std::vector<const NXArchInfo *>();

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

            macho_architectures.emplace_back(macho_container_architecture_info);

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

                        auto &id_dylib_command = *(struct dylib_command *)load_cmd;
                        if (macho_container_should_swap) {
                            swap_dylib_command(&id_dylib_command, NX_LittleEndian);
                        }

                        const auto &id_dylib = ((struct dylib_command *)load_cmd)->dylib;
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
                        auto &reexport_dylib_command = *(struct dylib_command *)load_cmd;
                        if (macho_container_should_swap) {
                            swap_dylib_command(&reexport_dylib_command, NX_LittleEndian);
                        }

                        const auto &reexport_dylib = ((struct dylib_command *)load_cmd)->dylib;
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
                            reexports.emplace_back(reexport_dylib_string);
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

                return false;
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

            if (!added_uuid && tbd_version == 2) {
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
        if (architecture_infos.empty()) {
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
        } else {
            groups.emplace_back(architecture_infos);

            auto &group = groups.front();
            for (const auto &reexport : reexports) {
                group.add_reexport(reexport);
            }

            for (const auto &symbol : symbols) {
                group.add_symbol(symbol);
            }
        }

        std::sort(groups.begin(), groups.end(), [](const group &lhs, const group &rhs) {
            const auto &lhs_symbols = lhs.symbols();
            const auto lhs_symbols_size = lhs_symbols.size();

            const auto &rhs_symbols = rhs.symbols();
            const auto rhs_symbols_size = rhs_symbols.size();

            return lhs_symbols_size < rhs_symbols_size;
        });

        auto platform = std::string();
        while (platform.empty() || (platform != "ios" && platform != "macos" && platform != "watchos" && platform != "tvos")) {
            std::cout << "Please provied a platform (ios, macos, watchos, or tvos): ";
            getline(std::cin, platform);
        }

        fputs("---", output_file);
        if (tbd_version == 2) {
            fputs(" !tapi-tbd-v2", output_file);
        }

        fprintf(output_file, "\narchs:%-17s[ ", "");
        if (architecture_infos.empty()) {
            auto architecture_info_begin = macho_architectures.begin();
            fprintf(output_file, "%s", (*architecture_info_begin)->name);

            for (architecture_info_begin++; architecture_info_begin != macho_architectures.end(); architecture_info_begin++) {
                fprintf(output_file, ", %s", (*architecture_info_begin)->name);
            }

            fputs(" ]\n", output_file);
        } else {
            auto architecture_info_begin = architecture_infos.begin();
            fprintf(output_file, "%s", (*architecture_info_begin)->name);

            for (architecture_info_begin++; architecture_info_begin != architecture_infos.end(); architecture_info_begin++) {
                fprintf(output_file, ", %s", (*architecture_info_begin)->name);
            }

            fputs(" ]\n", output_file);
        }

        if (tbd_version == 2) {
            fprintf(output_file, "uuids:%-17s[ ", "");

            auto counter = 1;
            auto uuids_begin = uuids.begin();

            for (auto architecture_infos_begin = macho_architectures.begin(); architecture_infos_begin < macho_architectures.end(); architecture_infos_begin++, uuids_begin++) {
                fprintf(output_file, "'%s: %s'", (*architecture_infos_begin)->name, uuids_begin->data());

                if (architecture_infos_begin != macho_architectures.end() - 1) {
                    fputs(", ", output_file);

                    if (counter % 2 == 0) {
                        fprintf(output_file, "%-27s", "\n");
                    }

                    counter++;
                }
            }

            fputs(" ]\n", output_file);
        }

        fprintf(output_file, "platform:%-14s%s\n", "", platform.data());
        fprintf(output_file, "install-name:%-10s%s\n", "", installation_name.data());

        fprintf(output_file, "current-version:%-7s%s\n", "", current_version.data());
        fprintf(output_file, "compatibility-version: %s\n", compatibility_version.data());

        if (tbd_version == 2) {
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

            print_symbols(ordinary_symbols, "symbols");
            print_symbols(weak_symbols, "weak-def-symbols");
            print_symbols(objc_classes, "objc-classes");
            print_symbols(objc_ivars, "objc-ivars");
            print_symbols(reexports, "reexports");
        }

        fputs("...\n", output_file);

        output_paths_index++;
        if (output_file != stdout) {
            fclose(output_file);
        }
    }
}
