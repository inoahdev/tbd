//
//  include/yaml.h
//  tbd
//
//  Created by inoahdev on 11/25/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef YAML_H
#define YAML_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "notnull.h"

bool yaml_c_str_needs_quotes(const char *__notnull string, uint64_t length);

#endif /* YAML_H */
