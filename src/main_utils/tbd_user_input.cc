//
//  src/main_utils/tbd_user_input.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "tbd_print_field_information.h"
#include "tbd_user_input.h"

namespace main_utils {
    void request_input(std::string &string, const char *prompt, const std::initializer_list<const char *> &acceptable_inputs) noexcept {
        do {
            // fflush is required here otherwise
            // its printed out of order (at least in
            // terminal, works fine here in Xcode)

            fputs(prompt, stdout);
            fflush(stdout);

            if (acceptable_inputs.size() != 0) {
                fprintf(stdout, " (%s", *acceptable_inputs.begin());
                fflush(stdout);

                const auto acceptable_inputs_end = acceptable_inputs.end();
                for (auto iter = acceptable_inputs.begin() + 1; iter != acceptable_inputs_end; iter++) {
                    fprintf(stderr, ", %s", *iter);
                }

                fputs(")", stdout);
                fflush(stdout);
            }

            fputs(": ", stdout);
            fflush(stdout);

            getline(std::cin, string);

            if (acceptable_inputs.size() == 0) {
                break;
            }

            auto done = false;
            for (const auto &input : acceptable_inputs) {
                if (string == input) {
                    done = true;
                    break;
                }
            }

            if (done) {
                break;
            }
        } while (true);
    }

    enum create_tbd_retained : uint64_t {
        create_tbd_retained_never_replace_platform          = 1 << 0,
        create_tbd_retained_never_replace_installation_name = 1 << 1,
        create_tbd_retained_never_replace_objc_constraint   = 1 << 2,
        create_tbd_retained_never_replace_swift_version     = 1 << 3,
        create_tbd_retained_never_replace_parent_umbrella   = 1 << 4,
        create_tbd_retained_never_ignore_flags              = 1 << 5,
        create_tbd_retained_never_ignore_uuids              = 1 << 6,
        create_tbd_retained_never_ignore_non_unique_uuids   = 1 << 7,
        create_tbd_retained_never_ignore_missing_uuids      = 1 << 8
    };

    bool request_new_platform(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
           if (*info & create_tbd_retained_never_replace_platform) {
               return false;
           }
        }

        if (all.creation_options.ignores_platform() &&
            all.info.platform != macho::utils::tbd::platform::none) {
            return true;
        }

        auto result = std::string();

        if (info != nullptr) {
           request_input(result, "Replace platform?", { "all", "yes", "no", "never" });
        } else {
           request_input(result, "Replace platform?", { "yes", "no" });
        }

        if (result == "never") {
           if (info != nullptr) {
               *info |= create_tbd_retained_never_replace_platform;
           }

           return false;
        } else if (result == "none") {
           return false;
        }

        auto apply_to_all = result == "all";
        auto new_platform = macho::utils::tbd::platform::none;

        do {
           request_input(result, "Replacement platform: (--list-platform to see a list of platforms)", {});
           if (result == "--list-platform") {
               print_tbd_platforms();
           } else {
               new_platform = macho::utils::tbd::platform_from_string(result.c_str());
           }
        } while (new_platform == macho::utils::tbd::platform::none);

        tbd.info.platform = new_platform;
        tbd.creation_options.ignore_platform();

        if (apply_to_all) {
           all.info.platform = new_platform;
           all.creation_options.ignore_platform();
        }

        return true;
    }

    bool request_new_installation_name(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
            if (*info & create_tbd_retained_never_replace_installation_name) {
                return false;
            }
        }

        if (all.creation_options.ignores_install_name() &&
            all.info.install_name.size() != 0) {
            return true;
        }

        auto result = std::string();

        if (info != nullptr) {
            request_input(result, "Replace installation-name?", { "all", "yes", "no", "never" });
        } else {
            request_input(result, "Replace installation-name?", { "yes", "no" });
        }

        if (result == "never") {
            if (info != nullptr) {
                *info |= create_tbd_retained_never_replace_installation_name;
            }

            return false;
        } else if (result == "none") {
            return false;
        }

        auto apply_to_all = result == "all";

        do {
            request_input(result, "Replacement installation-name", {});
        } while (result.empty());

        tbd.info.install_name = std::move(result);
        tbd.creation_options.ignore_install_name();

        if (apply_to_all) {
            all.info.install_name = tbd.info.install_name;
            all.creation_options.ignore_install_name();
        }

        return true;
    }

    bool request_new_objc_constraint(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
            if (*info & create_tbd_retained_never_replace_objc_constraint) {
                return false;
            }
        }

        if (all.creation_options.ignores_objc_constraint() &&
            all.info.objc_constraint != macho::utils::tbd::objc_constraint::none) {
            return true;
        }

        auto result = std::string();

        if (info != nullptr) {
            request_input(result, "Replace objc-constraint?", { "all", "yes", "no", "never" });
        } else {
            request_input(result, "Replace objc-constraint?", { "yes", "no" });
        }

        if (result == "never") {
            if (info != nullptr) {
                *info |= create_tbd_retained_never_replace_objc_constraint;
            }

            return false;
        } else if (result == "no") {
            return false;
        }

        auto apply_to_all = result == "all";
        auto new_objc_constraint = macho::utils::tbd::objc_constraint::none;

        do {
            request_input(result, "Replacement objc-constraint", {});
            new_objc_constraint = macho::utils::tbd::objc_constraint_from_string(result.c_str());

            if (result == "none") {
                break;
            }
        } while (true);

        tbd.info.objc_constraint = new_objc_constraint;
        tbd.creation_options.ignore_objc_constraint();

        if (apply_to_all) {
            all.info.objc_constraint = new_objc_constraint;
            all.creation_options.ignore_objc_constraint();
        }

        return true;
    }

    bool request_new_parent_umbrella(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
            if (*info & create_tbd_retained_never_replace_parent_umbrella) {
                return false;
            }
        }

        if (all.creation_options.ignores_parent_umbrella() &&
            all.info.parent_umbrella.size() != 0) {
            return true;
        }

        auto result = std::string();

        if (info != nullptr) {
            request_input(result, "Replace parent-umbrella?", { "all", "yes", "no", "never" });
        } else {
            request_input(result, "Replace parent-umbrella?", { "yes", "no" });
        }

        if (result == "never") {
            if (info != nullptr) {
                *info |= create_tbd_retained_never_replace_parent_umbrella;
            }

            return false;
        } else if (result == "no") {
            return false;
        }

        auto apply_to_all = result == "all";
        do {
            request_input(result, "Replacement parent-umbrella", {});
        } while (result.empty());

        tbd.info.parent_umbrella = std::move(result);
        tbd.creation_options.ignore_parent_umbrella();

        if (apply_to_all) {
            all.info.parent_umbrella = tbd.info.parent_umbrella;
            all.creation_options.ignore_parent_umbrella();
        }

        return true;
    }

    bool request_new_swift_version(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
            if (*info & create_tbd_retained_never_replace_swift_version) {
                return false;
            }
        }

        if (all.creation_options.ignores_swift_version() &&
            all.info.swift_version != 0) {
            return true;
        }

        auto result = std::string();

        if (info != nullptr) {
            request_input(result, "Replace swift-version?", { "all", "yes", "no", "never" });
        } else {
            request_input(result, "Replace swift-version?", { "yes", "no" });
        }

        if (result == "never") {
            if (info != nullptr) {
                *info |= create_tbd_retained_never_replace_swift_version;
            }

            return false;
        } else if (result == "no") {
            return false;
        }

        auto apply_to_all = result == "all";
        auto new_swift_version = uint32_t();

        do {
            request_input(result, "Replacement swift-version", {});
        } while ((new_swift_version = (uint32_t)strtoul(result.c_str(), nullptr, 10)) != 0);

        tbd.info.swift_version = (uint32_t)new_swift_version;
        tbd.creation_options.ignore_swift_version();

        if (apply_to_all) {
            all.info.swift_version = new_swift_version;
            all.creation_options.ignore_swift_version();
        }

        return true;
    }

    bool request_if_should_ignore_flags(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
            if (*info & create_tbd_retained_never_replace_swift_version) {
                return false;
            }
        }

        if (all.creation_options.ignores_flags()) {
            return true;
        }

        auto result = std::string();
        if (info != nullptr) {
            request_input(result, "Ignore flags?", { "all", "yes", "no", "never" });
        } else {
            request_input(result, "Ignore flags?", { "yes", "no" });
        }

        if (result == "never") {
            if (info != nullptr) {
                *info |= create_tbd_retained_never_replace_swift_version;
            }

            return false;
        } else if (result == "no") {
            return false;
        }

        tbd.creation_options.ignore_flags();
        tbd.write_options.ignore_flags();

        if (result == "all") {
            all.creation_options.ignore_flags();
            all.write_options.ignore_flags();
        }

        return true;
    }

    bool request_if_should_ignore_uuids(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
            if (*info & create_tbd_retained_never_ignore_uuids) {
                return false;
            }
        }

        if (all.creation_options.ignores_uuids()) {
            return true;
        }

        auto result = std::string();
        if (info != nullptr) {
            request_input(result, "Ignore uuids?", { "all", "yes", "no", "never" });
        } else {
            request_input(result, "Ignore uuids?", { "yes", "no" });
        }

        if (result == "never") {
            if (info != nullptr) {
                *info |= create_tbd_retained_never_ignore_uuids;
            }

            return false;
        } else if (result == "no") {
            return false;
        }

        tbd.creation_options.ignore_uuids();
        tbd.write_options.ignore_uuids();

        if (result == "all") {
            all.creation_options.ignore_uuids();
            all.write_options.ignore_uuids();
        }

        return true;
    }

    bool request_if_should_ignore_non_unique_uuids(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
            if (*info & create_tbd_retained_never_ignore_non_unique_uuids) {
                return false;
            }
        }

        if (all.creation_options.ignores_non_unique_uuids()) {
            return true;
        }

        auto result = std::string();

        if (info != nullptr) {
            request_input(result, "Ignore non-unique uuids?", { "all", "yes", "no", "never" });
        } else {
            request_input(result, "Ignore non-unique uuids?", { "yes", "no" });
        }

        if (result == "never") {
            if (info != nullptr) {
                *info |= create_tbd_retained_never_ignore_non_unique_uuids;
            }

            return false;
        } else if (result == "no") {
            return false;
        }

        tbd.creation_options.ignore_non_unique_uuids();
        if (result == "all") {
            all.creation_options.ignore_non_unique_uuids();
        }

        return true;
    }

    bool request_if_should_ignore_missing_uuids(tbd_with_options &all, tbd_with_options &tbd, uint64_t *info) noexcept {
        if (info != nullptr) {
            if (*info & create_tbd_retained_never_ignore_missing_uuids) {
                return false;
            }
        }

        if (all.creation_options.ignores_missing_uuids()) {
            return true;
        }

        auto result = std::string();

        if (info != nullptr) {
            request_input(result, "Ignore missing uuids?", { "all", "yes", "no", "never" });
        } else {
            request_input(result, "Ignore missing uuids?", { "yes", "no" });
        }

        if (result == "never") {
            if (info != nullptr) {
                *info |= create_tbd_retained_never_ignore_missing_uuids;
            }

            return false;
        } else if (result == "no") {
            return false;
        }

        tbd.creation_options.ignore_missing_uuids();
        if (result == "all") {
            all.creation_options.ignore_missing_uuids();
        }

        return true;
    }
}
