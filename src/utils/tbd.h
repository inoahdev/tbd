//
//  src/utils/tbd.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

#include "../mach-o/architecture_info.h"
#include "../mach-o/file.h"

#include "../mach-o/utils/load_commands/data.h"
#include "../mach-o/utils/strings/table/data.h"

#include "../mach-o/utils/symbols/table/data.h"
#include "../mach-o/utils/symbols/table_64/data.h"

#include "../objc/image_info.h"

namespace utils {
    struct tbd {
        struct flags {
            explicit inline flags() = default;
            explicit inline flags(uint64_t value) : value(value) {};

            union {
                uint64_t value = 0;

                struct {
                    bool flat_namespace         : 1;
                    bool not_app_extension_safe : 1;
                } __attribute__((packed));;
            };

            inline bool has_none() const noexcept { return this->value == 0; }
            inline void clear() noexcept { this->value = 0; }

            inline bool operator==(const flags &flags) const noexcept { return this->value == flags.value; }
            inline bool operator!=(const flags &flags) const noexcept { return this->value != flags.value; }
        };

        struct reexport;
        struct client;
        struct symbol;

        struct export_group {
            explicit export_group(uint64_t architectures) noexcept;

            explicit export_group(const client *client) noexcept;
            explicit export_group(const reexport *reexport) noexcept;
            explicit export_group(const symbol *symbol) noexcept;

            // Store a pointer to the first symbol and/or reexport
            // to appear in the group which contains all the information
            // we need with no extra cost

            const reexport *reexport = nullptr;
            const client *client = nullptr;
            const symbol *symbol = nullptr;

            uint64_t architectures = uint64_t();
            
            inline bool operator==(const struct client &client) const noexcept {
                return this->architectures == client.architectures;
            }
            
            inline bool operator!=(const struct client &client) const noexcept {
                return this->architectures != client.architectures;
            }

            inline bool operator==(const struct reexport &reexport) const noexcept {
                return this->architectures == reexport.architectures;
            }

            inline bool operator!=(const struct reexport &reexport) const noexcept {
                return this->architectures != reexport.architectures;
            }

            inline bool operator==(const struct symbol &symbol) const noexcept {
                return this->architectures == symbol.architectures;
            }

            inline bool operator!=(const struct symbol &symbol) const noexcept {
                return this->architectures != symbol.architectures;
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
            uint32_t value;
            
            struct components {
                uint8_t revision;
                uint8_t minor;

                uint16_t major;
            } __attribute__((packed)) components;

            inline bool operator==(const packed_version &version) const noexcept {
                return this->value == version.value;
            }
            
            inline bool operator!=(const packed_version &version) const noexcept {
                return this->value != version.value;
            }
            
            inline void clear() noexcept {
                this->value = 0;
            }
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

        // client, reexport, and symbol are the three structures holding data
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

        struct client {
            explicit client() = default;
            explicit client(uint64_t architectures, std::string &string) noexcept;
            explicit client(uint64_t architectures, std::string &&string) noexcept;
            
            uint64_t architectures = uint64_t();
            std::string string;
            
            inline bool has_architecture(uint64_t index) const noexcept {
                return this->architectures & 1ull << index;
            }
            
            inline void add_architecture(uint64_t index) noexcept {
                this->architectures |= 1ull << index;
            }
            
            inline void set_all_architectures() noexcept {
                this->architectures = ~0ull;
            }
            
            inline bool operator==(const uint64_t &architectures) const noexcept {
                return this->architectures == architectures;
            }
            
            inline bool operator!=(const uint64_t &architectures) const noexcept {
                return this->architectures != architectures;
            }
            
            inline bool operator==(const std::string &client) const noexcept {
                return this->string == client;
            }
            
            inline bool operator!=(const std::string &client) const noexcept {
                return this->string != client;
            }
            
            inline bool operator==(const struct client &client) const noexcept {
                return this->architectures == client.architectures && this->string == client.string;
            }
            
            inline bool operator!=(const struct client &client) const noexcept {
                return this->architectures != client.architectures || this->string != client.string;
            }
        };
        
        struct reexport {
            explicit reexport() = default;
            explicit reexport(uint64_t architectures, std::string &string) noexcept;
            explicit reexport(uint64_t architectures, std::string &&string) noexcept;

            uint64_t architectures = uint64_t();
            std::string string;

            inline bool has_architecture(uint64_t index) const noexcept {
                return this->architectures & 1ull << index;
            }

            inline void add_architecture(uint64_t index) noexcept {
                this->architectures |= 1ull << index;
            }
            
            inline void set_all_architectures() noexcept {
                this->architectures = ~0ull;
            }

            inline bool operator==(const uint64_t &architectures) const noexcept {
                return this->architectures == architectures;
            }

            inline bool operator!=(const uint64_t &architectures) const noexcept {
                return this->architectures != architectures;
            }

            inline bool operator==(const std::string &string) const noexcept {
                return this->string == string;
            }

            inline bool operator!=(const std::string &string) const noexcept {
                return this->string != string;
            }

            inline bool operator==(const reexport &reexport) const noexcept {
                return this->architectures == reexport.architectures && this->string == reexport.string;
            }

            inline bool operator!=(const reexport &reexport) const noexcept {
                return this->architectures != reexport.architectures && this->string != reexport.string;
            }
        };

        struct symbol {
            enum class type {
                normal,
                objc_class,
                objc_ivar,
                weak
            };

            static type type_from_symbol_string(const char *string,
                                                macho::symbol_table::desc n_desc,
                                                size_t *stripped_index = nullptr) noexcept;

            explicit symbol() = default;
            explicit symbol(uint64_t architectures, std::string &string, type type) noexcept;
            explicit symbol(uint64_t architectures, std::string &&string, type type) noexcept;

            uint64_t architectures = uint64_t();

            std::string string;
            type type;

            inline bool has_architecture(uint64_t index) const noexcept {
                return this->architectures & 1ull << index;
            }

            inline void add_architecture(uint64_t index) noexcept {
                this->architectures |= 1ull << index;
            }
            
            inline void set_all_architectures() noexcept {
                this->architectures = ~0ull;
            }

            inline bool operator==(const uint64_t &architectures) const noexcept {
                return this->architectures == architectures;
            }

            inline bool operator!=(const uint64_t &architectures) const noexcept {
                return this->architectures != architectures;
            }

            inline bool operator==(const std::string &string) const noexcept {
                return this->string == string;;
            }

            inline bool operator!=(const std::string &string) const noexcept {
                return this->string != string;
            }

            inline bool operator==(const symbol &symbol) const noexcept {
                return this->type == type && this->architectures == symbol.architectures && this->string == symbol.string;
            }

            inline bool operator!=(const symbol &symbol) const noexcept {
                return this->type != type || this->architectures != symbol.architectures || this->string != symbol.string;
            }
        };

        struct uuid_pair {
            explicit uuid_pair() = default;

            const macho::architecture_info *architecture = nullptr;
            std::unique_ptr<const uint8_t> unique_uuid;

            inline const uint8_t *uuid() const noexcept {
                return this->unique_uuid.get();
            }

            inline void set_uuid(const uint8_t *uuid) noexcept {
                auto new_uuid = new uint8_t[sizeof(macho::uuid_command::uuid)];
                memcpy(new_uuid, uuid, sizeof(macho::uuid_command::uuid));

                this->unique_uuid.reset(new_uuid);
            }
            
            inline bool empty() const noexcept {
                return unique_uuid.get() == nullptr;
            }

            inline bool operator==(const macho::architecture_info *architecture) const noexcept {
                return this->architecture == architecture;
            }

            inline bool operator!=(const macho::architecture_info *architecture) const noexcept {
                return this->architecture != architecture;
            }

            inline bool operator==(const uint8_t *uuid) const noexcept {
                return memcmp(this->uuid(), uuid, sizeof(macho::uuid_command::uuid)) == 0;
            }

            inline bool operator!=(const uint8_t *uuid) const noexcept {
                return memcmp(this->uuid(), uuid, sizeof(macho::uuid_command::uuid)) != 0;
            }

            inline bool operator==(const std::unique_ptr<uint8_t> &uuid) const noexcept {
                return memcmp(this->uuid(), uuid.get(), sizeof(macho::uuid_command::uuid)) == 0;
            }

            inline bool operator!=(const std::unique_ptr<uint8_t> &uuid) const noexcept {
                return memcmp(this->uuid(), uuid.get(), sizeof(macho::uuid_command::uuid)) != 0;
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
        
        packed_version current_version = { uint32_t() };
        packed_version compatibility_version = { uint32_t() };

        flags flags;
        platform platform = platform::none;

        std::string install_name;
        std::string parent_umbrella;

        std::vector<client> clients;

        std::vector<reexport> reexports;
        std::vector<symbol> symbols;

        uint32_t swift_version = uint32_t();

        objc_constraint objc_constraint = objc_constraint::none;
        std::vector<uuid_pair> uuids;

        struct creation_options {
            explicit inline creation_options() = default;
            explicit inline creation_options(uint64_t value) : value(value) {};

            union {
                uint64_t value = 0;

                struct {
                    bool allow_private_normal_symbols     : 1;
                    bool allow_private_weak_symbols       : 1;
                    bool allow_private_objc_class_symbols : 1;
                    bool allow_private_objc_ivar_symbols  : 1;

                    // Ignores all errors surrounding
                    // gathering of information that is
                    // ignored, also doesn't fill information
                    // in tbd fields

                    bool ignore_architectures         : 1;
                    bool ignore_clients               : 1;
                    bool ignore_current_version       : 1;
                    bool ignore_compatibility_version : 1;
                    bool ignore_exports               : 1;
                    bool ignore_flags                 : 1;
                    bool ignore_install_name          : 1;
                    bool ignore_objc_constraint       : 1;
                    bool ignore_parent_umbrella       : 1;
                    bool ignore_platform              : 1;
                    bool ignore_reexports             : 1;
                    bool ignore_swift_version         : 1;
                    bool ignore_uuids                 : 1;

                    // Ignores all errors relating to
                    // gathering of information that isn't
                    // necessary for tbd.version

                    // Like the other ignore options above,
                    // tbd information fields for information
                    // ignored is not filled

                    // ignore_unnecessary_fields_for_version will
                    // still parse exports, use ignore_exports to
                    // ignore
                    
                    bool ignore_unnecessary_fields_for_version : 1;
                    bool parse_unsupported_fields_for_version : 1;

                    // Ignore errors for required fields

                    bool ignore_missing_identification : 1;
                    bool ignore_missing_platform       : 1;
                    bool ignore_missing_symbol_table   : 1;
                    bool ignore_missing_uuids          : 1;
                    bool ignore_non_unique_uuids       : 1;
                } __attribute__((packed));
            };

            inline bool has_none() const noexcept { return this->value == 0; }
            inline void clear() noexcept { this->value = 0; }

            inline bool operator==(const creation_options &options) const noexcept { return this->value == options.value; }
            inline bool operator!=(const creation_options &options) const noexcept { return this->value != options.value; }
        };

        // "Multiple" is to mean the same types of
        // information with different values were
        // found in the same containers

        // However, multiple values are ignored if
        // the all match the first value found

        // "Mismatch" on the other hand is to mean
        // the same types of information with different
        // values were found in the other containers

        enum class creation_from_macho_result {
            ok,
            
            multiple_containers_for_same_architecture,
            
            invalid_container_header_subtype,
            unrecognized_container_cputype_subtype_pair,

            flags_mismatch,
            failed_to_retrieve_load_cammands,

            unsupported_platform,
            unrecognized_platform,

            multiple_platforms,
            platform_mismatch,

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

            invalid_dysymtab_command,
            
            multiple_uuids,
            non_unique_uuid,

            container_is_missing_dynamic_library_identification,
            container_is_missing_platform,
            container_is_missing_uuid,
            container_is_missing_dynamic_symbol_table,
            container_is_missing_symbol_table,

            failed_to_retrieve_string_table,
            failed_to_retrieve_symbol_table,
            
            recursive_indirect_symbol,
            invalid_indirect_symbol
        };

        creation_from_macho_result
        create_from_macho_file(const macho::file &file,
                               const creation_options &options,
                               const version &version) noexcept;

        creation_from_macho_result
        create_from_macho_container(const macho::container &file,
                                    const creation_options &options,
                                    const version &version) noexcept;
        
        std::vector<export_group> export_groups() const noexcept;
        std::vector<export_group> single_export_group() const noexcept;

        struct write_options {
            explicit inline write_options() = default;
            explicit inline write_options(uint64_t value) : value(value) {};

            union {
                uint64_t value = 0;

                struct {
                    bool enforce_has_exports : 1;

                    // Each of the following
                    // "ignores" field, thereby
                    // not printing it out

                    bool ignore_architectures         : 1;
                    bool ignore_current_version       : 1;
                    bool ignore_compatibility_version : 1;
                    bool ignore_exports               : 1;
                    bool ignore_flags                 : 1;
                    bool ignore_footer                : 1;
                    bool ignore_header                : 1;
                    bool ignore_install_name          : 1;
                    bool ignore_objc_constraint       : 1;
                    bool ignore_parent_umbrella       : 1;
                    bool ignore_platform              : 1;
                    bool ignore_swift_version         : 1;
                    bool ignore_uuids                 : 1;

                    bool ignore_clients               : 1;
                    bool ignore_reexports             : 1;
                    bool ignore_normal_symbols        : 1;
                    bool ignore_objc_class_symbols    : 1;
                    bool ignore_objc_ivar_symbols     : 1;
                    bool ignore_weak_symbols          : 1;
                    
                    // Ignore each reexport, symbol's architecture
                    // for tbd's architectures
                    
                    bool ignore_export_architectures  : 1;

                    // ignore_unnecessary_fields_for_version will
                    // still write exports, use ignore_exports to
                    // ignore
                    
                    bool ignore_unnecessary_fields_for_version : 1;
                    bool write_unsupported_fields_for_version : 1;

                    // Orders field information by
                    // architecture-info table, default
                    // behavior ordering by container

                    bool order_by_architecture_info_table : 1;
                } __attribute__((packed));
            };

            inline bool has_none() const noexcept { return this->value == 0; }
            inline void clear() noexcept { this->value = 0; }

            inline bool operator==(const write_options &options) const noexcept { return this->value == options.value; }
            inline bool operator!=(const write_options &options) const noexcept { return this->value != options.value; }
        };

        enum class write_result {
            ok,
            
            has_no_architectures,
            has_no_exports,
            
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

            failed_to_write_clients,
            failed_to_write_reexports,
            failed_to_write_normal_symbols,
            failed_to_write_objc_class_symbols,
            failed_to_write_objc_ivar_symbols,
            failed_to_write_weak_symbols
        };

        write_result write_to(const char *path,
                              const write_options &options,
                              const version &version) const noexcept;
        
        write_result write_to(int descriptor,
                              const write_options &options,
                              const version &version) const noexcept;
        
        write_result write_to(FILE *file,
                              const write_options &options,
                              const version &version) const noexcept;

        write_result write_with_export_groups_to(const char *path,
                                                 const write_options &options,
                                                 const version &version,
                                                 const std::vector<export_group> &groups) const noexcept;
        
        write_result write_with_export_groups_to(int descriptor,
                                                 const write_options &options,
                                                 const version &version,
                                                 const std::vector<export_group> &groups) const noexcept;
        
        write_result write_with_export_groups_to(FILE *file,
                                                 const write_options &options,
                                                 const version &version,
                                                 const std::vector<export_group> &groups) const noexcept;

        void clear() noexcept;
        
    private:
        // A bunch of helpers for various functions
        // Would be nice if c++ had some sort of class extensions
        
        // tbd-macho helpers
        
        // macho-core-information is a struct of info
        // that may differentiate between containers
        
        struct macho_core_information {
            packed_version current_version = { uint32_t() };
            packed_version compatibility_version = { uint32_t() };
            
            struct flags flags;
            enum platform platform = platform::none;
            
            std::string install_name;
            std::string parent_umbrella;
            
            std::vector<client> *clients = nullptr;
            std::vector<reexport> *reexports = nullptr;
            std::vector<symbol> *symbols = nullptr;
            
            uint32_t swift_version = uint32_t();
            
            enum objc_constraint objc_constraint = objc_constraint::none;
            uuid_pair uuid;
        };
        
        enum creation_from_macho_result
        parse_macho_container_information(const macho::container &container,
                                          const creation_options &options,
                                          const version &version,
                                          struct macho_core_information &info_out) noexcept;
        
        enum creation_from_macho_result
        parse_macho_load_command_information(const macho::container &container,
                                             const creation_options &options,
                                             const version &version,
                                             uint64_t architecture_info_index,
                                             struct macho_core_information &info_out,
                                             struct macho::symtab_command &symtab_out,
                                             struct macho::dysymtab_command &dysymtab_out) noexcept;
        
        enum creation_from_macho_result
        parse_macho_objc_imageinfo_section(const macho::container &container,
                                           const objc::image_info &imageinfo,
                                           const creation_options &options,
                                           bool found_container_objc_information,
                                           struct macho_core_information &info_out) noexcept;
        
        enum creation_from_macho_result
        parse_macho_symbol_table(const macho::container &container,
                                 const macho::symtab_command &symtab,
                                 const macho::dysymtab_command &dysymtab,
                                 const creation_options &options,
                                 uint64_t architecture_info_index,
                                 struct macho_core_information &info_out) noexcept;
    
        enum creation_from_macho_result
        parse_macho_symbol_table_entry(const macho::utils::strings::table::data &string_table,
                                       uint32_t index,
                                       macho::symbol_table::desc desc,
                                       macho::symbol_table::n_type type,
                                       const creation_options &options,
                                       uint64_t architecture_info_index,
                                       struct macho_core_information &info_out) noexcept;
    };
}
