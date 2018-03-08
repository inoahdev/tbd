//
//  src/main_utils/tbd_create.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "tbd_create.h"

namespace main_utils {
    bool create_tbd(tbd_with_options &all, tbd_with_options &tbd, macho::file &file, const create_tbd_options &options, create_tbd_retained_user_info *user_input_info, const char *path) {
        do {
            switch (tbd.info.create(file, tbd.creation_options)) {
                case macho::utils::tbd::creation_result::ok:
                    return true;

                case macho::utils::tbd::creation_result::multiple_containers_for_same_architecture:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple containers for the same cputype and subtype\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple containers for the same cputype and subtype\n", stderr);
                    }
                    
                    return false;
                    
                case macho::utils::tbd::creation_result::invalid_container_header_subtype:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an unrecognized cpu-subtype\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an unrecognized cpu-subtype\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::unrecognized_container_cputype_subtype_pair:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o fie (at path %s) has an unrecognized cputype-subtype pair\n", path);
                    } else {
                        fputs("Mach-o fie at provided path has an unrecognized cputype-subtype pair\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::flags_mismatch: {
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has different flags in different architectures\n", path);
                    } else {
                        fputs("Mach-o file at provided path has different flags in different architectures\n", stderr);
                    }

                    if (!request_if_should_ignore_flags(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;
                }

                case macho::utils::tbd::creation_result::failed_to_retrieve_load_cammands:
                    if (options.print_paths) {
                        fprintf(stderr, "Failed to retrieve load-commands of mach-o file (at path %s)\n", path);
                    } else {
                        fputs("Failed to retrieve load-commands of mach-o file at provided path\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::unsupported_platform:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an unsupported platform\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an unsupported platform\n", stderr);
                    }

                    if (!request_new_platform(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::unrecognized_platform:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an unrecognized platform\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an unrecognized platform\n", stderr);
                    }

                    if (!request_new_platform(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::multiple_platforms:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple different platforms\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple different platforms\n", stderr);
                    }

                    if (!request_new_platform(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::platforms_mismatch:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has different platforms for different architectures\n", path);
                    } else {
                        fputs("Mach-o file at provided path has different platforms for different architectures\n", stderr);
                    }

                    if (!request_new_platform(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::invalid_build_version_command:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an invalid build-version load-command\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an invalid build-version load-command\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::invalid_dylib_command:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an invalid dylib-command load-command\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an invalid dylib-command load-command\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::invalid_segment_command:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an invalid segment-command load-command\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an invalid segment-command load-command\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::invalid_segment_command_64:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an invalid 64-bit segment-command load-command\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an invalid 64-bit segment-command load-command\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::empty_install_name:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an empty installation-name\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an empty installation-name\n", stderr);
                    }

                    if (!request_new_installation_name(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::multiple_current_versions:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple different current-versions\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple different current-versions\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::current_versions_mismatch:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has different current-versions for different architectures\n", path);
                    } else {
                        fputs("Mach-o file at provided path has different current-versions for different architectures\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::multiple_compatibility_versions:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple different compatibility-versions\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple different compatibility-versions\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::compatibility_versions_mismatch:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has different compatibility-versions for different architectures\n", path);
                    } else {
                        fputs("Mach-o file at provided path has different compatibility-versions for different architectures\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::multiple_install_names:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple different installation-names\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple different installation-names\n", stderr);
                    }

                    if (!request_new_installation_name(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::install_name_mismatch:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has different installation-names for different architectures\n", path);
                    } else {
                        fputs("Mach-o file at provided path has different installation-names for different architectures\n", stderr);
                    }

                    if (!request_new_installation_name(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::invalid_objc_imageinfo_segment_section:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an invalid objc image-info segment-section\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an invalid objc image-info segment-section\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::stream_seek_error:
                case macho::utils::tbd::creation_result::stream_read_error:
                    if (options.print_paths) {
                        fprintf(stderr, "Encountered a stream error while parsing mach-o file (at path %s)\n", path);
                    } else {
                        fputs("Encountered a stream error while parsing mach-o file at provided path\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::multiple_objc_constraint:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple different objc-constraint types\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple different objc-constraint types\n", stderr);
                    }

                    if (!request_new_objc_constraint(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::objc_constraint_mismatch:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has different objc-constraint types for different containers\n", path);
                    } else {
                        fputs("Mach-o file at provided path has different objc-constraint types for different containers\n", stderr);
                    }

                    if (!request_new_objc_constraint(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::multiple_swift_version:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple different swift-versions\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple different swift-versions\n", stderr);
                    }

                    if (!request_new_swift_version(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::swift_version_mismatch:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has different swift-versions for different architectures\n", path);
                    } else {
                        fputs("Mach-o file at provided path has different swift-versions for different architectures\n", stderr);
                    }

                    if (!request_new_swift_version(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::invalid_sub_client_command:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an invalid sub-client load-command\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an invalid sub-client load-command\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::invalid_sub_umbrella_command:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an invalid sub-umbrella load-command\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an invalid sub-umbrella load-command\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::empty_parent_umbrella:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an empty parent-umbrella\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an empty parent-umbrella\n", stderr);
                    }

                    if (!request_new_parent_umbrella(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::multiple_parent_umbrella:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple different parent-umbrellas\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple different parent-umbrellas\n", stderr);
                    }

                    if (!request_new_parent_umbrella(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::parent_umbrella_mismatch:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has different parent-umbrella for different containers\n", path);
                    } else {
                        fputs("Mach-o file at provided path has different parent-umbrella for different containers\n", stderr);
                    }

                    if (!request_new_parent_umbrella(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::invalid_symtab_command:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an invalid symtab-command load-command\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an invalid symtab-command load-command\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::multiple_symbol_tables:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has a container with multiple different symbol-tables\n", path);
                    } else {
                        fputs("Mach-o file at provided path has a container with multiple different symbol-tables\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::multiple_uuids: {
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has a container with multiple different uuids\n", path);
                    } else {
                        fputs("Mach-o file at provided path has a container with multiple different uuids\n", stderr);
                    }

                    if (!request_if_should_ignore_uuids(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;
                }

                case macho::utils::tbd::creation_result::non_unique_uuid: {
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has multiple containers with the same uuid\n", path);
                    } else {
                        fputs("Mach-o file at provided path has multiple containers with the same uuid\n", stderr);
                    }

                    if (!request_if_should_ignore_non_unique_uuids(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;
                }

                case macho::utils::tbd::creation_result::container_is_missing_dynamic_library_identification:
                    if (options.ignore_missing_dynamic_library_information) {
                        return false;
                    }

                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) is missing dynamic-library information\n", path);
                    } else {
                        fputs("Mach-o file at provided path is missing dynamic-library information\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::container_is_missing_platform:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an architecture that is missing a platform\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an architecture that is missing a platform\n", stderr);
                    }

                    if (!request_new_platform(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;

                case macho::utils::tbd::creation_result::container_is_missing_uuid: {
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an architecture that is missing its uuid\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an architecture that is missing its uuid\n", stderr);
                    }

                    if (!request_if_should_ignore_missing_uuids(all, tbd, user_input_info)) {
                        return false;
                    }

                    break;
                }

                case macho::utils::tbd::creation_result::container_is_missing_symbol_table:
                    if (options.print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an architecture that is missing its symbol-table\n", path);
                    } else {
                        fputs("Mach-o file at provided path has an architecture that is missing its symbol-table\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::failed_to_retrieve_string_table:
                    if (options.print_paths) {
                        fprintf(stderr, "Failed to retrieve a string-table of one of mach-o file (at path %s)'s containers\n", path);
                    } else {
                        fputs("Failed to retrieve a string-table of one of mach-o file at provided path's containers\n", stderr);
                    }

                    return false;

                case macho::utils::tbd::creation_result::failed_to_retrieve_symbol_table:
                    if (options.print_paths) {
                        fprintf(stderr, "Failed to retrieve a symbol-table of one of mach-o file (at path %s)'s containers\n", path);
                    } else {
                        fputs("Failed to retrieve a symbol-table of one of mach-o file at provided path's containers\n", stderr);
                    }

                    return false;
            }
        } while (true);
        
        return true;
    }
}
