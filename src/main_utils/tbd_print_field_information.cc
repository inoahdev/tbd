//
//  src/main_utils/tbd_print_field_information.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "../mach-o/utils/tbd.h"

namespace main_utils {
    void print_tbd_platforms() noexcept {
       fputs(macho::utils::tbd::platform_to_string(static_cast<enum macho::utils::tbd::platform>(1)), stdout);
       for (int i = 2; i < 31; i++) {
           fprintf(stdout, ", %s", macho::utils::tbd::platform_to_string(static_cast<enum macho::utils::tbd::platform>(i)));
       }

       fputc('\n', stdout);
    }

    void print_tbd_versions() noexcept {
        fputs("v1\n", stdout);
        fputs("v2\n", stdout);
    }
}
