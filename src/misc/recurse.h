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
    enum class options : uint64_t {
        recurse_subdirectories = 1 << 0,
        print_warnings = 1 << 1
    };

    inline uint64_t operator|(const uint64_t &lhs, const options &rhs) noexcept { return lhs | static_cast<uint64_t>(rhs); }
    inline void operator|=(uint64_t &lhs, const options &rhs) noexcept { lhs |= static_cast<uint64_t>(rhs); }

    inline options operator|(const options &lhs, const uint64_t &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) | rhs); }
    inline void operator|=(options &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) | rhs); }

    inline options operator|(const options &lhs, const options &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }
    inline void operator|=(options &lhs, const options &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }

    inline uint64_t operator&(const uint64_t &lhs, const options &rhs) noexcept { return lhs & static_cast<uint64_t>(rhs); }
    inline void operator&=(uint64_t &lhs, const options &rhs) noexcept { lhs &= static_cast<uint64_t>(rhs); }

    inline options operator&(const options &lhs, const uint64_t &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) & rhs); }
    inline void operator&=(options &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) & rhs); }

    inline options operator&(const options &lhs, const options &rhs) noexcept { return static_cast<options>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }
    inline void operator&=(options &lhs, const options &rhs) noexcept { lhs = static_cast<options>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }

    enum class operation_result {
        ok,
        failed_to_open_directory
    };

    operation_result macho_libraries(const char *directory_path, uint64_t options, const std::function<void(std::string &, macho::file &)> &callback);
    inline operation_result macho_libraries(const char *directory_path, options options, const std::function<void(std::string &, macho::file &)> &callback) {
        return macho_libraries(directory_path, (uint64_t)options, callback);
    }

    operation_result macho_library_paths(const char *directory_path, uint64_t options, const std::function<void(std::string &)> &callback);
    inline operation_result macho_library_paths(const char *directory_path, options options, const std::function<void(std::string &)> &callback) {
        return macho_library_paths(directory_path, (uint64_t)options, callback);
    }
}
