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

int
tbd_write_archs_for_header(FILE *__notnull const file,
                           const uint64_t archs,
                           const int archs_count)
{
    if (archs_count == 0) {
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
             * If we're already at the end, simply write the end bracket for
             * the arch-info list and return.
             */

            if (fputs(" ]\n", file) < 0) {
                return 1;
            }

            return 0;
        }

        index += 1;
    } while (true);

    /*
     * After writing the first arch, write the following archs with a preceding
     * comma.
     *
     * Count the amount of archs on one line, starting off with one as we just
     * wrote one before looping over the rest.
     *
     * Break lines for every seven archs written out.
     */

    int i = 1;
    uint64_t counter = 1;

    while (i != archs_count) {
        index += 1;

        if (archs_iter & 1) {
            const struct arch_info *const arch = arch_info_list + index;
            if (fprintf(file, ", %s", arch->name) < 0) {
                return 1;
            }

            /*
             * Break lines for every seven archs written out.
             */

            i++;
            counter++;

            if (counter == 7) {
                if (fprintf(file, "\n%-19s", "") < 0) {
                    return 1;
                }

                counter = 0;
            }
        }

        archs_iter >>= 1;
    }

    /*
     * Write the end bracket for the arch-info list and return.
     */

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

static int
write_archs_for_symbol_arrays(FILE *__notnull const file,
                              const uint64_t archs,
                              const uint64_t archs_count)
{
    if (archs_count == 0) {
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
            if (fprintf(file, "  - archs:%-16s[ %s", "", arch->name) < 0) {
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
     * After writing the first arch, write the following archs with a preceding
     * comma.
     *
     * Count the amount of archs on one line, starting off with one as we just
     * wrote one before looping over the rest. When the counter reaches 7, print
     * a newline and reset the counter.
     */

    uint64_t counter = 1;
    uint64_t i = 1;

    while (i != archs_count) {
        index += 1;

        if (archs_iter & 1) {
            const struct arch_info *const arch = arch_info_list + index;
            if (fprintf(file, ", %s", arch->name) < 0) {
                return 1;
            }

            /*
             * Break lines for every seven archs written out.
             */

            i++;
            counter++;

            if (counter == 7) {
                if (fprintf(file, "\n%-28s", "") < 0) {
                    return 1;
                }

                counter = 0;
            }
        }

        archs_iter >>= 1;
    }

    /*
     * Write the end bracket for the arch-info list and return.
     */

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

static int
write_packed_version(FILE *__notnull const file, const uint32_t version) {
    /*
     * The revision for a packed-version is stored in the LSB.
     */

    const uint8_t revision = version & 0xff;

    /*
     * The minor for a packed-version is stored in the second LSB.
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
         * Write out a .0 version-component because if minor was zero, we didn't
         * write it as a component.
         */

        if (minor == 0) {
            if (fprintf(file, ".0.%" PRIu8, revision) < 0) {
                return 1;
            }
        } else {
            if (fprintf(file, ".%" PRIu8, revision) < 0) {
                return 1;
            }
        }
    }

    if (fputc('\n', file) == EOF) {
        return 1;
    }

    return 0;
}

int
tbd_write_current_version(FILE *__notnull const file, const uint32_t version) {
    if (fprintf(file, "current-version:%-7s", "") < 0) {
        return 1;
    }

    return write_packed_version(file, version);
}

int
tbd_write_compatibility_version(FILE *__notnull const file,
                                const uint32_t version)
{
    if (fputs("compatibility-version: ", file) < 0) {
        return 1;
    }

    return write_packed_version(file, version);
}

int tbd_write_footer(FILE *__notnull const file) {
    return (fputs("...\n", file) < 0);
}

int tbd_write_flags(FILE *__notnull const file, const uint64_t flags) {
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
write_yaml_string(FILE *__notnull const file,
                  const char *__notnull const string,
                  const uint64_t length,
                  const bool needs_quotes)
{
    if (needs_quotes) {
        if (fprintf(file, "\"%s\"", string) < 0) {
            return 1;
        }
    } else {
        if (fwrite(string, length, 1, file) != 1) {
            return 1;
        }
    }

    return 0;
}

int
tbd_write_install_name(FILE *__notnull const file,
                       const struct tbd_create_info *__notnull const info)
{
    if (fprintf(file, "install-name:%-10s", "") < 0) {
        return 1;
    }

    const char *const install_name = info->fields.install_name;
    const uint32_t length = info->fields.install_name_length;

    const bool needs_quotes =
        (info->flags & F_TBD_CREATE_INFO_INSTALL_NAME_NEEDS_QUOTES);

    if (write_yaml_string(file, install_name, length, needs_quotes)) {
        return 1;
    }

    if (fputc('\n', file) == EOF) {
        return 1;
    }

    return 0;
}

int
tbd_write_objc_constraint(FILE *__notnull const file,
                          const enum tbd_objc_constraint constraint)
{
    switch (constraint) {
        case TBD_OBJC_CONSTRAINT_NO_VALUE:
            break;

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

int
tbd_write_magic(FILE *__notnull const file, const enum tbd_version version) {
    switch (version) {
        case TBD_VERSION_NONE:
            return 1;

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
tbd_write_parent_umbrella(FILE *__notnull const file,
                          const struct tbd_create_info *__notnull const info)
{
    const char *const umbrella = info->fields.parent_umbrella;
    if (umbrella == NULL) {
        return 0;
    }

    if (fprintf(file, "parent-umbrella:%-7s", "") < 0) {
        return 1;
    }

    const uint32_t length = info->fields.parent_umbrella_length;
    const bool needs_quotes =
        (info->flags & F_TBD_CREATE_INFO_PARENT_UMBRELLA_NEEDS_QUOTES);

    if (write_yaml_string(file, umbrella, length, needs_quotes)) {
        return 1;
    }

    if (fputc('\n', file) == EOF) {
        return 1;
    }

    return 0;
}

int
tbd_write_platform(FILE *__notnull const file, const enum tbd_platform platform)
{
    switch (platform) {
        case TBD_PLATFORM_NONE:
            return 1;

        case TBD_PLATFORM_MACOS:
            if (fprintf(file, "platform:%-14smacosx\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_IOS:
        case TBD_PLATFORM_IOS_SIMULATOR:
            if (fprintf(file, "platform:%-14sios\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_TVOS:
        case TBD_PLATFORM_TVOS_SIMULATOR:
            if (fprintf(file, "platform:%-14stvos\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_WATCHOS:
        case TBD_PLATFORM_WATCHOS_SIMULATOR:
            if (fprintf(file, "platform:%-14swatchos\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_BRIDGEOS:
            if (fprintf(file, "platform:%-14sbridgeos\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_MACCATALYST:
            if (fprintf(file, "platform:%-14siosmac\n", "") < 0) {
                return 1;
            }

            break;

        case TBD_PLATFORM_ZIPPERED:
            if (fprintf(file, "platform:%-14szippered\n", "") < 0) {
                return 1;
            }

            break;
    }

    return 0;
}

int
tbd_write_swift_version(FILE *__notnull const file,
                        const enum tbd_version tbd_version,
                        const uint32_t swift_version)
{
    if (swift_version == 0) {
        return 0;
    }

    switch (tbd_version) {
        case TBD_VERSION_NONE:
            return 1;

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
write_uuid(FILE *__notnull const file,
           const struct arch_info *__notnull const arch,
           const uint8_t *__notnull const uuid,
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

int
tbd_write_uuids(FILE *__notnull const file,
                const struct array *__notnull const uuids)
{
    if (uuids->item_count == 0) {
        return 0;
    }

    if (fprintf(file, "uuids:%-17s[ ", "") < 0) {
        return 1;
    }

    const struct tbd_uuid_info *uuid = uuids->data;
    const struct tbd_uuid_info *const end = uuids->data_end;

    if (write_uuid(file, uuid->arch, uuid->uuid, false)) {
        return 1;
    }

    /*
     * Keep a counter of the uuids written out, to limit line-lengths.
     *
     * Start off with a counter of 1 since we write one uuid-pair first before
     * looping over the rest.
     *
     * When the counter reaches 2, print a newline and reset the counter.
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
write_symbol_type_key(FILE *__notnull const file,
                      const enum tbd_symbol_type type,
                      const enum tbd_version version,
                      const bool is_export)
{
    switch (type) {
        case TBD_SYMBOL_TYPE_NONE:
            return 1;

        case TBD_SYMBOL_TYPE_CLIENT: {
            if (version == TBD_VERSION_V1) {
                if (fprintf(file, "%-4sallowed-clients:%-6s[ ", "", "") < 0) {
                    return 1;
                }
            } else {
                if (fprintf(file, "%-4sallowable-clients:%-4s[ ", "", "") < 0) {
                    return 1;
                }
            }

            break;
        }

        case TBD_SYMBOL_TYPE_REEXPORT:
            if (fprintf(file, "%-4sre-exports:%11s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_SYMBOL_TYPE_NORMAL:
            if (fprintf(file, "%-4ssymbols:%14s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_SYMBOL_TYPE_OBJC_CLASS:
            if (fprintf(file, "%-4sobjc-classes:%9s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_SYMBOL_TYPE_OBJC_EHTYPE:
            if (fprintf(file, "%-4sobjc-eh-types:%8s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_SYMBOL_TYPE_OBJC_IVAR:
            if (fprintf(file, "%-4sobjc-ivars:%11s[ ", "", "") < 0) {
                return 1;
            }

            break;

        case TBD_SYMBOL_TYPE_WEAK_DEF:
            if (is_export) {
                if (fprintf(file, "%-4sweak-def-symbols:%5s[ ", "", "") < 0) {
                    return 1;
                }
            } else {
                if (fprintf(file, "%-4sweak-ref-symbols:%5s[ ", "", "") < 0) {
                    return 1;
                }
            }

            break;

        case TBD_SYMBOL_TYPE_THREAD_LOCAL:
            if (!is_export) {
                return 1;
            }

            if (fprintf(file, "%-4sthread-local-symbols: [ ", "") < 0) {
                return 1;
            }

            break;
    }

    return 0;
}

static inline int end_written_symbol_array(FILE *__notnull const file) {
    static const char *const end = " ]\n";
    if (fwrite(end, 3, 1, file) != 1) {
        return 1;
    }

    return 0;
}

static uint32_t line_length_initial = 28;
static uint32_t line_length_max = 80;

/*
 * Write either a comma or a newline depending on either the current or new
 * line length, or the string-length.
 *
 * Requires:
 *     line_length > line_length_initial
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
write_comma_or_newline(FILE *__notnull const file,
                       const uint64_t line_length,
                       const uint64_t string_length)
{
    /*
     * If the string is as long, or longer than the max-line limit, bend the
     * rules slightly to allow the symbol to fit, albeit on a seperate line.
     */

    const uint64_t max_string_length = line_length_max - line_length_initial;
    if (string_length >= max_string_length) {
        if (fprintf(file, ",\n%-28s", "") < 0) {
            return E_WRITE_COMMA_WRITE_FAIL;
        }

        return E_WRITE_COMMA_RESET_LINE_LENGTH;
    }

    /*
     * If writing the symbol (and it's corresponding comma + space) gets the
     * current line to exceed the line-length limit, write a comma+newline to
     * have the symbol written on the next line.
     */

    const uint64_t new_line_length = line_length + string_length + 2;
    if (new_line_length > line_length_max) {
        if (fprintf(file, ",\n%-28s", "") < 0) {
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
write_symbol_info(FILE *__notnull const file,
                  const struct tbd_symbol_info *__notnull const info)
{
    const bool needs_quotes =
        (info->flags & F_TBD_SYMBOL_INFO_STRING_NEEDS_QUOTES);

    return write_yaml_string(file, info->string, info->length, needs_quotes);
}

int
tbd_write_exports(FILE *__notnull const file,
                  const struct array *__notnull const exports,
                  const enum tbd_version version)
{
    const struct tbd_symbol_info *info = exports->data;
    const struct tbd_symbol_info *const end = exports->data_end;

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

    uint64_t line_length = 0;

    do {
        const uint64_t archs = info->archs;
        const uint64_t archs_count = info->archs_count;

        if (write_archs_for_symbol_arrays(file, archs, archs_count)) {
            return 1;
        }

        enum tbd_symbol_type type = info->type;
        if (write_symbol_type_key(file, type, version, true)) {
            return 1;
        }

        if (write_symbol_info(file, info)) {
            return 1;
        }

        line_length = line_length_initial + info->length;

        /*
         * Iterate over the rest of the symbol-infos, writing all the
         * symbol-type arrays until we reach different archs.
         */

        do {
            info++;

            /*
             * If we've written out all exports, simply end the current
             * symbol-type array and return.
             */

            if (info == end) {
                if (end_written_symbol_array(file)) {
                    return 1;
                }

                return 0;
            }

            /*
             * If the current symbol-info doesn't have matching archs to the
             * previous ones, end the current symbol-type array and break out.
             */

            const uint64_t inner_archs = info->archs;
            if (inner_archs != archs) {
                if (end_written_symbol_array(file)) {
                    return 1;
                }

                break;
            }

            /*
             * If the current symbol-info has a different type from the previous
             * ones written out, end the current-symbol-info array and create
             * the new one of the current symbol-info.
             */

            const enum tbd_symbol_type inner_type = info->type;
            if (inner_type != type) {
                if (end_written_symbol_array(file)) {
                    return 1;
                }

                if (write_symbol_type_key(file, inner_type, version, true)) {
                    return 1;
                }

                if (write_symbol_info(file, info)) {
                    return 1;
                }

                /*
                 * Reset the line-length after ending the previous symbol-type
                 * array.
                 */

                line_length = line_length_initial + info->length;
                type = inner_type;

                continue;
            }

            /*
             * Write either a comma or a newline before writing the next export
             * to preserve a limit on line-lengths.
             */

            const uint64_t length = info->length;
            const enum write_comma_result write_comma_result =
                write_comma_or_newline(file, line_length, length);

            switch (write_comma_result) {
                case E_WRITE_COMMA_OK:
                    break;

                case E_WRITE_COMMA_WRITE_FAIL:
                    return 1;

                case E_WRITE_COMMA_RESET_LINE_LENGTH:
                    line_length = line_length_initial;
                    break;
            }

            if (write_symbol_info(file, info)) {
                return 1;
            }

            line_length += length;
        } while (true);
    } while (info != end);

    return 0;
}

int
tbd_write_exports_with_full_archs(
    const struct tbd_create_info *__notnull const info,
    const struct array *__notnull const exports,
    FILE *__notnull const file)
{
    const struct tbd_symbol_info *export = exports->data;
    const struct tbd_symbol_info *const end = exports->data_end;

    if (export == end) {
        return 0;
    }

    if (fputs("exports:\n", file) < 0) {
        return 1;
    }

    const uint64_t archs = export->archs;
    const uint64_t archs_count = export->archs_count;

    if (write_archs_for_symbol_arrays(file, archs, archs_count)) {
        return 1;
    }

    const enum tbd_version version = info->version;
    enum tbd_symbol_type type = export->type;

    if (write_symbol_type_key(file, type, version, true)) {
        return 1;
    }

    if (write_symbol_info(file, export)) {
        return 1;
    }

    uint64_t line_length = line_length_initial + export->length;

    do {
        export++;

        /*
         * If we've written out all exports, simply end the current symbol-type
         * array and return.
         */

        if (export == end) {
            if (end_written_symbol_array(file)) {
                return 1;
            }

            return 0;
        }

        /*
         * If the current symbol-info has a different type from the previous
         * ones written out, end the current-symbol-info array and create the
         * new one of the current symbol-info.
         */

        const enum tbd_symbol_type inner_type = export->type;
        if (inner_type != type) {
            if (end_written_symbol_array(file)) {
                return 1;
            }

            if (write_symbol_type_key(file, inner_type, version, true)) {
                return 1;
            }

            if (write_symbol_info(file, export)) {
                return 1;
            }

            /*
             * Reset the line-length after ending the previous symbol-type
             * array.
             */

            line_length = line_length_initial + export->length;
            type = inner_type;

            continue;
        }

        /*
         * Write either a comma or a newline before writing the next export to
         * preserve a limit on line-lengths.
         */

        const uint64_t length = export->length;
        const enum write_comma_result write_comma_result =
            write_comma_or_newline(file, line_length, length);

        switch (write_comma_result) {
            case E_WRITE_COMMA_OK:
                break;

            case E_WRITE_COMMA_WRITE_FAIL:
                return 1;

            case E_WRITE_COMMA_RESET_LINE_LENGTH:
                line_length = line_length_initial;
                break;
        }

        if (write_symbol_info(file, export)) {
            return 1;
        }

        line_length += length;
    } while (true);

    return 0;
}

int
tbd_write_undefineds(FILE *__notnull const file,
                     const struct array *__notnull const undefineds,
                     const enum tbd_version version)
{
    const struct tbd_symbol_info *info = undefineds->data;
    const struct tbd_symbol_info *const end = undefineds->data_end;

    if (info == end) {
        return 0;
    }

    if (fputs("undefineds:\n", file) < 0) {
        return 1;
    }

    /*
     * Store the length of the current-line written out (excluding the
     * beginning spaces).
     */

    uint64_t line_length = 0;

    do {
        const uint64_t archs = info->archs;
        const uint64_t archs_count = info->archs_count;

        if (write_archs_for_symbol_arrays(file, archs, archs_count)) {
            return 1;
        }

        enum tbd_symbol_type type = info->type;
        if (write_symbol_type_key(file, type, version, false)) {
            return 1;
        }

        if (write_symbol_info(file, info)) {
            return 1;
        }

        line_length = line_length_initial + info->length;

        /*
         * Iterate over the rest of the symbol-infos, writing all the
         * symbol-type arrays until we reach different archs.
         */

        do {
            info++;

            /*
             * If we've written out all exports, simply end the current
             * symbol-type array and return.
             */

            if (info == end) {
                if (end_written_symbol_array(file)) {
                    return 1;
                }

                return 0;
            }

            /*
             * If the current symbol-info doesn't have matching archs to the
             * previous ones, end the current symbol-type array and break out.
             */

            const uint64_t inner_archs = info->archs;
            if (inner_archs != archs) {
                if (end_written_symbol_array(file)) {
                    return 1;
                }

                break;
            }

            /*
             * If the current symbol-info has a different type from the previous
             * ones written out, end the current-symbol-info array and create
             * the new one of the current symbol-info.
             */

            const enum tbd_symbol_type inner_type = info->type;
            if (inner_type != type) {
                if (end_written_symbol_array(file)) {
                    return 1;
                }

                if (write_symbol_type_key(file, inner_type, version, false)) {
                    return 1;
                }

                if (write_symbol_info(file, info)) {
                    return 1;
                }

                /*
                 * Reset the line-length after ending the previous symbol-type
                 * array.
                 */

                line_length = line_length_initial + info->length;
                type = inner_type;

                continue;
            }

            /*
             * Write either a comma or a newline before writing the next export
             * to preserve a limit on line-lengths.
             */

            const uint64_t length = info->length;
            const enum write_comma_result write_comma_result =
                write_comma_or_newline(file, line_length, length);

            switch (write_comma_result) {
                case E_WRITE_COMMA_OK:
                    break;

                case E_WRITE_COMMA_WRITE_FAIL:
                    return 1;

                case E_WRITE_COMMA_RESET_LINE_LENGTH:
                    line_length = line_length_initial;
                    break;
            }

            if (write_symbol_info(file, info)) {
                return 1;
            }

            line_length += length;
        } while (true);
    } while (info != end);

    return 0;
}

int
tbd_write_undefineds_with_full_archs(
    const struct tbd_create_info *__notnull const info,
    const struct array *__notnull const undefineds,
    FILE *__notnull const file)
{
    const struct tbd_symbol_info *undefined = undefineds->data;
    const struct tbd_symbol_info *const end = undefineds->data_end;

    if (undefined == end) {
        return 0;
    }

    if (fputs("undefineds:\n", file) < 0) {
        return 1;
    }

    const uint64_t archs = undefined->archs;
    const uint64_t archs_count = undefined->archs_count;

    if (write_archs_for_symbol_arrays(file, archs, archs_count)) {
        return 1;
    }

    const enum tbd_version version = info->version;
    enum tbd_symbol_type type = undefined->type;

    if (write_symbol_type_key(file, type, version, false)) {
        return 1;
    }

    if (write_symbol_info(file, undefined)) {
        return 1;
    }

    uint64_t line_length = line_length_initial + undefined->length;

    do {
        undefined++;

        /*
         * If we've written out all exports, simply end the current symbol-type
         * array and return.
         */

        if (undefined == end) {
            if (end_written_symbol_array(file)) {
                return 1;
            }

            return 0;
        }

        /*
         * If the current symbol-info has a different type from the previous
         * ones written out, end the current-symbol-info array and create the
         * new one of the current symbol-info.
         */

        const enum tbd_symbol_type inner_type = undefined->type;
        if (inner_type != type) {
            if (end_written_symbol_array(file)) {
                return 1;
            }

            if (write_symbol_type_key(file, inner_type, version, false)) {
                return 1;
            }

            if (write_symbol_info(file, undefined)) {
                return 1;
            }

            /*
             * Reset the line-length after ending the previous symbol-type
             * array.
             */

            line_length = line_length_initial + undefined->length;
            type = inner_type;

            continue;
        }

        /*
         * Write either a comma or a newline before writing the next export to
         * preserve a limit on line-lengths.
         */

        const uint64_t length = undefined->length;
        const enum write_comma_result write_comma_result =
            write_comma_or_newline(file, line_length, length);

        switch (write_comma_result) {
            case E_WRITE_COMMA_OK:
                break;

            case E_WRITE_COMMA_WRITE_FAIL:
                return 1;

            case E_WRITE_COMMA_RESET_LINE_LENGTH:
                line_length = line_length_initial;
                break;
        }

        if (write_symbol_info(file, undefined)) {
            return 1;
        }

        line_length += length;
    } while (true);

    return 0;
}
