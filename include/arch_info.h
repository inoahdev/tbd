//
//  include/arch_info.h
//  tbd
//
//  Created by inoahdev on 11/19/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef ARCH_INFO_H
#define ARCH_INFO_H

#include "mach-o/loader.h"

struct arch_info {
    cpu_type_t cputype;
    cpu_subtype_t cpusubtype;

    const char *name;
};

const struct arch_info *arch_info_get_list(void);
const uint64_t arch_info_list_get_size(void);

const struct arch_info *
arch_info_for_cputype(cpu_type_t cputype, cpu_subtype_t cpusubtype);

const struct arch_info *arch_info_for_name(const char *name);

#endif /* ARCH_INFO_H */
