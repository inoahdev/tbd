//
//  misc/recurse.cc
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cerrno>
#include <dirent.h>

#include "../mach-o/file.h"
#include "recurse.h"

namespace recurse {
    bool _macho_file_should_try_reopen_with_open_result(const macho::file::open_result &open_result) {
        switch (open_result) {
            case macho::file::open_result::ok:
            case macho::file::open_result::failed_to_open_stream:
            case macho::file::open_result::failed_to_allocate_memory:
            case macho::file::open_result::failed_to_retrieve_information:
            case macho::file::open_result::stream_seek_error:
            case macho::file::open_result::stream_read_error:
            case macho::file::open_result::zero_architectures:
            case macho::file::open_result::architectures_goes_past_end_of_file:
            case macho::file::open_result::invalid_container:
            case macho::file::open_result::not_a_macho:
                break;

            case macho::file::open_result::not_a_library:
            case macho::file::open_result::not_a_dynamic_library:
                return true;                    
        }

        return false;       
    }

    macho::file::open_result _macho_open_file_for_filetypes(macho::file &macho_file, const char *path, macho_file_type filetypes) {
        auto open_result = macho::file::open_result::ok;
        if ((filetypes & macho_file_type::dynamic_library) != macho_file_type::none) {
            open_result = macho_file.open_from_dynamic_library(path);
            if ((filetypes & macho_file_type::library) != macho_file_type::none) {
                if (_macho_file_should_try_reopen_with_open_result(open_result)) {
                    open_result = macho_file.open_from_library(path);
                    if ((filetypes & macho_file_type::any) != macho_file_type::none) {
                        if (_macho_file_should_try_reopen_with_open_result(open_result)) {
                            open_result = macho_file.open(path);
                        }
                    }
                }
            }
        } else if ((filetypes & macho_file_type::library) != macho_file_type::none) {
            open_result = macho_file.open_from_library(path);
            if ((filetypes & macho_file_type::any) != macho_file_type::none) {
                if (_macho_file_should_try_reopen_with_open_result(open_result)) {
                    open_result = macho_file.open(path);
                }
            }
        } else if ((filetypes & macho_file_type::any) != macho_file_type::none) {
            open_result = macho_file.open(path);
        }

        return open_result;
    }

    bool _macho_check_file_for_filetypes(const char *path, macho_file_type filetypes, macho::file::check_error &error) {
        auto result = false;
        if ((filetypes & macho_file_type::dynamic_library) != macho_file_type::none) {
            result = macho::file::is_valid_dynamic_library(path, &error);
            if ((filetypes & macho_file_type::library) != macho_file_type::none) {
                if (!result) {
                    result = macho::file::is_valid_library(path, &error);
                    if ((filetypes & macho_file_type::any) != macho_file_type::none) {
                        if (!result) {
                            result = macho::file::is_valid_file(path, &error);
                        }
                    }
                }
            }
        } else if ((filetypes & macho_file_type::library) != macho_file_type::none) {
            result = macho::file::is_valid_library(path, &error);
            if ((filetypes & macho_file_type::any) != macho_file_type::none) {
                if (!result) {
                    result = macho::file::is_valid_file(path, &error);
                }
            }
        } else if ((filetypes & macho_file_type::any) != macho_file_type::none) {
            result = macho::file::is_valid_file(path, &error);
        }
        
        return result;
    }
}
