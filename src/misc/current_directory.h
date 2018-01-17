//
//  src/misc/current_directory.h
//  tbd
//
//  Created by inoahdev on 1/14/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#pragma once
#include <string>

namespace misc {
    const char *retrieve_current_directory();
    std::string path_with_current_directory(const char *root);
}
