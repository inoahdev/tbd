//
//  misc/recurse.h
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <dirent.h>

#include <functional>
#include <string>

#include "../mach-o/file.h"

namespace recurse {
    enum class options : unsigned int {
        recurse_subdirectories = 1 << 0,
        print_warnings = 1 << 1
    };

    inline unsigned int operator|(const unsigned int &lhs, const options &rhs) noexcept { return lhs | (unsigned int)rhs; }
    inline void operator|=(unsigned int &lhs, const options &rhs) noexcept { lhs |= (unsigned int)rhs; }

    inline options operator|(const options &lhs, const unsigned int &rhs) noexcept { return (options)((unsigned int)lhs | rhs); }
    inline void operator|=(options &lhs, const unsigned int &rhs) noexcept { lhs = (options)((unsigned int)lhs | rhs); }

    inline options operator|(const options &lhs, const options &rhs) noexcept { return (options)((unsigned int)lhs | (unsigned int)rhs); }
    inline void operator|=(options &lhs, const options &rhs) noexcept { lhs = (options)((unsigned int)lhs | (unsigned int)rhs); }

    inline unsigned int operator&(const unsigned int &lhs, const options &rhs) noexcept { return lhs & (unsigned int)rhs; }
    inline void operator&=(unsigned int &lhs, const options &rhs) noexcept { lhs &= (unsigned int)rhs; }

    inline options operator&(const options &lhs, const unsigned int &rhs) noexcept { return (options)((unsigned int)lhs & rhs); }
    inline void operator&=(options &lhs, const unsigned int &rhs) noexcept { lhs = (options)((unsigned int)lhs & rhs); }

    inline options operator&(const options &lhs, const options &rhs) noexcept { return (options)((unsigned int)lhs & (unsigned int)rhs); }
    inline void operator&=(options &lhs, const options &rhs) noexcept { lhs = (options)((unsigned int)lhs & (unsigned int)rhs); }

    enum class operation_result {
        ok,
        failed_to_open_directory
    };

    operation_result macho_libraries(const char *directory_path, unsigned int options, const std::function<void(std::string &, macho::file &)> &callback);
    inline operation_result macho_libraries(const char *directory_path, options options, const std::function<void(std::string &, macho::file &)> &callback) {
        return macho_libraries(directory_path, (unsigned int)options, callback);
    }

    operation_result macho_library_paths(const char *directory_path, unsigned int options, const std::function<void(std::string &)> &callback);
    inline operation_result macho_library_paths(const char *directory_path, options options, const std::function<void(std::string &)> &callback) {
        return macho_library_paths(directory_path, (unsigned int)options, callback);
    }
}
