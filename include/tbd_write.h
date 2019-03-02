//
//  include/tbd_write.h
//  tbd
//
//  Created by inoahdev on 11/23/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef TBD_WRITE_H
#define TBD_WRITE_H

#include "tbd.h"

int tbd_write_archs_for_header(FILE *file, uint64_t archs);
int tbd_write_current_version(FILE *file, uint32_t version);
int tbd_write_compatibility_version(FILE *file, uint32_t version);

int
tbd_write_exports(FILE *file,
                  const struct array *exports,
                  enum tbd_version version);

int tbd_write_flags(FILE *file, uint64_t flags);
int tbd_write_footer(FILE *file);

int tbd_write_install_name(FILE *file, const struct tbd_create_info *info);
int tbd_write_magic(FILE *file, enum tbd_version version);

int tbd_write_parent_umbrella(FILE *file, const struct tbd_create_info *info);
int tbd_write_platform(FILE *file, enum tbd_platform platform);
int tbd_write_objc_constraint(FILE *file, enum tbd_objc_constraint constraint);
int tbd_write_uuids(FILE *file, const struct array *uuids);

int
tbd_write_swift_version(FILE *file,
                        enum tbd_version tbd_vers,
                        uint32_t swift_vers);

#endif /* TBD_WRITE_H */
