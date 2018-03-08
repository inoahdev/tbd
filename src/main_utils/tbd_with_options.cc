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

        if (this->info.platform == macho::utils::tbd::platform::none) {
            this->info.platform = tbd.info.platform;
        }

        if (tbd.options.replaced_objc_constraint) {
            this->info.objc_constraint = tbd.info.objc_constraint;
        }

        if (this->info.swift_version == 0) {
            this->info.swift_version = tbd.info.swift_version;
        }

        if (this->info.version == macho::utils::tbd::version::none) {
            this->info.version = tbd.info.version;
        }
    }

    void tbd_with_options::apply_local_options(int argc, const char *argv[]) noexcept {
        if (this->local_option_start == 0) {
            return;
        }

        struct macho::utils::tbd::flags flags_add;
        struct macho::utils::tbd::flags flags_re;

        for (auto index = this->local_option_start; index != argc; index++) {
            const auto &option = argv[index];
            if (strcmp(option, "add-flags") == 0) {
                if (this->options.replace_flags ^ this->options.remove_flags) {
                    continue;
                }

                main_utils::parse_flags(&flags_add, ++index, argc, argv);
            } else if (strcmp(option, "replace-flags") == 0) {
                if (this->options.remove_flags) {
                    continue;
                }

                this->info.flags.clear();
                main_utils::parse_flags(&flags_re, ++index, argc, argv);
            } else if (strcmp(option, "remove-flags") == 0) {
                if (this->options.replace_flags) {
                    continue;
                }

                main_utils::parse_flags(&flags_re, ++index, argc, argv);
            } else if (option[0] != '-') {
                break;
            }

            // Ignore handling of any unrecognized options
            // as by this point, argv has been fully validated
        }

        if (this->options.remove_flags) {
            this->info.flags.value |= flags_add.value;
            this->info.flags.value &= ~flags_re.value;
        } else {
            this->info.flags.value = flags_re.value;
        }
    }

    bool tbd_with_options::parse_option_from_argv(const char *option, int &index, int argc, const char *argv[]) noexcept {
        if (strcmp(option, "allow-private-normal-symbols") == 0) {
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
        } else if (strcmp(option, "ignore-everything") == 0) {
            this->creation_options.ignore_missing_symbol_table = true;
            this->creation_options.ignore_missing_uuids = true;
            this->creation_options.ignore_non_unique_uuids = true;
        } else if (strcmp(option, "ignore-missing-exports") == 0) {
            // "exports" includes sub-clients, reexports, and
            // symbols, but utils::tbd errors out only if
            // symbol-table is missing

            this->creation_options.ignore_missing_symbol_table = true;
        } else if (strcmp(option, "ignore-missing-uuids") == 0) {
            this->creation_options.ignore_missing_uuids = true;
        } else if (strcmp(option, "ignore-non-unique-uuids") == 0) {
            this->creation_options.ignore_non_unique_uuids = true;
        } else if (strcmp(option, "ignore-warnings") == 0) {
            this->options.ignore_warnings = true;
        } else if (strcmp(option, "remove-allowable-clients") == 0) {
            this->creation_options.ignore_allowable_clients = true;
            this->write_options.ignore_allowable_clients = true;
        } else if (strcmp(option, "remove-current-version") == 0) {
            this->creation_options.ignore_current_version = true;
            this->write_options.ignore_current_version = true;
        } else if (strcmp(option, "remove-compatibility-version") == 0) {
            this->creation_options.ignore_compatibility_version = true;
            this->write_options.ignore_compatibility_version = true;
        } else if (strcmp(option, "remove-exports") == 0) {
            this->creation_options.ignore_exports = true;
            this->write_options.ignore_exports = true;
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
            if (this->options.remove_architectures) {
                if (this->options.replace_architectures) {
                    fputs("Can't both replace architectures and add architectures to list found\n", stderr);
                } else {
                    fputs("Can't both replace architectures and remove select architectures from list found\n", stderr);
                }

                exit(1);
            }

            // Replacing architectures 'resets' the architectures list,
            // replacing not only add-archs options, but earlier provided
            // replace-archs options

            this->info.architectures = main_utils::parse_architectures_list(++index, argc, argv);

            // We need to stop tbd::create from
            // overiding these replaced architectures

            this->creation_options.ignore_architectures = true;
            this->options.replace_architectures = true;
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
            this->info.version = main_utils::parse_tbd_version(index, argc, argv);
        } else {
            return false;
        }

        return true;
    }

    bool tbd_with_options::parse_dont_option_from_argv(const char *option, int &index, int argc, const char *argv[], dont_options &donts) noexcept {
        if (strcmp(option, "dont-allow-private-normal-symbols") == 0) {
            this->creation_options.allow_private_normal_symbols = false;
            donts.creation_options.allow_private_normal_symbols = true;
        } else if (strcmp(option, "dont-allow-private-weak-symbols") == 0) {
            this->creation_options.allow_private_weak_symbols = false;
            donts.creation_options.allow_private_weak_symbols = true;
        } else if (strcmp(option, "dont-allow-private-objc-symbols") == 0) {
            this->creation_options.allow_private_objc_class_symbols = false;
            this->creation_options.allow_private_objc_ivar_symbols = false;

            donts.creation_options.allow_private_objc_class_symbols = true;
            donts.creation_options.allow_private_objc_ivar_symbols = true;
        } else if (strcmp(option, "dont-allow-private-objc-class-symbols") == 0) {
            this->creation_options.allow_private_objc_class_symbols = false;
            donts.creation_options.allow_private_objc_class_symbols = true;
        } else if (strcmp(option, "dont-allow-private-objc-ivar-symbols") == 0) {
            this->creation_options.allow_private_objc_ivar_symbols = false;
            donts.creation_options.allow_private_objc_ivar_symbols = true;
        } else if (strcmp(option, "dont-ignore-everything") == 0) {
            this->creation_options.ignore_missing_symbol_table = false;
            this->creation_options.ignore_missing_uuids = false;
            this->creation_options.ignore_non_unique_uuids = false;

            donts.creation_options.ignore_missing_symbol_table = true;
            donts.creation_options.ignore_missing_uuids = true;
            donts.creation_options.ignore_non_unique_uuids = true;
        } else if (strcmp(option, "dont-ignore-missing-exports") == 0) {
            // "exports" includes sub-clients, reexports, and
            // symbols, but utils::tbd errors out only if
            // symbol-table is missing

            this->creation_options.ignore_missing_symbol_table = false;
            donts.creation_options.ignore_missing_symbol_table = true;
        } else if (strcmp(option, "dont-ignore-missing-uuids") == 0) {
            this->creation_options.ignore_missing_uuids = false;
            donts.creation_options.ignore_missing_uuids = true;
        } else if (strcmp(option, "dont-ignore-non-unique-uuids") == 0) {
            this->creation_options.ignore_non_unique_uuids = false;
            donts.creation_options.ignore_non_unique_uuids = true;
        } else if (strcmp(option, "dont-ignore-warnings") == 0) {
            this->options.ignore_warnings = false;
            donts.options.ignore_warnings = true;
        } else if (strcmp(option, "dont-remove-allowable-clients-field") == 0) {
            this->creation_options.ignore_allowable_clients = false;
            donts.creation_options.ignore_allowable_clients = true;
        } else if (strcmp(option, "dont-remove-current-version") == 0) {
            this->creation_options.ignore_current_version = false;
            this->write_options.ignore_current_version = false;

            donts.creation_options.ignore_current_version = true;
            donts.write_options.ignore_current_version = true;
        } else if (strcmp(option, "dont-remove-compatibility-version") == 0) {
            this->creation_options.ignore_compatibility_version = false;
            this->write_options.ignore_compatibility_version = false;

            donts.creation_options.ignore_compatibility_version = true;
            donts.write_options.ignore_compatibility_version = true;
        } else if (strcmp(option, "dont-remove-exports") == 0) {
            this->creation_options.ignore_exports = false;
            this->write_options.ignore_exports = false;

            donts.creation_options.ignore_exports = true;
            donts.write_options.ignore_exports = true;
        } else if (strcmp(option, "dont-remove-flags-field") == 0) {
            this->creation_options.ignore_flags = false;
            this->write_options.ignore_flags = false;

            donts.creation_options.ignore_flags = true;
            donts.write_options.ignore_flags = true;
        } else if (strcmp(option, "dont-remove-install-name") == 0) {
            this->creation_options.ignore_install_name = false;
            this->write_options.ignore_install_name = false;

            donts.creation_options.ignore_install_name = true;
            donts.write_options.ignore_install_name = true;
        } else if (strcmp(option, "dont-remove-objc-constraint") == 0) {
            this->creation_options.ignore_objc_constraint = false;
            this->write_options.ignore_objc_constraint = false;

            donts.creation_options.ignore_objc_constraint = true;
            donts.write_options.ignore_objc_constraint = true;
        } else if (strcmp(option, "dont-remove-parent-umbrella") == 0) {
            this->creation_options.ignore_parent_umbrella = false;
            this->write_options.ignore_parent_umbrella = false;

            donts.creation_options.ignore_parent_umbrella = true;
            donts.write_options.ignore_parent_umbrella = true;
        } else if (strcmp(option, "dont-remove-platform") == 0) {
            this->creation_options.ignore_platform = false;
            this->write_options.ignore_platform = false;

            donts.creation_options.ignore_platform = true;
            donts.write_options.ignore_platform = true;
        } else if (strcmp(option, "dont-remove-reexports") == 0) {
            this->creation_options.ignore_reexports = false;
            this->write_options.ignore_reexports = false;

            donts.creation_options.ignore_reexports = true;
            donts.write_options.ignore_reexports = true;
        } else if (strcmp(option, "dont-remove-swift-version") == 0) {
            this->creation_options.ignore_swift_version = false;
            this->write_options.ignore_swift_version = false;

            donts.creation_options.ignore_swift_version = true;
            donts.write_options.ignore_swift_version = true;
        } else if (strcmp(option, "dont-remove-uuids") == 0) {
            this->creation_options.ignore_uuids = false;
            this->write_options.ignore_uuids = false;

            donts.creation_options.ignore_uuids = true;
            donts.write_options.ignore_uuids = true;
        } else {
            return false;
        }

        return true;
    }
}
