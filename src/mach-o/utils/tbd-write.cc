//
//  src/mach-o/utils/tbd-write.cc
//  tbd
//
//  Created by inoahdev on 1/7/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "tbd.h"

namespace macho::utils {
    // write_to helpers

    bool write_architectures_to_file(int descriptor, uint64_t architectures, bool dash) noexcept;
    bool write_architectures_to_file(FILE *file, uint64_t architectures, bool dash) noexcept;

    bool write_compatibility_version_to_file(int descriptor, const tbd::packed_version &version) noexcept;
    bool write_compatibility_version_to_file(FILE *file, const tbd::packed_version &version) noexcept;

    bool write_current_version_to_file(int descriptor, const tbd::packed_version &version) noexcept;
    bool write_current_version_to_file(FILE *file, const tbd::packed_version &version) noexcept;

    tbd::write_result write_exports_to_file(const tbd &tbd, int descriptor, const tbd::write_options &options, const std::vector<tbd::export_group> &groups) noexcept;
    tbd::write_result write_exports_to_file(const tbd &tbd, FILE *file, const tbd::write_options &options, const std::vector<tbd::export_group> &groups) noexcept;

    bool write_flags_to_file(int descriptor, const struct tbd::flags &flags);
    bool write_flags_to_file(FILE *file, const struct tbd::flags &flags);

    bool write_footer_to_file(int descriptor) noexcept;
    bool write_footer_to_file(FILE *descriptor) noexcept;

    bool write_header_to_file(int descriptor, const enum tbd::version &version) noexcept;
    bool write_header_to_file(FILE *file, const enum tbd::version &version) noexcept;

    bool write_install_name_to_file(int descriptor, const std::string &install_name) noexcept;
    bool write_install_name_to_file(FILE *file, const std::string &install_name) noexcept;

    tbd::write_result write_group_to_file(const tbd &tbd, int descriptor, const tbd::export_group &group, const tbd::write_options &options) noexcept;
    tbd::write_result write_group_to_file(const tbd &tbd, FILE *file, const tbd::export_group &group, const tbd::write_options &options) noexcept;

    bool write_packed_version_to_file(int descriptor, const tbd::packed_version &version) noexcept;
    bool write_packed_version_to_file(FILE *file, const tbd::packed_version &version) noexcept;

    bool write_parent_umbrella_to_file(int descriptor, const std::string &parent_umbrella) noexcept;
    bool write_parent_umbrella_to_file(FILE *file, const std::string &parent_umbrella) noexcept;

    bool write_platform_to_file(int descriptor, const enum tbd::platform &platform) noexcept;
    bool write_platform_to_file(FILE *file, const enum tbd::platform &platform) noexcept;

    bool write_objc_constraint_to_file(int descriptor, const enum tbd::objc_constraint &constraint) noexcept;
    bool write_objc_constraint_to_file(FILE *file, const enum tbd::objc_constraint &constraint) noexcept;

    bool write_swift_version_to_file(int descriptor, const uint32_t &version) noexcept;
    bool write_swift_version_to_file(FILE *file, const uint32_t &version) noexcept;

    static inline constexpr const auto line_length_max = 105ul;

    bool write_string_for_array_to_file(int descriptor, const std::string &string, size_t &line_length) noexcept;
    bool write_string_for_array_to_file(FILE *file, const std::string &string, size_t &line_length) noexcept;

    bool write_uuids_to_file(int descriptor, const std::vector<tbd::uuid_pair> &uuids, const tbd::write_options &options) noexcept;
    bool write_uuids_to_file(FILE *file, const std::vector<tbd::uuid_pair> &uuids, const tbd::write_options &options) noexcept;

    // write_group_to_file helpers

    bool write_reexports_array_to_file(int descriptor, const std::vector<tbd::reexport>::const_iterator &begin, const std::vector<tbd::reexport>::const_iterator &end, uint64_t architectures) noexcept;
    bool write_reexports_array_to_file(FILE *file, const std::vector<tbd::reexport>::const_iterator &begin, const std::vector<tbd::reexport>::const_iterator &end, uint64_t architectures) noexcept;

    bool write_normal_symbols_array_to_file(int descriptor, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept;
    bool write_normal_symbols_array_to_file(FILE *file, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept;

    bool write_objc_class_symbols_array_to_file(int descriptor, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept;
    bool write_objc_class_symbols_array_to_file(FILE *file, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept;

    bool write_objc_ivar_symbols_array_to_file(int descriptor, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept;
    bool write_objc_ivar_symbols_array_to_file(FILE *file, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept;

    bool write_weak_symbols_array_to_file(int descriptor, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept;
    bool write_weak_symbols_array_to_file(FILE *file, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept;

    std::vector<tbd::symbol>::const_iterator next_iterator_for_symbol(const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures, enum tbd::symbol::type type) noexcept;

    std::vector<tbd::export_group> tbd::export_groups() const noexcept {
        // Symbols created using tbd::create are kept
        // track of by a bitset of indexs into the
        // containers that hold them, but this differs
        // from the tbd-format, where symbols are tracked
        // with the *architecture* of where they're found

        // To generate export-groups, we need to convert the
        // container-index bitset to an architecture-info indexs
        // bitset but that presents an issue which may be a problem,
        // namely multiple containers with the same architecture.

        // Multiple containers with the same architecture may not
        // even be supported officially by llvm, either they don't,
        // they choose a "best" fitting architecture with some sort
        // or measurement, or combine all containers with the same
        // architecture into a single architecture export-group, which,
        // for now, is what we're doing

        // XXX: We might want to move to a different behavior for this,
        // or allow the caller to provide this different behavior, keeping
        // a default if the caller does not provide one

        auto groups = std::vector<export_group>();

        auto groups_begin = groups.begin();
        auto groups_end = groups.end();

        for (const auto &reexport : this->reexports) {
            auto groups_iter = std::find(groups_begin, groups_end, reexport);
            if (groups_iter != groups_end) {
                if (!groups_iter->reexport) {
                    groups_iter->reexport = &reexport;
                }

                continue;
            }

            groups.emplace_back(&reexport);

            groups_begin = groups.begin();
            groups_end = groups.end();
        }


        for (const auto &symbol : this->symbols) {
            auto groups_iter = std::find(groups_begin, groups_end, symbol);
            if (groups_iter != groups_end) {
                if (!groups_iter->symbol) {
                    groups_iter->symbol = &symbol;
                }

                continue;
            }

            groups.emplace_back(&symbol);

            groups_begin = groups.begin();
            groups_end = groups.end();
        }

        return groups;
    }

    tbd::tbd::write_result tbd::write_to(const char *path, const tbd::write_options &options) const noexcept {
        const auto file = fopen(path, "w");
        if (!file) {
            return write_result::failed_to_open_stream;
        }

        const auto result = this->write_to(file, options);
        fclose(file);

        return result;
    }

    tbd::write_result tbd::write_to(int descriptor, const tbd::write_options &options) const noexcept {
        return this->write_with_export_groups_to(descriptor, options, this->export_groups());
    }

    tbd::write_result tbd::write_to(FILE *file, const tbd::write_options &options) const noexcept {
        return this->write_with_export_groups_to(file, options, this->export_groups());
    }

    tbd::write_result tbd::write_with_export_groups_to(int descriptor, const tbd::write_options &options, const std::vector<tbd::export_group> &groups) const noexcept {
        if (!options.ignores_header()) {
            if (!write_header_to_file(descriptor, this->version)) {
                return write_result::failed_to_write_header;
            }
        }

        if (!options.ignores_architectures()) {
            if (!write_architectures_to_file(descriptor, this->architectures, false)) {
                return write_result::failed_to_write_architectures;
            }
        }

        if (!options.ignores_uuids()) {
            if (!write_uuids_to_file(descriptor, this->uuids, options)) {
                return write_result::failed_to_write_uuids;
            }
        }

        if (!options.ignores_platform()) {
            if (!write_platform_to_file(descriptor, this->platform)) {
                return write_result::failed_to_write_platform;
            }
        }

        if (!options.ignores_flags()) {
            if (!write_flags_to_file(descriptor, this->flags)) {
                return write_result::failed_to_write_flags;
            }
        }

        if (!options.ignores_install_name()) {
            if (!write_install_name_to_file(descriptor, this->install_name)) {
                return write_result::failed_to_write_install_name;
            }
        }

        if (!options.ignores_current_version()) {
            if (!write_current_version_to_file(descriptor, this->current_version)) {
                return write_result::failed_to_write_current_version;
            }
        }

        if (!options.ignores_compatibility_version()) {
            if (!write_compatibility_version_to_file(descriptor, this->compatibility_version)) {
                return write_result::failed_to_write_compatibility_version;
            }
        }

        if (!options.ignores_swift_version()) {
            if (this->version == version::v2 || !options.ignores_unneeded_fields_for_version()) {
                if (!write_swift_version_to_file(descriptor, this->swift_version)) {
                    return write_result::failed_to_write_swift_version;
                }
            }
        }

        if (!options.ignores_objc_constraint()) {
            if (this->version == version::v2 || !options.ignores_unneeded_fields_for_version()) {
                if (!write_objc_constraint_to_file(descriptor, this->objc_constraint)) {
                    return write_result::failed_to_write_objc_constraint;
                }
            }
        }

        if (!options.ignores_parent_umbrella()) {
            if (this->version == version::v2 || !options.ignores_unneeded_fields_for_version()) {
                if (!write_parent_umbrella_to_file(descriptor, this->parent_umbrella)) {
                    return write_result::failed_to_write_parent_umbrella;
                }
            }
        }

        if (!options.ignores_exports()) {
            const auto result = write_exports_to_file(*this, descriptor, options, this->export_groups());
            if (result != write_result::ok) {
                return result;
            }
        }

        if (!options.ignores_footer()) {
            if (!write_footer_to_file(descriptor)) {
                return write_result::failed_to_write_footer;
            }
        }

        return write_result::ok;
    }

    tbd::write_result tbd::write_with_export_groups_to(FILE *file, const tbd::write_options &options, const std::vector<export_group> &groups) const noexcept {
        if (!options.ignores_header()) {
            if (!write_header_to_file(file, this->version)) {
                return write_result::failed_to_write_header;
            }
        }

        if (!options.ignores_architectures()) {
            if (!write_architectures_to_file(file, this->architectures, false)) {
                return write_result::failed_to_write_architectures;
            }
        }

        if (!options.ignores_uuids()) {
            if (this->version == version::v2 || !options.ignores_unneeded_fields_for_version()) {
                if (!write_uuids_to_file(file, this->uuids, options)) {
                    return write_result::failed_to_write_uuids;
                }
            }
        }

        if (!options.ignores_platform()) {
            if (!write_platform_to_file(file, this->platform)) {
                return write_result::failed_to_write_platform;
            }
        }

        if (!options.ignores_flags()) {
            if (this->version == version::v2 || !options.ignores_unneeded_fields_for_version()) {
                if (!write_flags_to_file(file, this->flags)) {
                    return write_result::failed_to_write_flags;
                }
            }
        }

        if (!options.ignores_install_name()) {
            if (!write_install_name_to_file(file, this->install_name)) {
                return write_result::failed_to_write_install_name;
            }
        }

        if (!options.ignores_current_version()) {
            if (!write_current_version_to_file(file, this->current_version)) {
                return write_result::failed_to_write_current_version;
            }
        }

        if (!options.ignores_compatibility_version()) {
            if (!write_compatibility_version_to_file(file, this->compatibility_version)) {
                return write_result::failed_to_write_compatibility_version;
            }
        }

        if (!options.ignores_swift_version()) {
            if (this->version == version::v2 || !options.ignores_unneeded_fields_for_version()) {
                if (!write_swift_version_to_file(file, this->swift_version)) {
                    return write_result::failed_to_write_swift_version;
                }
            }
        }

        if (!options.ignores_objc_constraint()) {
            if (this->version == version::v2 || !options.ignores_unneeded_fields_for_version()) {
                if (!write_objc_constraint_to_file(file, this->objc_constraint)) {
                    return write_result::failed_to_write_objc_constraint;
                }
            }
        }

        if (!options.ignores_parent_umbrella()) {
            if (this->version == version::v2 || !options.ignores_unneeded_fields_for_version()) {
                if (!write_parent_umbrella_to_file(file, this->parent_umbrella)) {
                    return write_result::failed_to_write_parent_umbrella;
                }
            }
        }

        if (!options.ignores_exports()) {
            const auto result = write_exports_to_file(*this, file, options, this->export_groups());
            if (result != write_result::ok) {
                return result;
            }
        }

        if (!options.ignores_footer()) {
            if (!write_footer_to_file(file)) {
                return write_result::failed_to_write_footer;
            }
        }

        return write_result::ok;
    }

    bool write_architectures_to_file(int descriptor, uint64_t architectures, bool dash) noexcept {
        // Check if the relevant bits of architectures,
        // the bits 0..get_architecture_info_table_size(),
        // are empty

        const auto bit_size = sizeof(uint64_t) * 8;
        const auto architecture_info_size = get_architecture_info_table_size();

        if (!(architectures << (bit_size - architecture_info_size))) {
            return false;
        }

        // Find first architecture to write out, then
        // write out the rest with a leading comma

        auto first_architecture_index = uint64_t();
        for (; first_architecture_index < architecture_info_size; first_architecture_index++) {
            if (!(architectures & (1ull << first_architecture_index))) {
                continue;
            }

            break;
        }

        if (first_architecture_index == architecture_info_size) {
            return false;
        }

        const auto architecture_info = architecture_info_from_index(first_architecture_index);
        if (dash) {
            if (dprintf(descriptor, "  - archs:%-14s[ %s", "", architecture_info->name) == -1) {
                return false;
            }
        } else if (dprintf(descriptor, "archs:%-17s[ %s", "", architecture_info->name) == -1) {
            return false;
        }

        for (auto index = first_architecture_index + 1; index < architecture_info_size; index++) {
            if (!(architectures & (1ull << index))) {
                continue;
            }

            const auto architecture_info = architecture_info_from_index(index);
            if (dprintf(descriptor, ", %s", architecture_info->name) == -1) {
                return false;
            }
        }

        if (dprintf(descriptor, " ]\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_architectures_to_file(FILE *file, uint64_t architectures, bool dash) noexcept {
        // Check if the relevant bits of architectures,
        // the bits 0..get_architecture_info_table_size(),
        // are empty

        const auto bit_size = sizeof(uint64_t) * 8;
        const auto architecture_info_size = get_architecture_info_table_size();

        if (!(architectures << (bit_size - architecture_info_size))) {
            return false;
        }

        // Find first architecture to write out, then
        // write out the rest with a leading comma

        auto first_architecture_index = uint64_t();
        for (; first_architecture_index < architecture_info_size; first_architecture_index++) {
            if (!(architectures & (1ull << first_architecture_index))) {
                continue;
            }

            break;
        }

        if (first_architecture_index == architecture_info_size) {
            return false;
        }

        const auto architecture_info = architecture_info_from_index(first_architecture_index);
        if (dash) {
            if (fprintf(file, "  - archs:%-14s[ %s", "", architecture_info->name) == -1) {
                return false;
            }
        } else if (fprintf(file, "archs:%-17s[ %s", "", architecture_info->name) == -1) {
            return false;
        }

        for (auto index = first_architecture_index + 1; index < architecture_info_size; index++) {
            if (!(architectures & (1ull << index))) {
                continue;
            }

            const auto architecture_info = architecture_info_from_index(index);
            if (fprintf(file, ", %s", architecture_info->name) == -1) {
                return false;
            }
        }

        if (fputs(" ]\n", file) == -1) {
            return false;
        }

        return true;
    }

    bool write_compatibility_version_to_file(int descriptor, const tbd::packed_version &version) noexcept {
        if (dprintf(descriptor, "compatibility-version: ") == -1) {
            return false;
        }

        return write_packed_version_to_file(descriptor, version);
    }

    bool write_compatibility_version_to_file(FILE *file, const tbd::packed_version &version) noexcept {
        if (fputs("compatibility-version: ", file) == -1) {
            return false;
        }

        return write_packed_version_to_file(file, version);
    }

    bool write_current_version_to_file(int descriptor, const tbd::packed_version &version) noexcept {
        if (dprintf(descriptor, "current-version:%-7s", "") == -1) {
            return false;
        }

        return write_packed_version_to_file(descriptor, version);
    }

    bool write_current_version_to_file(FILE *file, const tbd::packed_version &version) noexcept {
        if (fprintf(file, "current-version:%-7s", "") == -1) {
            return false;
        }

        return write_packed_version_to_file(file, version);
    }

    tbd::write_result write_exports_to_file(const tbd &tbd, int descriptor, const tbd::write_options &options, const std::vector<tbd::export_group> &groups) noexcept {
        // Don't check if options.ignore_exports() as
        // caller is supposed to check if it's configured

        if (options.ignores_reexports() && options.ignores_normal_symbols() && options.ignores_weak_symbols() &&
            options.ignores_objc_class_symbols() && options.ignores_objc_ivar_symbols()) {
            return tbd::write_result::ok;
        }

        if (tbd.reexports.empty() && tbd.symbols.empty()) {
            if (options.enforces_has_exports()) {
                return tbd::write_result::has_no_exports;
            }

            return tbd::write_result::ok;
        }

        if (groups.empty()) {
            if (options.enforces_has_exports()) {
                return tbd::write_result::has_no_exports;
            }

            return tbd::write_result::ok;
        }

        if (dprintf(descriptor, "exports:\n") == -1) {
            return tbd::write_result::failed_to_write_exports;
        }

        for (const auto &group : groups) {
            const auto result = write_group_to_file(tbd, descriptor, group, options);
            if (result == tbd::write_result::ok) {
                continue;
            }

            return result;
        }

        return tbd::write_result::ok;
    }

    tbd::write_result write_exports_to_file(const tbd &tbd, FILE *file, const tbd::write_options &options, const std::vector<tbd::export_group> &groups) noexcept {
        // Don't check if options.ignore_exports() as
        // caller is supposed to check if it's configured

        if (options.ignores_reexports() && options.ignores_normal_symbols() && options.ignores_weak_symbols() &&
            options.ignores_objc_class_symbols() && options.ignores_objc_ivar_symbols()) {
            return tbd::write_result::ok;
        }

        if (tbd.reexports.empty() && tbd.symbols.empty()) {
            if (options.enforces_has_exports()) {
                return tbd::write_result::has_no_exports;
            }

            return tbd::write_result::ok;
        }

        if (groups.empty()) {
            if (options.enforces_has_exports()) {
                return tbd::write_result::has_no_exports;
            }

            return tbd::write_result::ok;
        }

        for (const auto &group : groups) {
            const auto result = write_group_to_file(tbd, file, group, options);
            if (result == tbd::write_result::ok) {
                continue;
            }

            return result;
        }

        return tbd::write_result::ok;
    }

    bool write_flags_to_file(int descriptor, const struct tbd::flags &flags) {
        if (flags.has_none()) {
            return true;
        }

        if (dprintf(descriptor, "flags:%-17s[ ", "") == -1) {
            return false;
        }

        if (flags.has_flat_namespace()) {
            if (dprintf(descriptor, "flat_namespace") == -1) {
                return false;
            }
        }

        if (flags.is_not_app_extension_safe()) {
            if (flags.has_flat_namespace()) {
                if (dprintf(descriptor, ", ") == -1) {
                    return false;
                }
            }

            if (dprintf(descriptor, "not_app_extension_safe") == -1) {
                return false;
            }
        }

        if (dprintf(descriptor, " ]\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_flags_to_file(FILE *file, const struct tbd::flags &flags) {
        if (flags.has_none()) {
            return true;
        }

        if (fprintf(file, "flags:%-17s[ ", "") == -1) {
            return false;
        }

        if (flags.has_flat_namespace()) {
            if (fputs("flat_namespace", file) == -1) {
                return false;
            }
        }

        if (flags.is_not_app_extension_safe()) {
            if (flags.has_flat_namespace()) {
                if (fputs(", ", file) == -1) {
                    return false;
                }
            }

            if (fputs("not_app_extension_safe", file) == -1) {
                return false;
            }
        }

        if (fputs(" ]\n", file) == -1) {
            return false;
        }

        return true;
    }

    bool write_footer_to_file(int descriptor) noexcept {
        return dprintf(descriptor, "...\n") != -1;
    }

    bool write_footer_to_file(FILE *file) noexcept {
        return fputs("...\n", file) != -1;
    }

    bool write_header_to_file(int descriptor, const enum tbd::version &version) noexcept {
        if (dprintf(descriptor, "---") == -1) {
            return false;
        }

        if (version == tbd::version::v2) {
            if (dprintf(descriptor, " !tapi-tbd-v2") == -1) {
                return false;
            }
        }

        if (dprintf(descriptor, "\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_header_to_file(FILE *file, const enum tbd::version &version) noexcept {
        if (fputs("---", file) == -1) {
            return false;
        }

        if (version == tbd::version::v2) {
            if (fputs(" !tapi-tbd-v2", file) == -1) {
                return false;
            }
        }

        if (fputc('\n', file) == -1) {
            return false;
        }

        return true;
    }

    bool write_install_name_to_file(int descriptor, const std::string &install_name) noexcept {
        if (install_name.empty()) {
            return true;
        }

        return dprintf(descriptor, "install-name:%-10s%s\n", "", install_name.c_str()) != -1;
    }

    bool write_install_name_to_file(FILE *file, const std::string &install_name) noexcept {
        if (install_name.empty()) {
            return true;
        }

        return fprintf(file, "install-name:%-10s%s\n", "", install_name.c_str()) != -1;
    }

    tbd::write_result write_group_to_file(const tbd &tbd, int descriptor, const tbd::export_group &group, const tbd::write_options &options) noexcept {
        // We have to check that atleast one category is written because
        // we need to write the architectures list of this group first
        // and we can't just write architectures and nothing else

        if (options.ignores_allowable_clients() && options.ignores_reexports() &&
            options.ignores_normal_symbols() && options.ignores_weak_symbols() &&
            options.ignores_objc_class_symbols() && options.ignores_objc_ivar_symbols()) {
            return tbd::write_result::ok;
        }

        const auto architectures = group.architectures();
        const auto symbols_end = tbd.symbols.cend();

        auto group_symbols_begin = symbols_end;
        if (group.symbol != nullptr) {
            group_symbols_begin = tbd.symbols.cbegin() + std::distance(tbd.symbols.data(), group.symbol);
        } else {
            group_symbols_begin = std::find(tbd.symbols.cbegin(), symbols_end, group.architectures());
        }

        auto normal_symbols_iter = symbols_end;
        auto objc_class_symbols_iter = symbols_end;
        auto objc_ivar_symbols_iter = symbols_end;
        auto weak_symbols_iter = symbols_end;

        if (group_symbols_begin != symbols_end) {
            const auto symbols_begin = group_symbols_begin + 1;

            if (group.symbol != nullptr) {
                switch (group.symbol->type) {
                    case tbd::symbol::type::normal:
                        normal_symbols_iter = group_symbols_begin;
                        break;

                    case tbd::symbol::type::objc_class:
                        objc_class_symbols_iter = group_symbols_begin;
                        break;

                    case tbd::symbol::type::objc_ivar:
                        objc_ivar_symbols_iter = group_symbols_begin;
                        break;

                    case tbd::symbol::type::weak:
                        weak_symbols_iter = group_symbols_begin;
                        break;
                }
            }

            const auto architectures = group.architectures();
            if (normal_symbols_iter != group_symbols_begin) {
                normal_symbols_iter = next_iterator_for_symbol(symbols_begin, symbols_end, architectures, tbd::symbol::type::normal);
            }

            if (objc_class_symbols_iter != group_symbols_begin) {
                objc_class_symbols_iter = next_iterator_for_symbol(symbols_begin, symbols_end, architectures, tbd::symbol::type::objc_class);
            }

            if (objc_ivar_symbols_iter != group_symbols_begin) {
                objc_ivar_symbols_iter = next_iterator_for_symbol(symbols_begin, symbols_end, architectures, tbd::symbol::type::objc_ivar);
            }

            if (weak_symbols_iter != group_symbols_begin) {
                weak_symbols_iter = next_iterator_for_symbol(symbols_begin, symbols_end, architectures, tbd::symbol::type::weak);
            }
        }

        // Like above, we have to check that atleast one category is written
        // because we need to write the architectures list of this group first
        // and we can't just write architectures and nothing else

        // We need this check because although group is guaranteed to have one
        // symbol, it may have been ignored by its type and options, so we need
        // to check each

        const auto reexports_begin = tbd.reexports.cbegin();
        const auto reexports_end = tbd.reexports.cend();

        auto reexports_iter = reexports_end;
        if (group.reexport != nullptr) {
            reexports_iter = reexports_begin + std::distance(tbd.reexports.data(), group.reexport);
        } else {
            reexports_iter = std::find(reexports_begin, reexports_end, architectures);
        }

        if ((!options.ignores_reexports() && reexports_iter == reexports_end) &&
            (!options.ignores_normal_symbols() && normal_symbols_iter == symbols_end) &&
            (!options.ignores_objc_class_symbols() && objc_class_symbols_iter == symbols_end) &&
            (!options.ignores_objc_ivar_symbols() && objc_ivar_symbols_iter == symbols_end) &&
            (!options.ignores_weak_symbols() && weak_symbols_iter == symbols_end)) {
            return tbd::write_result::ok;
        }

        if (!write_architectures_to_file(descriptor, architectures, true)) {
            return tbd::write_result::failed_to_write_architectures;
        }

        if (reexports_iter != reexports_end) {
            if (!write_reexports_array_to_file(descriptor, reexports_iter, reexports_end, architectures)) {
                return tbd::write_result::failed_to_write_reexports;
            }
        }

        if (normal_symbols_iter != symbols_end) {
            if (!write_normal_symbols_array_to_file(descriptor, normal_symbols_iter, symbols_end, architectures)) {
                return tbd::write_result::failed_to_write_normal_symbols;
            }
        }

        if (objc_class_symbols_iter != symbols_end) {
            if (!write_objc_class_symbols_array_to_file(descriptor, objc_class_symbols_iter, symbols_end, architectures)) {
                return tbd::write_result::failed_to_write_objc_class_symbols;
            }
        }

        if (objc_ivar_symbols_iter != symbols_end) {
            if (!write_objc_ivar_symbols_array_to_file(descriptor, objc_ivar_symbols_iter, symbols_end, architectures)) {
                return tbd::write_result::failed_to_write_objc_ivar_symbols;
            }
        }

        if (weak_symbols_iter != symbols_end) {
            if (!write_weak_symbols_array_to_file(descriptor, weak_symbols_iter, symbols_end, architectures)) {
                return tbd::write_result::failed_to_write_weak_symbols;
            }
        }

        return tbd::write_result::ok;
    }

    tbd::write_result write_group_to_file(const tbd &tbd, FILE *file, const tbd::export_group &group, const tbd::write_options &options) noexcept {
        // We have to check that atleast one category is written because
        // we need to write the architectures list of this group first
        // and we can't just write architectures and nothing else

        if (options.ignores_allowable_clients() && options.ignores_reexports() &&
            options.ignores_normal_symbols() && options.ignores_weak_symbols() &&
            options.ignores_objc_class_symbols() && options.ignores_objc_ivar_symbols()) {
            return tbd::write_result::ok;
        }

        const auto architectures = group.architectures();
        const auto symbols_end = tbd.symbols.cend();

        auto group_symbols_begin = symbols_end;
        if (group.symbol != nullptr) {
            group_symbols_begin = tbd.symbols.cbegin() + std::distance(tbd.symbols.data(), group.symbol);
        } else {
            group_symbols_begin = std::find(tbd.symbols.cbegin(), symbols_end, group.architectures());
        }

        auto normal_symbols_iter = symbols_end;
        auto objc_class_symbols_iter = symbols_end;
        auto objc_ivar_symbols_iter = symbols_end;
        auto weak_symbols_iter = symbols_end;

        if (group_symbols_begin != symbols_end) {
            const auto symbols_begin = group_symbols_begin + 1;

            if (group.symbol != nullptr) {
                switch (group.symbol->type) {
                    case tbd::symbol::type::normal:
                        normal_symbols_iter = group_symbols_begin;
                        break;

                    case tbd::symbol::type::objc_class:
                        objc_class_symbols_iter = group_symbols_begin;
                        break;

                    case tbd::symbol::type::objc_ivar:
                        objc_ivar_symbols_iter = group_symbols_begin;
                        break;

                    case tbd::symbol::type::weak:
                        weak_symbols_iter = group_symbols_begin;
                        break;
                }
            }

            const auto architectures = group.architectures();
            if (normal_symbols_iter != group_symbols_begin) {
                normal_symbols_iter = next_iterator_for_symbol(symbols_begin, symbols_end, architectures, tbd::symbol::type::normal);
            }

            if (objc_class_symbols_iter != group_symbols_begin) {
                objc_class_symbols_iter = next_iterator_for_symbol(symbols_begin, symbols_end, architectures, tbd::symbol::type::objc_class);
            }

            if (objc_ivar_symbols_iter != group_symbols_begin) {
                objc_ivar_symbols_iter = next_iterator_for_symbol(symbols_begin, symbols_end, architectures, tbd::symbol::type::objc_ivar);
            }

            if (weak_symbols_iter != group_symbols_begin) {
                weak_symbols_iter = next_iterator_for_symbol(symbols_begin, symbols_end, architectures, tbd::symbol::type::weak);
            }
        }

        // Like above, we have to check that atleast one category is written
        // because we need to write the architectures list of this group first
        // and we can't just write architectures and nothing else

        // We need this check because although group is guaranteed to have one
        // symbol, it may have been ignored by its type and options, so we need
        // to check each

        const auto reexports_begin = tbd.reexports.cbegin();
        const auto reexports_end = tbd.reexports.cend();

        auto reexports_iter = reexports_end;
        if (group.reexport != nullptr) {
            reexports_iter = reexports_begin + std::distance(tbd.reexports.data(), group.reexport);
        } else {
            reexports_iter = std::find(reexports_begin, reexports_end, architectures);
        }

        if ((!options.ignores_reexports() && reexports_iter == reexports_end) &&
            (!options.ignores_normal_symbols() && normal_symbols_iter == symbols_end) &&
            (!options.ignores_objc_class_symbols() && objc_class_symbols_iter == symbols_end) &&
            (!options.ignores_objc_ivar_symbols() && objc_ivar_symbols_iter == symbols_end) &&
            (!options.ignores_weak_symbols() && weak_symbols_iter == symbols_end)) {
            return tbd::write_result::ok;
        }

        if (!write_architectures_to_file(file, architectures, true)) {
            return tbd::write_result::ok;
        }

        if (reexports_iter != reexports_end) {
            if (!write_reexports_array_to_file(file, reexports_iter, reexports_end, architectures)) {
                return tbd::write_result::failed_to_write_reexports;
            }
        }

        if (normal_symbols_iter != symbols_end) {
            if (!write_normal_symbols_array_to_file(file, normal_symbols_iter, symbols_end, architectures)) {
                return tbd::write_result::failed_to_write_normal_symbols;
            }
        }

        if (objc_class_symbols_iter != symbols_end) {
            if (!write_objc_class_symbols_array_to_file(file, objc_class_symbols_iter, symbols_end, architectures)) {
                return tbd::write_result::failed_to_write_objc_class_symbols;
            }
        }

        if (objc_ivar_symbols_iter != symbols_end) {
            if (!write_objc_ivar_symbols_array_to_file(file, objc_ivar_symbols_iter, symbols_end, architectures)) {
                return tbd::write_result::failed_to_write_objc_ivar_symbols;
            }
        }

        if (weak_symbols_iter != symbols_end) {
            if (!write_weak_symbols_array_to_file(file, weak_symbols_iter, symbols_end, architectures)) {
                return tbd::write_result::failed_to_write_weak_symbols;
            }
        }

        return tbd::write_result::ok;
    }

    bool write_packed_version_to_file(int descriptor, const tbd::packed_version &version) noexcept {
        if (dprintf(descriptor, "%u", version.components.major) == -1) {
            return false;
        }

        if (version.components.minor != 0) {
            if (dprintf(descriptor, ".%u", version.components.minor) == -1) {
                return false;
            }
        }

        if (version.components.revision != 0) {
            if (version.components.minor == 0) {
                if (dprintf(descriptor, ".0") == -1) {
                    return false;
                }
            }

            if (dprintf(descriptor, ".%u", version.components.revision) == -1) {
                return false;
            }
        }

        if (dprintf(descriptor, "\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_packed_version_to_file(FILE *file, const tbd::packed_version &version) noexcept {
        if (fprintf(file, "%u", version.components.major) == -1) {
            return false;
        }

        if (version.components.minor != 0) {
            if (fprintf(file, ".%u", version.components.minor) == -1) {
                return false;
            }
        }

        if (version.components.revision != 0) {
            if (version.components.minor == 0) {
                if (fputs(".0", file) == -1) {
                    return false;
                }
            }

            if (fprintf(file, ".%u", version.components.revision) == -1) {
                return false;
            }
        }

        if (fputc('\n', file) == -1) {
            return false;
        }

        return true;
    }

    bool write_parent_umbrella_to_file(int descriptor, const std::string &parent_umbrella) noexcept {
        if (parent_umbrella.empty()) {
            return true;
        }

        return dprintf(descriptor, "parent-umbrella:%-7s%s\n", "", parent_umbrella.c_str()) != -1;
    }

    bool write_parent_umbrella_to_file(FILE *file, const std::string &parent_umbrella) noexcept {
        if (parent_umbrella.empty()) {
            return true;
        }

        return fprintf(file, "parent-umbrella:%-7s%s\n", "", parent_umbrella.c_str()) != -1;
    }

    bool write_platform_to_file(int descriptor, const enum tbd::platform &platform) noexcept {
        if (platform == tbd::platform::none) {
            return true;
        }

        return dprintf(descriptor, "platform:%-14s%s\n", "", tbd::platform_to_string(platform)) != -1;
    }

    bool write_platform_to_file(FILE *file, const enum tbd::platform &platform) noexcept {
        if (platform == tbd::platform::none) {
            return true;
        }

        return fprintf(file, "platform:%-14s%s\n", "", tbd::platform_to_string(platform)) != -1;
    }

    bool write_objc_constraint_to_file(int descriptor, const enum tbd::objc_constraint &constraint) noexcept {
        if (constraint == tbd::objc_constraint::none) {
            return true;
        }

        return dprintf(descriptor, "objc-constraint:%-7s%s\n", "", tbd::objc_constraint_to_string(constraint));
    }

    bool write_objc_constraint_to_file(FILE *file, const enum tbd::objc_constraint &constraint) noexcept {
        if (constraint == tbd::objc_constraint::none) {
            return true;
        }

        return fprintf(file, "objc-constraint:%-7s%s\n", "", tbd::objc_constraint_to_string(constraint)) != -1;
    }

    bool write_swift_version_to_file(int descriptor, const uint32_t &version) noexcept {
        if (version == 0) {
            return true;
        }

        if (dprintf(descriptor, "swift-version:%-9s", "") == -1) {
            return false;
        }

        switch (version) {
            case 1:
                if (dprintf(descriptor, "1\n") == -1) {
                    return false;
                }

                break;

            case 2:
                if (dprintf(descriptor, "1.2\n") == -1) {
                    return false;
                }

                break;

            default:
                if (dprintf(descriptor, "%d\n", version - 1) == -1) {
                    return false;
                }

                break;
        }

        return true;
    }

    bool write_swift_version_to_file(FILE *file, const uint32_t &version) noexcept {
        if (version == 0) {
            return true;
        }

        if (fprintf(file, "swift-version:%-9s", "") == -1) {
            return false;
        }

        switch (version) {
            case 1:
                if (fputs("1\n", file) == -1) {
                    return false;
                }

                break;

            case 2:
                if (fputs("1.2\n", file) == -1) {
                    return false;
                }

                break;

            default:
                if (fprintf(file, "%d\n", version - 1) == -1) {
                    return false;
                }

                break;
        }

        return true;
    }

    bool write_string_for_array_to_file(int descriptor, const std::string &string, size_t &line_length) noexcept {
        const auto total_string_length = string.length() + 2; // comma + trailing space

        // Go to a new-line either if the current-line is or is about to be
        // used up, or if the current-string itself is larger than line-length
        // max

        // For strings that larger than line-length-max, they are to be on a separate line
        // by themselves, serving as the only exception to the rule that lines can go upto
        // a certain length

        auto new_line_length = line_length + total_string_length;
        if (line_length != 0) {
            if (new_line_length >= line_length_max) {
                if (dprintf(descriptor, ",\n%-26s", "") == -1) {
                    return false;
                }

                new_line_length = total_string_length;
            } else {
                if (dprintf(descriptor, ", ") == -1) {
                    return false;
                }
            }
        }

        // Strings are printed with quotations
        // if they start out with $ld in Apple's
        // official, not sure if needed though

        if (strncmp(string.c_str(), "$ld", 3) == 0) {
            if (dprintf(descriptor, "\'%s\'", string.c_str()) == -1) {
                return false;
            }
        } else {
            if (dprintf(descriptor, "%s", string.c_str()) == -1) {
                return false;
            }
        }

        line_length = new_line_length;
        return true;
    }

    bool write_string_for_array_to_file(FILE *file_stream, const std::string &string, size_t &line_length) noexcept {
        const auto total_string_length = string.length() + 2; // comma + trailing space

        // Go to a new-line either if the current-line is or is about to be
        // used up, or if the current-string itself is larger than line-length
        // max

        // For strings that larger than line-length-max, they are to be on a separate line
        // by themselves, serving as the only exception to the rule that lines can go upto
        // a certain length

        auto new_line_length = line_length + total_string_length;
        if (line_length != 0) {
            if (new_line_length >= line_length_max) {
                if (fprintf(file_stream, ",\n%-26s", "") == -1) {
                    return false;
                }

                new_line_length = total_string_length;
            } else {
                if (fputs(", ", file_stream) == -1) {
                    return false;
                }
            }
        }

        // Strings are printed with quotations
        // if they start out with $ld in Apple's
        // official, not sure if needed though

        if (strncmp(string.c_str(), "$ld", 3) == 0) {
            if (fprintf(file_stream, "\'%s\'", string.c_str()) == -1) {
                return false;
            }
        } else {
            if (fputs(string.c_str(), file_stream) == -1) {
                return false;
            }
        }

        line_length = new_line_length;
        return true;
    }

    bool write_uuids_to_file(int descriptor, const std::vector<tbd::uuid_pair> &uuids, const tbd::write_options &options) noexcept {
        if (uuids.empty()) {
            return true;
        }

        if (dprintf(descriptor, "uuids:%-17s[ ", "") == -1) {
            return false;
        }

        auto tracker = uint64_t();

        const auto uuids_begin = uuids.cbegin();
        const auto uuids_end = uuids.cend();

        const auto uuids_size = uuids.size();

        if (options.orders_by_architecture_info_table()) {
            auto null_architectures_pair = tbd::uuid_pair();
            null_architectures_pair.architecture = nullptr;

            auto last_pair = const_cast<const tbd::uuid_pair *>(&null_architectures_pair);
            auto pair = uuids_begin;

            do {
                for (auto iter = uuids_begin; iter != uuids_end; iter++) {
                    if (pair->architecture < iter->architecture) {
                        continue;
                    }

                    if (last_pair->architecture >= iter->architecture) {
                        continue;
                    }

                    pair = iter;
                }

                const auto &uuid = pair->uuid();
                const auto result = dprintf(descriptor, "'%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X'", pair->architecture->name,
                                            uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8],
                                            uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
                if (result == -1) {
                    return false;
                }

                tracker++;
                if (tracker != uuids_size) {
                    if (dprintf(descriptor, ", ") == -1) {
                        return false;
                    }

                    if (!(tracker & 1)) {
                        if (dprintf(descriptor, "%-26s", "\n") == -1) {
                            return false;
                        }
                    }
                }

                last_pair = pair.base();
                pair = uuids_begin;
            } while (tracker != uuids_size);
        } else {
            for (auto pair = uuids.cbegin(); pair != uuids_end; pair++) {
                const auto &uuid = pair->uuid();
                const auto result = dprintf(descriptor, "'%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X'", pair->architecture->name,
                                            uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8],
                                            uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
                if (result == -1) {
                    return false;
                }

                tracker++;
                if (tracker != uuids_size) {
                    if (dprintf(descriptor, ", ") == -1) {
                        return false;
                    }

                    if (!(tracker & 1)) {
                        if (dprintf(descriptor, "%-26s", "\n") == -1) {
                            return false;
                        }
                    }
                }
            }
        }

        dprintf(descriptor, " ]\n");
        return true;
    }

    bool write_uuids_to_file(FILE *file, const std::vector<tbd::uuid_pair> &uuids, const tbd::write_options &options) noexcept {
        if (uuids.empty()) {
            return true;
        }

        if (fprintf(file, "uuids:%-17s[ ", "") == -1) {
            return false;
        }

        // Keep track of number of architecture-uuid pairs
        // printed as we only allow 2 per line

        auto tracker = uint64_t();

        const auto uuids_begin = uuids.cbegin();
        const auto uuids_end = uuids.cend();

        const auto uuids_size = uuids.size();

        if (options.orders_by_architecture_info_table()) {
            auto null_architectures_pair = tbd::uuid_pair();
            null_architectures_pair.architecture = nullptr;

            auto last_pair = const_cast<const tbd::uuid_pair *>(&null_architectures_pair);
            auto pair = uuids_begin;

            do {
                for (auto iter = uuids_begin; iter != uuids_end; iter++) {
                    if (pair->architecture < iter->architecture) {
                        continue;
                    }

                    if (last_pair->architecture >= iter->architecture) {
                        continue;
                    }

                    pair = iter;
                }

                const auto &uuid = pair->uuid();
                const auto result = fprintf(file, "'%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X'", pair->architecture->name,
                                            uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8],
                                            uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
                if (result == -1) {
                    return false;
                }

                tracker++;
                if (tracker != uuids_size) {
                    if (fputs(", ", file) == -1) {
                        return false;
                    }

                    if (!(tracker & 1)) {
                        if (fprintf(file, "%-26s", "\n") == -1) {
                            return false;
                        }
                    }
                }

                last_pair = pair.base();
                pair = uuids_begin;
            } while (tracker != uuids_size);
        } else {
            for (auto pair = uuids.cbegin(); pair != uuids_end; pair++) {
                const auto &uuid = pair->uuid();
                const auto result = fprintf(file, "'%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X%.2X%.2X%.2X%.2X'", pair->architecture->name,
                                            uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8],
                                            uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15]);
                if (result == -1) {
                    return false;
                }

                tracker++;
                if (tracker != uuids_size) {
                    if (fputs(", ", file) == -1) {
                        return false;
                    }

                    if (!(tracker & 1)) {
                        if (fprintf(file, "%-26s", "\n") == -1) {
                            return false;
                        }
                    }
                }
            }
        }

        fputs(" ]\n", file);
        return true;
    }

    bool write_reexports_array_to_file(int descriptor, const std::vector<tbd::reexport>::const_iterator &begin, const std::vector<tbd::reexport>::const_iterator &end, uint64_t architectures) noexcept {
        if (dprintf(descriptor, "%-4sre-exports:%9s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(descriptor, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = std::find(iter, end, architectures);
        }

        if (dprintf(descriptor, " ]\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_reexports_array_to_file(FILE *file, const std::vector<tbd::reexport>::const_iterator &begin, const std::vector<tbd::reexport>::const_iterator &end, uint64_t architectures) noexcept {
        if (fprintf(file, "%-4sre-exports:%9s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(file, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = std::find(iter, end, architectures);
        }

        if (fputs(" ]\n", file) == -1) {
            return false;
        }

        return true;
    }

    bool write_normal_symbols_array_to_file(int descriptor, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept {
        if (dprintf(descriptor, "%-4ssymbols:%12s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(descriptor, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = next_iterator_for_symbol(iter, end, architectures, tbd::symbol::type::normal);
        }

        if (dprintf(descriptor, " ]\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_normal_symbols_array_to_file(FILE *file, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept {
        if (fprintf(file, "%-4ssymbols:%12s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(file, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = next_iterator_for_symbol(iter, end, architectures, tbd::symbol::type::normal);
        }

        if (fputs(" ]\n", file) == -1) {
            return false;
        }

        return true;
    }

    bool write_objc_class_symbols_array_to_file(int descriptor, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept {
        if (dprintf(descriptor, "%-4sobjc-classes:%7s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(descriptor, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = next_iterator_for_symbol(iter, end, architectures, tbd::symbol::type::objc_class);
        }

        if (dprintf(descriptor, " ]\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_objc_class_symbols_array_to_file(FILE *file, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept {
        if (fprintf(file, "%-4sobjc-classes:%7s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(file, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = next_iterator_for_symbol(iter, end, architectures, tbd::symbol::type::objc_class);
        }

        if (fputs(" ]\n", file) == -1) {
            return false;
        }

        return true;
    }

    bool write_objc_ivar_symbols_array_to_file(int descriptor, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept {
        if (dprintf(descriptor, "%-4sobjc-ivars:%9s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(descriptor, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = next_iterator_for_symbol(iter, end, architectures, tbd::symbol::type::objc_ivar);
        }

        if (dprintf(descriptor, " ]\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_objc_ivar_symbols_array_to_file(FILE *file, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept {
        if (fprintf(file, "%-4sobjc-ivars:%9s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(file, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = next_iterator_for_symbol(iter, end, architectures, tbd::symbol::type::objc_ivar);
        }

        if (fputs(" ]\n", file) == -1) {
            return false;
        }

        return true;
    }

    bool write_weak_symbols_array_to_file(int descriptor, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept {
        if (dprintf(descriptor, "%-4sweak-def-symbols:%3s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(descriptor, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = next_iterator_for_symbol(iter, end, architectures, tbd::symbol::type::weak);
        }

        if (dprintf(descriptor, " ]\n") == -1) {
            return false;
        }

        return true;
    }

    bool write_weak_symbols_array_to_file(FILE *file, const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures) noexcept {
        if (fprintf(file, "%-4sweak-def-symbols:%3s[ ", "", "") == -1) {
            return false;
        }

        auto line_length = size_t();
        for (auto iter = begin; iter != end; ) {
            if (!write_string_for_array_to_file(file, iter->string, line_length)) {
                return false;
            }

            iter++;
            iter = next_iterator_for_symbol(iter, end, architectures, tbd::symbol::type::weak);
        }

        if (fputs(" ]\n", file) == -1) {
            return false;
        }

        return true;
    }

    std::vector<tbd::symbol>::const_iterator next_iterator_for_symbol(const std::vector<tbd::symbol>::const_iterator &begin, const std::vector<tbd::symbol>::const_iterator &end, uint64_t architectures, enum tbd::symbol::type type) noexcept {
        auto iter = begin;
        for (; iter != end; iter++) {
            if (iter->type != type) {
                continue;
            }

            if (iter->architectures != architectures) {
                continue;
            }

            break;
        }

        return iter;
    }
}
