//
//  src/utils/range.h
//  tbd
//
//  Created by inoahdev on 12/27/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace utils {
    typedef struct range {
        uint64_t begin = uint64_t();
        uint64_t end = uint64_t();
        
        explicit range() = default;
        explicit range(uint64_t begin, uint64_t end)
        : begin(begin), end(end) {}
        
        inline static bool is_valid_size_based_range(uint64_t begin, uint64_t size) noexcept {
            // Check for overflow when calculating
            return begin + size >= size;
        }
        
        inline bool contains_location(uint64_t location) const noexcept {
            return location >= this->begin && location < this->end;
        }
        
        inline bool overlaps_with_range(const range &range) const noexcept {
            return this->contains_location(range.begin) || this->contains_location(range.end);
        }
        
        inline bool is_within_range(const range &range) {
            return range.contains_location(begin) && range.contains_location(end);
        }
        
        inline bool is_out_of_bounds_of_range(const range &range) const noexcept {
            return !range.contains_location(begin) || !range.contains_location(range.end);
        }
        
        inline bool is_or_goes_past_end(uint64_t end) const noexcept {
            return this->begin > end || this->end > end;
        }
        
        inline bool is_or_goes_past_end_of_range(const range &range) const noexcept {
            return this->is_or_goes_past_end(range.end);
        }
        
    } range;
}
