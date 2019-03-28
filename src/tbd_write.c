//
//  src/tbd_write.c
//  tbd
//
//  Created by inoahdev on 11/23/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <inttypes.h>
#include <stdio.h>

#include "tbd_write.h"
#include "yaml.h"

int tbd_write_archs_for_header(FILE *const file, const uint64_t archs) {
    if (archs == 0) {
        return 1;
    }

    /*
     * We need to find the first arch-info to print the list, and then print
     * subsequent archs with a preceding comma.
     */

    const struct arch_info *arch_info_list = arch_info_get_list();

    uint64_t index = 0;
    uint64_t archs_iter = archs;

    do {
        if (archs_iter & 1) {
            const struct arch_info *const arch = arch_info_list + index;
            if (fprintf(file, "archs:%-17s[ %s", "", arch->name) < 0) {
                return 1;
            }

            archs_iter >>= 1;
            break;
        }

        archs_iter >>= 1;
        if (archs_iter == 0) {
            /*
             * If we're already at the end, simply write the end bracket for the
             * arch-info list and return.
             */

            if (fputs(" ]\n", file) < 0) {
                return 1;
            }

            return 0;
        }

        index += 1;
    } while (true);

    /*
     * After writing the first arch, write the following archs with a
     * preceding comma.
     *
     * Count the amount of archs on one line, starting off with one as we just
     * wrote one before looping over the rest. When the counter reaches 7, print
     * a newline and reset the counter.
     */

    uint64_t counter = 1;

    do {
        index += 1;

        if (archs_iter & 1) {
            const struct arch_info *const arch = arch_info_list + index;
            if (fprintf(file, ", %s", arch->name) < 0) {
                return 1;
            }

            /*
             * Break lines for every seven archs written out.
             */

            counter++;
            if (counter == 7) {
                if (fprintf(file, "\n%-19s", "") < 0) {
                    return 1;
                }

                counter = 0;
            }
        }

        archs_iter >>= 1;
        if (archs_iter == 0) {
            break;
        }
    } while (true);

    /*
     * Write the end bracket for the arch-info list and return.
     */

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

static int write_archs_for_exports(FILE *const file, const uint64_t archs) {
    if (archs == 0) {
        return 1;
    }

    /*
     * We need to find the first arch-info to print the list, and then print
     * subsequent archs with a preceding comma.
     */

    const struct arch_info *arch_info_list = arch_info_get_list();

    uint64_t index = 0;
    uint64_t archs_iter = archs;

    do {
        if (archs_iter & 1) {
            const struct arch_info *const arch = arch_info_list + index;
            if (fprintf(file, "  - archs:%-14s[ %s", "", arch->name) < 0) {
                return 1;
            }

            archs_iter >>= 1;
            break;
        }

        archs_iter >>= 1;
        if (archs_iter == 0) {
            /*
             * Write the end bracket for the arch-info list and return.
             */

            if (fputs(" ]\n", file) < 0) {
                return 1;
            }

            return 0;
        }

        index += 1;
    } while (true);

    /*
     * After writing the first arch, write the following archs with a
     * preceding comma.
     *
     * Count the amount of archs on one line, starting off with one as we just
     * wrote one before looping over the rest. When the counter reaches 7, print
     * a newline and reset the counter.
     */

    uint64_t counter = 1;

    do {
        index += 1;

        if (archs_iter & 1) {
            const struct arch_info *const arch = arch_info_list + index;
            if (fprintf(file, ", %s", arch->name) < 0) {
                return 1;
            }

            /*
             * Break lines for every seven archs written out.
             */

            counter++;
            if (counter == 7) {
                if (fprintf(file, "\n%-19s", "") < 0) {
                    return 1;
                }

                counter = 0;
            }
        }

        archs_iter >>= 1;
        if (archs_iter == 0) {
            break;
        }
    } while (true);

    /*
     * Write the end bracket for the arch-info list and return.
     */

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

static int write_packed_version(FILE *const file, const uint32_t version) {
    /*
     * The revision for a packed-version is stored in the LSB byte.
     */

    const uint8_t revision = version & 0xff;

    /*
     * The minor for a packed-version is stored in the second LSB byte.
     */

    const uint8_t minor = (version & 0xff00) >> 8;

    /*
     * The major for a packed-version is stored in the two MSB.
     */

    const uint16_t major = (version & 0xffff0000) >> 16;
    if (fprintf(file, "%" PRIu16, major) < 0) {
        return 1;
    }

    if (minor != 0) {
        if (fprintf(file, ".%" PRIu8, minor) < 0) {
            return 1;
        }
    }

    if (revision != 0) {
        /*
         * Write out a .0 version-component as if minor was zero, we didn't
         * write it as a component.
         */

        if (minor == 0) {
            if (fputs(".0", file) < 0) {
                return 1;
            }
        }

        if (fprintf(file, ".%" PRIu8, revision) < 0) {
            return 1;
        }
    }

    if (fputc('\n', file) < 0) {
        return 1;
    }

    return 0;
}

int tbd_write_current_version(FILE *const file, const uint32_t version) {
    if (fprintf(file, "current-version:%-7s", "") < 0) {
        return 1;
    }

    return write_packed_version(file, version);
}

int tbd_write_compatibility_version(FILE *const file, const uint32_t version) {
    if (fputs("compatibility-version: ", file) < 0) {
        return 1;
    }

    return write_packed_version(file, version);
}

int tbd_write_footer(FILE *const file) {
    if (fputs("...\n", file) < 0) {
        return 1;
    }

    return 0;
}

int tbd_write_flags(FILE *const file, const uint64_t flags) {
    if (flags == 0) {
        return 0;
    }

    if (flags & TBD_FLAG_FLAT_NAMESPACE) {
        if (fprintf(file, "flags:%-17s[ flat_namespace", "") < 0) {
            return 1;
        }

        if (flags & TBD_FLAG_NOT_APP_EXTENSION_SAFE) {
            if (fputs(", not_app_extension_safe", file) < 0) {
                return 1;
            }
        }

        if (fputs(" ]\n", file) < 0) {
            return 1;
        }
    } else if (flags & TBD_FLAG_NOT_APP_EXTENSION_SAFE) {
        if (fprintf(file, "flags:%-17s[ not_app_extension_safe ]\n", "") < 0) {
            return 1;
        }
    }

    return 0;
}

static int
write_yaml_string(FILE *const file,
                  const char *const string,
                  const uint64_t length,
                  const bool needs_quotes)
{
    if (needs_quotes) {
        if (fprintf(file, "\"%s\"", string) < 0) {
            return 1;
        }

        return 0;
    }

    if (fwrite(string, length, 1, file) != 1) {
        return 1;
    }

    return 0;
}

int
tbd_write_install_name(FILE *const file,
                       const struct tbd_create_info *const info)
{
    if (fprintf(file, "install-name:%-10s", "") < 0) {
        return 1;
    }

    const char *const install_name = info->install_name;
    const uint32_t length = info->install_name_length;

    const bool needs_quotes =
        info->flags & F_TBD_CREATE_INFO_INSTALL_NAME_NEEDS_QUOTES;

    if (write_yaml_string(file, install_name, length, needs_quotes)) {
        return 1;
    }

    if (fputc('\n', file) < 0) {
        return 1;
    }

    return 0;
}

int
tbd_write_objc_constraint(FILE *const file,
                          const enum tbd_objc_constraint constraint)
{
    switch (constraint) {
        case TBD_OBJC_CONSTRAINT_NONE:
            if (fprintf(file, "objc-constraint:%-7snone\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_OBJC_CONSTRAINT_GC:
            if (fprintf(file, "objc-constraint:%-7sgc\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_OBJC_CONSTRAINT_RETAIN_RELEASE:
            if (fprintf(file, "objc-constraint:%-7sretain_release\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_OR_GC: {
            const char *const str = "retain_release_or_gc";
            if (fprintf(file, "objc-constraint:%-7s%s\n", "", str) < 0) {
                return 1;
            }

            break;
        }

        case TBD_OBJC_CONSTRAINT_RETAIN_RELEASE_FOR_SIMULATOR: {
            const char *const str = "retain_release_for_simulator";
            if (fprintf(file, "objc-constraint:%-7s%s\n", "", str) < 0) {
                return 1;
            }

            break;
        }
    }

    return 0;
}

int tbd_write_magic(FILE *const file, const enum tbd_version version) {
    switch (version) {
        case TBD_VERSION_V1:
            if (fputs("---\n", file) < 0) {
                return 1;
            }

            break;

        case TBD_VERSION_V2:
            if (fputs("--- !tapi-tbd-v2\n", file) < 0) {
                return 1;
            }

            break;

        case TBD_VERSION_V3:
            if (fputs("--- !tapi-tbd-v3\n", file) < 0) {
                return 1;
            }

            break;
    }

    return 0;
}

int
tbd_write_parent_umbrella(FILE *const file,
                          const struct tbd_create_info *const info)
{
    const char *const umbrella = info->parent_umbrella;
    if (umbrella == NULL) {
        return 0;
    }

    if (fprintf(file, "parent-umbrella:%-7s", "") < 0) {
        return 1;
    }

    const uint32_t length = info->parent_umbrella_length;
    const bool needs_quotes =
        info->flags & F_TBD_CREATE_INFO_PARENT_UMBRELLA_NEEDS_QUOTES;

    if (write_yaml_string(file, umbrella, length, needs_quotes)) {
        return 1;
    }

    if (fputc('\n', file) < 0) {
        return 1;
    }

    return 0;
}

int tbd_write_platform(FILE *const file, const enum tbd_platform platform) {
    switch (platform) {
        case TBD_PLATFORM_MACOS:
            if (fprintf(file, "platform:%-14smacosx\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_IOS:
            if (fprintf(file, "platform:%-14sios\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_WATCHOS:
            if (fprintf(file, "platform:%-14swatchos\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_TVOS:
            if (fprintf(file, "platform:%-14stvos\n", "") < 0) {
                return 1;
            }

            break;

        default:
            return E_TBD_CREATE_WRITE_FAIL;
    }

    return 0;
}

int
tbd_write_swift_version(FILE *const file,
                        const enum tbd_version tbd_version,
                        const uint32_t swift_version)
{
    if (swift_version == 0) {
        return 0;
    }

    switch (tbd_version) {
        case TBD_VERSION_V1:
            return 0;

        case TBD_VERSION_V2:
            if (fprintf(file, "swift-version:%-9s", "") < 0) {
                return 1;
            }

            break;

        case TBD_VERSION_V3:
            if (fprintf(file, "swift-abi-version:%-5s", "") < 0) {
                return 1;
            }

            break;
    }

    switch (swift_version) {
        case 1:
            if (fputs("1\n", file) < 0) {
                return 1;
            }

            break;

        case 2:
            if (fputs("1.2\n", file) < 0) {
                return 1;
            }

            break;

        default:
            if (fprintf(file, "%" PRIu32 "\n", swift_version - 1) < 0) {
                return 1;
            }

            break;
    }

    return 0;
}

static inline int
write_uuid(FILE *const file,
           const struct arch_info *const arch,
           const uint8_t *const uuid,
           const bool has_comma)
{
    int ret = 0;
    if (has_comma) {
        ret =
            fprintf(file,
                    ", '%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X"
                    "%.2X%.2X%.2X%.2X%.2X'",
                    arch->name,
                    uuid[0],
                    uuid[1],
                    uuid[2],
                    uuid[3],
                    uuid[4],
                    uuid[5],
                    uuid[6],
                    uuid[7],
                    uuid[8],
                    uuid[9],
                    uuid[10],
                    uuid[11],
                    uuid[12],
                    uuid[13],
                    uuid[14],
                    uuid[15]);
    } else {
        ret =
            fprintf(file,
                    "'%s: %.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X"
                    "%.2X%.2X%.2X%.2X'",
                    arch->name,
                    uuid[0],
                    uuid[1],
                    uuid[2],
                    uuid[3],
                    uuid[4],
                    uuid[5],
                    uuid[6],
                    uuid[7],
                    uuid[8],
                    uuid[9],
                    uuid[10],
                    uuid[11],
                    uuid[12],
                    uuid[13],
                    uuid[14],
                    uuid[15]);
    }

    if (ret < 0) {
        return 1;
    }

    return 0;
}

int tbd_write_uuids(FILE *const file, const struct array *const uuids) {
    if (array_is_empty(uuids)) {
        return 1;
    }

    if (fprintf(file, "uuids:%-17s[ ", "") < 0) {
        return 1;
    }

    /*
     * Keep a counter of the uuids written out, to limit line-lengths.
     * Start off with a counter of 1 since we write one uuid-pair first before
     * looping over the rest.
     */

    const struct tbd_uuid_info *uuid = uuids->data;
    const struct tbd_uuid_info *const end = uuids->data_end;

    if (write_uuid(file, uuid->arch, uuid->uuid, false)) {
        return 1;
    }

    /*
     * Count the amount of archs on one line, starting off with one as we just
     * wrote one before looping over the rest. When the counter reaches 2, print
     * a newline and reset the counter.
     */

    uint64_t counter = 1;
    bool needs_comma = true;

    do {
        uuid = uuid + 1;
        if (uuid == end) {
            break;
        }

        if (write_uuid(file, uuid->arch, uuid->uuid, needs_comma)) {
            return 1;
        }

        /*
         * We place a limit on the number of uuid-arch pairs on one line to just
         * two to maintain short line-lengths.
         */

        counter++;
        if (counter == 2) {
            if (fprintf(file, ",%-26s", "\n") < 0) {
                return 1;
            }

            counter = 0;
            needs_comma = false;
        } else {
            needs_comma = true;
        }
    } while (true);

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

static int
write_export_type_key(FILE *const file,
                      const enum tbd_export_type type,
                      const enum tbd_version version)
{
    switch (type) {
        case TBD_EXPORT_TYPE_CLIENT: {
            if (version == TBD_VERSION_V1) {
                if (fprintf(file, "%-4sallowed-clients:%-4s[ ", "", "") < 0) {
                    return 1;
                }
            } else {
                if (fprintf(file, "%-4sallowable-clients:%-2s[ ", "", "") < 0) {
                    return 1;
                }
            }

            break;
        }

        case TBD_EXPORT_TYPE_REEXPORT:
            if (fprintf(file, "%-4sre-exports:%9s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_EXPORT_TYPE_NORMAL_SYMBOL:
            if (fprintf(file, "%-4ssymbols:%12s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_EXPORT_TYPE_OBJC_CLASS_SYMBOL:
            if (fprintf(file, "%-4sobjc-classes:%7s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_EXPORT_TYPE_OBJC_EHTYPE_SYMBOL:
            if (fprintf(file, "%-4sobjc-eh-types:%5s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_EXPORT_TYPE_OBJC_IVAR_SYMBOL:
            if (fprintf(file, "%-4sobjc-ivars:%9s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_EXPORT_TYPE_WEAK_DEF_SYMBOL:
            if (fprintf(file, "%-4sweak-def-symbols:%3s[ ", "", "") < 0) {
                return 1;
            }

            break;
    }

    return 0;
}

static inline int end_written_export_array(FILE *const file) {
    const char *const end = " ]\n";
    if (fwrite(end, 3, 1, file) != 1) {
        return 1;
    }

    return 0;
}

static uint32_t line_length_max = 105;

/*
 * Write either a comma or a newline depending on either the current or new
 * line length, or the string-length.
 *
 * Requires:
 *     line_length != 0
 *     string_length != 0
 *
 * Returns:
 *     string_length,
 */

enum write_comma_result {
    E_WRITE_COMMA_OK,
    E_WRITE_COMMA_WRITE_FAIL,
    E_WRITE_COMMA_RESET_LINE_LENGTH,
};

static enum write_comma_result
write_comma_or_newline(FILE *const file,
                       const uint32_t line_length,
                       const uint32_t string_length)
{
    /*
     * If the string is longer than the max-line limit, bend the rules slightly
     * to allow the symbol to fit, albeit on a seperate line.
     */

    if (string_length >= line_length_max) {
        if (line_length != 0) {
            if (fprintf(file, ",\n%-26s", "") < 0) {
                return E_WRITE_COMMA_WRITE_FAIL;
            }
        }

        return E_WRITE_COMMA_RESET_LINE_LENGTH;
    }

    /*
     * If writing the symbol (and it's corresponding comma + space) gets the
     * line's length to beyond the limit, write a comma+newline to have the
     * symbol written on the next line.
     */

    const uint64_t new_line_length = line_length + string_length + 2;
    if (new_line_length > line_length_max) {
        if (fprintf(file, ",\n%-26s", "") < 0) {
            return E_WRITE_COMMA_WRITE_FAIL;
        }

        return E_WRITE_COMMA_RESET_LINE_LENGTH;
    }

    /*
     * Simply have the last symbol written out retrofitted with a comma+space,
     * before writing the next symbol.
     */

    const char *const comma_space = ", ";
    if (fwrite(comma_space, 2, 1, file) != 1) {
        return E_WRITE_COMMA_WRITE_FAIL;
    }

    return E_WRITE_COMMA_OK;
}

static inline int
write_export_info(FILE *const file,
                  const struct tbd_export_info *const info)
{
    const bool needs_quotes =
        info->flags & F_TBD_EXPORT_INFO_STRING_NEEDS_QUOTES;

    return write_yaml_string(file, info->string, info->length, needs_quotes);
}

int
tbd_write_exports(FILE *const file,
                  const struct array *const exports,
                  const enum tbd_version version)
{
    const struct tbd_export_info *info = exports->data;
    const struct tbd_export_info *const end = exports->data_end;

    if (info == end) {
        return 0;
    }

    if (fputs("exports:\n", file) < 0) {
        return 1;
    }

    /*
     * Store the length of the current-line written out (excluding the
     * beginning spaces).
     */

    uint32_t line_length = 0;

    do {
        const uint64_t archs = info->archs;
        if (write_archs_for_exports(file, archs)) {
            return 1;
        }

        enum tbd_export_type type = info->type;
        if (write_export_type_key(file, type, version)) {
            return 1;
        }

        if (write_export_info(file, info)) {
            return 1;
        }

        line_length = info->length;

        /*
         * Iterate over the rest of the export-infos, writing all the
         * export-type arrays until we reach different archs.
         */

        do {
            info++;

            /*
             * If we've written out all exports, simply end the current
             * export-type array and return.
             */

            if (info == end) {
                if (end_written_export_array(file)) {
                    return 1;
                }

                return 0;
            }

            /*
             * If the current export-info doesn't have matching archs to the
             * previous ones, end the current export-type array and break out.
             */

            const uint64_t inner_archs = info->archs;
            if (inner_archs != archs) {
                if (end_written_export_array(file)) {
                    return 1;
                }

                break;
            }

            /*
             * If the current export-info has a different type from the previous
             * ones written out, end the current-export-info array and create
             * the new one of the current export-info.
             */

            const enum tbd_export_type inner_type = info->type;
            if (inner_type != type) {
                if (end_written_export_array(file)) {
                    return 1;
                }

                if (write_export_type_key(file, inner_type, version)) {
                    return 1;
                }

                if (write_export_info(file, info)) {
                    return 1;
                }

                /*
                 * Reset the line-length after ending the previous export-type
                 * array.
                 */

                line_length = info->length;
                type = inner_type;

                continue;
            }

            /*
             * Write either a comma or a newline before writing the next export
             * to preserve a limit on line-lengths
             */

            const uint32_t length = info->length;
            const enum write_comma_result write_comma_result =
                write_comma_or_newline(file, line_length, length);

            switch (write_comma_result) {
                case E_WRITE_COMMA_OK:
                    break;

                case E_WRITE_COMMA_WRITE_FAIL:
                    return 1;

                case E_WRITE_COMMA_RESET_LINE_LENGTH:
                    line_length = 0;
                    break;
            }

            if (write_export_info(file, info)) {
                return 1;
            }

            line_length += length;
        } while (true);
    } while (info != end);

    return 0;
}
