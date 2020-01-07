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
tbd_write_archs_for_header(FILE *__notnull const file,
                           const struct target_list list);

int
tbd_write_targets_for_header(FILE *__notnull file,
                             struct target_list list,
                             enum tbd_version version);

int tbd_write_current_version(FILE *__notnull file, uint32_t version);
int tbd_write_compatibility_version(FILE *__notnull file, uint32_t version);

int tbd_write_flags(FILE *__notnull file, struct tbd_flags flags);
int tbd_write_footer(FILE *__notnull file);

int tbd_write_install_name(FILE *__notnull file,
                           const struct tbd_create_info *__notnull info);

int tbd_write_magic(FILE *__notnull file, enum tbd_version version);

int
tbd_write_parent_umbrella_for_archs(
    FILE *__notnull file,
    const struct tbd_create_info *__notnull info);

int
tbd_write_platform(FILE *__notnull file,
                   const struct tbd_create_info *__notnull info,
                   enum tbd_version version);

int
tbd_write_objc_constraint(FILE *__notnull file,
                          enum tbd_objc_constraint constraint);

int
tbd_write_swift_version(FILE *__notnull file,
                        enum tbd_version version,
                        uint32_t swift_version);

int
tbd_write_metadata(FILE *__notnull file,
                   const struct tbd_create_info *__notnull info_in,
                   struct tbd_create_options options);

int
tbd_write_metadata_with_full_targets(
    FILE *__notnull file,
    const struct tbd_create_info *__notnull info_in,
    struct tbd_create_options options);

int
tbd_write_uuids_for_archs(FILE *__notnull file,
                          const struct array *__notnull uuids);

int
tbd_write_uuids_for_targets(FILE *__notnull file,
                            const struct array *__notnull uuids,
                            enum tbd_version version);

int
tbd_write_symbols_for_archs(FILE *__notnull file,
                            const struct tbd_create_info *__notnull info,
                            struct tbd_create_options options);

int
tbd_write_symbols_for_targets(FILE *__notnull const file,
                              const struct tbd_create_info *__notnull info,
                              struct tbd_create_options options);

int
tbd_write_symbols_with_full_archs(FILE *__notnull file,
                                  const struct tbd_create_info *__notnull info,
                                  struct tbd_create_options options);

int
tbd_write_symbols_with_full_targets(
    FILE *__notnull file,
    const struct tbd_create_info *__notnull info,
    struct tbd_create_options options);

#endif /* TBD_WRITE_H */
