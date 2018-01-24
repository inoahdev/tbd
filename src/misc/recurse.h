//
//  src/misc/recurse.h
//  tbd
//
//  Created by inoahdev on 8/25/17.
//  Copyright © 2017 - 2018 inoahdev. All rights reserved.
//

#pragma once

#include <cerrno>
#include <string>

#include "../utils/directory.h"
#include "../mach-o/file.h"

namespace misc::recurse {
    struct options {
        enum class shifts : uint64_t {
            recurse_subdirectories,
            print_warnings
        };
        
        uint64_t bits = uint64_t();
        
        inline bool has_none() const noexcept {
            return this->bits == 0;
        }
        
        inline bool has_shift(shifts shift) const noexcept {
            return this->bits & static_cast<uint64_t>(shift);
        }
        
        inline bool recurses_subdirectories() const noexcept {
            return this->has_shift(shifts::recurse_subdirectories);
        }
        
        inline bool prints_warnings() const noexcept {
            return this->has_shift(shifts::print_warnings);
        }
        
        inline void add_shift(shifts shift) noexcept {
            this->bits |= (1ull << static_cast<uint64_t>(shift));
        }
        
        inline void recurse_subdirectories() noexcept {
            this->add_shift(shifts::recurse_subdirectories);
        }
        
        inline void print_warnings() noexcept {
            this->add_shift(shifts::print_warnings);
        }
        
        inline void remove_shift(shifts shift) noexcept {
            this->bits &= ~(1ull << static_cast<uint64_t>(shift));
        }
        
        inline void clear() noexcept {
            this->bits = 0;
        }
        
        inline void dont_recurse_subdirectories() noexcept {
            this->remove_shift(shifts::recurse_subdirectories);
        }
        
        inline void dont_print_warnings() noexcept {
            this->remove_shift(shifts::print_warnings);
        }
    };
    
    struct filetypes {
        enum class shifts {
            file,
            library,
            dynamic_library
        };
        
        uint64_t bits = uint64_t();
        
        inline bool has_none() const noexcept {
            return this->bits == 0;
        }
        
        inline bool has_shift(shifts shift) const noexcept {
            return this->bits & static_cast<uint64_t>(shift);
        }
        
        inline bool has_file() const noexcept {
            return this->has_shift(shifts::file);
        }
        
        inline bool has_library() const noexcept {
            return this->has_shift(shifts::library);
        }
        
        inline bool has_dynamic_library() const noexcept {
            return this->has_shift(shifts::dynamic_library);
        }
        
        inline void add_shift(shifts shift) noexcept {
            this->bits |= (1ull << static_cast<uint64_t>(shift));
        }
        
        inline void add_file() noexcept {
            this->add_shift(shifts::file);
        }
        
        inline void add_library() noexcept {
            this->add_shift(shifts::library);
        }
        
        inline void add_dynamic_library() noexcept {
            this->add_shift(shifts::dynamic_library);
        }
        
        inline void remove_shift(shifts shift) noexcept {
            this->bits &= ~(1ull << static_cast<uint64_t>(shift));
        }
        
        inline void clear() noexcept {
            this->bits = 0;
        }
        
        inline void remove_file() noexcept {
            this->remove_shift(shifts::file);
        }
        
        inline void remove_library() noexcept {
            this->remove_shift(shifts::library);
        }
        
        inline void remove_dynamic_library() noexcept {
            this->remove_shift(shifts::dynamic_library);
        }
    };
    
    bool is_valid_file_for_filetypes(macho::file &macho_file, const filetypes &filetypes);
    bool is_valid_macho_file_at_path(macho::file &file, const char *path, const filetypes &filetypes, const options &options);
    
    enum class operation_result {
        ok,
        failed_to_open_directory,
        found_no_matching_files
    };
    
    template <typename T>
    operation_result macho_files(const char *directory_path, const filetypes &filetypes, options options, T &&callback) noexcept { // <void(std::string &, macho::file &)>
        if (filetypes.has_none()) {
            return operation_result::ok;
        }
        
        auto failed_to_open_directory = false;
        if (!recurse_directory(directory_path, filetypes, options, callback, false, &failed_to_open_directory)) {
            if (failed_to_open_directory) {
                return operation_result::failed_to_open_directory;
            }
            
            return operation_result::found_no_matching_files;
        }
        
        return operation_result::ok;
    }
    
    template <typename T>
    bool recurse_directory(const char *directory_path, const filetypes &filetypes, const options &options, T &&callback, bool is_subdirectory, bool *failed_to_open_directory = nullptr) noexcept {
        auto directory = utils::directory();
        auto directory_open_result = directory.open(directory_path);
        
        switch (directory_open_result) {
            case utils::directory::open_result::ok:
                break;
                
            case utils::directory::open_result::failed_to_open_directory:
                if (options.prints_warnings()) {
                    if (is_subdirectory) {
                        fprintf(stderr, "Warning: Failed to open sub-directory at path (%s), failing with error: %s\n", directory_path, strerror(errno));
                    }
                }
                
                if (failed_to_open_directory != nullptr) {
                    *failed_to_open_directory = true;
                }
                
                return false;
        }
        
        auto found_valid_files = false;
        auto has_skipped_past_dots = 0;
        
        for (auto iter = directory.entries.begin; iter != directory.entries.end; iter++) {
            auto should_exit = false;
            switch (iter->d_type) {
                case DT_REG: {
                    auto file = macho::file();
                    auto path = iter.entry_path_from_directory_path(directory_path);
                    
                    if (is_valid_macho_file_at_path(file, path.c_str(), filetypes, options)) {
                        found_valid_files = true;
                        should_exit = !callback(file, path);
                    }
                    
                    break;
                }
                    
                case DT_DIR: {
                    if (!(has_skipped_past_dots & 1)) {
                        if (strcmp(iter->d_name, ".") == 0) {
                            has_skipped_past_dots |= 1;
                            continue;
                        }
                    }
                    
                    if (!(has_skipped_past_dots & 1 << 1)) {
                        if (strcmp(iter->d_name, "..") == 0) {
                            has_skipped_past_dots |= 1 << 1;
                            continue;
                        }
                    }
                    
                    auto path = iter.entry_path_from_directory_path(directory_path);
                    if (recurse_directory(path.c_str(), filetypes, options, callback, true)) {
                        found_valid_files = true;
                    }
                    
                    break;
                }
                    
                default:
                    break;
            }
            
            if (should_exit) {
                break;
            }
        }
        
        return found_valid_files;
    }
}
