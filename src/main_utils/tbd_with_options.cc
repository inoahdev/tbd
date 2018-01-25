//
//  src/main_utils/tbd_with_options.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

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
}
