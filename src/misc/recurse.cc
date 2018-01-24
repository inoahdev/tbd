//
//  src/misc/recurse.cc
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#include "../mach-o/utils/validation/as_library.h"
#include "../mach-o/utils/validation/as_dynamic_library.h"

#include "recurse.h"

namespace misc::recurse {
    bool is_valid_file_for_filetypes(macho::file &file, const filetypes &filetypes) {
        if (filetypes.has_file()) {
            return true;
        }
        
        const auto is_library = macho::utils::validation::as_library(file);
        if (!is_library) {
            return false;
        }
        
        if (!filetypes.has_dynamic_library()) {
            return is_library;
        }
        
        return macho::utils::validation::as_dynamic_library(file);
    }
    
    bool is_valid_macho_file_at_path(macho::file &file, const char *path, const filetypes &filetypes, const options &options) {
        switch (file.open(path)) {
            case macho::file::open_result::ok:
                break;
                
            case macho::file::open_result::failed_to_open_stream:
                if (options.prints_warnings()) {
                    fprintf(stderr, "Warning: Failed to open file (at path %s), failing with error: %s\n", path, strerror(errno));
                }
                
                return false;

            case macho::file::open_result::failed_to_retrieve_information:
                if (options.prints_warnings()) {
                    fprintf(stderr, "Warning: Failed to retrieve information necessary to process file (at path %s), failing with error: %s\n", path, strerror(errno));
                }
                
                return false;

            case macho::file::open_result::not_a_macho:
            case macho::file::open_result::invalid_macho:
            case macho::file::open_result::stream_seek_error:
            case macho::file::open_result::stream_read_error:
            case macho::file::open_result::zero_containers:
            case macho::file::open_result::too_many_containers:
            case macho::file::open_result::containers_goes_past_end_of_file:
            case macho::file::open_result::overlapping_containers:
            case macho::file::open_result::invalid_container:
                return false;
        }
        
        return is_valid_file_for_filetypes(file, filetypes);
    }
}
