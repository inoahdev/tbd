//
//  src/mach-o/utils/tbd-structure.cc
//  tbd
//
//  Created by inoahdev on 1/7/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "tbd.h"

namespace macho::utils {
    enum tbd::platform tbd::platform_from_string(const char *string) noexcept {
        if (strcmp(string, "aix") == 0) {
            return macho::utils::tbd::platform::aix;
        } else if (strcmp(string, "amdhsa") == 0) {
            return macho::utils::tbd::platform::amdhsa;
        } else if (strcmp(string, "ananas") == 0) {
            return macho::utils::tbd::platform::ananas;
        } else if (strcmp(string, "cloudabi") == 0) {
            return macho::utils::tbd::platform::cloudabi;
        } else if (strcmp(string, "cnk") == 0) {
            return macho::utils::tbd::platform::cnk;
        } else if (strcmp(string, "contiki") == 0) {
            return macho::utils::tbd::platform::contiki;
        } else if (strcmp(string, "cuda") == 0) {
            return macho::utils::tbd::platform::cuda;
        } else if (strcmp(string, "darwin") == 0) {
            return macho::utils::tbd::platform::darwin;
        } else if (strcmp(string, "dragonfly") == 0) {
            return macho::utils::tbd::platform::dragonfly;
        } else if (strcmp(string, "elfiamcu") == 0) {
            return macho::utils::tbd::platform::elfiamcu;
        } else if (strcmp(string, "freebsd") == 0) {
            return macho::utils::tbd::platform::freebsd;
        } else if (strcmp(string, "fuchsia") == 0) {
            return macho::utils::tbd::platform::fuchsia;
        } else if (strcmp(string, "haiku") == 0) {
            return macho::utils::tbd::platform::haiku;
        } else if (strcmp(string, "ios") == 0) {
            return macho::utils::tbd::platform::ios;
        } else if (strcmp(string, "kfreebsd") == 0) {
            return macho::utils::tbd::platform::kfreebsd;
        } else if (strcmp(string, "linux") == 0) {
            return macho::utils::tbd::platform::linux;
        } else if (strcmp(string, "lv2") == 0) {
            return macho::utils::tbd::platform::lv2;
        } else if (strcmp(string, "macosx") == 0) {
            return macho::utils::tbd::platform::macosx;
        } else if (strcmp(string, "mesa3d") == 0) {
            return macho::utils::tbd::platform::mesa3d;
        } else if (strcmp(string, "minix") == 0) {
            return macho::utils::tbd::platform::minix;
        } else if (strcmp(string, "nacl") == 0) {
            return macho::utils::tbd::platform::nacl;
        } else if (strcmp(string, "netbsd") == 0) {
            return macho::utils::tbd::platform::netbsd;
        } else if (strcmp(string, "nvcl") == 0) {
            return macho::utils::tbd::platform::nvcl;
        } else if (strcmp(string, "openbsd") == 0) {
            return macho::utils::tbd::platform::openbsd;
        } else if (strcmp(string, "ps4") == 0) {
            return macho::utils::tbd::platform::ps4;
        } else if (strcmp(string, "rtems") == 0) {
            return macho::utils::tbd::platform::rtems;
        } else if (strcmp(string, "solaris") == 0) {
            return macho::utils::tbd::platform::solaris;
        } else if (strcmp(string, "tvos") == 0) {
            return macho::utils::tbd::platform::tvos;
        } else if (strcmp(string, "watchos") == 0) {
            return macho::utils::tbd::platform::watchos;
        } else if (strcmp(string, "windows") == 0) {
            return macho::utils::tbd::platform::windows;
        }

        return platform::none;
    }

    const char *tbd::platform_to_string(const enum platform &platform) noexcept {
        switch (platform) {
            case platform::none:
                return "none";

            case platform::aix:
                return "aix";

            case platform::amdhsa:
                return "amdhsa";

            case platform::ananas:
                return "ananas";

            case platform::cloudabi:
                return "cloudabi";

            case platform::cnk:
                return "cnk";

            case platform::contiki:
                return "contiki";

            case platform::cuda:
                return "cuda";

            case platform::darwin:
                return "darwin";

            case platform::dragonfly:
                return "dragonfly";

            case platform::elfiamcu:
                return "elfiamcu";

            case platform::freebsd:
                return "freebsd";

            case platform::kfreebsd:
                return "kfreebsd";

            case platform::fuchsia:
                return "fuchsia";

            case platform::haiku:
                return "haiki";

            case platform::ios:
                return "ios";

            case platform::linux:
                return "linux";

            case platform::lv2:
                return "lv2";

            case platform::macosx:
                return "macosx";

            case platform::mesa3d:
                return "mesa3d";

            case platform::minix:
                return "minix";

            case platform::nacl:
                return "nacl";

            case platform::netbsd:
                return "netbsd";

            case platform::nvcl:
                return "nvcl";

            case platform::openbsd:
                return "openbsd";

            case platform::ps4:
                return "ps4";

            case platform::rtems:
                return "rtems";

            case platform::solaris:
                return "solaris";

            case platform::tvos:
                return "tvos";

            case platform::watchos:
                return "watchos";

            case platform::windows:
                return "windows";
        }

        return nullptr;
    }

    enum tbd::symbol::type tbd::symbol::type_from_symbol_string(const char *string, symbol_table::desc n_desc, size_t *stripped_index) noexcept {
        if (n_desc.weak_definition) {
            return symbol::type::weak;
        }

        if (strncmp(string, "_OBJC_CLASS_$", 13) == 0) {
            if (stripped_index != nullptr) {
                *stripped_index = 13;
            }
            
            return symbol::type::objc_class;
        }

        if (strncmp(string, ".objc_class_name", 16) == 0) {
            if (stripped_index != nullptr) {
                *stripped_index = 16;
            }
            
            return symbol::type::objc_class;
        }

        if (strncmp(string, "_OBJC_METACLASS_$", 17) == 0) {
            if (stripped_index != nullptr) {
                *stripped_index = 17;
            }
            
            return symbol::type::objc_class;
        }

        if (strncmp(string, "_OBJC_IVAR_$", 12) == 0) {
            if (stripped_index != nullptr) {
                *stripped_index = 12;
            }
            
            return symbol::type::objc_ivar;
        }

        return symbol::type::normal;
    }

    enum tbd::objc_constraint tbd::objc_constraint_from_string(const char *string) noexcept {
        if (strcmp(string, "none") == 0) {
            return objc_constraint::none;
        } else if (strcmp(string, "retain_release") == 0) {
            return objc_constraint::retain_release;
        } else if (strcmp(string, "retain_release_for_simulator") == 0) {
            return objc_constraint::retain_release_for_simulator;
        } else if (strcmp(string, "retain_release_or_gc") == 0) {
            return objc_constraint::retain_release_or_gc;
        } else if (strcmp(string, "gc") == 0) {
            return objc_constraint::gc;
        }

        return objc_constraint::none;
    }

    const char *tbd::objc_constraint_to_string(const enum objc_constraint &constraint) noexcept {
        switch (constraint) {
            case objc_constraint::none:
                return "none";

            case objc_constraint::retain_release:
                return "retain_release";

            case objc_constraint::retain_release_or_gc:
                return "retain_release_or_gc";

            case objc_constraint::retain_release_for_simulator:
                return "retain_release_for_simulator";

            case objc_constraint::gc:
                return "gc";
        }
    }

    const char *tbd::version_to_string(enum version version) noexcept {
        switch (version) {
            case version::none:
                return "none";

            case version::v1:
                return "v1";

            case version::v2:
                return "v2";
        }
    }

    enum tbd::version tbd::version_from_string(const char *string) noexcept {
        if (strcmp(string, "none") == 0) {
            return version::none;
        } else if (strcmp(string, "v1") == 0) {
            return version::v1;
        } else if (strcmp(string, "v2") == 0) {
            return version::v2;
        }

        return version::none;
    }

    tbd::export_group::export_group(uint64_t architectures) noexcept
    : architectures(architectures) {}
    
    tbd::export_group::export_group(const struct client *client) noexcept
    : client(client) {}

    tbd::export_group::export_group(const struct reexport *reexport) noexcept
    : reexport(reexport) {}

    tbd::export_group::export_group(const struct symbol *symbol) noexcept
    : symbol(symbol) {}

    tbd::reexport::reexport(uint64_t architectures, std::string &string) noexcept
    : architectures(architectures), string(string) {}

    tbd::reexport::reexport(uint64_t architectures, std::string &&string) noexcept
    : architectures(architectures), string(string) {}

    tbd::client::client(uint64_t architectures, std::string &string) noexcept
    : architectures(architectures), string(string) {}

    tbd::client::client(uint64_t architectures, std::string &&string) noexcept
    : architectures(architectures), string(string) {}

    tbd::symbol::symbol(uint64_t architectures, std::string &string, enum type type) noexcept
    : architectures(architectures), string(string), type(type) {}

    tbd::symbol::symbol(uint64_t architectures, std::string &&string, enum type type) noexcept
    : architectures(architectures), string(string), type(type) {}
}
