//
//  src/our_io.c
//  tbd
//
//  Created by inoahdev on 10/18/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "our_io.h"

int our_open(const char *const path, const int flags, const int mode) {
    do {
#ifdef O_CLOEXEC
        const int fd = open(path, flags | O_CLOEXEC, mode);
#else
        const int fd = open(path, flags, mode);
#endif

        if (fd != -1) {
            return fd;
        }
    } while (errno == EINTR);

    return -1;
}

int our_openat(const int dirfd, const char *const path, const int flags) {
    do {
#ifdef O_CLOEXEC
        const int fd = openat(dirfd, path, flags | O_CLOEXEC);
#else
        const int fd = openat(dirfd, path, flags);
#endif

        if (fd != -1) {
            return fd;
        }
    } while (errno == EINTR);

    return -1;
}

int our_mkdir(const char *const path, const mode_t mode) {
    do {
        const int ret = mkdir(path, mode);
        if (ret == 0) {
            return ret;
        }
    } while (errno == EINTR);

    return -1;
}

int our_unlink(const char *const path) {
    do {
        const int ret = unlink(path);
        if (ret == 0) {
            return ret;
        }
    } while (errno == EINTR);

    return -1;
}

int our_rmdir(const char *const path) {
    do {
        const int ret = rmdir(path);
        if (ret == 0) {
            return ret;
        }
    } while (errno == EINTR);

    return -1;
}

off_t our_lseek(const int fd, const off_t offset, const int whence) {
    do {
        const off_t pos = lseek(fd, offset, whence);
        if (pos != -1) {
            return pos;
        }
    } while (errno == EINTR);

    return -1;
}

ssize_t our_read(const int fd, void *const buf, const size_t size) {
    do {
        const ssize_t num = read(fd, buf, size);
        if (num != -1) {
            return num;
        }
    } while (errno == EINTR);

    return -1;
}

DIR *our_fdopendir(const int fd) {
    do {
        DIR *const dir = fdopendir(fd);
        if (dir != NULL) {
            return dir;
        }
    } while (errno == EINTR);

    return NULL;
}

struct dirent *our_readdir(DIR *const dir) {
    do {
        struct dirent *const ent = readdir(dir);
        if (ent != NULL) {
            return ent;
        }
    } while (errno == EINTR);

    return NULL;
}

ssize_t our_getline(char **const lineptr, size_t *const n, FILE *const stream) {
    do {
        const ssize_t ret = getline(lineptr, n, stream);
        if (ret >= 0) {
            return ret;
        }
    } while (errno == EINTR);

    return 0;
}
