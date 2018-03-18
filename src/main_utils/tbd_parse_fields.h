//
//  src/main_utils/tbd_parse_fields.h
//  tbd
//
//  Created by inoahdev on 1/23/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include "../utils/tbd.h"

namespace main_utils {
    void parse_flags(struct utils::tbd::flags *flags, int &index, int argc, const char *argv[]);
    uint32_t parse_swift_version_from_argument(int &index, int argc, const char *argv[]);
    
    enum utils::tbd::objc_constraint parse_objc_constraint_from_argument(int &index, int argc, const char *argv[]);
    enum utils::tbd::platform parse_platform_from_argument(int &index, int argc, const char *argv[]);
    enum utils::tbd::version parse_tbd_version(int &index, int argc, const char *argv[]);
}
