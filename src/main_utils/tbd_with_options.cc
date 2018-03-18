//
//  src/main_utils/tbd_with_options.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "parse_architectures_list.h"

#include "tbd_parse_fields.h"
#include "tbd_with_options.h"

namespace main_utils {
    void tbd_with_options::apply_missing_from(const tbd_with_options &tbd) noexcept {
        if (this->info.architectures == 0) {
            this->info.architectures = tbd.info.architectures;
        }

        if (this->info.current_version.value == 0) {
            this->info.current_version.value = tbd.info.current_version.value;
        }

        if (this->info.compatibility_version.value == 0) {
            this->info.compatibility_version.value = tbd.info.compatibility_version.value;
        }

        if (this->info.flags.value == 0) {
            this->info.flags.value = tbd.info.flags.value;
        }

        if (this->info.install_name.empty()) {
            this->info.install_name = tbd.info.install_name;
        }

        if (this->info.parent_umbrella.empty()) {
            this->info.parent_umbrella = tbd.info.parent_umbrella;
        }

        if (this->info.platform == utils::tbd::platform::none) {
            this->info.platform = tbd.info.platform;
        }

        if (tbd.options.replaced_objc_constraint) {
            this->info.objc_constraint = tbd.info.objc_constraint;
        }

        if (this->info.swift_version == 0) {
            this->info.swift_version = tbd.info.swift_version;
        }
        
        if (this->version == utils::tbd::version::none) {
            this->version = tbd.version;
        }
    }

    void tbd_with_options::apply_local_options(int argc, const char *argv[]) noexcept {
        if (this->local_option_start == 0) {
            return;
        }

        uint64_t architectures_add = uint64_t();
        uint64_t architectures_re = uint64_t();

        struct utils::tbd::flags flags_add;
        struct utils::tbd::flags flags_re;

        for (auto index = this->local_option_start; index != argc; index++) {
            const auto &option = argv[index];
            if (strcmp(option, "add-archs") == 0) {
                architectures_add |= main_utils::parse_architectures_list(++index, argc, argv);
            } else if (strcmp(option, "add-flags") == 0) {
                main_utils::parse_flags(&flags_add, ++index, argc, argv);
            } else if (strcmp(option, "remove-archs") == 0) {
                architectures_re = main_utils::parse_architectures_list(++index, argc, argv);
            } else if (strcmp(option, "remove-flags") == 0) {
                if (this->options.replace_flags) {
                    continue;
                }
                
                main_utils::parse_flags(&flags_re, ++index, argc, argv);
            } else if (strcmp(option, "replace-archs") == 0) {
                architectures_re = main_utils::parse_architectures_list(++index, argc, argv);
            } else if (strcmp(option, "replace-flags") == 0) {
                this->info.flags.clear();
                main_utils::parse_flags(&flags_re, ++index, argc, argv);
            } else if (option[0] != '-') {
                break;
            }

            // Ignore handling of any unrecognized options
            // as by this point, argv has been fully validated
        }

        if (this->options.remove_architectures) {
            this->info.architectures |= architectures_add;
            this->info.architectures &= ~architectures_re;
        } else if (this->options.replace_architectures) {
            this->info.architectures = architectures_add;
        }
        
        if (this->options.remove_flags) {
            this->info.flags.value |= flags_add.value;
            this->info.flags.value &= ~flags_re.value;
        } else if (this->options.replace_flags) {
            this->info.flags.value = flags_re.value;
        }
    }
    
    void tbd_with_options::apply_dont_options() noexcept {
        this->creation_options.value &= ~this->donts.creation_options.value;
        this->write_options.value &= ~this->donts.write_options.value;
        this->options.value &= ~this->donts.options.value;
    }
    
    bool tbd_with_options::parse_option_from_argv(const char *option, int &index, int argc, const char *argv[]) noexcept {
        if (strcmp(option, "add-archs") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of architectures to add onto list found\n", stderr);
                exit(1);
            }
            
            if (this->options.replace_architectures && !this->options.remove_architectures) {
                fputs("Can't both add architectures and replace select architectures from list found\n", stderr);
                exit(1);
            }
            
            // Simply validate the architecture arguments for now
            
            main_utils::parse_architectures_list(index, argc, argv);
            
            this->options.remove_architectures = true;
            this->options.replace_architectures = true;
            
            // Write out replaced architectures
            // over each export's architectures
            
            this->write_options.ignore_export_architectures = true;
        } else if (strcmp(option, "add-flags") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of flags to add onto list found\n", stderr);
                exit(1);
            }
            
            if (this->options.replace_flags && !this->options.remove_flags) {
                fputs("Can't both replace flags found and add onto flags found\n", stderr);
                exit(1);
            }
            
            // Simply validate flag arguments for now
            
            struct utils::tbd::flags flags;
            main_utils::parse_flags(&flags, index, argc, argv);
            
            this->options.remove_flags = true;
            this->options.replace_flags = true;
        } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
            this->creation_options.allow_private_normal_symbols = true;
        } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
            this->creation_options.allow_private_weak_symbols = true;
        } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
            this->creation_options.allow_private_objc_class_symbols = true;
            this->creation_options.allow_private_objc_ivar_symbols = true;
        } else if (strcmp(option, "allow-private-objc-class-symbols") == 0) {
            this->creation_options.allow_private_objc_class_symbols = true;
        } else if (strcmp(option, "allow-private-objc-ivar-symbols") == 0) {
            this->creation_options.allow_private_objc_ivar_symbols = true;
        } else if (strcmp(option, "dont-print-warnings") == 0) {
            this->options.ignore_warnings = true;
        } else if (strcmp(option, "ignore-everything") == 0) {
            this->creation_options.ignore_missing_symbol_table = true;
            this->creation_options.ignore_missing_uuids = true;
            this->creation_options.ignore_non_unique_uuids = true;
        } else if (strcmp(option, "ignore-missing-exports") == 0) {
            // "exports" includes sub-clients, reexports, and
            // symbols, but utils::tbd errors out only if
            // symbol-table is missing

            this->creation_options.ignore_missing_symbol_table = true;
            this->write_options.enforce_has_exports = false;
        } else if (strcmp(option, "ignore-missing-uuids") == 0) {
            this->creation_options.ignore_missing_uuids = true;
        } else if (strcmp(option, "ignore-non-unique-uuids") == 0) {
            this->creation_options.ignore_non_unique_uuids = true;
        } else if (strcmp(option, "ignore-warnings") == 0) {
            this->options.ignore_warnings = true;
        } else if (strcmp(option, "remove-clients") == 0) {
            this->creation_options.ignore_clients = true;
            this->write_options.ignore_clients = true;
        } else if (strcmp(option, "remove-archs") == 0) {
            if (this->options.replace_architectures && !this->options.remove_architectures) {
                fputs("Can't both remove architectures and replace select architectures from list found\n", stderr);
                exit(1);
            }
            
            // Simply validate architectures for now
            
            main_utils::parse_architectures_list(++index, argc, argv);
            this->options.remove_architectures = true;
            
            // Write out replaced architectures
            // over each export's architectures
            
            this->write_options.ignore_export_architectures = true;
        } else if (strcmp(option, "remove-current-version") == 0) {
            this->creation_options.ignore_current_version = true;
            this->write_options.ignore_current_version = true;
        } else if (strcmp(option, "remove-compatibility-version") == 0) {
            this->creation_options.ignore_compatibility_version = true;
            this->write_options.ignore_compatibility_version = true;
        } else if (strcmp(option, "remove-exports") == 0) {
            this->creation_options.ignore_exports = true;
            this->write_options.ignore_exports = true;
        } else if (strcmp(option, "remove-flags") == 0) {
            if (this->options.replace_flags && !this->options.remove_flags) {
                fputs("Can't both remove architectures and replace select architectures from list found\n", stderr);
                exit(1);
            }
            
            // Simply validate flags for now
            
            struct utils::tbd::flags flags;
            main_utils::parse_flags(&flags, index, argc, argv);
            
            this->options.remove_flags = true;
        } else if (strcmp(option, "remove-flags-field") == 0) {
            this->creation_options.ignore_flags = true;
            this->write_options.ignore_flags = true;
        } else if (strcmp(option, "remove-install-name") == 0) {
            this->creation_options.ignore_install_name = true;
            this->write_options.ignore_install_name = true;
        } else if (strcmp(option, "remove-objc-constraint") == 0) {
            this->creation_options.ignore_objc_constraint = true;
            this->write_options.ignore_objc_constraint = true;
        } else if (strcmp(option, "remove-parent-umbrella") == 0) {
            this->creation_options.ignore_parent_umbrella = true;
            this->write_options.ignore_parent_umbrella = true;
        } else if (strcmp(option, "remove-platform") == 0) {
            this->creation_options.ignore_platform = true;
            this->write_options.ignore_platform = true;
        } else if (strcmp(option, "remove-reexports") == 0) {
            this->creation_options.ignore_reexports = true;
            this->write_options.ignore_reexports = true;
        } else if (strcmp(option, "remove-swift-version") == 0) {
            this->creation_options.ignore_swift_version = true;
            this->write_options.ignore_swift_version = true;
        } else if (strcmp(option, "remove-uuids") == 0) {
            this->creation_options.ignore_uuids = true;
            this->write_options.ignore_uuids = true;
        } else if (strcmp(option, "replace-archs") == 0) {
            if (this->options.remove_architectures && !this->options.replace_architectures) {
                fputs("Can't both replace architectures and remove select architectures from list found\n", stderr);
                exit(1);
            }
            
            // Simply validate architectures for now

            main_utils::parse_architectures_list(++index, argc, argv);

            // We need to stop tbd::create from
            // overiding these replaced architectures

            this->creation_options.ignore_architectures = true;
            this->options.replace_architectures = true;
            
            // Don't write out uuids for
            // architectures we don't have
            
            this->creation_options.ignore_uuids = true;
            this->write_options.ignore_uuids = true;
            
            // Write out replaced architectures
            // over each export's architectures
            
            this->write_options.ignore_export_architectures = true;
        } else if (strcmp(option, "replace-objc-constraint") == 0) {
            this->info.objc_constraint = main_utils::parse_objc_constraint_from_argument(index, argc, argv);
            this->creation_options.ignore_objc_constraint = true;
            this->options.replaced_objc_constraint = true;
        } else if (strcmp(option, "replace-platform") == 0) {
            this->info.platform = main_utils::parse_platform_from_argument(index, argc, argv);
            this->creation_options.ignore_platform = true;
        } else if (strcmp(option, "replace-swift-version") == 0) {
            this->info.swift_version = parse_swift_version_from_argument(index, argc, argv);
            this->creation_options.ignore_swift_version = true;
        } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
            this->version = main_utils::parse_tbd_version(index, argc, argv);
        } else {
            return false;
        }

        return true;
    }

    bool tbd_with_options::parse_dont_option_from_argv(const char *option, int &index, int argc, const char *argv[]) noexcept {
        if (strcmp(option, "dont-allow-private-normal-symbols") == 0) {
            this->creation_options.allow_private_normal_symbols = false;
           this->donts.creation_options.allow_private_normal_symbols = true;
        } else if (strcmp(option, "dont-allow-private-weak-symbols") == 0) {
            this->creation_options.allow_private_weak_symbols = false;
           this->donts.creation_options.allow_private_weak_symbols = true;
        } else if (strcmp(option, "dont-allow-private-objc-symbols") == 0) {
            this->creation_options.allow_private_objc_class_symbols = false;
            this->creation_options.allow_private_objc_ivar_symbols = false;

           this->donts.creation_options.allow_private_objc_class_symbols = true;
           this->donts.creation_options.allow_private_objc_ivar_symbols = true;
        } else if (strcmp(option, "dont-allow-private-objc-class-symbols") == 0) {
            this->creation_options.allow_private_objc_class_symbols = false;
           this->donts.creation_options.allow_private_objc_class_symbols = true;
        } else if (strcmp(option, "dont-allow-private-objc-ivar-symbols") == 0) {
            this->creation_options.allow_private_objc_ivar_symbols = false;
           this->donts.creation_options.allow_private_objc_ivar_symbols = true;
        } else if (strcmp(option, "dont-ignore-everything") == 0) {
            this->creation_options.ignore_missing_symbol_table = false;
            this->creation_options.ignore_missing_uuids = false;
            this->creation_options.ignore_non_unique_uuids = false;

           this->donts.creation_options.ignore_missing_symbol_table = true;
           this->donts.creation_options.ignore_missing_uuids = true;
           this->donts.creation_options.ignore_non_unique_uuids = true;
        } else if (strcmp(option, "dont-ignore-missing-exports") == 0) {
            // "exports" includes sub-clients, reexports, and
            // symbols, but utils::tbd errors out only if
            // symbol-table is missing

            this->creation_options.ignore_missing_symbol_table = false;
            this->donts.creation_options.ignore_missing_symbol_table = true;
            
            this->write_options.enforce_has_exports = true;
            this->donts.write_options.enforce_has_exports = false;
        } else if (strcmp(option, "dont-ignore-missing-uuids") == 0) {
            this->creation_options.ignore_missing_uuids = false;
           this->donts.creation_options.ignore_missing_uuids = true;
        } else if (strcmp(option, "dont-ignore-non-unique-uuids") == 0) {
            this->creation_options.ignore_non_unique_uuids = false;
           this->donts.creation_options.ignore_non_unique_uuids = true;
        } else if (strcmp(option, "dont-ignore-warnings") == 0) {
            this->options.ignore_warnings = false;
           this->donts.options.ignore_warnings = true;
        } else if (strcmp(option, "dont-remove-clients") == 0) {
            this->creation_options.ignore_clients = false;
           this->donts.creation_options.ignore_clients = true;
        } else if (strcmp(option, "dont-remove-current-version") == 0) {
            this->creation_options.ignore_current_version = false;
            this->write_options.ignore_current_version = false;

           this->donts.creation_options.ignore_current_version = true;
           this->donts.write_options.ignore_current_version = true;
        } else if (strcmp(option, "dont-remove-compatibility-version") == 0) {
            this->creation_options.ignore_compatibility_version = false;
            this->write_options.ignore_compatibility_version = false;

           this->donts.creation_options.ignore_compatibility_version = true;
           this->donts.write_options.ignore_compatibility_version = true;
        } else if (strcmp(option, "dont-remove-exports") == 0) {
            this->creation_options.ignore_exports = false;
            this->write_options.ignore_exports = false;

           this->donts.creation_options.ignore_exports = true;
           this->donts.write_options.ignore_exports = true;
        } else if (strcmp(option, "dont-remove-flags-field") == 0) {
            this->creation_options.ignore_flags = false;
            this->write_options.ignore_flags = false;

           this->donts.creation_options.ignore_flags = true;
           this->donts.write_options.ignore_flags = true;
        } else if (strcmp(option, "dont-remove-install-name") == 0) {
            this->creation_options.ignore_install_name = false;
            this->write_options.ignore_install_name = false;

           this->donts.creation_options.ignore_install_name = true;
           this->donts.write_options.ignore_install_name = true;
        } else if (strcmp(option, "dont-remove-objc-constraint") == 0) {
            this->creation_options.ignore_objc_constraint = false;
            this->write_options.ignore_objc_constraint = false;

           this->donts.creation_options.ignore_objc_constraint = true;
           this->donts.write_options.ignore_objc_constraint = true;
        } else if (strcmp(option, "dont-remove-parent-umbrella") == 0) {
            this->creation_options.ignore_parent_umbrella = false;
            this->write_options.ignore_parent_umbrella = false;

           this->donts.creation_options.ignore_parent_umbrella = true;
           this->donts.write_options.ignore_parent_umbrella = true;
        } else if (strcmp(option, "dont-remove-platform") == 0) {
            this->creation_options.ignore_platform = false;
            this->write_options.ignore_platform = false;

           this->donts.creation_options.ignore_platform = true;
           this->donts.write_options.ignore_platform = true;
        } else if (strcmp(option, "dont-remove-reexports") == 0) {
            this->creation_options.ignore_reexports = false;
            this->write_options.ignore_reexports = false;

           this->donts.creation_options.ignore_reexports = true;
           this->donts.write_options.ignore_reexports = true;
        } else if (strcmp(option, "dont-remove-swift-version") == 0) {
            this->creation_options.ignore_swift_version = false;
            this->write_options.ignore_swift_version = false;

           this->donts.creation_options.ignore_swift_version = true;
           this->donts.write_options.ignore_swift_version = true;
        } else if (strcmp(option, "dont-remove-uuids") == 0) {
            this->creation_options.ignore_uuids = false;
            this->write_options.ignore_uuids = false;

           this->donts.creation_options.ignore_uuids = true;
           this->donts.write_options.ignore_uuids = true;
        } else {
            return false;
        }

        return true;
    }
}
