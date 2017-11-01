//
//  src/objc/image_info.h
//  tbd
//
//  Created by inoahdev on 8/15/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstdint>

namespace objc {
    typedef struct image_info {
        uint32_t version;
        uint32_t flags;

        enum class flags : uint32_t {
            is_replacement       = 1 << 0,
            supports_gc          = 1 << 1,
            requires_gc          = 1 << 2,
            optimized_by_dyld    = 1 << 3,
            corrected_synthesize = 1 << 4,
            is_simulated         = 1 << 5,
            has_category_class_properties = 1 << 6
        };

        enum class swift_version : uint32_t {
            shift = 8,
            mask = 0xff
        };
    } image_info;

    inline uint32_t operator|(const uint32_t &lhs, const enum image_info::flags &rhs) noexcept { return lhs | static_cast<uint32_t>(rhs); }
    inline void operator|=(uint32_t &lhs, const enum image_info::flags &rhs) noexcept { lhs |= static_cast<uint32_t>(rhs); }

    inline enum image_info::flags operator|(const enum image_info::flags &lhs, const uint32_t &rhs) noexcept { return static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) | rhs); }
    inline void operator|=(enum image_info::flags &lhs, const uint32_t &rhs) noexcept { lhs = static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) | rhs); }

    inline enum image_info::flags operator|(const enum image_info::flags &lhs, const enum image_info::flags &rhs) noexcept { return static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); }
    inline void operator|=(enum image_info::flags &lhs, const enum image_info::flags &rhs) noexcept { lhs = static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); }

    inline uint32_t operator&(const uint32_t &lhs, const enum image_info::flags &rhs) noexcept { return lhs & static_cast<uint32_t>(rhs); }
    inline void operator&=(uint32_t &lhs, const enum image_info::flags &rhs) noexcept { lhs &= static_cast<uint32_t>(rhs); }

    inline enum image_info::flags operator&(const enum image_info::flags &lhs, const uint32_t &rhs) noexcept { return static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) & rhs); }
    inline void operator&=(enum image_info::flags &lhs, const uint32_t &rhs) noexcept { lhs = static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) & rhs); }

    inline enum image_info::flags operator&(const enum image_info::flags &lhs, const enum image_info::flags &rhs) noexcept { return static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); }
    inline void operator&=(enum image_info::flags &lhs, const enum image_info::flags &rhs) noexcept { lhs = static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); }

    inline uint32_t operator|(const uint32_t &lhs, const enum image_info::swift_version &rhs) noexcept { return lhs | static_cast<uint32_t>(rhs); }
    inline void operator|=(uint32_t &lhs, const enum image_info::swift_version &rhs) noexcept { lhs |= static_cast<uint32_t>(rhs); }

    inline enum image_info::swift_version operator|(const enum image_info::swift_version &lhs, const uint32_t &rhs) noexcept { return static_cast<enum image_info::swift_version>(static_cast<uint32_t>(lhs) | rhs); }
    inline void operator|=(enum image_info::swift_version &lhs, const uint32_t &rhs) noexcept { lhs = static_cast<enum image_info::swift_version>(static_cast<uint32_t>(lhs) | rhs); }

    inline enum image_info::swift_version operator|(const enum image_info::swift_version &lhs, const enum image_info::swift_version &rhs) noexcept { return static_cast<enum image_info::swift_version>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); }
    inline void operator|=(enum image_info::swift_version &lhs, const enum image_info::swift_version &rhs) noexcept { lhs = static_cast<enum image_info::swift_version>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)); }

    inline uint32_t operator&(const uint32_t &lhs, const enum image_info::swift_version &rhs) noexcept { return lhs & static_cast<uint32_t>(rhs); }
    inline void operator&=(uint32_t &lhs, const enum image_info::swift_version &rhs) noexcept { lhs &= static_cast<uint32_t>(rhs); }

    inline enum image_info::swift_version operator&(const enum image_info::swift_version &lhs, const uint32_t &rhs) noexcept { return static_cast<enum image_info::swift_version>(static_cast<uint32_t>(lhs) & rhs); }
    inline void operator&=(enum image_info::swift_version &lhs, const uint32_t &rhs) noexcept { lhs = static_cast<enum image_info::swift_version>(static_cast<uint32_t>(lhs) & rhs); }

    inline enum image_info::swift_version operator&(const enum image_info::swift_version &lhs, const enum image_info::swift_version &rhs) noexcept { return static_cast<enum image_info::swift_version>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); }
    inline void operator&=(enum image_info::swift_version &lhs, const enum image_info::swift_version &rhs) noexcept { lhs = static_cast<enum image_info::swift_version>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)); }

    inline uint32_t operator>>(const uint32_t &lhs, const enum image_info::swift_version &rhs) noexcept { return lhs << static_cast<uint32_t>(rhs); }
    inline uint32_t operator<<(const uint32_t &lhs, const enum image_info::swift_version &rhs) noexcept { return lhs << static_cast<uint32_t>(rhs); }

    inline uint32_t operator>>(const enum image_info::flags &lhs, const enum image_info::swift_version &rhs) noexcept { return static_cast<uint32_t>(lhs) << static_cast<uint32_t>(rhs); }
    inline uint32_t operator<<(const enum image_info::flags &lhs, const enum image_info::swift_version &rhs) noexcept { return static_cast<uint32_t>(lhs) << static_cast<uint32_t>(rhs); }

    inline void operator>>=(uint32_t &lhs, const enum image_info::swift_version &rhs) noexcept { lhs = lhs << static_cast<uint32_t>(rhs); }
    inline void operator<<=(uint32_t &lhs, const enum image_info::swift_version &rhs) noexcept { lhs = lhs << static_cast<uint32_t>(rhs); }

    inline void operator>>=(enum image_info::flags &lhs, const enum image_info::swift_version &rhs) noexcept { lhs = static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) << static_cast<uint32_t>(rhs)); }
    inline void operator<<=(enum image_info::flags &lhs, const enum image_info::swift_version &rhs) noexcept { lhs = static_cast<enum image_info::flags>(static_cast<uint32_t>(lhs) << static_cast<uint32_t>(rhs)); }
}
