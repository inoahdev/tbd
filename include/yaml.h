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

int yaml_write_string(FILE *file, const char *string);
int
yaml_write_string_with_len(FILE *file,
                           const char *string,
                           uint32_t len);

#endif /* YAML_H */
