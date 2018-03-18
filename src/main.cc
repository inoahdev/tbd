//
//  src/main.cc
//  tbd
//
//  Created by inoahdev on 4/16/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "mach-o/utils/validation/as_dynamic_library.h"

#include "main_utils/parse_architectures_list.h"
#include "main_utils/parse_list_argument.h"

#include "main_utils/print_help_menu.h"

#include "main_utils/tbd_create.h"
#include "main_utils/tbd_parse_fields.h"
#include "main_utils/tbd_write.h"

#include "main_utils/recursive_mkdir.h"
#include "main_utils/recursive_remove_with_terminator.h"

#include "misc/current_directory.h"
#include "misc/recurse.h"

int main(int argc, const char *argv[]) {
    if (argc < 2) {
        fputs("Please run -h (--help) or -u (--usage) to see a list of options\n", stderr);
        return 1;
    }

    auto tbds = std::vector<main_utils::tbd_with_options>();
    auto tbds_current_index = 0;

    // global is used to store all "global" information.
    // Its purpose is to be the x of any tbd which
    // wasn't provided a different x of the same type

    // From an arguments point of view, If an argument is given
    // in a global context, it will apply to each and every local
    // (local as in options in (-p options /path) or (-o options /path))
    // that wasn't given the exact same option.

    auto global = main_utils::tbd_with_options();

    // Apply "default" behavior and information for tbd

    global.version = utils::tbd::version::v2;

    global.write_options.order_by_architecture_info_table = true;
    global.write_options.enforce_has_exports = true;

    // For global options, which can be anywhere in argv [0.. argc], it
    // would be inefficient to recurse and re-parse all provided arguments,
    // for each field that allows adding, removing, and replacing, we use
    // two variables, one for adding, and one referred to as "re", for
    // removing and replacing

    // Since we don't allow removing x values of a field and replace
    // the contents of field x, we use bits remove_x and replace_x being
    // both 1 to indicate adding flags, which is store in variable x_add
    // while removing or replacing is in the second "_re" variable, depending
    // on the option bits
    
    // Reimplement these here as there is no sense of "local" options
    // we can parse later from

    auto global_architectures_to_override_to_add = uint64_t();
    auto global_architectures_to_override_to_re = uint64_t();

    struct utils::tbd::flags global_tbd_flags_to_add;
    struct utils::tbd::flags global_tbd_flags_to_re;

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

        if (strcmp(option, "add-archs") == 0) {
            // XXX: add-archs will add onto replaced architecture overrides
            // provided earlier in a 'replace-archs' options. This may not
            // be wanted behavior

            if (global.options.replace_architectures && !global.options.remove_architectures) {
                fputs("Can't both replace architectures found and add onto architectures found\n", stderr);
                return 1;
            }

            global_architectures_to_override_to_add |= main_utils::parse_architectures_list(++index, argc, argv);
            
            global.options.remove_architectures = true;
            global.options.replace_architectures = true;
            
            // Write out replaced architectures
            // over each export's architectures
            
            global.write_options.ignore_export_architectures = true;
        } else if (strcmp(option, "add-flags") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of flags to add\n", stderr);
                return 1;
            }

            if (global.options.replace_flags && !global.options.remove_flags) {
                fputs("Can't both replace flags found and add onto flags found\n", stderr);
                return 1;
            }

            // XXX: add-flags will add onto replaced tbd flags provided
            // earlier in a 'replace-flags' options. This may not be
            // wanted behavior

            main_utils::parse_flags(&global_tbd_flags_to_add, index, argc, argv);

            global.options.remove_flags = true;
            global.options.replace_flags = true;
        } else if (strcmp(option, "h") == 0 || strcmp(option, "help") == 0 ||
                   strcmp(option, "u") == 0 || strcmp(option, "usage") == 0) {
            if (index != 1 || index != argc - 1) {
                // Use argument to print out provided option
                // as it is the full string with dashes that the
                // user provided

                fprintf(stderr, "Option (%s) needs to be run by itself\n", argument);
                return 1;
            }

            main_utils::print_help_menu();
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

            // XXX: Might want to change this behavior in the future

            struct main_utils::tbd_with_options::options options;

            // Keep track of whether or not the user
            // provided an output-path, as we cannot
            // tell when iterating because we get the
            // reference to our tbd_with_options object
            // when parsing path

            auto user_did_provide_output_path = false;

            for (; index != argc; index++) {
                const auto argument = argv[index];
                if (argument[0] == '-') {
                    auto option = &argument[1];
                    if (option[0] == '-') {
                        option = &option[1];
                    }

                    if (strcmp(option, "maintain-directories") == 0) {
                        options.maintain_directories = true;
                    } else if (strcmp(option, "no-overwrite") == 0) {
                        options.no_overwrite = true;
                    } else if (strcmp(option, "replace-path-extension") == 0) {
                        options.replace_path_extension = true;
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
                if (const auto &front = path.front(); front != '/' && front != '\\') {
                    path = misc::path_with_current_directory(path.c_str());
                }

                const auto tbds_size = tbds.size();
                if (tbds_current_index == tbds_size) {
                    fprintf(stderr, "Output arguments with path (%s) is missing a corresponding tbd input\n", path.c_str());
                    return 1;
                }

                auto &tbd = tbds.at(tbds_current_index);

                if (tbd.options.recurse_directories_at_path) {
                    if (path == "stdout") {
                        fputs("Cannot write files found while recursing to stdout. Please provide a directory to write created tbd files to\n", stderr);
                        return 1;
                    }
                } else {
                    if (options.maintain_directories) {
                        fputs("There are no directories to maintain when there is no recursion\n", stderr);
                        return 1;
                    }

                    if (options.replace_path_extension) {
                        fputs("Replacing path-extension is an option meant to use be used for recursing directories\n", stderr);
                        return 1;
                    }
                }

                struct stat information;
                if (stat(path.c_str(), &information) == 0) {
                    if (S_ISREG(information.st_mode)) {
                        if (tbd.options.recurse_directories_at_path) {
                            fputs("Cannot write created tbd files found while recursing to a file. Please provide a directory to write created tbd files to\n", stderr);
                            return 1;
                        }
                    } else if (S_ISDIR(information.st_mode)) {
                        if (!tbd.options.recurse_directories_at_path) {
                            fputs("Cannot write a created tbd file to a directory object, Please provide a path to a file\n", stderr);
                            return 1;
                        }
                    }
                }

                if (tbd.options.recurse_directories_at_path) {
                    if (const auto path_back = path.back(); path_back != '/' && path_back != '\\') {
                        path.append(1, '/');
                    }
                }

                tbd.options.value |= options.value;
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

            auto tbd = main_utils::tbd_with_options();

            // An unused structure that is only written
            // to for parsing of local don't options

            struct main_utils::tbd_with_options::dont_options donts;

            // Validate all local-options, but leave the multi-argument
            // local-options to be parsed after tbd-creation with
            // tbd_with_options::local_options_start

            // However, parse architecture-list related options as they
            // simply require and OR on the architectures-list

            for (; index != argc; index++) {
                const auto argument = argv[index];
                if (argument[0] == '-') {
                    if (!tbd.local_option_start) {
                        tbd.local_option_start = index;
                    }

                    auto option = &argument[1];
                    if (option[0] == '-') {
                        option = &option[1];
                    }

                    if (strcmp(option, "r") == 0 || strcmp(option, "recurse") == 0) {
                        tbd.options.recurse_directories_at_path = true;

                        if (index + 1 < argc) {
                            const auto argument = argv[index + 1];
                            const auto &argument_front = argument[0];

                            if (argument_front != '-') {
                                if (strcmp(argument, "all") == 0) {
                                    tbd.options.recurse_subdirectories_at_path = true;
                                    index++;
                                } else if (strcmp(argument, "once") == 0) {
                                    index++;
                                }
                            }
                        }
                    } else if (tbd.parse_option_from_argv(option, index, argc, argv)) {
                        continue;
                    } else if (tbd.parse_dont_option_from_argv(option, index, argc, argv)) {
                        continue;
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
                    if (tbd.options.recurse_directories_at_path) {
                        fprintf(stderr, "Cannot recurse file at path (%s). Please provide a path to a directory to recurse in\n", path.c_str());
                        return 1;
                    }
                } else if (S_ISDIR(information.st_mode)) {
                    if (!tbd.options.recurse_directories_at_path) {
                        fprintf(stderr, "Cannot open directory at path (%s) as a mach-o file. Please provide -r (--recurse) (with an optional recurse-type) to recurse the directory\n", path.c_str());
                        return 1;
                    }

                    if (const auto &back = path.back(); back != '/' && back != '\\') {
                        path.append(1, '/');
                    }
                } else {
                    fprintf(stderr, "Unrecognized object at path (%s). tbd current only supports regular files\n", path.c_str());
                    return 1;
                }
                
                tbd.apply_dont_options();

                // Check if any options overlap with one another
                // This can occur in any of the following situations
                //    I. Removing a field and modifying its value in some way

                if (tbd.write_options.ignore_architectures) {
                    if (tbd.options.replace_architectures || tbd.options.remove_architectures) {
                        fprintf(stderr, "Cannot both modify tbd architectures field and remove the field entirely for path (%s)\n", path.c_str());
                        return 1;
                    }
                }

                if (tbd.write_options.ignore_flags) {
                    if (tbd.options.replace_flags || tbd.options.remove_flags) {
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
        } else if (strcmp(option, "remove-archs") == 0) {
            if (global.options.replace_architectures && !global.options.remove_architectures) {
                fputs("Can't both remove architectures and replace select architectures from list found\n", stderr);
                exit(1);
            }
            
            global_architectures_to_override_to_re = main_utils::parse_architectures_list(++index, argc, argv);
            global.options.remove_architectures = true;
            
            // Write out replaced architectures
            // over each export's architectures
            
            global.write_options.ignore_export_architectures = true;
        } else if (strcmp(option, "remove-flags") == 0) {
            if (global.options.replace_flags && !global.options.remove_flags) {
                fputs("Can't both remove flags and replace select flags from list found\n", stderr);
                exit(1);
            }
            
            // Simply validate flags for now
            
            main_utils::parse_flags(&global_tbd_flags_to_re, ++index, argc, argv);
            global.options.remove_flags = true;
        } else if (strcmp(option, "replace-archs") == 0) {
            // Replacing architectures 'resets' the global architectures
            // list, replacing not only add-archs options, but earlier provided
            // global archs options

            if (global.options.remove_architectures) {
                fputs("Can't both replace and add/remove architectures\n", stderr);
                return 1;
            }

            global_architectures_to_override_to_re = main_utils::parse_architectures_list(++index, argc, argv);
            
            global.creation_options.ignore_architectures = true;
            global.options.replace_architectures = true;
            
            // Don't write out uuids for
            // architectures we don't have
            
            global.creation_options.ignore_uuids = true;
            global.write_options.ignore_uuids = true;
            
            // Write out replaced architectures
            // over each export's architectures
            
            global.write_options.ignore_export_architectures = true;
        } else if (strcmp(option, "replace-flags") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a list of flags to replace ones found\n", stderr);
                return 1;
            }

            if (global.options.remove_flags) {
                fputs("Can't both replace flags found and add/remove select flags from flags found\n", stderr);
                return 1;
            }

            global_tbd_flags_to_re.clear();
            main_utils::parse_flags(&global_tbd_flags_to_re, index, argc, argv);

            // We need to stop tbd::create from
            // overiding these replaced flags

            global.options.replace_flags = true;
        } else if (global.parse_option_from_argv(option, index, argc, argv)) {
            continue;
        } else if (global.parse_dont_option_from_argv(option, index, argc, argv)) {
            continue;
        } else if (main_utils::parse_list_argument(argument, index, argc, argv)) {
            continue;
        } else if (strcmp(option, "validate-macho-dylib") == 0) {
            index++;
            if (index == argc) {
                fputs("Please provide a path to a file to validate as a mach-o dynamic library\n", stderr);
                return 1;
            }

            for (; index != argc; index++) {
                auto path = std::string(argv[index]);
                if (const auto &front = path.front(); front != '/' && front != '\\') {
                    path = misc::path_with_current_directory(path.c_str());
                }

                struct stat information;
                if (stat(path.c_str(), &information) == 0) {
                    if (!S_ISREG(information.st_mode)) {
                        if (S_ISDIR(information.st_mode)) {
                            fprintf(stderr, "Unable to open directory at path (%s). Please provide a path to a regular file\n", path.c_str());
                        } else {
                            fprintf(stderr, "Unable to open object at path (%s). Please provide a path to a regular file\n", path.c_str());
                        }

                        return 1;
                    }
                }

                auto file = macho::file();
                switch (file.open(path.c_str())) {
                    case macho::file::open_result::ok:
                        break;

                    case macho::file::open_result::not_a_macho:
                        fprintf(stderr, "File (at path %s) is not a valid mach-o file\n", path.c_str());
                        continue;

                    case macho::file::open_result::invalid_macho:
                        fprintf(stderr, "File (at path %s) is an invalid mach-o file\n", path.c_str());
                        continue;

                    case macho::file::open_result::failed_to_open_stream:
                        fprintf(stderr, "Failed to open file (at path %s), failing with error: %s\n", path.c_str(), strerror(file.stream.error()));
                        continue;

                    case macho::file::open_result::stream_seek_error:
                    case macho::file::open_result::stream_read_error:
                        fprintf(stderr, "Encountered an error while parsing file (at path %s)\n", path.c_str());
                        continue;

                    case macho::file::open_result::zero_containers:
                        fprintf(stderr, "Mach-o file (at path %s) has no architectures\n", path.c_str());
                        continue;

                    case macho::file::open_result::too_many_containers:
                        fprintf(stderr, "Mach-o file (at path %s) has too many architectures to fit inside the mach-o file\n", path.c_str());
                        continue;

                    case macho::file::open_result::overlapping_containers:
                        fprintf(stderr, "Mach-o file (at path %s) has architectures overlap with one another\n", path.c_str());
                        continue;

                    case macho::file::open_result::invalid_container:
                        fprintf(stderr, "Mach-o file (at path %s) has architecture that is invalid\n", path.c_str());
                        continue;

                    default:
                        break;
                }

                auto is_macho_dynamic_library = macho::utils::validation::as_dynamic_library(file);
                if (is_macho_dynamic_library) {
                    fprintf(stdout, "File at path %s is a valid mach-o dynamic library\n", path.c_str());
                } else {
                    fprintf(stdout, "File at path %s is not a mach-o dynamic library\n", path.c_str());
                }
            }

            return 0;
        } else {
            fprintf(stderr, "Unrecognized option: %s\n", argument);
            return 1;
        }
    }

    if (tbds.empty()) {
        fputs("Please provide paths to either mach-o files or a directory to recurse\n", stderr);
        return 1;
    }

    auto is_writing_to_stdout = false;
    for (const auto &tbd : tbds) {
        if (!tbd.write_path.empty()) {
            continue;
        }
        
        if (is_writing_to_stdout) {
            fputs("Can't allow more than one mach-o file to be written as a tbd to stdout\n", stderr);
            return 1;
        }
        
        is_writing_to_stdout = true;
    }
    
    auto should_print_paths = tbds.size() != 1;
    auto failed_to_process_tbd = false;
    
    for (auto &tbd : tbds) {

        // Apply global variables from global tbd
        
        tbd.apply_missing_from(global);
        
        // Apply global options

        tbd.creation_options.value |= global.creation_options.value;
        tbd.write_options.value |= global.write_options.value;
        tbd.options.value |= global.options.value;

        // Apply global "don't" options

        tbd.creation_options.value &= ~global.donts.creation_options.value;
        tbd.write_options.value &= ~global.donts.write_options.value;
        tbd.options.value &= ~global.donts.options.value;

        // Parse global-flags after creating tbd as local
        // flags options are parsed after the fact, and
        // as global flags receive priority, we need to
        // apply global-flags ourselves afterwards

        if (tbd.options.recurse_directories_at_path) {
            // A check here is necessary as we cannot check before
            // this whether or not a corresponding write-path was
            // provided for a tbd

            if (tbd.write_path.empty()) {
                fprintf(stderr, "Cannot write created tbd files found while recursing directory (at path %s) to stdout. Please provide a directory to write created tbd files to\n", tbd.path.c_str());
                continue;
            }

            auto user_input_info = main_utils::create_tbd_retained_user_info();
            auto all = main_utils::tbd_with_options();

            auto filetypes = misc::recurse::filetypes();
            auto options = misc::recurse::options();

            if (tbd.options.recurse_subdirectories_at_path) {
                options.recurse_subdirectories = true;
            }

            if (!tbd.options.ignore_warnings) {
                options.print_warnings = true;
            }

            // Validating a mach-o file as a dynamic-library
            // requires loading and parsing its load-commands buffer,
            // before macho::utils::tbd, which also loads and parses its own
            // load-command buffer, which is unnecessary, and very expensive in the long run

            // Instead, allow any mach-o file to pass through,
            // and ignore any errors tbd throws about being
            // unable to find an identification load-command

            filetypes.file = true;
            const auto recurse_result = misc::recurse::macho_files(tbd.path.c_str(), filetypes, options, [&](macho::file &file, std::string &path) {
                auto options = main_utils::create_tbd_options();

                options.print_paths = true;
                options.ignore_missing_dynamic_library_information = true;

                const auto creation_result = main_utils::create_tbd(all, tbd, file, options, &user_input_info, path.c_str());
                if (creation_result) {
                    tbd.apply_local_options(argc, argv);
                    
                    if (global.options.remove_architectures) {
                        tbd.info.architectures |= global_architectures_to_override_to_add;
                        tbd.info.architectures &= ~global_architectures_to_override_to_re;
                    } else if (global.options.replace_architectures) {
                        tbd.info.architectures = global_architectures_to_override_to_re;
                    }
                    
                    if (global.options.remove_flags) {
                        tbd.info.flags.value |= global_tbd_flags_to_add.value;
                        tbd.info.flags.value &= ~global_tbd_flags_to_re.value;
                    } else if (global.options.replace_flags) {
                        tbd.info.flags.value = global_tbd_flags_to_re.value;
                    }
                    
                    // directories_front is an index to the start of the
                    // directories we are supposed to maintain (if needed)

                    auto directories_front = tbd.path.length();
                    if (!tbd.options.maintain_directories) {
                        directories_front = utils::path::find_last_slash(path.cbegin(), path.cend()) - path.cbegin();
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

                    if (tbd.options.replace_path_extension) {
                        extracted_directories_end = utils::path::find_extension(extracted_directories_begin, extracted_directories_end);
                    }

                    utils::path::append_component(write_path, extracted_directories_begin, extracted_directories_end);
                    write_path.append(".tbd");

                    auto terminator = static_cast<char *>(nullptr);
                    auto descriptor = -1;

                    main_utils::recursive_mkdir_last_as_file(write_path.data(), tbd.options.no_overwrite, &terminator, &descriptor);
                    if (descriptor != -1) {
                        if (!main_utils::tbd_write(descriptor, tbd, path, write_path, options.print_paths)) {
                            main_utils::recursively_remove_with_terminator(write_path.data(), terminator, options.print_paths);
                            failed_to_process_tbd = true;
                        }
                    
                        close(descriptor);
                    }
                }

                tbd.info.clear();
                tbd.apply_missing_from(global);

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

                case macho::file::open_result::overlapping_containers:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has architectures that overlap with one another\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path has architectures that overlap with one another\n", stderr);
                    }

                    continue;

                case macho::file::open_result::invalid_container:
                    if (should_print_paths) {
                        fprintf(stderr, "Mach-o file (at path %s) has an architecture that is invalid\n", tbd.path.c_str());
                    } else {
                        fputs("Mach-o file at provided path has an architecture that is invalid\n", stderr);
                    }

                    continue;
            }

            auto options = main_utils::create_tbd_options();

            options.print_paths = should_print_paths;
            options.ignore_missing_dynamic_library_information = true;

            const auto creation_result = main_utils::create_tbd(tbd, tbd, file, options, nullptr, tbd.path.c_str());
            if (!creation_result) {
                failed_to_process_tbd = true;
                continue;
            }

            // Parse local-options for modification of tbd-fields
            // after creation and before write to avoid any extra hoops

            // Do this before applying any global variables as global
            // variables have preference over local variables

            tbd.apply_local_options(argc, argv);

            if (global.options.remove_architectures) {
                tbd.info.architectures |= global_architectures_to_override_to_add;
                tbd.info.architectures &= ~global_architectures_to_override_to_re;
            } else if (global.options.replace_architectures) {
                tbd.info.architectures = global_architectures_to_override_to_re;
            }
            
            if (global.options.remove_flags) {
                tbd.info.flags.value |= global_tbd_flags_to_add.value;
                tbd.info.flags.value &= ~global_tbd_flags_to_re.value;
            } else if (global.options.replace_flags) {
                tbd.info.flags.value = global_tbd_flags_to_re.value;
            }

            auto terminator = static_cast<char *>(nullptr);

            if (!tbd.write_path.empty()) {
                auto descriptor = -1;
                main_utils::recursive_mkdir_last_as_file(tbd.write_path.data(), tbd.options.no_overwrite, &terminator, &descriptor);

                if (descriptor == -1) {
                    continue;
                }
                
                if (!main_utils::tbd_write(descriptor, tbd, tbd.path, tbd.write_path, should_print_paths)) {
                    main_utils::recursively_remove_with_terminator(tbd.write_path.data(), terminator, should_print_paths);
                    failed_to_process_tbd = true;
                }
                
                close(descriptor);
            } else {
                if (!main_utils::tbd_write(STDOUT_FILENO, tbd, tbd.path, tbd.write_path, should_print_paths)) {
                    failed_to_process_tbd = true;
                }
            }
        }
    }
    
    if (failed_to_process_tbd) {
        return 1;
    }

    return 0;
}
