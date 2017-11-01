//
//  src/mach-o/architecture_info.h
//  tbd
//
//  Created by inoahdev on 7/18/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <cstddef>
#include "headers/cputype.h"

namespace macho {
    typedef struct architecture_info {
        cputype cputype;
        subtype subtype;

        const char *name;
    } architecture_info;

    const architecture_info *get_architecture_info_table();
    const char *name_from_cputype(cputype cputype, subtype subtype);

    const size_t get_architecture_info_table_size();

    const architecture_info *architecture_info_from_index(size_t index);
    const architecture_info *architecture_info_from_name(const char *name);
    const architecture_info *architecture_info_from_cputype(cputype cputype, subtype subtype);

    size_t architecture_info_index_from_name(const char *name);
    size_t architecture_info_index_from_cputype(cputype cputype, subtype subtype);

    bool is_valid_architecture_name(const char *architecture_name);
}
