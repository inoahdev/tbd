//
//  src/main_utils/parse_list_argument.h
//  tbd
//
//  Created by inoahdev on 1/26/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#pragma once

namespace main_utils {
    bool parse_list_argument(const char *argument, int &index, int argc, const char *argv[]) noexcept;
}
