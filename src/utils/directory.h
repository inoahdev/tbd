//
//  src/utils/directory.h
//  tbd
//
//  Created by inoahdev on 10/17/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once

#include <dirent.h>
#include <string>

#include "path.h"

namespace utils {
    class directory {
    public:
        explicit directory() noexcept = default;

        explicit directory(const directory &) = delete;
        explicit directory(directory &&) noexcept;

        directory &operator=(const directory &) = delete;
        directory &operator=(directory &&) noexcept;

        ~directory() noexcept;

        DIR *dir = nullptr;
        std::string path;

        enum class open_result {
            ok,
            failed_to_open_directory
        };

        open_result open(const char *path) noexcept;
        open_result open(const std::string &path) noexcept;

        open_result open(std::string &&path) noexcept;

        void close() noexcept;

        enum class recursion_options : uint64_t {
            none,
            recurse_subdirectories  = 1 << 0,
            ignore_warnings         = 1 << 1,
            skip_current_directory  = 1 << 2,
            skip_parent_directory   = 1 << 3
        };

        enum class recursion_filetypes : uint64_t {
            none,
            pipe             = 1 << 0,
            character_device = 1 << 1,
            directory        = 1 << 2,
            block_device     = 1 << 3,
            regular_file     = 1 << 4,
            symbolic_link    = 1 << 5,
            socket           = 1 << 6
        };

        enum class recursion_warning {
            failed_to_open_sub_directory
        };

        enum class recursion_result {
            ok,
            directory_not_opened,
        };

        template <typename T1, typename T2>
        recursion_result recurse(recursion_filetypes filetypes, recursion_options options, T1 &&callback, T2 &&warning_callback = [](){}) noexcept;

    private:
        static std::string _construct_path(const char *path, size_t path_length, const char *name, bool is_directory = false) noexcept;
    };

    inline uint64_t operator|(const uint64_t &lhs, const directory::recursion_options &rhs) noexcept { return lhs | static_cast<uint64_t>(rhs); }
    inline void operator|=(uint64_t &lhs, const directory::recursion_options &rhs) noexcept { lhs |= static_cast<uint64_t>(rhs); }

    inline directory::recursion_options operator|(const directory::recursion_options &lhs, const uint64_t &rhs) noexcept { return static_cast<directory::recursion_options>(static_cast<uint64_t>(lhs) | rhs); }
    inline void operator|=(directory::recursion_options &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<directory::recursion_options>(static_cast<uint64_t>(lhs) | rhs); }

    inline directory::recursion_options operator|(const directory::recursion_options &lhs, const directory::recursion_options &rhs) noexcept { return static_cast<directory::recursion_options>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }
    inline void operator|=(directory::recursion_options &lhs, const directory::recursion_options &rhs) noexcept { lhs = static_cast<directory::recursion_options>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }

    inline uint64_t operator&(const uint64_t &lhs, const directory::recursion_options &rhs) noexcept { return lhs & static_cast<uint64_t>(rhs); }
    inline void operator&=(uint64_t &lhs, const directory::recursion_options &rhs) noexcept { lhs &= static_cast<uint64_t>(rhs); }

    inline directory::recursion_options operator&(const directory::recursion_options &lhs, const uint64_t &rhs) noexcept { return static_cast<directory::recursion_options>((static_cast<uint64_t>(lhs) & rhs)); }
    inline void operator&=(directory::recursion_options &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<directory::recursion_options>(static_cast<uint64_t>(lhs) & rhs); }

    inline directory::recursion_options operator&(const directory::recursion_options &lhs, const directory::recursion_options &rhs) noexcept { return static_cast<directory::recursion_options>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }
    inline void operator&=(directory::recursion_options &lhs, const directory::recursion_options &rhs) noexcept { lhs = static_cast<directory::recursion_options>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }

    inline directory::recursion_options operator~(const directory::recursion_options &lhs) noexcept { return static_cast<directory::recursion_options>(~static_cast<uint64_t>(lhs)); }

    inline uint64_t operator|(const uint64_t &lhs, const directory::recursion_filetypes &rhs) noexcept { return lhs | static_cast<uint64_t>(rhs); }
    inline void operator|=(uint64_t &lhs, const directory::recursion_filetypes &rhs) noexcept { lhs |= static_cast<uint64_t>(rhs); }

    inline directory::recursion_filetypes operator|(const directory::recursion_filetypes &lhs, const uint64_t &rhs) noexcept { return static_cast<directory::recursion_filetypes>(static_cast<uint64_t>(lhs) | rhs); }
    inline void operator|=(directory::recursion_filetypes &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<directory::recursion_filetypes>(static_cast<uint64_t>(lhs) | rhs); }

    inline directory::recursion_filetypes operator|(const directory::recursion_filetypes &lhs, const directory::recursion_filetypes &rhs) noexcept { return static_cast<directory::recursion_filetypes>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }
    inline void operator|=(directory::recursion_filetypes &lhs, const directory::recursion_filetypes &rhs) noexcept { lhs = static_cast<directory::recursion_filetypes>(static_cast<uint64_t>(lhs) | static_cast<uint64_t>(rhs)); }

    inline uint64_t operator&(const uint64_t &lhs, const directory::recursion_filetypes &rhs) noexcept { return lhs & static_cast<uint64_t>(rhs); }
    inline void operator&=(uint64_t &lhs, const directory::recursion_filetypes &rhs) noexcept { lhs &= static_cast<uint64_t>(rhs); }

    inline directory::recursion_filetypes operator&(const directory::recursion_filetypes &lhs, const uint64_t &rhs) noexcept { return static_cast<directory::recursion_filetypes>((static_cast<uint64_t>(lhs) & rhs)); }
    inline void operator&=(directory::recursion_filetypes &lhs, const uint64_t &rhs) noexcept { lhs = static_cast<directory::recursion_filetypes>(static_cast<uint64_t>(lhs) & rhs); }

    inline directory::recursion_filetypes operator&(const directory::recursion_filetypes &lhs, const directory::recursion_filetypes &rhs) noexcept { return static_cast<directory::recursion_filetypes>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }
    inline void operator&=(directory::recursion_filetypes &lhs, const directory::recursion_filetypes &rhs) noexcept { lhs = static_cast<directory::recursion_filetypes>(static_cast<uint64_t>(lhs) & static_cast<uint64_t>(rhs)); }

    inline directory::recursion_filetypes operator~(const directory::recursion_filetypes &lhs) noexcept { return static_cast<directory::recursion_filetypes>(~static_cast<uint64_t>(lhs)); }

    template <typename T1, typename T2>
    directory::recursion_result directory::recurse(recursion_filetypes filetypes, recursion_options options, T1 &&callback, T2 &&warning_callback) noexcept {
        if (!dir) {
            return recursion_result::directory_not_opened;
        }

        if (filetypes == recursion_filetypes::none) {
            return recursion_result::ok;
        }

        if (options == recursion_options::none) {
            return recursion_result::ok;
        }

        auto directory_entry = readdir(dir);
        while (directory_entry != nullptr) {
            switch (directory_entry->d_type) {
                case DT_FIFO:
                    if ((filetypes & recursion_filetypes::pipe) != recursion_filetypes::none) {
                        callback(*this, _construct_path(path.data(), path.length(), directory_entry->d_name));
                    }

                    break;

                case DT_CHR:
                    if ((filetypes & recursion_filetypes::character_device) != recursion_filetypes::none) {
                        callback(*this, _construct_path(path.data(), path.length(), directory_entry->d_name));
                    }

                    break;

                case DT_DIR: {
                    auto is_current_directory = false;
                    auto is_parent_directory = false;

                    const auto directory_entry_name_length = strlen(directory_entry->d_name);
                    if (strncmp(directory_entry->d_name, ".", directory_entry_name_length) == 0) {
                        if ((options & recursion_options::skip_current_directory) != recursion_options::none) {
                            break;
                        }

                        is_current_directory = true;
                    }

                    if (strncmp(directory_entry->d_name, "..", directory_entry_name_length) == 0) {
                        if ((options & recursion_options::skip_parent_directory) != recursion_options::none) {
                            break;
                        }

                        is_parent_directory = true;
                    }

                    if ((filetypes & recursion_filetypes::directory) != recursion_filetypes::none) {
                        callback(*this, _construct_path(path.data(), path.length(), directory_entry->d_name));
                    }

                    if ((options & recursion_options::recurse_subdirectories) != recursion_options::none) {
                        if (is_current_directory || is_parent_directory) {
                            break;
                        }


                        auto sub_directory = directory();
                        auto sub_directory_open_result = sub_directory.open(_construct_path(path.data(), path.length(), directory_entry->d_name));

                        switch (sub_directory_open_result) {
                            case open_result::ok: {
                                const auto result = sub_directory.recurse(filetypes, options, callback, warning_callback);
                                if (result != recursion_result::ok) {
                                    return result;
                                }

                                break;
                            }

                            case open_result::failed_to_open_directory:
                                warning_callback(recursion_warning::failed_to_open_sub_directory, (const void *)&sub_directory.path);
                                break;
                        }
                    }

                    break;
                }

                case DT_BLK:
                    if ((filetypes & recursion_filetypes::block_device) != recursion_filetypes::none) {
                        callback(*this, _construct_path(path.data(), path.length(), directory_entry->d_name));
                    }

                    break;

                case DT_REG:
                    if ((filetypes & recursion_filetypes::regular_file) != recursion_filetypes::none) {
                        callback(*this, _construct_path(path.data(), path.length(), directory_entry->d_name));
                    }

                    break;

                case DT_LNK:
                    if ((filetypes & recursion_filetypes::symbolic_link) != recursion_filetypes::none) {
                        callback(*this, _construct_path(path.data(), path.length(), directory_entry->d_name));
                    }

                    break;

                case DT_SOCK:
                    if ((filetypes & recursion_filetypes::socket) != recursion_filetypes::none) {
                        callback(*this, _construct_path(path.data(), path.length(), directory_entry->d_name));
                    }

                    break;

                default:
                    break;
            };

            directory_entry = readdir(dir);
        }

        return recursion_result::ok;
    }
}
