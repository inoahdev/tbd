//
//  src/recursive/remove.h
//  tbd
//
//  Created by inoahdev on 10/15/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

namespace recursive::remove {
    enum class result {
        ok,

        failed_to_remove_directory,
        failed_to_remove_subdirectories,

        directory_doesnt_exist,
        sub_directory_doesnt_exist
    };

    result perform(char *path);
    result perform(char *path, char *begin);
}
