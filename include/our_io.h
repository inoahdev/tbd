//
//  include/our_io.h
//  tbd
//
//  Created by inoahdev on 10/18/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#ifndef OUR_IO_H
#define OUR_IO_H

#include <sys/types.h>

#include <dirent.h>
#include <stddef.h>
#include <stdio.h>

int our_open(const char *path, int flags, int mode);
int our_openat(int dirfd, const char *pathname, int flags);

int our_mkdir(const char *path, mode_t mode);
int our_unlink(const char *path);
int our_rmdir(const char *path);

off_t our_lseek(int fd, off_t offset, int whence);
ssize_t our_read(int fd, void *buf, size_t size);

DIR *our_fdopendir(int fd);
struct dirent *our_readdir(DIR *dir);

ssize_t our_getline(char **lineptr, size_t *n, FILE *stream);

#endif /* OUR_IO_H */
