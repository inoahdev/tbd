//
//  include/yaml.h
//  tbd
//
//  Created by inoahdev on 11/25/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#ifndef YAML_H
#define YAML_H

#include <stdint.h>
#include <stdio.h>

void
yaml_check_c_str(const char *string, uint64_t length, bool *needs_quotes_out);

#endif /* YAML_H */
