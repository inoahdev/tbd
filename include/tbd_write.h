//
//  include/tbd_write.h
//  tbd
//
//  Created by inoahdev on 11/23/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#ifndef TBD_WRITE_H
#define TBD_WRITE_H

#include "notnull.h"
#include "tbd.h"

int
tbd_write_archs_for_header(FILE *__notnull file,
                           uint64_t archs,
                           int archs_count);

int tbd_write_current_version(FILE *__notnull file, uint32_t version);
int tbd_write_compatibility_version(FILE *__notnull file, uint32_t version);

int
tbd_write_exports(FILE *__notnull file,
                  const struct array *__notnull exports,
                  enum tbd_version version);

int
tbd_write_undefineds(FILE *__notnull file,
                     const struct array *__notnull undefineds,
                     enum tbd_version version);

int
tbd_write_exports_with_full_archs(const struct tbd_create_info *__notnull info,
                                  const struct array *__notnull exports,
                                  FILE *__notnull file);

int
tbd_write_undefineds_with_full_archs(
    const struct tbd_create_info *__notnull info,
    const struct array *__notnull undefineds,
    FILE *__notnull file);

int tbd_write_flags(FILE *__notnull file, uint64_t flags);
int tbd_write_footer(FILE *__notnull file);

int tbd_write_install_name(FILE *__notnull file,
                           const struct tbd_create_info *__notnull info);

int tbd_write_magic(FILE *__notnull file, enum tbd_version version);

int tbd_write_parent_umbrella(FILE *__notnull file,
                              const struct tbd_create_info *__notnull info);

int tbd_write_platform(FILE *__notnull file, enum tbd_platform platform);
int tbd_write_objc_constraint(FILE *__notnull file,
                              enum tbd_objc_constraint constraint);

int tbd_write_uuids(FILE *__notnull file, const struct array *__notnull uuids);

int
tbd_write_swift_version(FILE *__notnull file,
                        enum tbd_version tbd_vers,
                        uint32_t swift_vers);

#endif /* TBD_WRITE_H */
