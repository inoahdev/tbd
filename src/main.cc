//
//  src/main.cc
//  tbd
//
//  Created by inoahdev on 4/16/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include <sys/stat.h>
#include <iostream>

#include "mach-o/utils/tbd.h"

#include "misc/current_directory.h"
#include "misc/recurse.h"

#include "recursive/mkdir.h"
#include "recursive/remove.h"

uint64_t parse_architectures_list(int &index, int argc, const char *argv[]) {
    if (index == argc) {
        fputs("Please provide a list of architectures\n", stderr);
        exit(1);
    }

    auto architectures = uint64_t();
    do {
        const auto &architecture_string = argv[index];
        const auto &architecture_string_front = architecture_string[0];

        // Quickly filter out an option or path instead of a (relatively)
        // expensive call to macho::architecture_info_from_name().

        if (architecture_string_front == '-' || architecture_string_front == '/' || architecture_string_front == '\\') {
            // If the architectures vector is empty, the user did not provide any architectures
            // but did provided the architecture option, which requires at least one architecture
            // being provided.

            if (!architectures) {
                fputs("Please provide a list of architectures to override the ones in the provided mach-o file(s)\n", stderr);
                exit(1);
            }

            break;
        }

        const auto architecture_info_table_index = macho::architecture_info_index_from_name(architecture_string);
        if (architecture_info_table_index == -1) {
            // macho::architecture_info_index_from_name() returning -1 can be the result of one
            // of two scenarios, The string stored in architecture being invalid, such as being
            // misspelled, or the string being the path object inevitably following the architecture
            // argument.

            // If the architectures vector is empty, the user did not provide any architectures
            // but did provide the architecture option, which requires at least one architecture
            // being provided.

            if (!architectures) {
                fprintf(stderr, "Unrecognized architecture with name: %s\n", architecture_string);
                exit(1);
            }

            break;
        }

        architectures |= (1ull << architecture_info_table_index);
        index++;
    } while (index < argc);

    // As the caller of this function is in a for loop,
    // the index is incremented once again once this function
    // returns. To make up for this, decrement index by 1.

    index--;
    return architectures;
}

void request_input(std::string &string, const char *prompt, const std::initializer_list<const char *> &acceptable_inputs) {
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

struct tbd_with_options {
    macho::utils::tbd info;
    
    macho::utils::tbd::creation_options creation_options;
    macho::utils::tbd::write_options write_options;
    
    // path is main argument provided
    // to option -p.
    
    std::string path;
    
    // If write_path is empty, that is
    // the user didn't provide an output-path,
    // tbd should reroute its output to stdout,
    // except when recursing
    
    std::string write_path;
    
    // Pointer to first option in local-options
    // Used to apply any options for writing
    
    int local_option_start = 0;
    
    // architectures is a bitset of indexes to the
    // architecture_info_table to supply as architecture
    // list
    
    uint64_t architectures = uint64_t();
    
    // containers is a bitset of indexes of
    // containers to create tbd from
    
    // XXX: Create code to handle custom
    // containers for tbd-create
    
    utils::bits containers;
    uint64_t options = uint64_t();
};

void print_tbd_platforms() {
    fputs(macho::utils::tbd::platform_to_string(static_cast<enum macho::utils::tbd::platform>(1)), stdout);
    for (int i = 2; i < 31; i++) {
        fprintf(stdout, ", %s", macho::utils::tbd::platform_to_string(static_cast<enum macho::utils::tbd::platform>(i)));
    }
    
    fputc('\n', stdout);
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

bool request_new_platform(tbd_with_options &all, tbd_with_options &tbd, uint64_t *retain) {
    if (retain != nullptr) {
        if (*retain & create_tbd_retained_never_replace_platform) {
            return false;
        }
    }
    
    auto result = std::string();
    
    if (retain != nullptr) {
        request_input(result, "Replace platform?", { "all", "yes", "no", "never" });
    } else {
        request_input(result, "Replace platform?", { "yes", "no" });
    }
    
    if (result == "never") {
        if (retain != nullptr) {
            *retain |= create_tbd_retained_never_replace_installation_name;
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

bool request_new_installation_name(tbd_with_options &all, tbd_with_options &tbd, uint64_t *retain) {
    if (retain != nullptr) {
        if (*retain & create_tbd_retained_never_replace_installation_name) {
            return false;
        }
    }
    
    auto result = std::string();
    
    if (retain != nullptr) {
        request_input(result, "Replace installation-name?", { "all", "yes", "no", "never" });
    } else {
        request_input(result, "Replace installation-name?", { "yes", "no" });
    }
    
    if (result == "never") {
        if (retain != nullptr) {
            *retain |= create_tbd_retained_never_replace_installation_name;
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

bool request_new_objc_constraint(tbd_with_options &all, tbd_with_options &tbd, uint64_t *retain) {
    if (retain != nullptr) {
        if (*retain & create_tbd_retained_never_replace_objc_constraint) {
            return false;
        }
    }
    
    auto result = std::string();
    
    if (retain != nullptr) {
        request_input(result, "Replace objc-constraint?", { "all", "yes", "no", "never" });
    } else {
        request_input(result, "Replace objc-constraint?", { "yes", "no" });
    }
    
    if (result == "never") {
        if (retain != nullptr) {
            *retain |= create_tbd_retained_never_replace_objc_constraint;
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

bool request_new_parent_umbrella(tbd_with_options &all, tbd_with_options &tbd, uint64_t *retain) {
    if (retain != nullptr) {
        if (*retain & create_tbd_retained_never_replace_parent_umbrella) {
            return false;
        }
    }
    
    auto result = std::string();
    
    if (retain != nullptr) {
        request_input(result, "Replace parent-umbrella?", { "all", "yes", "no", "never" });
    } else {
        request_input(result, "Replace parent-umbrella?", { "yes", "no" });
    }
    
    if (result == "never") {
        if (retain != nullptr) {
            *retain |= create_tbd_retained_never_replace_parent_umbrella;
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

bool request_new_swift_version(tbd_with_options &all, tbd_with_options &tbd, uint64_t *retain) {
    if (retain != nullptr) {
        if (*retain & create_tbd_retained_never_replace_swift_version) {
            return false;
        }
    }
    
    auto result = std::string();
    
    if (retain != nullptr) {
        request_input(result, "Replace swift-version?", { "all", "yes", "no", "never" });
    } else {
        request_input(result, "Replace swift-version?", { "yes", "no" });
    }
    
    if (result == "never") {
        if (retain != nullptr) {
            *retain |= create_tbd_retained_never_replace_parent_umbrella;
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

enum create_tbd_options : uint64_t {
    create_tbd_option_ignore_missing_dynamic_library_information = 1 << 0
};

bool create_tbd(tbd_with_options &all, tbd_with_options &tbd, macho::file &file, uint64_t options, uint64_t *retained, const char *path, bool should_print_paths) {
    do {
        switch (tbd.info.create(file, tbd.creation_options)) {
            case macho::utils::tbd::creation_result::ok:
                return true;
                
            case macho::utils::tbd::creation_result::invalid_container_header_subtype:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an unrecognized cpu-subtype\n", path);
                } else {
                    fputs("Mach-o file at provided path has an unrecognized cpu-subtype\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::unrecognized_container_cputype_subtype_pair:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o fie (at path %s) has an unrecognized cputype-subtype pair\n", path);
                } else {
                    fputs("Mach-o fie at provided path has an unrecognized cputype-subtype pair\n", stderr);
                }
                
                return false;

            case macho::utils::tbd::creation_result::flags_mismatch: {
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has different flags in different architectures\n", path);
                } else {
                    fputs("Mach-o file at provided path has different flags in different architectures\n", stderr);
                }
                
                auto result = std::string();
                if (retained != nullptr) {
                    request_input(result, "Ignore flags?", { "all", "yes", "no", "never" });
                } else {
                    request_input(result, "Ignore flags?", { "yes", "no" });
                }
                
                if (result == "never") {
                    *retained |= create_tbd_retained_never_ignore_flags;
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
                
                return false;
            }

            case macho::utils::tbd::creation_result::failed_to_retrieve_load_cammands:
                if (should_print_paths) {
                    fprintf(stderr, "Failed to retrieve load-commands of mach-o file (at path %s)\n", path);
                } else {
                    fputs("Failed to retrieve load-commands of mach-o file at provided path\n", stderr);
                }
                
                return false;

            case macho::utils::tbd::creation_result::unsupported_platform:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an unsupported platform\n", path);
                } else {
                    fputs("Mach-o file at provided path has an unsupported platform\n", stderr);
                }
                
                if (!request_new_platform(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::unrecognized_platform:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an unrecognized platform\n", path);
                } else {
                    fputs("Mach-o file at provided path has an unrecognized platform\n", stderr);
                }
                
                if (!request_new_platform(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::multiple_platforms:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has multiple different platforms\n", path);
                } else {
                    fputs("Mach-o file at provided path has multiple different platforms\n", stderr);
                }
                
                if (!request_new_platform(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::platforms_mismatch:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has different platforms for different architectures\n", path);
                } else {
                    fputs("Mach-o file at provided path has different platforms for different architectures\n", stderr);
                }
                
                if (!request_new_platform(all, tbd, retained)) {
                    return false;
                }
                
                break;
            
            case macho::utils::tbd::creation_result::invalid_build_version_command:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an invalid build-version load-command\n", path);
                } else {
                    fputs("Mach-o file at provided path has an invalid build-version load-command\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::invalid_dylib_command:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an invalid dylib-command load-command\n", path);
                } else {
                    fputs("Mach-o file at provided path has an invalid dylib-command load-command\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::invalid_segment_command:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an invalid segment-command load-command\n", path);
                } else {
                    fputs("Mach-o file at provided path has an invalid segment-command load-command\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::invalid_segment_command_64:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an invalid 64-bit segment-command load-command\n", path);
                } else {
                    fputs("Mach-o file at provided path has an invalid 64-bit segment-command load-command\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::empty_install_name:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an empty installation-name\n", path);
                } else {
                    fputs("Mach-o file at provided path has an empty installation-name\n", stderr);
                }
                
                if (!request_new_installation_name(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::multiple_current_versions:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has multiple different current-versions\n", path);
                } else {
                    fputs("Mach-o file at provided path has multiple different current-versions\n", stderr);
                }
                
                return false;

            case macho::utils::tbd::creation_result::current_versions_mismatch:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has different current-versions for different architectures\n", path);
                } else {
                    fputs("Mach-o file at provided path has different current-versions for different architectures\n", stderr);
                }
                
                return false;

            case macho::utils::tbd::creation_result::multiple_compatibility_versions:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has multiple different compatibility-versions\n", path);
                } else {
                    fputs("Mach-o file at provided path has multiple different compatibility-versions\n", stderr);
                }
                
                return false;

            case macho::utils::tbd::creation_result::compatibility_versions_mismatch:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has different compatibility-versions for different architectures\n", path);
                } else {
                    fputs("Mach-o file at provided path has different compatibility-versions for different architectures\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::multiple_install_names:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has multiple different installation-names\n", path);
                } else {
                    fputs("Mach-o file at provided path has multiple different installation-names\n", stderr);
                }
                
                if (!request_new_installation_name(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::install_name_mismatch:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has different installation-names for different architectures\n", path);
                } else {
                    fputs("Mach-o file at provided path has different installation-names for different architectures\n", stderr);
                }
                
                if (!request_new_installation_name(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::invalid_objc_imageinfo_segment_section:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an invalid objc image-info segment-section\n", path);
                } else {
                    fputs("Mach-o file at provided path has an invalid objc image-info segment-section\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::stream_seek_error:
            case macho::utils::tbd::creation_result::stream_read_error:
                if (should_print_paths) {
                    fprintf(stderr, "Encountered a stream error while parsing mach-o file (at path %s)\n", path);
                } else {
                    fputs("Encountered a stream error while parsing mach-o file at provided path\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::multiple_objc_constraint:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has multiple different objc-constraint types\n", path);
                } else {
                    fputs("Mach-o file at provided path has multiple different objc-constraint types\n", stderr);
                }
                
                if (!request_new_objc_constraint(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::objc_constraint_mismatch:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has different objc-constraint types for different containers\n", path);
                } else {
                    fputs("Mach-o file at provided path has different objc-constraint types for different containers\n", stderr);
                }
                
                if (!request_new_objc_constraint(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::multiple_swift_version:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has multiple different swift-versions\n", path);
                } else {
                    fputs("Mach-o file at provided path has multiple different swift-versions\n", stderr);
                }
                
                if (!request_new_swift_version(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::swift_version_mismatch:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has different swift-versions for different architectures\n", path);
                } else {
                    fputs("Mach-o file at provided path has different swift-versions for different architectures\n", stderr);
                }
                
                if (!request_new_swift_version(all, tbd, retained)) {
                    return false;
                }
                
                break;

            case macho::utils::tbd::creation_result::invalid_sub_client_command:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an invalid sub-client load-command\n", path);
                } else {
                    fputs("Mach-o file at provided path has an invalid sub-client load-command\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::invalid_sub_umbrella_command:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an invalid sub-umbrella load-command\n", path);
                } else {
                    fputs("Mach-o file at provided path has an invalid sub-umbrella load-command\n", stderr);
                }
                
                return false;
            
            case macho::utils::tbd::creation_result::empty_parent_umbrella:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an empty parent-umbrella\n", path);
                } else {
                    fputs("Mach-o file at provided path has an empty parent-umbrella\n", stderr);
                }
                
                if (!request_new_parent_umbrella(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::multiple_parent_umbrella:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has multiple different parent-umbrellas\n", path);
                } else {
                    fputs("Mach-o file at provided path has multiple different parent-umbrellas\n", stderr);
                }
                
                if (!request_new_parent_umbrella(all, tbd, retained)) {
                    return false;
                }
                
                break;
                
            case macho::utils::tbd::creation_result::parent_umbrella_mismatch:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has different parent-umbrella for different containers\n", path);
                } else {
                    fputs("Mach-o file at provided path has different parent-umbrella for different containers\n", stderr);
                }
                
                if (!request_new_parent_umbrella(all, tbd, retained)) {
                    return false;
                }
                
                break;
            
            case macho::utils::tbd::creation_result::invalid_symtab_command:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an invalid symtab-command load-command\n", path);
                } else {
                    fputs("Mach-o file at provided path has an invalid symtab-command load-command\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::multiple_symbol_tables:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has a container with multiple different symbol-tables\n", path);
                } else {
                    fputs("Mach-o file at provided path has a container with multiple different symbol-tables\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::multiple_uuids: {
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has a container with multiple different uuids\n", path);
                } else {
                    fputs("Mach-o file at provided path has a container with multiple different uuids\n", stderr);
                }
                
                auto result = std::string();
                
                if (retained != nullptr) {
                    request_input(result, "Ignore uuids?", { "all", "yes", "no", "never" });
                } else {
                    request_input(result, "Ignore uuids?", { "yes", "no" });
                }
                
                if (result == "never") {
                    *retained |= create_tbd_retained_never_ignore_uuids;
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
                
                break;
            }
                
            case macho::utils::tbd::creation_result::non_unique_uuid: {
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has multiple containers with the same uuid\n", path);
                } else {
                    fputs("Mach-o file at provided path has multiple containers with the same uuid\n", stderr);
                }
                
                auto result = std::string();
                
                if (retained != nullptr) {
                    request_input(result, "Ignore non-unique uuids?", { "all", "yes", "no", "never" });
                } else {
                    request_input(result, "Ignore non-unique uuids?", { "yes", "no" });
                }
                
                if (result == "never") {
                    *retained |= create_tbd_retained_never_ignore_non_unique_uuids;
                    return false;
                } else if (result == "no") {
                    return false;
                }
                
                tbd.creation_options.ignore_non_unique_uuids();
                
                if (result == "all") {
                    all.creation_options.ignore_non_unique_uuids();
                }
                
                break;
            }
                
            case macho::utils::tbd::creation_result::container_is_missing_dynamic_library_identification:
                if (options & create_tbd_option_ignore_missing_dynamic_library_information) {
                    break;
                }
                
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) is missing dynamic-library information\n", path);
                } else {
                    fputs("Mach-o file at provided path is missing dynamic-library information\n", stderr);
                }
                
                return false;
                
            case macho::utils::tbd::creation_result::container_is_missing_platform:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an architecture that is missing a platform\n", path);
                } else {
                    fputs("Mach-o file at provided path has an architecture that is missing a platform\n", stderr);
                }
                
                if (!request_new_platform(all, tbd, retained)) {
                    return false;
                }
                
                break;
        
            case macho::utils::tbd::creation_result::container_is_missing_uuid: {
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an architecture that is missing its uuid\n", path);
                } else {
                    fputs("Mach-o file at provided path has an architecture that is missing its uuid\n", stderr);
                }
                
                auto result = std::string();
                
                if (retained != nullptr) {
                    request_input(result, "Ignore missing uuids?", { "all", "yes", "no", "never" });
                } else {
                    request_input(result, "Ignore missing uuids?", { "yes", "no" });
                }
                
                if (result == "never") {
                    *retained |= create_tbd_retained_never_ignore_missing_uuids;
                    return false;
                } else if (result == "no") {
                    return false;
                }
                
                tbd.creation_options.ignore_missing_uuids();
                if (result == "all") {
                    all.creation_options.ignore_missing_uuids();
                }
                
                break;
            }
                
            case macho::utils::tbd::creation_result::container_is_missing_symbol_table:
                if (should_print_paths) {
                    fprintf(stderr, "Mach-o file (at path %s) has an architecture that is missing its symbol-table\n", path);
                } else {
                    fputs("Mach-o file at provided path has an architecture that is missing its symbol-table\n", stderr);
                }
                
                break;
                
            case macho::utils::tbd::creation_result::failed_to_retrieve_string_table:
                if (should_print_paths) {
                    fprintf(stderr, "Failed to retrieve a string-table of one of mach-o file (at path %s)'s containers\n", path);
                } else {
                    fputs("Failed to retrieve a string-table of one of mach-o file at provided path's containers\n", stderr);
                }
                
                break;
                
            case macho::utils::tbd::creation_result::failed_to_retrieve_symbol_table:
                if (should_print_paths) {
                    fprintf(stderr, "Failed to retrieve a symbol-table of one of mach-o file (at path %s)'s containers\n", path);
                } else {
                    fputs("Failed to retrieve a symbol-table of one of mach-o file at provided path's containers\n", stderr);
                }
                
                break;
        }
    } while (true);
    
    return true;
}

void recursively_remove_with_terminator(char *path, char *terminator, bool should_print_paths) {
    if (terminator == nullptr) {
        return;
    }
    
    switch (recursive::remove::perform(path, terminator)) {
        case recursive::remove::result::ok:
            break;
            
        case recursive::remove::result::failed_to_remove_directory:
            if (should_print_paths) {
                // Terminate at terminator to get original created directory
                
                *terminator = '\0';
                fprintf(stderr, "Failed to remove created-directory (at path %s)\n", path);
            } else {
                fputs("Failed to remove created-directory at provided output-path\n", stderr);
            }
            
            break;
            
        case recursive::remove::result::directory_doesnt_exist:
            if (should_print_paths) {
                // Terminate at terminator to get original created directory
                
                *terminator = '\0';
                fprintf(stderr, "[Internal Error] Created directory doesn't exist (at path %s)\n", path);
            } else {
                fputs("[Internal Error] Created directory doesn't exist (at path %s)\n", stderr);
            }
            
            break;
            
        case recursive::remove::result::failed_to_remove_subdirectories:
            if (should_print_paths) {
                // Terminate at terminator to get original created directory
                *terminator = '\0';
                
                fprintf(stderr, "Failed to remove created sub-directories (of directory at path %s)\n", path);
                *terminator = '/';
                
                fprintf(stderr, " at path: %s\n", path);
            } else {
                fprintf(stderr, "Failed to remove created sub-directories of provided output-path %s)\n", path);
            }
            
            break;
            
        case recursive::remove::result::sub_directory_doesnt_exist:
            // Terminate at terminator to get original created directory
            *terminator = '\0';
            
            fprintf(stderr, "[Internal Error] Created sub-directories (of directory at path %s)\n", path);
            *terminator = '/';
            
            fprintf(stderr, " (at path %s) doesn't exist\n", path);
            break;
            
    }
}

/*
   tbd has several different types of options;
   - an option that is provided by itself (such as -h (--help) or -u (--usage)
   - an option that is used with other options to provide additional information.
     As they are within other options, and to differentiate from global options,
     they are referred to as local options
   - an option that is provided not from within another option but meant to
     apply to other options that don't manually supply them
   - an option that accepts local options in between the option and its main
     requested information (such as -p (--path) local-options /main/requested/path)
*/

void print_usage() {
    fputs("Usage: tbd [-p file-paths] [-o/--output output-paths-or-stdout]\n", stdout);
    fputs("Main options:\n", stdout);
    fputs("    -h, --help,   Print this message\n", stdout);
    fputs("    -o, --output, Path(s) to output file(s) to write converted tbd files. If provided file(s) already exists, contents will be overridden. Can also provide \"stdout\" to print to stdout\n", stdout);
    fputs("    -p, --path,   Path(s) to mach-o file(s) to convert to a tbd file. Can also provide \"stdin\" to use stdin\n", stdout);
    fputs("    -u, --usage,  Print this message\n", stdout);

    fputc('\n', stdout);
    fputs("Path options:\n", stdout);
    fputs("Usage: tbd -p [-a/--arch architectures] [--archs architecture-overrides] [--platform platform] [-r/--recurse/ -r=once/all / --recurse=once/all] [-v/--version v1/v2] /path/to/macho/library\n", stdout);
    fputs("    -a, --arch,     Specify architecture(s) to output to tbd\n", stdout);
    fputs("        --archs,    Specify architecture(s) to use, instead of the ones in the provided mach-o file(s)\n", stdout);
    fputs("        --platform, Specify platform for all mach-o library files provided\n", stdout);
    fputs("    -r, --recurse,  Specify directory to recurse and find mach-o library files in\n", stdout);
    fputs("    -v, --version,  Specify version of tbd to convert to (default is v2)\n", stdout);

    fputc('\n', stdout);
    fputs("Outputting options:\n", stdout);
    fputs("Usage: tbd -o [--maintain-directories] /path/to/output/file\n", stdout);
    fputs("        --maintain-directories,   Maintain directories where mach-o library files were found in (subtracting the path provided)\n", stdout);
    fputs("        --replace-path-extension, Replace path-extension on provided mach-o file(s) when creating an output-file (Replace instead of appending .tbd)\n", stdout);

    fputc('\n', stdout);
    fputs("Global options:\n", stdout);
    fputs("    -a, --arch,     Specify architecture(s) to output to tbd (where architectures were not already specified)\n", stdout);
    fputs("        --archs,    Specify architecture(s) to override architectures found in file (where default architecture-overrides were not already provided)\n", stdout);
    fputs("        --platform, Specify platform for all mach-o library files provided (applying to all mach-o library files where platform was not provided)\n", stdout);
    fputs("    -v, --version,  Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tbd-version was not provided)\n", stdout);

    fputc('\n', stdout);
    fputs("Miscellaneous options:\n", stdout);
    fputs("        --dont-print-warnings, Don't print any warnings (both path and global option)\n", stdout);

    fputc('\n', stdout);
    fputs("Symbol options: (Both path and global options)\n", stdout);
    fputs("        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-normal-symbols, Allow all non-external symbols (of no type) (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars\n", stdout);
    fputs("        --allow-private-objc-classes,   Allow all non-external objc-classes\n", stdout);
    fputs("        --allow-private-objc-ivars,     Allow all non-external objc-ivars\n", stdout);

    fputc('\n', stdout);
    fputs("tbd field replacement options: (Both path and global options)\n", stdout);
    fputs("        --replace-flags,           Specify flags to add onto ones found in provided mach-o file(s)\n", stdout);
    fputs("        --replace-objc-constraint, Specify objc-constraint to use instead of one(s) found in provided mach-o file(s)\n", stdout);

    fputc('\n', stdout);
    fputs("tbd field remove options: (Both path and global options)\n", stdout);
    fputs("        --remove-current-version,            Remove current-version field from outputted tbds\n", stdout);
    fputs("        --remove-compatibility-version,      Remove compatibility-version field from outputted tbds\n", stdout);
    fputs("        --remove-exports,                    Remove exports field from outputted tbds\n", stdout);
    fputs("        --remove-flags,                      Remove flags field from outputted tbds\n", stdout);
    fputs("        --remove-objc-constraint,            Remove objc-constraint field from outputted tbds\n", stdout);
    fputs("        --remove-parent-umbrella,            Remove parent-umbrella field from outputted tbds\n", stdout);
    fputs("        --remove-swift-version,              Remove swift-version field from outputted tbds\n", stdout);
    fputs("        --remove-uuids,                      Remove uuids field from outputted tbds\n", stdout);
    fputs("        --dont-remove-current-version,       Don't remove current-version field from outputted tbds\n", stdout);
    fputs("        --dont-remove-compatibility-version, Don't remove compatibility-version field from outputted tbds\n", stdout);
    fputs("        --dont-remove-exports,               Don't remove exports field from outputted tbds\n", stdout);
    fputs("        --dont-remove-flags,                 Don't remove flags field from outputted tbds\n", stdout);
    fputs("        --dont-remove-objc-constraint,       Don't remove objc-constraint field from outputted tbds\n", stdout);
    fputs("        --dont-remove-parent-umbrella,       Don't remove parent-umbrella field from outputted tbds\n", stdout);
    fputs("        --dont-remove-swift-version,         Don't remove swift-version field from outputted tbds\n", stdout);
    fputs("        --dont-remove-uuids,                 Don't remove uuids field from outputted tbds\n", stdout);

    fputc('\n', stdout);
    fputs("tbd field warning ignore options: (Both path and global options)\n", stdout);
    fputs("        --ignore-everything,            Ignore all tbd ignorable field warnings (sets the options below)\n", stdout);
    fputs("        --ignore-missing-exports,       Ignore if no symbols or reexpors to output are found in provided mach-o file(s)\n", stdout);
    fputs("        --ignore-missing-uuids,         Ignore if uuids are not found in provided mach-o file(s)\n", stdout);
    fputs("        --ignore-non-unique-uuids,      Ignore if uuids found in provided mach-o file(s) are not unique\n", stdout);
    fputs("        --dont-ignore-everything,       Reset all tbd field warning ignore options\n", stdout);
    fputs("        --dont-ignore-missing-exports,  Reset ignorning of missing symbols and reexports to output in provided mach-o file(s)\n", stdout);
    fputs("        --dont-ignore-missing-uuids,    Reset ignoring of missing uuids in provided mach-o file(s)\n", stdout);
    fputs("        --dont-ignore-non-unique-uuids, Reset ignoring of uuids found in provided mach-o file(s) not being unique\n", stdout);

    fputc('\n', stdout);
    fputs("List options:\n", stdout);
    fputs("        --list-architectures,           List all valid architectures for tbd files. Also able to list architectures of a provided mach-o file\n", stdout);
    fputs("        --list-tbd-flags,               List all valid flags for tbd files\n", stdout);
    fputs("        --list-macho-dynamic-libraries, List all valid mach-o libraries in current-directory (or at provided path(s))\n", stdout);
    fputs("        --list-objc-constraints,        List all valid objc-constraint options for tbd files\n", stdout);
    fputs("        --list-platform,                List all valid platforms\n", stdout);
    fputs("        --list-recurse,                 List all valid recurse options for parsing directories\n", stdout);
    fputs("        --list-tbd-versions,            List all valid versions for tbd files\n", stdout);

    fputc('\n', stdout);
    fputs("Validation options:\n", stdout);
    fputs("        --validate-macho-dynamic-library, Check if file(s) at provided path(s) are valid mach-o dynamic-libraries\n", stdout);
}

void parse_flags(struct macho::utils::tbd::flags *flags, int &index, int argc, const char *argv[]) {
    auto jndex = index;

    for (; jndex < argc; jndex++) {
        const auto argument = argv[jndex];
        if (strcmp(argument, "flat_namespace") == 0) {
            if (flags != nullptr) {
                flags->set_has_flat_namespace();
            }
        } else if (strcmp(argument, "not_app_extension_safe") == 0) {
            if (flags != nullptr) {
                flags->set_not_app_extension_safe();
            }
        } else {
            // Make sure the user provided at-least one flag

            if (jndex == index) {
                fprintf(stderr, "Unrecognized tbd-flag: %s\n", argument);
                exit(1);
            }

            break;
        }
    }

    index = jndex;
}

enum macho::utils::tbd::objc_constraint parse_objc_constraint_from_argument(int &index, int argc, const char *argv[]) {
    index++;
    if (index == argc) {
        fputs("Please provide an objc-constraint to replace one found\n", stderr);
        exit(1);
    }

    const auto objc_constraint = macho::utils::tbd::objc_constraint_from_string(argv[index]);
    if (objc_constraint == macho::utils::tbd::objc_constraint::none && strcmp(argv[index], "none") != 0) {
        fprintf(stderr, "Unrecognized objc-constraint value: %s\n", argv[index]);
        exit(1);
    }

    return objc_constraint;
}

enum macho::utils::tbd::platform parse_platform_from_argument(int &index, int argc, const char *argv[]) {
    ++index;
    if (index == argc) {
        fputs("Please provide a tbd-platform. Run --list-platforms to see a list of valid platforms\n", stderr);
        exit(1);
    }

    const auto argument = argv[index];
    const auto platform = macho::utils::tbd::platform_from_string(argument);

    if (platform == macho::utils::tbd::platform::none) {
        fprintf(stderr, "Unrecognized tbd-platform: %s\n", argument);
        exit(1);
    }
    
    return platform;
}

enum macho::utils::tbd::version parse_tbd_version(int &index, int argc, const char *argv[]) {
    ++index;
    if (index == argc) {
        fputs("Please provide a tbd-version. Run --list-versions to see a list of tbd-versions\n", stderr);
    }

    const auto argument = argv[index];
    if (strcmp(argument, "v1") == 0) {
        return macho::utils::tbd::version::v1;
    } else if (strcmp(argument, "v2") == 0) {
        return macho::utils::tbd::version::v2;
    }

    fprintf(stderr, "Unrecognized tbd-version: %s\n", argument);
    exit(1);
}

int main(int argc, const char *argv[]) {
    //sleep(50);
    if (argc < 2) {
        fputs("Please run -h (--help) or -u (--usage) to see a list of options\n", stderr);
        return 1;
    }

    auto tbds = std::vector<tbd_with_options>();
    auto tbds_current_index = 0;

    // option_masks contains both options
    // relevant to tbd and other miscellaneous
    // information for the tool's use

    enum option_masks {
        recurse_directories_at_path    = 1 << 0,
        recurse_subdirectories_at_path = 1 << 1,

        // Stop warnings from being printed
        // to the user

        ignore_warnings = 1 << 2,

        // There are supposed to be three
        // different variations - add, replace,
        // and remove, but only two bits are
        // used (replaced, removed). add is
        // used when replaced and removed
        // is 1, so having both replacing and
        // removing is not allowed

        // Replace options are set if
        // intended to replace x in tbd,
        // otherwise we add on

        replace_architectures = 1 << 3,
        replace_flags         = 1 << 4,
        replace_clients       = 1 << 5,

        // Remove options are set if intended
        // to remove x in tbd, otherwise nothing
        // happens.

        remove_architectures = 1 << 6,
        remove_flags         = 1 << 7,
        remove_clients       = 1 << 8,

        // Options provided for output

        // Preserves directories following root path
        // provided for recursion when creating an
        // output-file

        maintain_directories   = 1 << 9,
        replace_path_extension = 1 << 10,
        
        replaced_objc_constraint = 1 << 11
    };

    // global is used to store all the global information.
    // Their purpose is to be the x of any tbd which
    // wasn't provided a different x of the same type

    // From an arguments point of view, If an argument is given
    // in a global context, it will apply to each and every local
    // (local as in options in (-p options /path) or (-o options /path))
    // that wasn't given the exact same option.

    auto global = tbd_with_options();

    // For global options, which can be anywhere in argv [0.. argc], it
    // would be inefficient to recurse and re-parse all provided arguments,
    // for each field that allows adding, removing, and replacing, we use
    // two variables, one for adding, and one referred to as "re", for
    // removing and replacing

    // Since it is not possible to remove x values of a field and replace
    // the contents of field x, we use bits remove_x and replace_x being
    // both 1 to indicate adding flags, which is store in variable x_add
    // while removing or replacing is in the second "_re" variable, depending
    // on the option bits

    auto global_architectures_to_override_to_add = uint64_t();
    auto global_architectures_to_override_to_re = uint64_t();

    struct macho::utils::tbd::flags global_tbd_flags_to_add = {};
    struct macho::utils::tbd::flags global_tbd_flags_to_re = {};

    // Keep a bitset of dont options to
    // and later with individual tbds
    
    struct macho::utils::tbd::creation_options global_dont_creation_options = {};
    struct macho::utils::tbd::write_options global_dont_write_options = {};

    auto global_dont_options = ~uint64_t();
    
    global_dont_creation_options.bits = ~global_dont_creation_options.bits;
    global_dont_write_options.bits = ~global_dont_write_options.bits;

    for (auto index = 1; index != argc; index++) {
        const auto argument = argv[index];
        if (argument[0] != '-') {
            fprintf(stderr, "Unrecognized argument: %s\n", argument);
            return 1;
        }

        auto option = &argument[1];
        if (option[0] == '\0') {
            fputs("Please provide a valid option\n", stderr);
            return 1;
        }
        
        if (option[0] == '-') {
            option = &option[1];
        }

        if (strcmp(option, "a") == 0 || strcmp(option, "arch") == 0) {
            global.architectures |= parse_architectures_list(index, argc, argv);
        } if (strcmp(option, "add-archs") == 0) {
            // XXX: add-archs will add onto replaced architecture overrides
            // provided earlier in a 'replace-archs' options. This may not
            // be wanted behavior

            if (global.options & remove_architectures) {
                fputs("Can't both replace and remove architectures\n", stderr);
                return 1;
            }

            global_architectures_to_override_to_add |= parse_architectures_list(++index, argc, argv);
        } else if (strcmp(option, "allow-all-private-symbols") == 0) {
            global.creation_options.allow_private_normal_symbols();
            global.creation_options.allow_private_weak_symbols();
            global.creation_options.allow_private_objc_class_symbols();
            global.creation_options.allow_private_objc_ivar_symbols();
        } else if (strcmp(option, "add-allowable-clients") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of allowable-clients to add\n", stderr);
                return 1;
            }

            if (global.options & replace_clients) {
                fputs("Can't both replace allowable-clients found and add onto allowable-clients found\n", stderr);
                return 1;
            }

            parse_flags(&global_tbd_flags_to_add, index, argc, argv);
            global.options |= remove_clients | replace_clients;
        } else if (strcmp(option, "add-flags") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of flags to add\n", stderr);
                return 1;
            }
            
            if (global.options & replace_flags) {
                fputs("Can't both replace flags found and add onto flags found\n", stderr);
                return 1;
            }

            // XXX: add-flags will add onto replaced tbd flags provided
            // earlier in a 'replace-flags' options. This may not be
            // wanted behavior

            parse_flags(&global_tbd_flags_to_add, index, argc, argv);
            global.options |= remove_flags | replace_flags;
        } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
            global.creation_options.allow_private_normal_symbols();
        } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
            global.creation_options.allow_private_weak_symbols();
        } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
            global.creation_options.allow_private_objc_class_symbols();
            global.creation_options.allow_private_objc_ivar_symbols();
        } else if (strcmp(option, "allow-private-objc-class-symbols") == 0) {
            global.creation_options.allow_private_objc_class_symbols();
        } else if (strcmp(option, "allow-private-objc-ivar-symbols") == 0) {
            global.creation_options.allow_private_objc_ivar_symbols();
        } else if (strcmp(option, "dont-allow-private-normal-symbols") == 0) {
            global.creation_options.dont_allow_private_normal_symbols();
            global_dont_creation_options.dont_allow_private_normal_symbols();
        } else if (strcmp(option, "dont-allow-private-weak-symbols") == 0) {
            global.creation_options.dont_allow_private_weak_symbols();
            global_dont_creation_options.dont_allow_private_weak_symbols();
        } else if (strcmp(option, "dont-allow-private-objc-symbols") == 0) {
            global.creation_options.dont_allow_private_objc_class_symbols();
            global.creation_options.dont_allow_private_objc_ivar_symbols();
            
            global_dont_creation_options.dont_allow_private_objc_class_symbols();
            global_dont_creation_options.dont_allow_private_objc_ivar_symbols();
        } else if (strcmp(option, "dont-allow-private-objc-class-symbols") == 0) {
            global.creation_options.dont_allow_private_objc_class_symbols();
            global_dont_creation_options.dont_allow_private_objc_class_symbols();
        } else if (strcmp(option, "dont-allow-private-objc-ivar-symbols") == 0) {
            global.creation_options.dont_allow_private_objc_ivar_symbols();
            global_dont_creation_options.dont_allow_private_objc_ivar_symbols();
        } else if (strcmp(option, "dont-ignore-everything") == 0) {
            global.creation_options.dont_ignore_missing_symbol_table();
            global.creation_options.dont_ignore_missing_uuids();
            global.creation_options.dont_ignore_non_unique_uuids();

            global_dont_creation_options.dont_ignore_missing_symbol_table();
            global_dont_creation_options.dont_ignore_missing_uuids();
            global_dont_creation_options.dont_ignore_non_unique_uuids();
        } else if (strcmp(option, "dont-ignore-missing-exports") == 0) {
            // "exports" includes sub-clients, reexports, and
            // symbols, but utils::tbd errors out only if
            // symbol-table is missing

            global.creation_options.dont_ignore_missing_symbol_table();
            global_dont_creation_options.dont_ignore_missing_symbol_table();
        } else if (strcmp(option, "dont-ignore-missing-uuids") == 0) {
            global.creation_options.dont_ignore_missing_uuids();
            global_dont_creation_options.dont_ignore_missing_uuids();
        } else if (strcmp(option, "dont-ignore-non-unique-uuids") == 0) {
            global.creation_options.dont_ignore_non_unique_uuids();
            global_dont_creation_options.dont_ignore_non_unique_uuids();
        } else if (strcmp(option, "dont-ignore-warnings") == 0) {
            global.options &= ~ignore_warnings;
            global_dont_options &= ~ignore_warnings;
        } else if (strcmp(option, "dont-remove-allowable-clients-field") == 0) {
            global.creation_options.dont_ignore_allowable_clients();
            global_dont_creation_options.dont_ignore_allowable_clients();
        } else if (strcmp(option, "dont-remove-current-version") == 0) {
            global.creation_options.dont_ignore_current_version();
            global.write_options.dont_ignore_current_version();
            
            global_dont_creation_options.dont_ignore_current_version();
            global_dont_write_options.dont_ignore_current_version();
        } else if (strcmp(option, "dont-remove-compatibility-version") == 0) {
            global.creation_options.dont_ignore_compatibility_version();
            global.write_options.dont_ignore_compatibility_version();
        
            global_dont_creation_options.dont_ignore_compatibility_version();
            global_dont_write_options.dont_ignore_compatibility_version();
        } else if (strcmp(option, "dont-remove-exports") == 0) {
            global.creation_options.dont_ignore_exports();
            global.write_options.dont_ignore_exports();
            
            global_dont_creation_options.dont_ignore_exports();
            global_dont_write_options.dont_ignore_exports();
        } else if (strcmp(option, "dont-remove-flags-field") == 0) {
            global.creation_options.dont_ignore_flags();
            global.write_options.dont_ignore_flags();
            
            global_dont_creation_options.dont_ignore_flags();
            global_dont_write_options.dont_ignore_flags();
        } else if (strcmp(option, "dont-remove-install-name") == 0) {
            global.creation_options.dont_ignore_install_name();
            global.write_options.dont_ignore_install_name();
            
            global_dont_creation_options.dont_ignore_install_name();
            global_dont_write_options.dont_ignore_install_name();
        } else if (strcmp(option, "dont-remove-objc-constraint") == 0) {
            global.creation_options.dont_ignore_objc_constraint();
            global.write_options.dont_ignore_objc_constraint();
            
            global_dont_creation_options.dont_ignore_objc_constraint();
            global_dont_write_options.dont_ignore_objc_constraint();
        } else if (strcmp(option, "dont-remove-parent-umbrella") == 0) {
            global.creation_options.dont_ignore_parent_umbrella();
            global.write_options.dont_ignore_parent_umbrella();
            
            global_dont_creation_options.dont_ignore_parent_umbrella();
            global_dont_write_options.dont_ignore_parent_umbrella();
        } else if (strcmp(option, "dont-remove-platform") == 0) {
            global.creation_options.dont_ignore_platform();
            global.write_options.dont_ignore_platform();
            
            global_dont_creation_options.dont_ignore_platform();
            global_dont_write_options.dont_ignore_platform();
        } else if (strcmp(option, "dont-remove-reexports") == 0) {
            global.creation_options.dont_ignore_reexports();
            global.write_options.dont_ignore_reexports();
            
            global_dont_creation_options.dont_ignore_reexports();
            global_dont_write_options.dont_ignore_reexports();
        } else if (strcmp(option, "dont-remove-swift-version") == 0) {
            global.creation_options.dont_ignore_swift_version();
            global.write_options.dont_ignore_swift_version();
            
            global_dont_creation_options.dont_ignore_swift_version();
            global_dont_write_options.dont_ignore_swift_version();
        } else if (strcmp(option, "dont-remove-uuids") == 0) {
            global.creation_options.dont_ignore_uuids();
            global.write_options.dont_ignore_uuids();
            
            global_dont_creation_options.dont_ignore_uuids();
            global_dont_write_options.dont_ignore_uuids();
        } else if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
            if (index != 1 || index != argc - 1) {
                // Use argument to print out provided option
                // as it is the full string with dashes that the
                // user provided

                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                return 1;
            }

            print_usage();
            return 0;
        } else if (strcmp(option, "ignore-everything") == 0) {
            global.creation_options.ignore_missing_symbol_table();
            global.creation_options.ignore_missing_uuids();
            global.creation_options.ignore_non_unique_uuids();
        } else if (strcmp(option, "ignore-missing-exports") == 0) {
            // "exports" includes sub-clients, reexports, and
            // symbols, but utils::tbd errors out only if
            // symbol-table is missing

            global.creation_options.ignore_missing_symbol_table();
        } else if (strcmp(option, "ignore-missing-uuids") == 0) {
            global.creation_options.ignore_missing_uuids();
        } else if (strcmp(option, "ignore-non-unique-uuids") == 0) {
            global.creation_options.ignore_non_unique_uuids();
        } else if (strcmp(option, "ignore-warnings") == 0) {
            global.options |= ignore_warnings;
        } else if (strcmp(option, "list-architectures") == 0) {
            if (index != 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself or with a path to a mach-o file \n", argument);
                return 1;
            }
            
            // Two modes exist for --list-architectures, either list out the
            // architecture-info table, or list the architectures of a single
            // provided path to a mach-o file
            
            if (index == argc - 1) {
                auto architecture_info = macho::get_architecture_info_table();
                
                fputs(architecture_info->name, stdout);
                architecture_info++;
                
                while (architecture_info->name != nullptr) {
                    fprintf(stdout, ", %s", architecture_info->name);
                    architecture_info++;
                }
            } else {
                index++;
                
                // Don't allow other arguments
                // to --list-architectures
                
                if (index + 2 <= argc) {
                    fprintf(stderr, "Unrecognized argument: %s\n", argv[index + 1]);
                    return 1;
                }
                
                auto file = macho::file();
                auto path = misc::path_with_current_directory(argv[index]);
                
                struct stat sbuf;
                if (stat(path.c_str(), &sbuf) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object at provided path, failing with error: %s\n", strerror(errno));
                    return 1;
                }
                
                if (!S_ISREG(sbuf.st_mode)) {
                    fputs("Object at provided path is not a regular file\n", stderr);
                    return 1;
                }
                
                switch (file.open(path.c_str())) {
                    case macho::file::open_result::ok:
                        break;
                
                    case macho::file::open_result::not_a_macho:
                        fputs("File at provided path is not a mach-o\n", stderr);
                        return 1;
                        
                    case macho::file::open_result::invalid_macho:
                        fputs("File at provided path is an invalid mach-o\n", stderr);
                        return 1;
                        
                    case macho::file::open_result::failed_to_open_stream:
                        fprintf(stderr, "Failed to open stream for file at provided path, failing with error: %s\n", strerror(errno));
                        return 1;
                        
                    case macho::file::open_result::failed_to_retrieve_information:
                        fprintf(stderr, "Failed to retrieve information on object at provided path, failing with error: %s\n", strerror(errno));
                        return 1;
                        
                    case macho::file::open_result::stream_seek_error:
                    case macho::file::open_result::stream_read_error:
                        fputs("Encountered an error while parsing file at provided path\n", stderr);
                        return 1;
                        
                    case macho::file::open_result::zero_containers:
                        fputs("Mach-o file at provided path has zero architectures\n", stderr);
                        return 1;
                        
                    case macho::file::open_result::too_many_containers:
                        fputs("Mach-o file at provided path has too many architectures for its file-size\n", stderr);
                        return 1;
                        
                    case macho::file::open_result::containers_goes_past_end_of_file:
                        fputs("Mach-o file at provided path's architectures goes past end of file\n", stderr);
                        return 1;
                        
                    case macho::file::open_result::overlapping_containers:
                        fputs("Mach-o file at provided path has overlapping architectures\n", stderr);
                        return 1;
                        
                    case macho::file::open_result::invalid_container:
                        fputs("Mach-o file at provided path has an invalid architecture\n", stderr);
                        return 1;
                }
                
                // store architecture names in a vector before printing
                // so we can handle any errors encountered first
                
                auto cndex = 0;
                auto names = std::vector<const char *>();
                
                for (const auto &container : file.containers) {
                    const auto container_subtype = macho::subtype_from_cputype(macho::cputype(container.header.cputype), container.header.cpusubtype);
                    if (container_subtype == macho::subtype::none) {
                        fprintf(stderr, "Unrecognized cpu-subtype for architecture at index %d\n", cndex);
                        return 1;
                    }
                    
                    const auto architecture_info = macho::architecture_info_from_cputype(macho::cputype(container.header.cputype), container_subtype);
                    if (!architecture_info) {
                        fprintf(stderr, "Unrecognized cputype information for architecture at index %d\n", cndex);
                        return 1;
                    }
                    
                    cndex++;
                }
                
                fputs(names.front(), stdout);
                for (auto iter = names.cbegin() + 1; iter != names.cend(); iter++) {
                    fprintf(stdout, ", %s", *iter);
                }
            }
            
            fputc('\n', stdout);
            return 0;
        } else if (strcmp(option, "list-macho-dynamic-libraries") == 0) {
            if (index != 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself or with a path to a mach-o file\n", argument);
                return 1;
            }
            
            // Two different modes exist for --list-macho-dynamic-libraries;
            // either recurse current-directory with default options, or
            // recurse provided director(ies) with provided options
            
            index++;
            if (index != argc) {
                auto paths = std::vector<std::pair<uint64_t, std::string>>();
                auto options = uint64_t();
                
                options |= recurse_directories_at_path;
                
                for (; index != argc; index++) {
                    const auto &argument = argv[index];
                    const auto &argument_front = argument[0];
                    
                    if (argument_front == '-') {
                        auto option = &argument[1];
                        if (option[0] == '\0') {
                            fputs("Please provide a valid option\n", stderr);
                        }
                        
                        if (option[0] == '-') {
                            option++;
                        }
                        
                        if (strcmp(option, "dont-print-warnings") == 0) {
                            options |= ignore_warnings;
                        } else if (strcmp(option, "r") == 0) {
                            if (index + 1 != argc) {
                                if (strcmp(argv[index + 1], "once") == 0) {
                                    index++;
                                } else if (strcmp(argv[index + 1], "all") == 0) {
                                    options |= recurse_subdirectories_at_path;
                                    index++;
                                }
                            }
                        } else {
                            fprintf(stderr, "Unrecognized argument: %s\n", argument);
                        }
                        
                        continue;
                    }
                    
                    auto path = misc::path_with_current_directory(argv[index]);
                    
                    struct stat sbuf;
                    if (stat(path.c_str(), &sbuf) != 0) {
                        if (index == argc - 1) {
                            fprintf(stderr, "Failed to retrieve information on file at provided path, failing with error: %s\n", strerror(errno));
                        } else {
                            fprintf(stderr, "Failed to retrieve information on file (at path %s), failing with error: %s\n", path.c_str(), strerror(errno));
                        }
                        
                        return 1;
                    }
                    
                    if (!S_ISDIR(sbuf.st_mode)) {
                        if (index == argc - 1) {
                            fputs("Object at provided path is not a directory\n", stderr);
                        } else {
                            fprintf(stderr, "Object (at path %s) is not a directory\n", path.c_str());
                        }
                        
                        return 1;
                    }
                    
                    paths.emplace_back(options, std::move(path));
                    options = uint64_t();
                }
                
                if (paths.empty()) {
                    fputs("No directories have been provided to recurse in\n", stderr);
                    return 1;
                }
                
                const auto &back = paths.back();
                for (const auto &pair : paths) {
                    const auto &options = pair.first;
                    const auto &path = pair.second;
                    
                    auto recurse_options = misc::recurse::options();
                    if (!(options & ignore_warnings)) {
                        recurse_options.print_warnings();
                    }
                    
                    if (options & recurse_subdirectories_at_path) {
                        recurse_options.recurse_subdirectories();
                    }
                    
                    auto filetypes = misc::recurse::filetypes();
                    filetypes.add_dynamic_library();
                    
                    const auto recursion_result = misc::recurse::macho_files(path.c_str(), filetypes, recurse_options, [](const macho::file &file, const std::string &path) {
                        fprintf(stderr, "%s\n", path.c_str());
                        return true;
                    });
                    
                    switch (recursion_result) {
                        case misc::recurse::operation_result::ok:
                            break;
                            
                        case misc::recurse::operation_result::failed_to_open_directory:
                            if (paths.size() != 1) {
                                fprintf(stderr, "Failed to open directory (at path %s), failing with error: %s\n", path.c_str(), strerror(errno));
                            } else {
                                fprintf(stderr, "Failed to open directory at provided path, failing with error: %s\n", strerror(errno));
                            }
                            
                            break;
                            
                        case misc::recurse::operation_result::found_no_matching_files:
                            if (options & recurse_subdirectories_at_path) {
                                if (paths.size() != 1) {
                                    fprintf(stderr, "Found no mach-o dynamic library files while recursing through directory and its sub-directories at path: %s\n", path.c_str());
                                } else {
                                    fputs("Found no mach-o dynamic library files while recursing through directory and its sub-directories at provided path\n", stderr);
                                }
                            } else {
                                if (paths.size() != 1) {
                                    fprintf(stderr, "Found no mach-o dynamic library files while recursing through directory at path: %s\n", path.c_str());
                                } else {
                                    fputs("Found no mach-o dynamic library files while recursing through directory at provided path\n", stderr);

                                }
                            }
                            
                            break;
                            
                        default:
                            break;
                    }
                
                    // Print a newline between each pair
                    if (pair != back) {
                        fputc('\n', stdout);
                    }
                }
            } else {
                auto path = misc::retrieve_current_directory();
                
                auto recurse_options = misc::recurse::options();
                
                recurse_options.print_warnings();
                recurse_options.recurse_subdirectories();
                
                auto filetypes = misc::recurse::filetypes();
                filetypes.add_dynamic_library();
                
                const auto recursion_result = misc::recurse::macho_files(path, filetypes, recurse_options, [](const macho::file &file, const std::string &path) {
                    fprintf(stderr, "%s\n", path.c_str());
                    return true;
                });
                
                switch (recursion_result) {
                    case misc::recurse::operation_result::ok:
                        break;
                        
                    case misc::recurse::operation_result::failed_to_open_directory:
                        fprintf(stderr, "Failed to open directory at current-directory, failing with error: %s\n", strerror(errno));
                        break;
                        
                    case misc::recurse::operation_result::found_no_matching_files:
                        fputs("Found no mach-o dynamic library files while recursing through directory and its sub-directories at provided path\n", stderr);
                        break;
                        
                    default:
                        break;
                }
            }
            
            return 0;
        } else if (strcmp(option, "list-objc-constraint") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                return 1;
            }
            
            fputs("none\nretain_release\nretain_release_or_gc\nretain_release_for_simulator\ngc\n", stderr);
            return 0;
        } else if (strcmp(option, "list-platform") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                return 1;
            }
            
            print_tbd_platforms();
            return 0;
        } else if (strcmp(option, "list-recurse") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                return 1;
            }
            
            fputs("once, Recurse through all of a directory's files (default)\n", stdout);
            fputs("all,  Recurse through all of a directory's files and sub-directories\n", stdout);
            
            return 0;
        } else if (strcmp(option, "list-tbd-flags") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                return 1;
            }
            
            fputs("flat_namespace\nnot_app_extension_safe\n", stdout);
            return  0;
        } else if (strcmp(option, "list-tbd-versions") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                return 1;
            }
            
            fputs("v1\n", stdout);
            fputs("v2 (default)\n", stdout);
            
            return 0;
        } else if (strcmp(option, "o") == 0 || strcmp(option, "output") == 0) {
            ++index;
            if (index == argc) {
                fputs("Please provide an output-path\n", stderr);
                return 1;
            }

            // Store retrieved options temporarily
            // in a separate variable so we can get
            // the output-path to print out if there
            // is a mistake

            // XXX: Might want to change
            // this behavior in the future

            auto options = uint64_t();

            // Keep track of whether or not the user
            // provided an output-path, as we cannot
            // tell when iterating

            auto user_did_provide_output_path = false;

            for (; index != argc; index++) {
                const auto argument = argv[index];
                if (argument[0] == '-') {
                    auto option = &argument[1];
                    if (option[0] == '-') {
                        option = &option[1];
                    }

                    if (strcmp(option, "maintain-directories") == 0) {
                        options |= maintain_directories;
                    } else if (strcmp(option, "replace-path-extension") == 0) {
                        options |= replace_path_extension;
                    } else {
                        fprintf(stderr, "Unrecognized option: %s\n", argument);
                        return 1;
                    }

                    continue;
                }

                // Get path first before validating tbds_current_index
                // so we can print to the user exactly what output is
                // wrong

                auto path = std::string(argv[index]);
                if (path.front() != '/') {
                    path = misc::path_with_current_directory(path.c_str());
                }

                const auto tbds_size = tbds.size();
                if (tbds_current_index == tbds_size) {
                    fprintf(stderr, "Output arguments with path (%s) is missing a corresponding tbd input\n", path.c_str());
                    return 1;
                }

                auto &tbd = tbds.at(tbds_current_index);

                if (tbd.options & recurse_directories_at_path) {
                    if (path == "stdout") {
                        fputs("Cannot output files found while recursing to stdout. Please provide a directory to output .tbd files to\n", stderr);
                        return 1;
                    }
                } else {
                    if (options & maintain_directories) {
                        fputs("There are no directories to maintain when there is no recursion\n", stderr);
                        return 1;
                    }

                    if (options & replace_path_extension) {
                        fputs("Replacing path-extension is an option meant to use be used for recursing directories\n", stderr);
                        return 1;
                    }
                }

                struct stat information;
                if (stat(path.c_str(), &information) != 0) {
                    if (const auto path_back = path.back(); path_back != '/' && path_back != '\\') {
                        path.append(1, '/');
                    }
                } else {
                    if (S_ISREG(information.st_mode)) {
                        if (tbd.options & recurse_directories_at_path) {
                            fputs("Cannot write .tbd files created found while recursing to a file. Please provide a directory to write created .tbd files to\n", stderr);
                            return 1;
                        }
                    } else if (S_ISDIR(information.st_mode)) {
                        if (!(tbd.options & recurse_directories_at_path)) {
                            fputs("Cannot write a created .tbd file to a directory object itself, Please provide a path to a file\n", stderr);
                            return 1;
                        }

                        if (const auto path_back = path.back(); path_back != '/' && path_back != '\\') {
                            path.append(1, '/');
                        }
                    }
                }

                tbd.options |= options;
                tbd.write_path = std::move(path);

                user_did_provide_output_path = true;
                break;
            }

            if (!user_did_provide_output_path) {
                fputs("Please provide an output-path\n", stderr);
                return 1;
            }

            tbds_current_index++;
        } else if (strcmp(option, "p") == 0 || strcmp(option, "path") == 0) {
            ++index;
            if (index == argc) {
                fputs("Please provide a path to a mach-o file or directory to recurse in\n", stderr);
                return 1;
            }

            auto tbd = tbd_with_options();

            // Apply "default" behavior and information
            
            tbd.local_option_start = index;
            tbd.creation_options.ignore_unneeded_fields_for_version();
            tbd.info.version = macho::utils::tbd::version::v2;
            
            tbd.write_options.ignore_unneeded_fields_for_version();
            tbd.write_options.order_by_architecture_info_table();
            tbd.write_options.enforce_has_exports();
            
            // Validate all local-options, but leave the multi-argument
            // local-options to be parsed after tbd-creation with
            // tbd_with_options::local_options_start

            // However, parse architecture-list related options as they
            // simply require and OR on the architectures-list

            for (; index != argc; index++) {
                const auto argument = argv[index];
                if (argument[0] == '-') {
                    auto option = &argument[1];
                    if (option[0] == '-') {
                        option = &option[1];
                    }

                    if (strcmp(option, "a") == 0 || strcmp(option, "arch") == 0) {
                        tbd.architectures |= parse_architectures_list(index, argc, argv);
                    } else if (strcmp(option, "add-archs") == 0) {
                        // XXX: add-archs will add onto replaced architecture overrides
                        // provided earlier in a 'replace-archs' options. This may not
                        // be wanted behavior

                        if (tbd.options & replace_architectures) {
                            fputs("Can't both add architectures to list found and replace architectures found\n", stderr);
                            return 1;
                        }

                        tbd.info.architectures |= parse_architectures_list(++index, argc, argv);
                    } else if (strcmp(option, "allow-all-private-symbols") == 0) {
                        tbd.creation_options.allow_private_normal_symbols();
                        tbd.creation_options.allow_private_weak_symbols();
                        tbd.creation_options.allow_private_objc_class_symbols();
                        tbd.creation_options.allow_private_objc_ivar_symbols();
                    } else if (strcmp(option, "add-flags") == 0) {
                        if (index == argc - 1) {
                            fputs("Please provide a list of flags to add\n", stderr);
                            return 1;
                        }

                        if (tbd.options & replace_flags) {
                            fputs("Can't both replace flags found and add onto flags found\n", stderr);
                            return 1;
                        }

                        // Ignore this argument for now, just validating now
                        // and parsing later after creation of tbd, but before
                        // output

                        tbd.options |= remove_flags | replace_flags;
                    } else if (strcmp(option, "allow-private-normal-symbols") == 0) {
                        tbd.creation_options.allow_private_normal_symbols();
                    } else if (strcmp(option, "allow-private-weak-symbols") == 0) {
                        tbd.creation_options.allow_private_weak_symbols();
                    } else if (strcmp(option, "allow-private-objc-symbols") == 0) {
                        tbd.creation_options.allow_private_objc_class_symbols();
                        tbd.creation_options.allow_private_objc_ivar_symbols();
                    } else if (strcmp(option, "allow-private-objc-class-symbols") == 0) {
                        tbd.creation_options.allow_private_objc_class_symbols();
                    } else if (strcmp(option, "allow-private-objc-ivar-symbols") == 0) {
                        tbd.creation_options.allow_private_objc_ivar_symbols();
                    } else if (strcmp(option, "dont-allow-private-normal-symbols") == 0) {
                        tbd.creation_options.dont_allow_private_normal_symbols();
                    } else if (strcmp(option, "dont-allow-private-weak-symbols") == 0) {
                        tbd.creation_options.dont_allow_private_weak_symbols();
                    } else if (strcmp(option, "dont-allow-private-objc-symbols") == 0) {
                        tbd.creation_options.dont_allow_private_objc_class_symbols();
                        tbd.creation_options.dont_allow_private_objc_ivar_symbols();
                    } else if (strcmp(option, "dont-allow-private-objc-class-symbols") == 0) {
                        tbd.creation_options.dont_allow_private_objc_class_symbols();
                    } else if (strcmp(option, "dont-allow-private-objc-ivar-symbols") == 0) {
                        tbd.creation_options.dont_allow_private_objc_ivar_symbols();
                    } else if (strcmp(option, "dont-ignore-missing-exports") == 0) {
                        tbd.creation_options.dont_ignore_missing_symbol_table();
                        tbd.creation_options.dont_ignore_missing_uuids();
                        tbd.creation_options.dont_ignore_non_unique_uuids();
                    } else if (strcmp(option, "dont-ignore-missing-exports") == 0) {
                        // "exports" includes sub-clients, reexports, and
                        // symbols, but utils::tbd errors out only if
                        // symbol-table is missing

                        tbd.creation_options.dont_ignore_missing_symbol_table();
                    } else if (strcmp(option, "dont-ignore-missing-uuids") == 0) {
                        tbd.creation_options.dont_ignore_missing_uuids();
                    } else if (strcmp(option, "dont-ignore-non-unique-uuids") == 0) {
                        tbd.creation_options.dont_ignore_non_unique_uuids();
                    } else if (strcmp(option, "dont-ignore-warnings") == 0) {
                        tbd.options &= ~ignore_warnings;
                    } else if (strcmp(option, "dont-remove-allowable-clients-field") == 0) {
                        tbd.creation_options.dont_ignore_allowable_clients();
                        tbd.write_options.dont_ignore_allowable_clients();
                    } else if (strcmp(option, "dont-remove-current-version") == 0) {
                        tbd.creation_options.dont_ignore_current_version();
                        tbd.write_options.dont_ignore_current_version();
                    } else if (strcmp(option, "dont-remove-compatibility-version") == 0) {
                        tbd.creation_options.dont_ignore_compatibility_version();
                        tbd.write_options.dont_ignore_compatibility_version();
                    } else if (strcmp(option, "dont-remove-exports") == 0) {
                        tbd.creation_options.dont_ignore_exports();
                        tbd.write_options.dont_ignore_exports();
                    } else if (strcmp(option, "dont-remove-flags-field") == 0) {
                        tbd.creation_options.dont_ignore_flags();
                        tbd.write_options.dont_ignore_flags();
                    } else if (strcmp(option, "dont-remove-install-name") == 0) {
                        tbd.creation_options.dont_ignore_install_name();
                        tbd.write_options.dont_ignore_install_name();
                    } else if (strcmp(option, "dont-remove-objc-constraint") == 0) {
                        tbd.creation_options.dont_ignore_objc_constraint();
                        tbd.write_options.ignore_objc_constraint();
                    } else if (strcmp(option, "dont-remove-parent-umbrella") == 0) {
                        tbd.creation_options.dont_ignore_parent_umbrella();
                        tbd.write_options.dont_ignore_parent_umbrella();
                    } else if (strcmp(option, "dont-remove-platform") == 0) {
                        tbd.creation_options.dont_ignore_platform();
                        tbd.write_options.dont_ignore_platform();
                    } else if (strcmp(option, "dont-remove-reexports") == 0) {
                        tbd.creation_options.dont_ignore_reexports();
                        tbd.write_options.dont_ignore_reexports();
                    } else if (strcmp(option, "dont-remove-swift-version") == 0) {
                        tbd.creation_options.dont_ignore_swift_version();
                        tbd.write_options.dont_ignore_swift_version();
                    } else if (strcmp(option, "dont-remove-uuids") == 0) {
                        tbd.creation_options.dont_ignore_uuids();
                        tbd.write_options.dont_ignore_uuids();
                    } else if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0) {
                        if (index != 1 || index != argc - 1) {
                            // Use argument to print out provided option
                            // as it is the full string with dashes that the
                            // user provided

                            fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                            return 1;
                        }

                        print_usage();
                        return 0;
                    } else if (strcmp(option, "ignore-missing-exports") == 0) {
                        // "exports" includes sub-clients, reexports, and
                        // symbols, but utils::tbd errors out only if
                        // symbol-table is missing

                        tbd.creation_options.ignore_missing_symbol_table();
                    } else if (strcmp(option, "ignore-missing-uuids") == 0) {
                        tbd.creation_options.ignore_missing_uuids();
                    } else if (strcmp(option, "ignore-non-unique-uuids") == 0) {
                        tbd.creation_options.ignore_non_unique_uuids();
                    } else if (strcmp(option, "ignore-warnings") == 0) {
                        tbd.options |= ignore_warnings;
                    } else if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                        tbd.options |= recurse_directories_at_path;

                        if (index + 1 < argc) {
                            const auto argument = argv[index + 1];
                            const auto &argument_front = argument[0];
                            
                            if (argument_front != '-') {
                                if (strcmp(argument, "all") == 0) {
                                    tbd.options |= recurse_subdirectories_at_path;
                                    index++;
                                } else if (strcmp(argument, "once") == 0) {
                                    index++;
                                }
                            }
                        }
                    } else if (strcmp(option, "remove-allowable-clients-field") == 0) {
                        tbd.creation_options.ignore_allowable_clients();
                        tbd.write_options.ignore_allowable_clients();
                    } else if (strcmp(option, "remove-current-version") == 0) {
                        tbd.creation_options.ignore_current_version();
                        tbd.write_options.ignore_current_version();
                    } else if (strcmp(option, "remove-compatibility-version") == 0) {
                        tbd.creation_options.ignore_compatibility_version();
                        tbd.write_options.ignore_compatibility_version();
                    } else if (strcmp(option, "remove-exports") == 0) {
                        tbd.creation_options.ignore_exports();
                        tbd.write_options.ignore_exports();
                    } else if (strcmp(option, "remove-flags") == 0) {
                        if (index == argc - 1) {
                            fputs("Please provide a list of flags to remove\n", stderr);
                            return 1;
                        }

                        if (tbd.options & replace_flags) {
                            fputs("Can't both remove select flags from flags found and replace flags found\n", stderr);
                            return 1;
                        }

                        tbd.options |= remove_flags;
                    } else if (strcmp(option, "remove-flags-field") == 0) {
                        if (tbd.options & replace_flags) {
                            if (tbd.options & remove_flags) {
                                fputs("Can't both add flags to flags found and remove the tbd-flags field\n", stderr);
                            } else {
                                fputs("Can't both replace flags found and remove the tbd-flags field\n", stderr);
                            }

                            return 1;
                        }

                        if (tbd.options & remove_flags) {
                            fputs("Can't both remove select flags from flags found and remove the tbd-flags field\n", stderr);
                            return 1;
                        }

                        tbd.creation_options.ignore_flags();
                        tbd.write_options.ignore_flags();
                    } else if (strcmp(option, "remove-install-name") == 0) {
                        tbd.creation_options.ignore_install_name();
                        tbd.write_options.ignore_install_name();
                    } else if (strcmp(option, "remove-objc-constraint") == 0) {
                        tbd.creation_options.ignore_objc_constraint();
                        tbd.write_options.ignore_objc_constraint();
                    } else if (strcmp(option, "remove-parent-umbrella") == 0) {
                        tbd.creation_options.ignore_parent_umbrella();
                        tbd.write_options.ignore_parent_umbrella();
                    } else if (strcmp(option, "remove-platform") == 0) {
                        tbd.creation_options.ignore_platform();
                        tbd.write_options.ignore_platform();
                    } else if (strcmp(option, "remove-reexports") == 0) {
                        tbd.creation_options.ignore_reexports();
                        tbd.write_options.ignore_reexports();
                    } else if (strcmp(option, "remove-swift-version") == 0) {
                        tbd.creation_options.ignore_swift_version();
                        tbd.write_options.ignore_swift_version();
                    } else if (strcmp(option, "remove-uuids") == 0) {
                        tbd.creation_options.ignore_uuids();
                        tbd.write_options.ignore_uuids();
                    } else if (strcmp(option, "replace-archs") == 0) {
                        if (tbd.options & remove_architectures) {
                            if (tbd.options & replace_architectures) {
                                fputs("Can't both replace architectures and add architectures to list founs\n", stderr);
                            } else {
                                fputs("Can't both replace architectures and remove select architectures from list founs\n", stderr);
                            }

                            return 1;
                        }

                        // Replacing architectures 'resets' the architectures list,
                        // replacing not only add-archs options, but earlier provided
                        // replace-archs options

                        tbd.info.architectures = parse_architectures_list(++index, argc, argv);

                        // We need to stop tbd::create from
                        // overiding these replaced architectures

                        tbd.creation_options.ignore_architectures();
                        tbd.options |= replace_architectures;
                    } else if (strcmp(option, "replace-flags") == 0) {
                        index++;
                        if (index == argc) {
                            fputs("Please provide a list of flags to replace ones found\n", stderr);
                            return 1;
                        }

                        if (tbd.options & remove_flags) {
                            if (tbd.options & replace_flags) {
                                fputs("Can't both replace flags found and add flags to flags found\n", stderr);
                            } else {
                                fputs("Can't both replace flags found and remove select flags from flags found\n", stderr);
                            }

                            return 1;
                        }

                        // Only validate following flag arguments,
                        // We will parse these again later

                        parse_flags(nullptr, index, argc, argv);

                        // We need to stop tbd::create from
                        // overiding these replaced flags

                        tbd.options |= replace_flags;
                        tbd.creation_options.ignore_flags();
                    } else if (strcmp(option, "replace-objc-constraint") == 0) {
                        tbd.info.objc_constraint = parse_objc_constraint_from_argument(index, argc, argv);
                        tbd.creation_options.ignore_objc_constraint();
                        tbd.options |= replaced_objc_constraint;
                    } else if (strcmp(option, "replace-platform") == 0) {
                        tbd.info.platform = parse_platform_from_argument(index, argc, argv);
                        tbd.creation_options.ignore_platform();
                    } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
                        if (index != 1 || index != argc - 1) {
                            fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                            return 1;
                        }

                        print_usage();
                        return 0;
                    } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
                        tbd.info.version = parse_tbd_version(index, argc, argv);
                    } else {
                        fprintf(stderr, "Unrecognized option: %s\n", argument);
                        return 1;
                    }

                    continue;
                }

                auto path = std::string(argv[index]);
                if (const auto front = path.front(); front != '/' && front != '\\') {
                    path = misc::path_with_current_directory(path.c_str());
                }

                struct stat information;
                if (stat(path.c_str(), &information) != 0) {
                    fprintf(stderr, "Failed to retrieve information on object at path (%s), failing with error: %s\n", path.c_str(), strerror(errno));
                    return 1;
                }

                if (S_ISREG(information.st_mode)) {
                    if (tbd.options & recurse_directories_at_path) {
                        fprintf(stderr, "Cannot open directory at path (%s) as a mach-o file. Please provide -r (--recurse) (with an optional recurse-type) to recurse the directory\n", path.c_str());
                        return 1;
                    }
                } else if (S_ISDIR(information.st_mode)) {
                    if (!(tbd.options & recurse_directories_at_path)) {
                        fprintf(stderr, "Cannot recurse file at path (%s). Please provide a path to a directory to recurse in\n", path.c_str());
                        return 1;
                    }

                    if (const auto &back = path.back(); back != '/' && back != '\\') {
                        path.append(1, '/');
                    }
                } else {
                    fprintf(stderr, "Unrecognized object at path (%s). tbd current only supports regular files\n", path.c_str());
                    return 1;
                }

                // Check if any options overlap with one another
                // This can occur in any of the following situations
                //    I. Removing a field and modifying its value in some way

                if (tbd.write_options.ignores_architectures()) {
                    if (tbd.options & replace_architectures || tbd.options & remove_architectures) {
                        fprintf(stderr, "Cannot both modify tbd architectures field and remove the field entirely for path (%s)\n", path.c_str());
                        return 1;
                    }
                }

                if (tbd.write_options.ignores_allowable_clients()) {
                    if (tbd.options & replace_clients || tbd.options & remove_clients) {
                        fprintf(stderr, "Cannot both modify tbd allowable-clients field and remove the field entirely for path (%s)\n", path.c_str());
                        return 1;
                    }
                }

                if (tbd.write_options.ignores_flags()) {
                    if (tbd.options & replace_flags || tbd.options & remove_flags) {
                        fprintf(stderr, "Cannot both modify tbd flags field and remove the field entirely for path (%s)\n", path.c_str());
                        return 1;
                    }
                }

                tbd.path = std::move(path);
                break;
            }

            if (tbd.path.empty()) {
                fputs("Please provide a path to a mach-o file or a directory to recurse to\n", stderr);
                return 1;
            }

            tbds.emplace_back(std::move(tbd));
        } else if (strcmp(option, "remove-allowable-clients-field") == 0) {
            if (global.options & replace_clients) {
                fputs("Can't both remove select flags from allowable-clients found and replace allowable-clients found\n", stderr);
                return 1;
            }

            // We need to stop tbd::create from modifying our
            // changed allowable-clients field

            global.creation_options.ignore_allowable_clients();
            global.write_options.ignore_allowable_clients();
        } else if (strcmp(option, "remove-current-version") == 0) {
            global.creation_options.ignore_current_version();
            global.write_options.ignore_current_version();
        } else if (strcmp(option, "remove-compatibility-version") == 0) {
            global.creation_options.ignore_compatibility_version();
            global.write_options.ignore_compatibility_version();
        } else if (strcmp(option, "remove-exports") == 0) {
            global.creation_options.ignore_exports();
            global.write_options.ignore_exports();
        } else if (strcmp(option, "remove-flags") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of flags to remove\n", stderr);
                return 1;
            }

            if (global.options & replace_flags) {
                fputs("Can't both remove select flags from flags found and replace flags found\n", stderr);
                return 1;
            }

            parse_flags(&global_tbd_flags_to_re, index, argc, argv);

            global.options |= remove_flags;
            global.creation_options.ignore_flags();
        } else if (strcmp(option, "remove-flags-field") == 0) {
            if (global.options & replace_flags) {
                if (global.options & remove_flags) {
                    fputs("Can't both add flags to flags found and remove the tbd-flags field\n", stderr);
                } else {
                    fputs("Can't both replace flags found and remove the tbd-flags field\n", stderr);
                }

                return 1;
            }

            if (global.options & remove_flags) {
                fputs("Can't both remove select flags from flags found and remove the tbd-flags field\n", stderr);
                return 1;
            }

            global.creation_options.ignore_flags();
            global.write_options.ignore_flags();
        } else if (strcmp(option, "remove-install-name") == 0) {
            global.creation_options.ignore_install_name();
            global.write_options.ignore_install_name();
        } else if (strcmp(option, "remove-objc-constraint") == 0) {
            global.creation_options.ignore_objc_constraint();
            global.write_options.ignore_objc_constraint();
        } else if (strcmp(option, "remove-parent-umbrella") == 0) {
            global.creation_options.ignore_parent_umbrella();
            global.write_options.ignore_parent_umbrella();
        } else if (strcmp(option, "remove-platform") == 0) {
            global.creation_options.ignore_platform();
            global.write_options.ignore_platform();
        } else if (strcmp(option, "remove-reexports") == 0) {
            global.creation_options.ignore_reexports();
            global.write_options.ignore_reexports();
        } else if (strcmp(option, "remove-swift-version") == 0) {
            global.creation_options.ignore_swift_version();
            global.write_options.ignore_swift_version();
        } else if (strcmp(option, "remove-uuids") == 0) {
            global.creation_options.ignore_uuids();
            global.write_options.ignore_uuids();
        } else if (strcmp(option, "replace-allowable-clients") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of allowable-clients to replace ones found\n", stderr);
                return 1;
            }

            if (global.options & remove_clients) {
                fputs("Can't both replace allowable-clients found and remove select allowable-clients from allowable-clients found\n", stderr);
                return 1;
            }

            parse_flags(&global_tbd_flags_to_re, index, argc, argv);

            // We need to stop tbd::create from
            // overiding these replaced sub-clients

            global.options |= replace_clients;
            global.creation_options.ignore_allowable_clients();
        } else if (strcmp(option, "replace-archs") == 0) {
            // Replacing architectures 'resets' the global architectures
            // list, replacing not only add-archs options, but earlier provided
            // global archs options

            if (global.options & remove_architectures) {
                fputs("Can't both replace and remove architectures\n", stderr);
                return 1;
            }

            global_architectures_to_override_to_re = parse_architectures_list(++index, argc, argv);
            global.options |= replace_architectures;
        } else if (strcmp(option, "replace-flags") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of flags to replace ones found\n", stderr);
                return 1;
            }

            if (global.options & remove_flags) {
                fputs("Can't both replace flags found and remove select flags from flags found\n", stderr);
                return 1;
            }

            global_tbd_flags_to_re.clear();
            parse_flags(&global_tbd_flags_to_re, index, argc, argv);

            // We need to stop tbd::create from
            // overiding these replaced flags

            global.options |= replace_flags;
        } else if (strcmp(option, "replace-objc-constraint") == 0) {
            global.info.objc_constraint = parse_objc_constraint_from_argument(index, argc, argv);
            global.creation_options.ignore_objc_constraint();
            global.options |= replaced_objc_constraint;
        } else if (strcmp(option, "replace-platform") == 0) {
            global.info.platform = parse_platform_from_argument(index, argc, argv);
            global.creation_options.ignore_platform();
        } else if (strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (index != 1 || index != argc - 1) {
                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                return 1;
            }

            print_usage();
            return 0;
        } else if (strcmp(option, "v") == 0 || strcmp(option, "version") == 0) {
            global.info.version = parse_tbd_version(index, argc, argv);
        } else {
            fprintf(stderr, "Unrecognized option: %s\n", argument);
            return 1;
        }
    }

    if (tbds.empty()) {
        fputs("Please provide paths to either mach-o files or a directory to recurse\n", stderr);
        return 1;
    }

    auto should_print_paths = tbds.size() != 1;
    for (auto &tbd : tbds) {
        
        // Apply global options
        
        tbd.creation_options.bits |= global.creation_options.bits;
        tbd.write_options.bits |= global.write_options.bits;
        tbd.options |= global.options;
        
        // Apply global "don't" options
        
        tbd.creation_options.bits &= global_dont_creation_options.bits;
        tbd.write_options.bits &= global_dont_write_options.bits;
        tbd.options &= global_dont_options;
        
        // Apply global variables
        
        if (global.info.architectures != 0) {
            if (tbd.info.architectures == 0) {
                tbd.info.architectures = global.info.architectures;
            }
        }
        
        if (global.info.current_version.value != 0) {
            if (tbd.info.current_version.value == 0) {
                tbd.info.current_version.value = global.info.current_version.value;
            }
        }
        
        if (global.info.compatibility_version.value != 0) {
            if (tbd.info.compatibility_version.value == 0) {
                tbd.info.compatibility_version.value = global.info.compatibility_version.value;
            }
        }
        
        if (global.info.flags.bits != 0) {
            if (tbd.info.flags.bits == 0) {
                tbd.info.flags.bits = global.info.flags.bits;
            }
        }
        
        if (global.info.install_name.size() != 0) {
            if (tbd.info.install_name.empty()) {
                tbd.info.install_name = global.info.install_name;
            }
        }
        
        if (global.info.parent_umbrella.size() != 0) {
            if (tbd.info.parent_umbrella.empty()) {
                tbd.info.parent_umbrella = global.info.parent_umbrella;
            }
        }
        
        if (global.info.platform != macho::utils::tbd::platform::none) {
            if (tbd.info.platform != macho::utils::tbd::platform::none) {
                tbd.info.platform = global.info.platform;
            }
        }
        
        if (global.info.version != macho::utils::tbd::version::none) {
            if (tbd.info.version != macho::utils::tbd::version::none) {
                tbd.info.version = global.info.version;
            }
        }
        
        if (tbd.options & recurse_directories_at_path) {
            // A check here is necessary in the
            // rare scenario where user asked
            // to recurse a directory, but did not
            // provide an output-path, leaving
            // tbd.write_path empty, which is invalid
            
            if (tbd.write_path.empty()) {
                fprintf(stderr, "Cannot output .tbd files created while recursing directory (at path %s) to stdout. Please provide a directory to output created .tbd files to\n", tbd.path.c_str());
                continue;
            }
            
            // retained is used to store information gathered
            // from the user that is intended to apply to all
            // mach-o files found
            
            auto retained = uint64_t();
            auto all = tbd_with_options();
            
            auto filetypes = misc::recurse::filetypes();
            auto options = misc::recurse::options();
            
            if (tbd.options & recurse_subdirectories_at_path) {
                options.recurse_subdirectories();
            }
            
            if (!(tbd.options & ignore_warnings)) {
                options.print_warnings();
            }
            
            filetypes.add_dynamic_library();
            const auto recurse_result = misc::recurse::macho_files(tbd.path.c_str(), filetypes, options, [&](macho::file &file, std::string &path) {
                auto directories_front = tbd.path.length();
                if (!(tbd.options & maintain_directories)) {
                    directories_front = utils::path::find_last_slash(tbd.path.cbegin(), tbd.path.cend()) - tbd.path.cbegin();
                }
                
                auto write_path = std::string();
                
                // 4 for path-extension; ".tbd"
                auto extracted_directories_length = path.length() - directories_front;
                auto write_path_length = tbd.write_path.length() + extracted_directories_length + 4;
                
                write_path.reserve(write_path_length);
                write_path.append(tbd.write_path);
                
                // Replace path-extension by snipping out the
                // path-extension of the file provided at path
                
                auto extracted_directories_begin = path.cbegin() + directories_front;
                auto extracted_directories_end = extracted_directories_begin + extracted_directories_length;

                if (tbd.options & replace_path_extension) {
                    extracted_directories_end = utils::path::find_extension(extracted_directories_begin, extracted_directories_end);
                }
                
                utils::path::append_component(write_path, extracted_directories_begin, extracted_directories_end);
                write_path.append(".tbd");
                
                auto terminator = static_cast<char *>(nullptr);
                auto descriptor = -1;
                
                switch (recursive::mkdir::perform_with_last_as_file(write_path.data(), &terminator, &descriptor)) {
                    case recursive::mkdir::result::ok:
                    case recursive::mkdir::result::failed_to_create_last_as_directory:
                        break;
                        
                    case recursive::mkdir::result::failed_to_create_last_as_file:
                        fprintf(stderr, "Failed to create file (at path %s), failing with error: %s\n", write_path.c_str(), strerror(errno));
                        return true;
                        
                    case recursive::mkdir::result::failed_to_create_intermediate_directories:
                        fprintf(stderr, "Failed to create intermediate directories for file (at path %s), failing with error: %s\n", write_path.c_str(), strerror(errno));
                        return true;

                    case recursive::mkdir::result::last_already_exists_not_as_file:
                        fprintf(stderr, "Object at path (%s) already exists, but is not a file\n", write_path.c_str());
                        return true;

                    default:
                        break;
                }
                
                if (descriptor == -1) {
                    fprintf(stderr, "Failed to open output-path (%s), failing with error: %s\n", write_path.c_str(), strerror(errno));
                    return true;
                }
                
                if (global.info.current_version.value != 0) {
                    if (tbd.info.current_version.value == 0) {
                        tbd.info.current_version.value = all.info.current_version.value;
                    }
                }
                
                if (global.info.compatibility_version.value != 0) {
                    if (tbd.info.compatibility_version.value == 0) {
                        tbd.info.compatibility_version.value = all.info.compatibility_version.value;
                    }
                }
                
                if (global.info.flags.bits != 0) {
                    if (tbd.info.flags.bits == 0) {
                        tbd.info.flags.bits = all.info.flags.bits;
                    }
                }
                
                if (global.info.install_name.size() != 0) {
                    if (tbd.info.install_name.empty()) {
                        tbd.info.install_name = all.info.install_name;
                    }
                }
                
                if (global.info.parent_umbrella.size() != 0) {
                    if (tbd.info.parent_umbrella.empty()) {
                        tbd.info.parent_umbrella = all.info.parent_umbrella;
                    }
                }
                
                if (global.info.platform != macho::utils::tbd::platform::none) {
                    if (tbd.info.platform != macho::utils::tbd::platform::none) {
                        tbd.info.platform = all.info.platform;
                    }
                }
                
                if (!create_tbd(all, tbd, file, create_tbd_option_ignore_missing_dynamic_library_information, &retained, path.c_str(), true)) {
                    recursively_remove_with_terminator(write_path.data(), terminator, true);
                }
                
                // Parse local-options for modification of tbd-fields
                // after creation and before write to avoid any extra hoops
                
                if (tbd.local_option_start != 0) {
                    for (auto index = tbd.local_option_start; index != argc; index++) {
                        const auto &option = argv[index];
                        if (strcmp(option, "add-flags") == 0) {
                            if ((tbd.options & replace_flags) ^ (tbd.options & remove_flags)) {
                                continue;
                            }
                            
                            parse_flags(&tbd.info.flags, ++index, argc, argv);
                        } else if (strcmp(option, "replace-flags") == 0) {
                            if (tbd.options & remove_flags) {
                                continue;
                            }
                            
                            tbd.info.flags.clear();
                            parse_flags(&tbd.info.flags, ++index, argc, argv);
                        } else if (option[0] != '-') {
                            break;
                        }
                        
                        // Ignore handling of any unrecognized options
                        // as by this point, argv has been fully validated
                    }
                }
                
                tbd.write_options.order_by_architecture_info_table();
                
                switch (tbd.info.write_to(descriptor, tbd.write_options)) {
                    case macho::utils::tbd::write_result::ok:
                        break;
                        
                    case macho::utils::tbd::write_result::has_no_exports:
                        fprintf(stderr, "Mach-o file (at path %s) has no reexports or symbols to be written out\n", path.c_str());
                        recursively_remove_with_terminator(write_path.data(), terminator, should_print_paths);
                        
                        break;
                        
                    default:
                        fprintf(stderr, "Failed to write .tbd to output-file at path: %s\n", write_path.c_str());
                        recursively_remove_with_terminator(write_path.data(), terminator, should_print_paths);
                        
                        break;
                }
                
                // "clear" tbd's info fields as we are
                // to reuse the structure in the next file
                
                if (!tbd.creation_options.ignores_architectures()) {
                    tbd.info.architectures = 0;
                }

                if (!tbd.creation_options.ignores_current_version()) {
                    tbd.info.current_version.value = 0;
                }

                if (!tbd.creation_options.ignores_compatibility_version()) {
                    tbd.info.compatibility_version.value = 0;
                }
                
                if (!tbd.creation_options.ignores_flags()) {
                    tbd.info.flags.clear();
                }
                
                if (!tbd.creation_options.ignores_platform()) {
                    tbd.info.platform = macho::utils::tbd::platform::none;
                }

                if (!tbd.creation_options.ignores_install_name()) {
                    tbd.info.install_name.clear();
                }

                if (!tbd.creation_options.ignores_parent_umbrella()) {
                    tbd.info.parent_umbrella.clear();
                }

                if (!tbd.creation_options.ignores_allowable_clients()) {
                    tbd.info.sub_clients.clear();
                }

                if (!tbd.creation_options.ignores_uuids()) {
                    tbd.info.objc_constraint = macho::utils::tbd::objc_constraint::none;
                }
                
                if (!tbd.creation_options.ignores_swift_version()) {
                    tbd.info.swift_version = 0;
                }
                
                tbd.info.reexports.clear();
                tbd.info.symbols.clear();
                tbd.info.uuids.clear();
                
                close(descriptor);
                return true;
            });
            
            switch (recurse_result) {
                case misc::recurse::operation_result::ok:
                    break;
                    
                case misc::recurse::operation_result::failed_to_open_directory:
                    fprintf(stderr, "Failed to open directory (at path %s), failing with error: %s\n", tbd.path.c_str(), strerror(errno));
                    break;
                    
                case misc::recurse::operation_result::found_no_matching_files:
                    fprintf(stderr, "Found no mach-o dynamic library files in directory at path: %s\n", tbd.path.c_str());
                    break;
                    
                default:
                    break;
            }
        } else {
            auto file = macho::file();
            switch (file.open(tbd.path.c_str())) {
                case macho::file::open_result::ok:
                    break;
                    
                case macho::file::open_result::not_a_macho:
                    if (should_print_paths) {
                        fprintf(stderr, "File (at path %s) is not a valid mach-o\n", tbd.path.c_str());
                    } else {
                        fputs("File at provided path is not a valid mach-o\n", stderr);
                    }
                    
                    continue;
                    
                case macho::file::open_result::invalid_macho:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) is invalid\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path is invalid\n", stderr);
                    }
                    
                    continue;
                    
                case macho::file::open_result::failed_to_open_stream:
                    if (should_print_paths) {
                        fprintf(stderr, "Failed to open stream for file (at path %s), failing with error: %s\n", tbd.path.c_str(), strerror(errno));
                    } else {
                        fprintf(stderr, "Failed to open stream for file at provided path, failing with error: %s\n", strerror(errno));
                    }
                    
                    continue;
                    
                case macho::file::open_result::failed_to_retrieve_information:
                    if (should_print_paths) {
                        fprintf(stderr, "Failed to retrieve information on file (at path %s), failing with error: %s\n", tbd.path.c_str(), strerror(errno));
                    } else {
                        fprintf(stderr, "Failed to retrieve information on file at provided path, failing with error: %s\n", strerror(errno));
                    }
                    
                    continue;
                    
                case macho::file::open_result::stream_seek_error:
                case macho::file::open_result::stream_read_error:
                    if (should_print_paths) {
                        fprintf(stderr, "Encountered an error while parsing file (at path %s)\n", tbd.path.c_str());
                    } else {
                        fputs("Encountered an error while parsing file at provided path\n", stderr);
                    }
                    
                    continue;
                    
                case macho::file::open_result::zero_containers:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file at path (%s) has no architectures\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path has no architectures\n", stderr);
                    }
                    
                    continue;
                    
                case macho::file::open_result::too_many_containers:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has too many architectures to fit inside the mach-o file\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path has too many architectures to fit inside the provided mach-o file\n", stderr);
                    }
                    
                    continue;
                    
                case macho::file::open_result::containers_goes_past_end_of_file:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an architecture that goes past end of file\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path has an architecture that goes past end of file\n", stderr);
                    }
                    
                    continue;
                    
                case macho::file::open_result::overlapping_containers:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has architectures that go past end of file\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path has architectures that go past end of file\n", stderr);
                    }
                    
                case macho::file::open_result::invalid_container:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an architecute that is invalid\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path has an architecture that is invalid\n", stderr);
                    }
                    
                    continue;
            }
            
            auto terminator = static_cast<char *>(nullptr);
            auto descriptor = -1;
            
            if (tbd.write_path.size() != 0) {
                switch (recursive::mkdir::perform_with_last_as_file(tbd.write_path.data(), &terminator, &descriptor)) {
                    case recursive::mkdir::result::ok:
                    case recursive::mkdir::result::failed_to_create_last_as_directory:
                        break;
                        
                    case recursive::mkdir::result::failed_to_create_last_as_file:
                        if (should_print_paths) {
                            fprintf(stderr, "Failed to create file (at path %s), failing with error: %s\n", tbd.write_path.c_str(), strerror(errno));
                        } else {
                            fprintf(stderr, "Failed to create file at provided path, failing with error: %s\n", strerror(errno));
                        }

                        continue;
                        
                    case recursive::mkdir::result::failed_to_create_intermediate_directories:
                        if (should_print_paths) {
                            fprintf(stderr, "Failed to create intermediate directories for file (at path %s), failing with error: %s\n", tbd.write_path.c_str(), strerror(errno));
                        } else {
                            fprintf(stderr, "Failed to create intermediate directories for file at provided path, failing with error: %s\n", strerror(errno));
                        }

                        continue;

                    case recursive::mkdir::result::last_already_exists_not_as_file:
                        if (should_print_paths) {
                            fprintf(stderr, "Object at path (%s) already exists, but is not a file\n", tbd.write_path.c_str());
                        } else {
                            fputs("Object at provided path already exists, but is not a file\n", stderr);
                        }
                        
                        continue;

                    default:
                        break;
                }
                
                if (descriptor == -1) {
                    fprintf(stderr, "Failed to open output-file (at path %s), failing with error: %s\n", tbd.write_path.c_str(), strerror(errno));
                    continue;
                }
            }
            
            if (!create_tbd(tbd, tbd, file, create_tbd_option_ignore_missing_dynamic_library_information, nullptr, tbd.path.c_str(), should_print_paths)) {
                recursively_remove_with_terminator(tbd.write_path.data(), terminator, should_print_paths);
            }
            
            // Parse local-options for modification of tbd-fields
            // after creation and before write to avoid any extra hoops
            
            if (tbd.local_option_start != 0) {
                for (auto index = tbd.local_option_start; index != argc; index++) {
                    const auto &option = argv[index];
                    if (strcmp(option, "add-flags") == 0) {
                        if ((tbd.options & replace_flags) ^ (tbd.options & remove_flags)) {
                            continue;
                        }
                        
                        parse_flags(&tbd.info.flags, ++index, argc, argv);
                    } else if (strcmp(option, "replace-flags") == 0) {
                        if (tbd.options & remove_flags) {
                            continue;
                        }
                        
                        tbd.info.flags.clear();
                        parse_flags(&tbd.info.flags, ++index, argc, argv);
                    } else if (option[0] != '-') {
                        break;
                    }
                    
                    // Ignore handling of any unrecognized options
                    // as by this point, argv has been fully validated
                }
            }
            
            auto write_result = macho::utils::tbd::write_result::ok;
            if (tbd.write_path.empty()) {
                write_result = tbd.info.write_to(stdout, tbd.write_options);
            } else {
                write_result = tbd.info.write_to(descriptor, tbd.write_options);
                close(descriptor);
            }
            
            switch (write_result) {
                case macho::utils::tbd::write_result::ok:
                    break;
                    
                case macho::utils::tbd::write_result::has_no_exports:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has no reexports or symbols to be written out\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path has no reexports or symbols to be written out\n", stderr);
                    }
                    
                    recursively_remove_with_terminator(tbd.write_path.data(), terminator, should_print_paths);
                    break;
                    
                default:
                    if (should_print_paths) {
                        fprintf(stderr, "Failed to write .tbd to output-file at path: %s\n", tbd.write_path.c_str());
                    } else {
                        fputs("Failed to write .tbd to output-file at provided output-path\n", stderr);
                    }
                    
                    recursively_remove_with_terminator(tbd.write_path.data(), terminator, should_print_paths);
                    break;
            }
        }
    }

    return 0;
}
