//
//  src/mach-o/utils/tbd.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

#include "../../utils/bits.h"

#include "../architecture_info.h"
#include "../file.h"

#include "load_commands/data.h"
#include "strings/table/data.h"

#include "symbols/table/data.h"
#include "symbols/table_64/data.h"

namespace macho::utils {
    struct tbd {
        struct flags {
            enum class shifts {
                flat_namespace,
                not_app_extension_safe
            };

            uint64_t bits = uint64_t();

            inline bool has_none() const noexcept {
                return this->bits == 0;
            }

            inline bool has_shift(shifts shift) const noexcept {
                return bits & (1ull << static_cast<uint64_t>(shift));
            }

            inline bool has_flat_namespace() const noexcept {
                return has_shift(shifts::flat_namespace);
            }

            inline bool is_not_app_extension_safe() const noexcept {
                return has_shift(shifts::not_app_extension_safe);
            }

            inline void add_shift(shifts shift) noexcept {
                bits |= (1ull << static_cast<uint64_t>(shift));
            }

            inline void set_has_flat_namespace() noexcept {
                add_shift(shifts::flat_namespace);
            }

            inline void set_not_app_extension_safe() noexcept {
                add_shift(shifts::not_app_extension_safe);
            }

            inline void remove_shift(shifts shift) noexcept {
                bits &= ~(1 << static_cast<uint64_t>(shift));
            }

            inline void clear() noexcept {
                this->bits = 0;
            }

            inline void set_doesnt_have_flat_namespace() noexcept {
                remove_shift(shifts::flat_namespace);
            }

            inline void set_app_extension_safe() noexcept {
                remove_shift(shifts::not_app_extension_safe);
            }

            inline bool operator==(const uint64_t bits) const noexcept {
                return this->bits == bits;
            }

            inline bool operator!=(const uint64_t bits) const noexcept {
                return this->bits != bits;
            }

            inline bool operator==(const flags &flags) const noexcept {
                return this->bits == flags.bits;
            }

            inline bool operator!=(const flags &flags) const noexcept {
                return this->bits != flags.bits;
            }
        };

        struct reexport;
        struct symbol;
        
        struct export_group {
            explicit export_group(const reexport *reexport) noexcept;
            explicit export_group(const symbol *symbol) noexcept;

            // Store a pointer to the first symbol
            // to appear in the group which contains
            // all the information we need with no
            // extra cost

            const reexport *reexport = nullptr;
            const symbol *symbol = nullptr;

            inline uint64_t architectures() const noexcept {
                if (this->reexport != nullptr) {
                    return reexport->architectures;
                }
                
                if (this->symbol != nullptr) {
                    return this->symbol->architectures;
                }
                
                return 0;
            }

            inline const ::utils::bits &containers() const noexcept {
                return symbol->containers;
            }
            
            inline bool operator==(const struct reexport &reexport) const noexcept {
                return this->architectures() == reexport.architectures;
            }
            
            inline bool operator!=(const struct reexport &reexport) const noexcept {
                return this->architectures() != reexport.architectures;
            }

            inline bool operator==(const struct symbol &symbol) const noexcept {
                return this->architectures() == symbol.architectures;
            }

            inline bool operator!=(const struct symbol &symbol) const noexcept {
                return this->architectures() != symbol.architectures;
            }
        };

        enum class platform {
            none,

            aix,
            amdhsa,
            ananas,
            cloudabi,
            cnk,
            contiki,
            cuda,
            darwin,
            dragonfly,
            elfiamcu,
            freebsd,
            fuchsia,
            haiku,
            ios,
            kfreebsd,
            linux,
            lv2,
            macosx,
            mesa3d,
            minix,
            nacl,
            netbsd,
            nvcl,
            openbsd,
            ps4,
            rtems,
            solaris,
            tvos,
            watchos,
            windows,
        };

        static enum platform platform_from_string(const char *string) noexcept;
        static const char *platform_to_string(const platform &platform) noexcept;

        union packed_version {
            struct components {
                uint8_t revision;
                uint8_t minor;

                uint16_t major;
            } __attribute__((packed)) components;

            uint32_t value;
        };

        enum class objc_constraint {
            none,

            retain_release,
            retain_release_for_simulator,
            retain_release_or_gc,

            gc
        };

        static enum objc_constraint objc_constraint_from_string(const char *string) noexcept;
        static const char *objc_constraint_to_string(const objc_constraint &constraint) noexcept;

        // reexport, sub_client, and symbol are the three structures holding data
        // that serve as the core of a .tbd file, forming an export-group.

        // All three maintain a bitset of indexs of containers which they are
        // mutually found in.

        // To help support the output of tbd, through tbd-write, We also maintain
        // a bitset of indexes of architecture-infos of the containers which they
        // are found in, note that this is different from the containers, as multiple
        // containers can have the same architecture-info, which result in a mesh of
        // their information, for now the default behavior.

        // tbd-create writes to the architecture-infos bitset but never uses or reads it,
        // while for tbd-write the opposite is true, atleast for using and reading, using
        // and reading only architecture-infos indexs bitset and ignoring the container indexes
        // bitset entirely

        struct reexport {
            explicit reexport() = default;
            explicit reexport(uint64_t architectures, ::utils::bits &containers, std::string &string) noexcept;
            explicit reexport(uint64_t architectures, ::utils::bits &&containers, std::string &&string) noexcept;

            uint64_t architectures = uint64_t();
            ::utils::bits containers;

            std::string string;

            inline bool has_architecture(uint64_t index) const noexcept {
                return this->architectures & 1ull << index;
            }

            inline void add_architecture(uint64_t index) noexcept {
                this->architectures |= 1ull << index;
            }

            inline bool has_container(uint64_t index) const noexcept {
                return this->containers.at(index);
            }

            void add_container(uint64_t index) noexcept {
                this->containers.cast(index, true);
            }

            inline bool operator==(const uint64_t &architectures) const noexcept {
                return this->architectures == architectures;
            }

            inline bool operator!=(const uint64_t &architectures) const noexcept {
                return this->architectures != architectures;
            }

            inline bool operator==(const ::utils::bits &containers) const noexcept {
                return this->containers == containers;
            }

            inline bool operator!=(const ::utils::bits &containers) const noexcept {
                return this->containers != containers;
            }

            inline bool operator==(const std::string &string) const noexcept {
                return this->string == string;
            }

            inline bool operator!=(const std::string &string) const noexcept {
                return this->string != string;
            }

            inline bool operator==(const reexport &reexport) const noexcept {
                return *this == reexport.containers && *this == reexport.string;
            }

            inline bool operator!=(const reexport &reexport) const noexcept {
                return *this != reexport.containers || *this != reexport.string;
            }
        };

        struct sub_client {
            explicit sub_client() = default;
            explicit sub_client(uint64_t architectures, ::utils::bits &containers, std::string &client) noexcept;
            explicit sub_client(uint64_t architectures, ::utils::bits &&containers, std::string &&client) noexcept;

            uint64_t architectures = uint64_t();
            ::utils::bits containers;

            std::string client;

            inline bool has_architecture(uint64_t index) const noexcept {
                return this->architectures & 1ull << index;
            }

            inline void add_architecture(uint64_t index) noexcept {
                this->architectures |= 1ull << index;
            }

            inline bool has_container(uint64_t index) const noexcept {
                return this->containers.at(index);
            }

            inline void add_container(uint64_t index) noexcept {
                this->containers.cast(index, true);
            }

            inline bool operator==(const uint64_t &architectures) const noexcept {
                return this->architectures == architectures;
            }

            inline bool operator!=(const uint64_t &architectures) const noexcept {
                return this->architectures != architectures;
            }

            inline bool operator==(const ::utils::bits &containers) const noexcept {
                return this->containers == containers;
            }

            inline bool operator!=(const ::utils::bits &containers) const noexcept {
                return this->containers != containers;
            }

            inline bool operator==(const std::string &client) const noexcept {
                return this->client == client;
            }

            inline bool operator!=(const std::string &client) const noexcept {
                return this->client != client;
            }

            inline bool operator==(const sub_client &client) const noexcept {
                return *this == client.containers && *this == client.client;
            }

            inline bool operator!=(const sub_client &client) const noexcept {
                return *this != client.containers || *this != client.client;
            }
        };

        struct symbol {
            enum class type {
                normal,
                objc_class,
                objc_ivar,
                weak
            };

            static type type_from_symbol_string(const char *& string, uint16_t n_desc) noexcept;

            explicit symbol() = default;
            explicit symbol(uint64_t architectures, ::utils::bits &containers, std::string &string, type type) noexcept;
            explicit symbol(uint64_t architectures, ::utils::bits &&containers, std::string &&string, type type) noexcept;

            uint64_t architectures = uint64_t();
            ::utils::bits containers;

            std::string string;
            type type;

            inline bool has_architecture(uint64_t index) const noexcept {
                return this->architectures & 1ull << index;
            }

            inline void add_architecture(uint64_t index) noexcept {
                this->architectures |= 1ull << index;
            }

            inline bool has_container(uint64_t index) const noexcept {
                return this->containers.at(index);
            }

            inline void add_container(uint64_t index) noexcept {
                this->containers.cast(index, true);
            }

            inline bool operator==(const uint64_t &architectures) const noexcept {
                return this->architectures == architectures;
            }

            inline bool operator!=(const uint64_t &architectures) const noexcept {
                return this->architectures != architectures;
            }

            inline bool operator==(const ::utils::bits &containers) const noexcept {
                return this->containers == containers;
            }

            inline bool operator!=(const ::utils::bits &containers) const noexcept {
                return this->containers != containers;
            }

            inline bool operator==(const std::string &string) const noexcept {
                return this->string == string;;
            }

            inline bool operator!=(const std::string &string) const noexcept {
                return this->string != string;
            }

            inline bool operator==(const symbol &symbol) const noexcept {
                return this->type == type && this->containers == symbol.containers && *this == symbol.string;
            }

            inline bool operator!=(const symbol &symbol) const noexcept {
                return this->type != type || this->containers != symbol.containers || *this != symbol.string;
            }
        };

        struct uuid_pair {
            explicit uuid_pair() = default;

            const architecture_info *architecture = nullptr;
            std::unique_ptr<const uint8_t> unique_uuid;

            inline const uint8_t *uuid() const noexcept {
                return this->unique_uuid.get();
            }

            inline void set_uuid(const uint8_t *uuid) noexcept {
                auto new_uuid = new uint8_t[sizeof(uuid_command::uuid)];
                memcpy(new_uuid, uuid, sizeof(uuid_command::uuid));

                this->unique_uuid.reset(new_uuid);
            }

            inline bool operator==(const architecture_info *architecture) const noexcept {
                return this->architecture == architecture;
            }

            inline bool operator!=(const architecture_info *architecture) const noexcept {
                return this->architecture != architecture;
            }

            inline bool operator==(const uint8_t *uuid) const noexcept {
                return memcmp(this->uuid(), uuid, sizeof(uuid_command::uuid)) == 0;
            }

            inline bool operator!=(const uint8_t *uuid) const noexcept {
                return memcmp(this->uuid(), uuid, sizeof(uuid_command::uuid)) != 0;
            }

            inline bool operator==(const std::unique_ptr<uint8_t> &uuid) const noexcept {
                return memcmp(this->uuid(), uuid.get(), sizeof(uuid_command::uuid)) == 0;
            }

            inline bool operator!=(const std::unique_ptr<uint8_t> &uuid) const noexcept {
                return memcmp(this->uuid(), uuid.get(), sizeof(uuid_command::uuid)) != 0;
            }

            inline bool operator==(const uuid_pair &pair) const noexcept {
                return this->architecture == pair.architecture && *this == pair.uuid();
            }

            inline bool operator!=(const uuid_pair &pair) const noexcept {
                return this->architecture != pair.architecture && *this != pair.uuid();
            }
        };

        enum class version {
            none,
            
            v1,
            v2
        };
        
        static const char *version_to_string(version version) noexcept;
        static version version_from_string(const char *string) noexcept;

        uint64_t architectures = uint64_t();

        packed_version current_version = {{ uint32_t() }};
        packed_version compatibility_version = {{ uint32_t() }};
        
        flags flags;
        platform platform = platform::none;

        std::string install_name;
        std::string parent_umbrella;

        std::vector<sub_client> sub_clients;

        std::vector<reexport> reexports;
        std::vector<symbol> symbols;

        uint32_t swift_version = uint32_t();

        objc_constraint objc_constraint = objc_constraint::none;
        version version = version::none;

        std::vector<uuid_pair> uuids;

        struct creation_options {
            enum class shifts : uint64_t {
                allow_private_normal_symbols,
                allow_private_weak_symbols,
                allow_private_objc_class_symbols,
                allow_private_objc_ivar_symbols,

                // Ignores all errors surrounding
                // gathering of information that is
                // ignored, also doesn't fill information
                // in tbd fields

                ignore_architectures,
                ignore_allowable_clients,
                ignore_current_version,
                ignore_compatibility_version,
                ignore_exports,
                ignore_flags,
                ignore_install_name,
                ignore_objc_constraint,
                ignore_parent_umbrella,
                ignore_platform,
                ignore_reexports,
                ignore_swift_version,
                ignore_uuids,

                // Ignores all errors relating to
                // gathering of information that isn't
                // necessary for tbd.version

                // Like the other ignore options above,
                // tbd information fields for information
                // ignored is not filled

                ignore_unneeded_fields_for_version,

                // Ignore errors for required fields

                ignore_missing_identification,
                ignore_missing_platform,
                ignore_missing_symbol_table,
                ignore_missing_uuids,
                ignore_non_unique_uuids
            };

            uint64_t bits = uint64_t();

            inline bool has_none() const noexcept {
                return this->bits == 0;
            }

            inline bool has_shift(shifts shift) const noexcept {
                return this->bits & (1ull << static_cast<uint64_t>(shift));
            }

            inline bool allows_private_normal_symbols() const noexcept {
                return this->has_shift(shifts::allow_private_normal_symbols);
            }

            inline bool allows_private_weak_symbols() const noexcept {
                return this->has_shift(shifts::allow_private_weak_symbols);
            }

            inline bool allows_private_objc_class_symbols() const noexcept {
                return this->has_shift(shifts::allow_private_objc_class_symbols);
            }

            inline bool allows_private_objc_ivar_symbols() const noexcept {
                return this->has_shift(shifts::allow_private_objc_ivar_symbols);
            }

            inline bool ignores_architectures() const noexcept {
                return this->has_shift(shifts::ignore_architectures);
            }

            inline bool ignores_allowable_clients() const noexcept {
                return this->has_shift(shifts::ignore_allowable_clients);
            }

            inline bool ignores_current_version() const noexcept {
                return this->has_shift(shifts::ignore_current_version);
            }

            inline bool ignores_compatibility_version() const noexcept {
                return this->has_shift(shifts::ignore_compatibility_version);
            }

            inline bool ignores_exports() const noexcept {
                return this->has_shift(shifts::ignore_exports);
            }

            inline bool ignores_flags() const noexcept {
                return this->has_shift(shifts::ignore_flags);
            }

            inline bool ignores_install_name() const noexcept {
                return this->has_shift(shifts::ignore_install_name);
            }

            inline bool ignores_objc_constraint() const noexcept {
                return this->has_shift(shifts::ignore_objc_constraint);
            }

            inline bool ignores_parent_umbrella() const noexcept {
                return this->has_shift(shifts::ignore_parent_umbrella);
            }

            inline bool ignores_platform() const noexcept {
                return this->has_shift(shifts::ignore_platform);
            }

            inline bool ignores_reexports() const noexcept {
                return this->has_shift(shifts::ignore_reexports);
            }

            inline bool ignores_swift_version() const noexcept {
                return this->has_shift(shifts::ignore_swift_version);
            }

            inline bool ignores_unneeded_fields_for_version() const noexcept {
                return this->has_shift(shifts::ignore_unneeded_fields_for_version);
            }

            inline bool ignores_uuids() const noexcept {
                return this->has_shift(shifts::ignore_uuids);
            }

            inline bool ignores_missing_symbol_table() const noexcept {
                return this->has_shift(shifts::ignore_missing_symbol_table);
            }

            inline bool ignores_missing_identification() const noexcept {
                return this->has_shift(shifts::ignore_missing_identification);
            }

            inline bool ignores_missing_platform() const noexcept {
                return this->has_shift(shifts::ignore_missing_platform);
            }

            inline bool ignores_missing_uuids() const noexcept {
                return this->has_shift(shifts::ignore_missing_uuids);
            }

            inline bool ignores_non_unique_uuids() const noexcept {
                return this->has_shift(shifts::ignore_non_unique_uuids);
            }

            inline void add_shift(shifts shift) noexcept {
                this->bits |= (1ull << static_cast<uint64_t>(shift));
            }

            inline void allow_private_normal_symbols() noexcept {
                this->add_shift(shifts::allow_private_normal_symbols);
            }

            inline void allow_private_weak_symbols() noexcept {
                this->add_shift(shifts::allow_private_weak_symbols);
            }

            inline void allow_private_objc_class_symbols() noexcept {
                this->add_shift(shifts::allow_private_objc_class_symbols);
            }

            inline void allow_private_objc_ivar_symbols() noexcept {
                this->add_shift(shifts::allow_private_objc_ivar_symbols);
            }

            inline void ignore_architectures() noexcept {
                this->add_shift(shifts::ignore_architectures);
            }

            inline void ignore_allowable_clients() noexcept {
                this->add_shift(shifts::ignore_allowable_clients);
            }

            inline void ignore_current_version() noexcept {
                this->add_shift(shifts::ignore_current_version);
            }

            inline void ignore_compatibility_version() noexcept {
                this->add_shift(shifts::ignore_compatibility_version);
            }

            inline void ignore_exports() noexcept {
                this->add_shift(shifts::ignore_exports);
            }

            inline void ignore_flags() noexcept {
                this->add_shift(shifts::ignore_flags);
            }

            inline void ignore_install_name() noexcept {
                this->add_shift(shifts::ignore_install_name);
            }

            inline void ignore_objc_constraint() noexcept {
                this->add_shift(shifts::ignore_objc_constraint);
            }

            inline void ignore_parent_umbrella() noexcept {
                this->add_shift(shifts::ignore_parent_umbrella);
            }

            inline void ignore_platform() noexcept {
                this->add_shift(shifts::ignore_platform);
            }

            inline void ignore_reexports() noexcept {
                this->add_shift(shifts::ignore_reexports);
            }

            inline void ignore_swift_version() noexcept {
                this->add_shift(shifts::ignore_swift_version);
            }

            inline void ignore_unneeded_fields_for_version() noexcept {
                this->add_shift(shifts::ignore_unneeded_fields_for_version);
            }

            inline void ignore_uuids() noexcept {
                this->add_shift(shifts::ignore_uuids);
            }

            inline void ignore_missing_identification() noexcept {
                this->add_shift(shifts::ignore_missing_identification);
            }

            inline void ignore_missing_platform() noexcept {
                this->add_shift(shifts::ignore_missing_platform);
            }

            inline void ignore_missing_symbol_table() noexcept {
                this->add_shift(shifts::ignore_missing_symbol_table);
            }

            inline void ignore_missing_uuids() noexcept {
                this->add_shift(shifts::ignore_missing_uuids);
            }

            inline void ignore_non_unique_uuids() noexcept {
                this->add_shift(shifts::ignore_non_unique_uuids);
            }

            inline void remove_shift(shifts shift) noexcept {
                this->bits &= ~(1ull << static_cast<uint64_t>(shift));
            }

            inline void clear() noexcept {
                this->bits = 0;
            }

            inline void dont_allow_private_normal_symbols() noexcept {
                this->remove_shift(shifts::allow_private_normal_symbols);
            }

            inline void dont_allow_private_weak_symbols() noexcept {
                this->remove_shift(shifts::allow_private_weak_symbols);
            }

            inline void dont_allow_private_objc_class_symbols() noexcept {
                this->remove_shift(shifts::allow_private_objc_class_symbols);
            }

            inline void dont_allow_private_objc_ivar_symbols() noexcept {
                this->remove_shift(shifts::allow_private_objc_ivar_symbols);
            }

            inline void dont_ignore_architectures() noexcept {
                this->remove_shift(shifts::ignore_architectures);
            }

            inline void dont_ignore_allowable_clients() noexcept {
                this->remove_shift(shifts::ignore_allowable_clients);
            }

            inline void dont_ignore_current_version() noexcept {
                this->remove_shift(shifts::ignore_current_version);
            }

            inline void dont_ignore_compatibility_version() noexcept {
                this->remove_shift(shifts::ignore_compatibility_version);
            }

            inline void dont_ignore_exports() noexcept {
                this->remove_shift(shifts::ignore_exports);
            }

            inline void dont_ignore_flags() noexcept {
                this->remove_shift(shifts::ignore_flags);
            }

            inline void dont_ignore_install_name() noexcept {
                this->remove_shift(shifts::ignore_install_name);
            }

            inline void dont_ignore_objc_constraint() noexcept {
                this->remove_shift(shifts::ignore_objc_constraint);
            }

            inline void dont_ignore_parent_umbrella() noexcept {
                this->remove_shift(shifts::ignore_parent_umbrella);
            }

            inline void dont_ignore_platform() noexcept {
                this->remove_shift(shifts::ignore_platform);
            }

            inline void dont_ignore_reexports() noexcept {
                this->remove_shift(shifts::ignore_reexports);
            }

            inline void dont_ignore_swift_version() noexcept {
                this->remove_shift(shifts::ignore_swift_version);
            }

            inline void dont_ignore_uuids() noexcept {
                this->remove_shift(shifts::ignore_uuids);
            }

            inline void dont_ignore_unneeded_fields_for_version() noexcept {
                this->remove_shift(shifts::ignore_unneeded_fields_for_version);
            }

            inline void dont_ignore_missing_identification() noexcept {
                this->add_shift(shifts::ignore_missing_identification);
            }

            inline void dont_ignore_missing_platform() noexcept {
                this->add_shift(shifts::ignore_missing_platform);
            }

            inline void dont_ignore_missing_symbol_table() noexcept {
                this->remove_shift(shifts::ignore_missing_symbol_table);
            }

            inline void dont_ignore_missing_uuids() noexcept {
                this->remove_shift(shifts::ignore_missing_uuids);
            }

            inline void dont_ignore_non_unique_uuids() noexcept {
                this->remove_shift(shifts::ignore_non_unique_uuids);
            }
        };

        // "Multiple" is to mean the same types of
        // information with different values were
        // found in the same containers

        // However, multiple values are ignored if
        // the all match the first value found

        // "Mismatch" on the other hand is to mean
        // the same types of information with different
        // values were found in the other containers

        enum class creation_result {
            ok,

            invalid_container_header_subtype,
            unrecognized_container_cputype_subtype_pair,

            flags_mismatch,
            failed_to_retrieve_load_cammands,

            unsupported_platform,
            unrecognized_platform,

            multiple_platforms,
            platforms_mismatch,

            invalid_build_version_command,
            invalid_dylib_command,

            invalid_segment_command,
            invalid_segment_command_64,

            empty_install_name,

            multiple_current_versions,
            current_versions_mismatch,

            multiple_compatibility_versions,
            compatibility_versions_mismatch,

            multiple_install_names,
            install_name_mismatch,

            invalid_objc_imageinfo_segment_section,

            stream_seek_error,
            stream_read_error,

            multiple_objc_constraint,
            objc_constraint_mismatch,

            multiple_swift_version,
            swift_version_mismatch,

            invalid_sub_client_command,
            invalid_sub_umbrella_command,

            empty_parent_umbrella,
            
            multiple_parent_umbrella,
            parent_umbrella_mismatch,

            invalid_symtab_command,
            multiple_symbol_tables,

            multiple_uuids,
            non_unique_uuid,

            container_is_missing_dynamic_library_identification,
            container_is_missing_platform,
            container_is_missing_uuid,
            container_is_missing_symbol_table,

            failed_to_retrieve_string_table,
            failed_to_retrieve_symbol_table
        };

        creation_result create(const file &file, const creation_options &options) noexcept;

        creation_result create(const container &container, load_commands::data &data, symbols::table::data &symbols, strings::table::data &strings, const creation_options &options) noexcept;
        creation_result create(const container &container, load_commands::data &data, symbols::table_64::data &symbols, strings::table::data &strings, const creation_options &options) noexcept;

        std::vector<export_group> export_groups() const noexcept;

        struct write_options {
            enum class shifts {
                enforce_has_exports,
                
                // Each of the following
                // "ignores" field, thereby
                // not printing it out

                ignore_architectures,
                ignore_allowable_clients,
                ignore_current_version,
                ignore_compatibility_version,
                ignore_exports,
                ignore_flags,
                ignore_footer,
                ignore_header,
                ignore_install_name,
                ignore_objc_constraint,
                ignore_parent_umbrella,
                ignore_platform,
                ignore_swift_version,
                ignore_uuids,

                ignore_reexports,
                ignore_normal_symbols,
                ignore_objc_class_symbols,
                ignore_objc_ivar_symbols,
                ignore_weak_symbols,

                // Ignore any fields that are not
                // required for tbd.version

                ignore_unneeded_fields_for_version,

                // Orders field information s by
                // architecture-infe table, default
                // behavior ordering by container

                order_by_architecture_info_table
            };

            uint64_t bits = uint64_t();

            inline bool has_none() const noexcept {
                return this->bits == 0;
            }

            inline bool has_shift(shifts shift) const noexcept {
                return this->bits & (1ull << static_cast<uint64_t>(shift));
            }

            inline bool enforces_has_exports() const noexcept {
                return this->has_shift(shifts::enforce_has_exports);
            }
            
            inline bool ignores_architectures() const noexcept {
                return this->has_shift(shifts::ignore_architectures);
            }

            inline bool ignores_allowable_clients() const noexcept {
                return this->has_shift(shifts::ignore_allowable_clients);
            }

            inline bool ignores_current_version() const noexcept {
                return this->has_shift(shifts::ignore_current_version);
            }

            inline bool ignores_compatibility_version() const noexcept {
                return this->has_shift(shifts::ignore_compatibility_version);
            }

            inline bool ignores_exports() const noexcept {
                return this->has_shift(shifts::ignore_exports);
            }

            inline bool ignores_flags() const noexcept {
                return this->has_shift(shifts::ignore_flags);
            }

            inline bool ignores_footer() const noexcept {
                return this->has_shift(shifts::ignore_footer);
            }

            inline bool ignores_header() const noexcept {
                return this->has_shift(shifts::ignore_header);
            }

            inline bool ignores_install_name() const noexcept {
                return this->has_shift(shifts::ignore_install_name);
            }

            inline bool ignores_objc_constraint() const noexcept {
                return this->has_shift(shifts::ignore_objc_constraint);
            }

            inline bool ignores_parent_umbrella() const noexcept {
                return this->has_shift(shifts::ignore_parent_umbrella);
            }

            inline bool ignores_platform() const noexcept {
                return this->has_shift(shifts::ignore_platform);
            }

            inline bool ignores_swift_version() const noexcept {
                return this->has_shift(shifts::ignore_swift_version);
            }

            inline bool ignores_uuids() const noexcept {
                return this->has_shift(shifts::ignore_uuids);
            }

            inline bool ignores_reexports() const noexcept {
                return this->has_shift(shifts::ignore_reexports);
            }

            inline bool ignores_normal_symbols() const noexcept {
                return this->has_shift(shifts::ignore_normal_symbols);
            }

            inline bool ignores_objc_class_symbols() const noexcept {
                return this->has_shift(shifts::ignore_objc_class_symbols);
            }

            inline bool ignores_objc_ivar_symbols() const noexcept {
                return this->has_shift(shifts::ignore_objc_ivar_symbols);
            }

            inline bool ignores_weak_symbols() const noexcept {
                return this->has_shift(shifts::ignore_weak_symbols);
            }

            inline bool orders_by_architecture_info_table() const noexcept {
                return this->has_shift(shifts::order_by_architecture_info_table);
            }

            inline bool ignores_unneeded_fields_for_version() const noexcept {
                return this->has_shift(shifts::ignore_unneeded_fields_for_version);
            }

            inline void add_shift(shifts shift) noexcept {
                this->bits |= (1ull << static_cast<uint64_t>(shift));
            }

            inline void enforce_has_exports() noexcept {
                this->add_shift(shifts::enforce_has_exports);
            }
            
            inline void ignore_architectures() noexcept {
                this->add_shift(shifts::ignore_architectures);
            }

            inline void ignore_allowable_clients() noexcept {
                this->add_shift(shifts::ignore_allowable_clients);
            }

            inline void ignore_current_version() noexcept {
                this->add_shift(shifts::ignore_current_version);
            }

            inline void ignore_compatibility_version() noexcept {
                this->add_shift(shifts::ignore_compatibility_version);
            }

            inline void ignore_exports() noexcept {
                this->add_shift(shifts::ignore_exports);
            }

            inline void ignore_footer() noexcept {
                this->add_shift(shifts::ignore_footer);
            }

            inline void ignore_flags() noexcept {
                this->add_shift(shifts::ignore_flags);
            }

            inline void ignore_header() noexcept {
                this->add_shift(shifts::ignore_header);
            }

            inline void ignore_install_name() noexcept {
                this->add_shift(shifts::ignore_install_name);
            }

            inline void ignore_objc_constraint() noexcept {
                this->add_shift(shifts::ignore_objc_constraint);
            }

            inline void ignore_parent_umbrella() noexcept {
                this->add_shift(shifts::ignore_parent_umbrella);
            }

            inline void ignore_platform() noexcept {
                this->add_shift(shifts::ignore_platform);
            }

            inline void ignore_swift_version() noexcept {
                this->add_shift(shifts::ignore_swift_version);
            }

            inline void ignore_uuids() noexcept {
                this->add_shift(shifts::ignore_uuids);
            }

            inline void ignore_reexports() noexcept {
                this->add_shift(shifts::ignore_reexports);
            }

            inline void ignore_normal_symbols() noexcept {
                this->add_shift(shifts::ignore_normal_symbols);
            }

            inline void ignores_objc_class_symbols() noexcept {
                this->add_shift(shifts::ignore_objc_class_symbols);
            }

            inline void ignore_objc_ivar_symbols() noexcept {
                this->add_shift(shifts::ignore_objc_ivar_symbols);
            }

            inline void ignore_weak_symbols() noexcept {
                this->add_shift(shifts::ignore_weak_symbols);
            }

            inline void ignore_unneeded_fields_for_version() noexcept {
                this->add_shift(shifts::ignore_unneeded_fields_for_version);
            }

            inline void order_by_architecture_info_table() noexcept {
                this->add_shift(shifts::order_by_architecture_info_table);
            }

            inline void remove_shift(shifts shift) noexcept {
                this->bits &= ~(1ull << static_cast<uint64_t>(shift));
            }

            inline void clear() noexcept {
                this->bits = 0;
            }
            
            inline void dont_enforce_has_exports() noexcept {
                this->remove_shift(shifts::enforce_has_exports);
            }

            inline void dont_ignore_architectures() noexcept {
                this->remove_shift(shifts::ignore_architectures);
            }

            inline void dont_ignore_allowable_clients() noexcept {
                this->remove_shift(shifts::ignore_allowable_clients);
            }

            inline void dont_ignore_current_version() noexcept {
                this->remove_shift(shifts::ignore_current_version);
            }

            inline void dont_ignore_compatibility_version() noexcept {
                this->remove_shift(shifts::ignore_compatibility_version);
            }

            inline void dont_ignore_exports() noexcept {
                this->remove_shift(shifts::ignore_exports);
            }

            inline void dont_ignore_footer() noexcept {
                this->remove_shift(shifts::ignore_footer);
            }

            inline void dont_ignore_flags() noexcept {
                this->remove_shift(shifts::ignore_flags);
            }

            inline void dont_ignore_header() noexcept {
                this->remove_shift(shifts::ignore_header);
            }

            inline void dont_ignore_install_name() noexcept {
                this->remove_shift(shifts::ignore_install_name);
            }

            inline void dont_ignore_objc_constraint() noexcept {
                this->remove_shift(shifts::ignore_objc_constraint);
            }

            inline void dont_ignore_parent_umbrella() noexcept {
                this->remove_shift(shifts::ignore_parent_umbrella);
            }

            inline void dont_ignore_platform() noexcept {
                this->remove_shift(shifts::ignore_platform);
            }

            inline void dont_ignore_swift_version() noexcept {
                this->remove_shift(shifts::ignore_swift_version);
            }

            inline void dont_ignore_uuids() noexcept {
                this->remove_shift(shifts::ignore_uuids);
            }

            inline void dont_ignore_reexports() noexcept {
                this->remove_shift(shifts::ignore_reexports);
            }

            inline void dont_ignore_normal_symbols() noexcept {
                this->remove_shift(shifts::ignore_normal_symbols);
            }

            inline void dont_ignores_objc_class_symbols() noexcept {
                this->remove_shift(shifts::ignore_objc_class_symbols);
            }

            inline void dont_ignore_objc_ivar_symbols() noexcept {
                this->remove_shift(shifts::ignore_objc_ivar_symbols);
            }

            inline void dont_ignore_weak_symbols() noexcept {
                this->remove_shift(shifts::ignore_weak_symbols);
            }

            inline void dont_ignore_unneeded_fields_for_version() noexcept {
                this->remove_shift(shifts::ignore_unneeded_fields_for_version);
            }

            inline void dont_order_by_architecture_info_table() noexcept {
                this->add_shift(shifts::order_by_architecture_info_table);
            }
        };

        enum class write_result {
            ok,
            failed_to_open_stream,

            failed_to_write_architectures,
            failed_to_write_current_version,
            failed_to_write_compatibility_version,
            failed_to_write_exports,
            failed_to_write_flags,
            failed_to_write_footer,
            failed_to_write_header,
            failed_to_write_install_name,
            failed_to_write_objc_constraint,
            failed_to_write_parent_umbrella,
            failed_to_write_platform,
            failed_to_write_swift_version,
            failed_to_write_uuids,

            has_no_exports,
            
            failed_to_write_reexports,
            failed_to_write_normal_symbols,
            failed_to_write_objc_class_symbols,
            failed_to_write_objc_ivar_symbols,
            failed_to_write_weak_symbols
        };

        write_result write_to(const char *path, const write_options &options) const noexcept;
        write_result write_to(int descriptor, const write_options &options) const noexcept;
        write_result write_to(FILE *file, const write_options &options) const noexcept;

        write_result write_with_export_groups_to(const char *path, const write_options &options, const std::vector<export_group> &groups) const noexcept;
        write_result write_with_export_groups_to(int descriptor, const write_options &options, const std::vector<export_group> &groups) const noexcept;
        write_result write_with_export_groups_to(FILE *file, const write_options &options, const std::vector<export_group> &groups) const noexcept;
    
        void clear() noexcept;
    };
}
