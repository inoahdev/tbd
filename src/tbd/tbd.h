//
//  src/tbd/tbd.h
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <mach-o/arch.h>
#include "../mach-o/file.h"

class tbd {
public:
    enum platform {
        ios = 1,
        macosx,
        watchos,
        tvos
    };

    static const char *platform_to_string(const platform &platform) noexcept;
    static platform string_to_platform(const char *platform) noexcept;

    enum class version {
        v1 = 1,
        v2
    };

    static version string_to_version(const char *version) noexcept;

    explicit tbd() = default;
    explicit tbd(const std::vector<std::string> &macho_files, const std::vector<std::string> &output_files, const platform &platform, const version &version, const std::vector<const NXArchInfo *> &architecture_overrides = std::vector<const NXArchInfo *>());

    void run();

    inline std::vector<std::string> &macho_files() noexcept { return macho_files_; }
    inline std::vector<std::string> &output_files() noexcept { return output_files_; }

    inline const std::vector<const NXArchInfo *> architectures() const noexcept { return architectures_; }

    inline void add_macho_file(const std::string &macho_file) noexcept {
        macho_files_.emplace_back(macho_file);
    }

    inline void add_output_file(const std::string &output_files) noexcept {
        output_files_.emplace_back(output_files);
    }

    inline void set_architectures(const std::vector<const NXArchInfo *> &architectures) noexcept {
        architectures_ = architectures;
    }

    inline void set_platform(const platform &platform) noexcept {
        platform_ = platform;
    }

    inline void set_version(const version &version) noexcept {
        version_ = version;
    }

    inline const platform &platform() const noexcept { return platform_; }
    inline const version &version() const noexcept { return version_; }

private:
    std::vector<std::string> macho_files_;
    std::vector<std::string> output_files_;

    enum platform platform_;
    enum version version_;

    std::vector<const NXArchInfo *> architectures_;
    void validate() const;
};
