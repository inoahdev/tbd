//
//  src/utils/tbd-create.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "../utils/range.h"
#include "../objc/image_info.h"

#include "../mach-o/headers/build.h"
#include "../mach-o/headers/flags.h"

#include "../mach-o/utils/segments/segment_with_sections_iterator.h"
#include "../mach-o/utils/swap.h"

#include "tbd.h"

namespace utils {
    tbd::creation_from_macho_result
    tbd::create_from_macho_file(const macho::file &file,
                                const creation_options &options,
                                const version &version) noexcept
    {
        if (version == version::none) {
            return tbd::creation_from_macho_result::has_no_version;
        }
        
        auto container_index = uint32_t();
        for (const auto &container : file.containers) {
            auto info = macho_core_information();
            
            info.clients = &this->clients;
            info.reexports = &this->reexports;
            info.symbols = &this->symbols;
            
            enum creation_from_macho_result parse_container_result =
                this->parse_macho_container_information(container,
                                                        options,
                                                        version,
                                                        info);
            
            if (parse_container_result != creation_from_macho_result::ok) {
                return parse_container_result;
            }
        
            if (container_index == 0) {
                this->current_version = info.current_version;
                this->compatibility_version = info.compatibility_version;
                this->flags = info.flags;
                this->platform = info.platform;
                this->install_name = std::move(info.install_name);
                this->parent_umbrella = std::move(info.parent_umbrella);
                this->uuids.emplace_back(std::move(info.uuid));
                
                continue;
            }
            
            if (!options.ignore_current_version) {
                if (!options.ignore_unnecessary_fields_for_version) {
                    if (this->current_version != info.current_version) {
                        return creation_from_macho_result::current_versions_mismatch;
                    }
                }
            }
            
            if (!options.ignore_compatibility_version) {
                if (!options.ignore_unnecessary_fields_for_version) {
                    if (this->compatibility_version != info.compatibility_version) {
                        return creation_from_macho_result::compatibility_versions_mismatch;
                    }
                }
            }
            
            if (!options.ignore_flags) {
                if (version != version::v1 || options.parse_unsupported_fields_for_version) {
                    if (!options.ignore_unnecessary_fields_for_version) {
                        return creation_from_macho_result::flags_mismatch;
                    }
                }
            }
            
            if (!options.ignore_platform && info.platform != platform::none) {
                if (this->platform != info.platform) {
                    return creation_from_macho_result::platform_mismatch;
                }
            }
            
            if (!options.ignore_install_name) {
                if (this->install_name != info.install_name) {
                    return creation_from_macho_result::install_name_mismatch;
                }
            }
            
            if (!options.ignore_parent_umbrella) {
                if (version != version::v1 || options.parse_unsupported_fields_for_version) {
                    if (!options.ignore_unnecessary_fields_for_version) {
                        return creation_from_macho_result::parent_umbrella_mismatch;
                    }
                }
            }
            
            if (!options.ignore_swift_version) {
                if (version != version::v1 || options.parse_unsupported_fields_for_version) {
                    if (!options.ignore_unnecessary_fields_for_version) {
                        return creation_from_macho_result::swift_version_mismatch;
                    }
                }
            }
            
            if (!options.ignore_objc_constraint) {
                if (version != version::v1 || options.parse_unsupported_fields_for_version) {
                    if (!options.ignore_unnecessary_fields_for_version) {
                        return creation_from_macho_result::objc_constraint_mismatch;
                    }
                }
            }
            
            if (!options.ignore_uuids && !info.uuid.empty()) {
                if (version != version::v1 || options.parse_unsupported_fields_for_version) {
                    if (!options.ignore_unnecessary_fields_for_version) {
                        for (const auto &uuid : this->uuids) {
                            if (uuid != info.uuid) {
                                continue;
                            }
                            
                            return creation_from_macho_result::non_unique_uuid;
                        }
                    }
                }
            }
            
            container_index++;
        }
        
        std::sort(this->clients.begin(), this->clients.end(), [](const client &lhs, const client &rhs) {
            return lhs.string.compare(rhs.string) < 0;
        });
        
        std::sort(this->reexports.begin(), this->reexports.end(), [](const reexport &lhs, const reexport &rhs) {
            return lhs.string.compare(rhs.string) < 0;
        });
        
        std::sort(this->symbols.begin(), this->symbols.end(), [](const symbol &lhs, const symbol &rhs) {
            return lhs.string.compare(rhs.string) < 0;
        });

        return creation_from_macho_result::ok;
    }
    
    tbd::creation_from_macho_result
    tbd::create_from_macho_container(const macho::container &container,
                                     const creation_options &options,
                                     const version &version) noexcept
    {
        auto info = macho_core_information();
        
        info.clients = &this->clients;
        info.reexports = &this->reexports;
        info.symbols = &this->symbols;
        
        enum creation_from_macho_result parse_container_result =
            this->parse_macho_container_information(container,
                                                    options,
                                                    version,
                                                    info);
        
        if (parse_container_result != creation_from_macho_result::ok) {
            return parse_container_result;
        }
        
        this->current_version = info.current_version;
        this->compatibility_version = info.compatibility_version;
        this->flags = info.flags;
        this->platform = info.platform;
        this->install_name = std::move(info.install_name);
        this->parent_umbrella = std::move(info.parent_umbrella);
        this->uuids.emplace_back(std::move(info.uuid));
        
        std::sort(this->clients.begin(), this->clients.end(), [](const client &lhs, const client &rhs) {
            return lhs.string.compare(rhs.string) < 0;
        });
        
        std::sort(this->reexports.begin(), this->reexports.end(), [](const reexport &lhs, const reexport &rhs) {
            return lhs.string.compare(rhs.string) < 0;
        });
        
        std::sort(this->symbols.begin(), this->symbols.end(), [](const symbol &lhs, const symbol &rhs) {
            return lhs.string.compare(rhs.string) < 0;
        });
        
        return creation_from_macho_result::ok;
    }

    void tbd::clear() noexcept {
        this->architectures = 0;
        this->current_version.clear();
        this->compatibility_version.clear();
        this->flags.clear();
        this->platform = platform::none;
        this->install_name.clear();
        this->parent_umbrella.clear();
        this->clients.clear();
        this->reexports.clear();
        this->symbols.clear();
        this->swift_version = 0;
        this->objc_constraint = objc_constraint::none;
        this->uuids.clear();
    }
}
