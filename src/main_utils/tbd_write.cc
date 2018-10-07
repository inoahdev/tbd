//
//  src/main_utils/tbd_write.cc
//  tbd
//
//  Created by inoahdev on 3/12/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "tbd_write.h"

const char *source_code_link = "https://github.com/inoahdev/tbd";

namespace main_utils {
    bool
    tbd_write(int descriptor,
              const tbd_with_options &tbd,
              const std::string &path,
              const std::string &write_path,
              bool should_print_paths)
    {
        switch (tbd.info.write_to(descriptor, tbd.write_options, tbd.version)) {
            case utils::tbd::write_result::ok:
                break;
                
            case utils::tbd::write_result::has_no_architectures:
                if (should_print_paths) {
                    fprintf(stderr,
                            "No architectures were retrieved while parsing mach-o file (at path %s). This is likely an "
                            "internal error, Please create an issue with contextual information at %s"
                            "\n",
                            path.c_str(),
                            source_code_link);
                } else {
                    fprintf(stderr,
                            "No architectures were retrieved while parsing mach-o file at provided path. This is "
                            "likely an internal error, Please create an issue with contextual information at %s\n",
                            source_code_link);
                }
                
                return false;

            case utils::tbd::write_result::has_no_exports:
                if (!tbd.options.ignore_warnings) {
                    fprintf(stderr,
                            "Mach-o file (at path %s) has no reexports and symbols to be written out\n",
                            path.c_str());
                }
                
                return false;
                
            case utils::tbd::write_result::has_no_version:
                fprintf(stderr,
                        "An internal error occurred where the tbd-version was not properly set, Please create an "
                        "issue with contextual information at %s\n",
                        source_code_link);
                
                break;
                
            case utils::tbd::write_result::failed_to_open_stream:
                return false;

            case utils::tbd::write_result::failed_to_write_architectures:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write architectures of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write architectures of mach-o file at provided path to file at provided "
                          "output-path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_current_version:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write current-version of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write current-version of mach-o file at provided path to file at provided "
                          "output-path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_compatibility_version:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write compatibility-version of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write compatibility-version of mach-o file at provided path to file at "
                          "provided output-path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_exports:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write exports of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write exports of mach-o file (at path %s) to file at provided output-path\n",
                          stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_flags:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write flags of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write flags of mach-o file at provided path to file at provided output-path\n",
                          stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_footer:
                if (should_print_paths) {
                    fprintf(stderr, "Failed to write footer of tbd-file to file (at path %s)\n", write_path.c_str());
                } else {
                    fputs("Failed to write footer of tbd-file to file at provided path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_header:
                if (should_print_paths) {
                    fprintf(stderr, "Failed to write header of tbd-file to file (at path %s)\n", write_path.c_str());
                } else {
                    fputs("Failed to write header of tbd-file to file at provided path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_install_name:
                if (should_print_paths) {
                    fprintf(stderr, "Failed to write install-name of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write install-name of mach-o file at provided path to file at provided output-path"
                          "\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_objc_constraint:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write objc-constraint of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write objc-constraint of mach-o file at provided path to file at provided "
                          "output-path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_parent_umbrella:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write parent-umbrella of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write parent-umbrella of mach-o file at provided path to file at provided "
                          "output-path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
            case utils::tbd::write_result::failed_to_write_platform:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write platform of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write platform of mach-o file at provided path to file provided output-path\n",
                          stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_swift_version:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write swift-version of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write swift-version of mach-o file at provided path to file provided output-path"
                          "\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_uuids:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write uuids of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write uuids of mach-o file at provided path to file provided output-path\n",
                          stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_reexports:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write re-exports of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write re-exports of mach-o file at provided path to file (at path %s)\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_normal_symbols:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write ordinary symbols of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write ordinary symbols of mach-o file at provided path to file at provided "
                          "output-path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_objc_class_symbols:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write objc-class symbols of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write objc-class symbols of mach-o file at provided path to file at provided "
                          "output-path\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_objc_ivar_symbols:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write objc-ivar symbols of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write objc-ivar symbols of mach-o file (at path %s) to file (at path %s)\n",
                          stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            case utils::tbd::write_result::failed_to_write_weak_symbols:
                if (should_print_paths) {
                    fprintf(stderr,
                            "Failed to write weak symbols of mach-o file (at path %s) to file (at path %s)\n",
                            path.c_str(),
                            write_path.c_str());
                } else {
                    fputs("Failed to write weak symbols of mach-o file (at path %s) to file (at path %s)\n", stderr);
                }
                
                goto single_tbd_internal_error_notice;
                
            default:
            single_tbd_internal_error_notice:
                fprintf(stderr,
                        "This is likely a stream error, please try again. If the issue persists, and other causes have "
                        "been eliminated (e.g No storage space left), Please create an issue with contextual "
                        "infromation at %s\n", source_code_link);

                return false;
        }
        
        return true;
    }
}
