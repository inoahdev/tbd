//
//  src/msic/recursively.cc
//  tbd
//
//  Created by inoahdev on 10/77/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#pragma once
#include <cstddef>

char *recursively_create_directories_from_file_path_ignoring_last(char *path, size_t index);

char *recursively_create_directories_from_file_path_creating_last_as_directory(char *path, size_t index);
char *recursively_create_directories_from_file_path_creating_last_as_file(char *path, size_t index, int *last_descriptor = nullptr);

void recursively_remove_directories_from_file_path(char *path, char *begin, char *end = nullptr);
