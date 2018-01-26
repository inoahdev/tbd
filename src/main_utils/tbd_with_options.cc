//
//  src/main_utils/tbd_with_options.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "tbd_parse_fields.h"
#include "tbd_with_options.h"

namespace main_utils {
    void tbd_with_options::apply_missing_from(const tbd_with_options &options) noexcept {
        if (this->info.architectures == 0) {
            this->info.architectures = options.info.architectures;
        }

        if (this->info.current_version.value == 0) {
            this->info.current_version.value = options.info.current_version.value;
        }

        if (this->info.compatibility_version.value == 0) {
            this->info.compatibility_version.value = options.info.compatibility_version.value;
        }

        if (this->info.flags.value == 0) {
            this->info.flags.value = options.info.flags.value;
        }

        if (this->info.install_name.empty()) {
            this->info.install_name = options.info.install_name;
        }

        if (this->info.parent_umbrella.empty()) {
            this->info.parent_umbrella = options.info.parent_umbrella;
        }

        if (this->info.platform == macho::utils::tbd::platform::none) {
            this->info.platform = options.info.platform;
        }

        if (this->info.version == macho::utils::tbd::version::none) {
            this->info.version = options.info.version;
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
}
