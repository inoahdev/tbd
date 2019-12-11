//
//  src/tbd_write.c
//  tbd
//
//  Created by inoahdev on 11/23/18.
//  Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <inttypes.h>
#include "tbd_write.h"

static const uint64_t MAX_ARCH_ON_LINE = 7;
static const uint64_t MAX_TARGET_ON_LINE = 5;

int
tbd_write_archs_for_header(FILE *__notnull const file,
                           const struct target_list list)
{
    if (list.set_count == 0) {
        return 1;
    }

    const struct arch_info *arch = NULL;
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    target_list_get_target(&list, 0, &arch, &platform);
    if (fprintf(file, "archs:%-17s[ %s", "", arch->name) < 0) {
        return 1;
    }

    int counter = 1;
    for (int i = 1; i != list.set_count; i++) {
        target_list_get_target(&list, i, &arch, &platform);

        /*
         * Go to the next line after having printed two targets.
         */

        const bool write_comma = (counter != 0);
        if (write_comma) {
            if (fputs(", ", file) < 0) {
                return 1;
            }
        }

        if (fwrite(arch->name, arch->name_length, 1, file) != 1) {
            return 1;
        }

        if (counter == MAX_ARCH_ON_LINE && i != (list.set_count - 1)) {
            if (fprintf(file, ",\n%-28s", "") < 0) {
                return 1;
            }

            counter = 0;
        } else {
            counter++;
        }
    }

    /*
     * Write the end bracket for the arch-info list and return.
     */

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

static inline int
write_target(FILE *__notnull const file,
             const struct arch_info *__notnull const arch,
             const enum tbd_platform platform,
             const enum tbd_version version,
             const bool has_comma)
{
    int ret = 0;
    if (has_comma) {
        ret =
            fprintf(file,
                    ", %s-%s",
                    arch->name,
                    tbd_platform_to_string(platform, version));
    } else {
        ret =
            fprintf(file,
                    "%s-%s",
                    arch->name,
                    tbd_platform_to_string(platform, version));
    }

    if (ret < 0) {
        return 1;
    }

    return 0;
}

int
tbd_write_targets_for_header(FILE *__notnull const file,
                             const struct target_list list,
                             const enum tbd_version version)
{
    if (list.set_count == 0) {
        return 1;
    }

    if (fprintf(file, "targets:%-15s[ ", "") < 0) {
        return 1;
    }

    const struct arch_info *arch = NULL;
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    target_list_get_target(&list, 0, &arch, &platform);
    if (write_target(file, arch, platform, version, false) < 0) {
        return 1;
    }

    int counter = 1;
    for (int i = 1; i != list.set_count; i++) {
        target_list_get_target(&list, i, &arch, &platform);

        /*
         * Go to the next line after having printed two targets.
         */

        const bool write_comma = (counter != 0);
        if (write_target(file, arch, platform, version, write_comma) < 0) {
            return 1;
        }

        if (counter == MAX_TARGET_ON_LINE && i != (list.set_count - 1)) {
            if (fprintf(file, ",\n%-28s", "") < 0) {
                return 1;
            }

            counter = 0;
        } else {
            counter++;
        }
    }

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

static int
write_archs_for_symbol_arrays(FILE *__notnull const file,
                              const struct target_list list,
                              const struct bit_list bits)
{
    if (bits.set_count == 0) {
        return 1;
    }

    const struct arch_info *arch = NULL;
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    uint64_t first = bit_list_find_first_bit(bits);
    target_list_get_target(&list, first, &arch, &platform);

    if (fprintf(file, "  - archs:%-16s[ %s", "", arch->name) < 0) {
        return 1;
    }

    int counter = 1;
    for (int i = 1; i != bits.set_count; i++) {
        first = bit_list_find_bit_after_last(bits, first);
        target_list_get_target(&list, i, &arch, &platform);

        /*
         * Go to the next line after having printed two targets.
         */

        const bool write_comma = (counter != 0);
        if (write_comma) {
            if (fputs(", ", file) < 0) {
                return 1;
            }
        }

        if (fwrite(arch->name, arch->name_length, 1, file) != 1) {
            return 1;
        }

        if (counter == MAX_ARCH_ON_LINE && i != (bits.set_count - 1)) {
            if (fprintf(file, ",\n%-28s", "") < 0) {
                return 1;
            }

            counter = 0;
        } else {
            counter++;
        }
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
write_targets_as_dict_key(FILE *__notnull const file,
                          const struct target_list list,
                          const struct bit_list bits,
                          const enum tbd_version version)
{
    if (bits.set_count == 0) {
        return 1;
    }

    if (fprintf(file, "  - targets:%-14s[ ", "") < 0) {
        return 1;
    }

    const struct arch_info *arch = NULL;
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    uint64_t first = bit_list_find_first_bit(bits);
    target_list_get_target(&list, first, &arch, &platform);

    if (write_target(file, arch, platform, version, false) < 0) {
        return 1;
    }

    int counter = 1;
    for (int i = 1; i != bits.set_count; i++) {
        first = bit_list_find_bit_after_last(bits, first);
        target_list_get_target(&list, i, &arch, &platform);

        /*
         * Go to the next line after having printed two targets.
         */

        const bool write_comma = (counter != 0);
        if (write_target(file, arch, platform, version, write_comma) < 0) {
            return 1;
        }

        if (counter == MAX_TARGET_ON_LINE && i != (bits.set_count != 1)) {
            if (fprintf(file, ",\n%-28s", "") < 0) {
                return 1;
            }

            counter = 0;
        } else {
            counter++;
        }
    }

    /*
     * Write the end bracket for the target-list and return.
     */

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

static
int write_packed_version(FILE *__notnull const file, const uint32_t version) {
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
    if (fputs("...\n", file) < 0) {
        return 1;
    }

    return 0;
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
    const uint64_t length = info->fields.install_name_length;

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

        case TBD_VERSION_V4:
            if (fprintf(file, "--- !tapi-tbd\ntbd-version:%-11s4\n", "") < 0) {
                return 1;
            }

            break;
    }

    return 0;
}

int
tbd_write_parent_umbrella_for_archs(
    FILE *__notnull const file,
    const struct tbd_create_info *__notnull const info)
{
    if (info->fields.metadata.item_count == 0) {
        return 0;
    }

    const struct tbd_metadata_info *const umbrella_info =
        (const struct tbd_metadata_info *)info->fields.metadata.data;

    if (umbrella_info->type != TBD_METADATA_TYPE_PARENT_UMBRELLA) {
        return 0;
    }

    const char *const umbrella = umbrella_info->string;
    if (umbrella == NULL) {
        return 1;
    }

    if (fprintf(file, "parent-umbrella:%-7s", "") < 0) {
        return 1;
    }

    const uint64_t length = umbrella_info->length;
    const bool needs_quotes =
        (umbrella_info->flags & F_TBD_DATA_INFO_STRING_NEEDS_QUOTES);

    if (write_yaml_string(file, umbrella, length, needs_quotes)) {
        return 1;
    }

    if (fputc('\n', file) == EOF) {
        return 1;
    }

    return 0;
}

int
tbd_write_platform(FILE *__notnull const file,
                   const struct tbd_create_info *__notnull const info,
                   const enum tbd_version version)
{
    const struct arch_info *arch = NULL;
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    target_list_get_target(&info->fields.targets, 0, &arch, &platform);

    const char *const platform_str = tbd_platform_to_string(platform, version);
    if (fprintf(file, "platform:%-14s%s\n", "", platform_str) < 0) {
        return 1;
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
        case TBD_VERSION_V4:
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
write_uuid(FILE *__notnull const file, const uint8_t *__notnull const uuid) {
    const int ret =
        fprintf(file,
                "'%.2X%.2X%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X%.2X-%.2X"
                "%.2X%.2X%.2X%.2X%.2X'",
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

    if (ret < 0) {
        return 1;
    }

    return 0;
}

static inline int
write_single_uuid_for_archs(FILE *__notnull const file,
                            const uint64_t target,
                            const uint8_t *__notnull const uuid,
                            const bool has_comma)
{
    const struct arch_info *const arch =
        (const struct arch_info *)(target & TARGET_ARCH_INFO_MASK);

    int ret = 0;
    if (has_comma) {
        ret = fprintf(file, ", '%s: ", arch->name);
    } else {
        ret = fprintf(file, "'%s: ", arch->name);
    }

    if (ret < 0) {
        return 1;
    }

    if (write_uuid(file, uuid)) {
        return 1;
    }

    return 0;
}

int
tbd_write_uuids_for_archs(FILE *__notnull const file,
                          const struct array *__notnull const uuids)
{
    if (uuids->item_count == 0) {
        return 0;
    }

    if (fprintf(file, "uuids:%-17s[ ", "") < 0) {
        return 1;
    }

    const struct tbd_uuid_info *info = uuids->data;
    const struct tbd_uuid_info *const end = uuids->data_end;

    if (write_single_uuid_for_archs(file, info->target, info->uuid, false)) {
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
        info = info + 1;
        if (info == end) {
            break;
        }

        const uint64_t target = info->target;
        const uint8_t *const uuid = info->uuid;

        if (write_single_uuid_for_archs(file, target, uuid, needs_comma)) {
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

static inline int
write_uuid_with_target(FILE *__notnull const file,
                       const uint64_t target,
                       const uint8_t *__notnull const uuid,
                       const enum tbd_version version)
{
    const struct arch_info *const arch =
        (const struct arch_info *)(target & TARGET_ARCH_INFO_MASK);

    const enum tbd_platform platform =
        (const enum tbd_platform)(target & TARGET_PLATFORM_MASK);

    const char *const platform_str = tbd_platform_to_string(platform, version);
    if (fprintf(file,
                "  - target: %s-%s\n%-4svalue: ",
                arch->name,
                platform_str,
                "") < 0)
    {
        return 1;
    }

    if (write_uuid(file, uuid)) {
        return 1;
    }

    if (fputc('\n', file) < 0) {
        return 1;
    }

    return 0;
}

int
tbd_write_uuids_for_targets(FILE *__notnull const file,
                            const struct array *__notnull const uuids,
                            const enum tbd_version version)
{
    if (uuids->item_count == 0) {
        return 0;
    }

    if (fputs("uuids:\n", file) < 0) {
        return 1;
    }

    const struct tbd_uuid_info *uuid = uuids->data;
    const struct tbd_uuid_info *const end = uuids->data_end;

    for (; uuid != end; uuid++) {
        const uint64_t target = uuid->target;
        if (write_uuid_with_target(file, target, uuid->uuid, version)) {
            return 1;
        }
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
 *     string_length
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
     * rules slightly to allow the string to fit, albeit on a seperate line.
     */

    const uint64_t max_string_length = line_length_max - line_length_initial;
    if (string_length >= max_string_length) {
        if (fprintf(file, ",\n%-28s", "") < 0) {
            return E_WRITE_COMMA_WRITE_FAIL;
        }

        return E_WRITE_COMMA_RESET_LINE_LENGTH;
    }

    /*
     * If writing the string (and it's corresponding comma + space) gets the
     * current line to exceed the line-length limit, write a comma+newline to
     * have the string written on the next line.
     */

    const uint64_t new_line_length = line_length + string_length + 2;
    if (new_line_length > line_length_max) {
        if (fprintf(file, ",\n%-28s", "") < 0) {
            return E_WRITE_COMMA_WRITE_FAIL;
        }

        return E_WRITE_COMMA_RESET_LINE_LENGTH;
    }

    /*
     * Simply have the last string written out retrofitted with a comma+space,
     * before writing the next string.
     */

    const char *const comma_space = ", ";
    if (fwrite(comma_space, 2, 1, file) != 1) {
        return E_WRITE_COMMA_WRITE_FAIL;
    }

    return E_WRITE_COMMA_OK;
}

static int
write_metadata_type(FILE *__notnull const file,
                    const enum tbd_metadata_type type)
{
    switch (type) {
        case TBD_METADATA_TYPE_NONE:
            return 1;

        case TBD_METADATA_TYPE_PARENT_UMBRELLA:
            if (fputs("parent-umbrella:\n", file) < 0) {
                return 1;
            }

            break;

        case TBD_METADATA_TYPE_CLIENT:
            if (fputs("allowable-clients:\n", file) < 0) {
                return 1;
            }

            break;

        case TBD_METADATA_TYPE_REEXPORTED_LIBRARY:
            if (fputs("reexported-libraries:\n", file) < 0) {
                return 1;
            }

            break;
    }

    return 0;
}

static inline int end_written_sequence(FILE *__notnull const file) {
    static const char *const end = " ]\n";
    if (fwrite(end, 3, 1, file) != 1) {
        return 1;
    }

    return 0;
}

static int
skip_metadata_if_needed(const struct tbd_metadata_info *__notnull info,
                        const struct tbd_metadata_info *__notnull const end,
                        const uint64_t create_options,
                        const struct tbd_metadata_info **__notnull info_out)
{
    for (; info != end; info++) {
        switch (info->type) {
            case TBD_METADATA_TYPE_NONE:
                return 0;

            case TBD_METADATA_TYPE_PARENT_UMBRELLA:
                if (create_options & O_TBD_CREATE_IGNORE_PARENT_UMBRELLAS) {
                    break;
                }

                *info_out = info;
                return 0;

            case TBD_METADATA_TYPE_CLIENT:
                if (create_options & O_TBD_CREATE_IGNORE_CLIENTS) {
                    break;
                }

                *info_out = info;
                return 0;

            case TBD_METADATA_TYPE_REEXPORTED_LIBRARY:
                if (create_options & O_TBD_CREATE_IGNORE_REEXPORTS) {
                    break;
                }

                *info_out = info;
                return 0;
        }
    }

    return 1;
}

static inline int
write_metadata_info(FILE *__notnull const file,
                    const struct tbd_metadata_info *__notnull const info)
{
    const bool needs_quotes =
        (info->flags & F_TBD_DATA_INFO_STRING_NEEDS_QUOTES);

    return write_yaml_string(file, info->string, info->length, needs_quotes);
}

static int
write_umbrella_list(FILE *__notnull const file,
                    const struct tbd_create_info *__notnull const info,
                    const struct tbd_metadata_info *__notnull m_info,
                    const struct tbd_metadata_info *__notnull const end,
                    const struct tbd_metadata_info **__notnull const m_info_out)
{
    const struct target_list targets = info->fields.targets;
    const enum tbd_version version = info->version;

    do {
        if (write_targets_as_dict_key(file, targets, m_info->targets, version)) {
            return 1;
        }

        if (fprintf(file, "%-4sumbrella:%-15s", "", "") < 0) {
            return 1;
        }

        if (write_metadata_info(file, m_info)) {
            return 1;
        }

        if (fputc('\n', file) < 0) {
            return 1;
        }

        m_info++;
        if (m_info == end) {
            return 0;
        }

        if (m_info->type != TBD_METADATA_TYPE_PARENT_UMBRELLA) {
            *m_info_out = m_info;
            return 2;
        }
    } while (true);

    return 0;
}

int
tbd_write_metadata(FILE *__notnull const file,
                   const struct tbd_create_info *__notnull const info_in,
                   const uint64_t create_options)
{
    const struct array *const metadata = &info_in->fields.metadata;
    const struct target_list targets = info_in->fields.targets;

    const struct tbd_metadata_info *info = metadata->data;
    const struct tbd_metadata_info *const end = metadata->data_end;

    if (info == end) {
        return 0;
    }

    const enum tbd_version version = info_in->version;
    enum tbd_metadata_type type = TBD_METADATA_TYPE_NONE;

    do {
    meta:
        if (skip_metadata_if_needed(info, end, create_options, &info)) {
            return 1;
        }

        type = info->type;
        if (write_metadata_type(file, type)) {
            return 1;
        }

        switch (type) {
            case TBD_METADATA_TYPE_NONE:
                return 1;

            case TBD_METADATA_TYPE_PARENT_UMBRELLA: {
                const int result =
                    write_umbrella_list(file, info_in, info, end, &info);

                if (result != 2) {
                    return result;
                }

                type = info->type;
                if (write_metadata_type(file, type)) {
                    return 1;
                }

                break;
            }

            case TBD_METADATA_TYPE_CLIENT:
            case TBD_METADATA_TYPE_REEXPORTED_LIBRARY:
                break;
        }

        struct bit_list bits = info->targets;
        uint64_t line_length = 0;

        do {
            if (write_targets_as_dict_key(file, targets, bits, version)) {
                return 1;
            }

            if (fprintf(file, "%-4slibraries:%-12s[ ", "", "") < 0) {
                return 1;
            }

            if (write_metadata_info(file, info)) {
                return 1;
            }

            line_length = line_length_initial + info->length;
            do {
                info++;
                if (info == end) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    return 0;
                }

                const enum tbd_metadata_type inner_type = info->type;
                if (inner_type != type) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    goto meta;
                }

                if (!bit_list_equal_counts_is_equal(bits, bits)) {
                    bits = info->targets;
                    break;
                }

                /*
                 * Write either a comma or a newline before writing the next
                 * export to preserve a limit on line-lengths.
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

                if (write_metadata_info(file, info)) {
                    return 1;
                }

                line_length += length;
            } while (true);
        } while (true);
    } while (true);

    return 0;
}

static int
write_full_targets(FILE *__notnull file,
                   const struct target_list list,
                   const enum tbd_version version)
{
    if (list.set_count == 0) {
        return 1;
    }

    if (fprintf(file, "  - targets:%-14s[ ", "") < 0) {
        return 1;
    }

    const struct arch_info *arch = NULL;
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    target_list_get_target(&list, 0, &arch, &platform);
    if (write_target(file, arch, platform, version, false) < 0) {
        return 1;
    }

    int counter = 1;
    for (int i = 1; i != list.set_count; i++) {
        target_list_get_target(&list, i, &arch, &platform);

        /*
         * Go to the next line after having printed two targets.
         */

        const bool write_comma = (counter != 0);
        if (write_target(file, arch, platform, version, write_comma) < 0) {
            return 1;
        }

        if (counter == MAX_TARGET_ON_LINE && (i != list.set_count - 1)) {
            if (fprintf(file, ",\n%-11s", "") < 0) {
                return 1;
            }

            counter = 0;
        } else {
            counter++;
        }
    }

    /*
     * Write the end bracket for the target-list and return.
     */

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}


static int
write_umbrella_list_with_full_targets(
    FILE *__notnull const file,
    const struct tbd_create_info *__notnull const info,
    const struct tbd_metadata_info *__notnull m_info,
    const struct tbd_metadata_info *__notnull const end,
    const struct tbd_metadata_info **__notnull const m_info_out)
{
    const struct target_list targets = info->fields.targets;
    const enum tbd_version version = info->version;

    do {
        if (write_full_targets(file, targets, version)) {
            return 1;
        }

        if (fprintf(file, "%-4sumbrella:%-15s", "", "") < 0) {
            return 1;
        }

        if (write_metadata_info(file, m_info)) {
            return 1;
        }

        if (fputc('\n', file) < 0) {
            return 1;
        }

        m_info++;
        if (m_info == end) {
            return 0;
        }

        if (m_info->type != TBD_METADATA_TYPE_PARENT_UMBRELLA) {
            *m_info_out = m_info;
            return 2;
        }
    } while (true);

    return 0;
}

int
tbd_write_metadata_with_full_targets(
    FILE *__notnull const file,
    const struct tbd_create_info *__notnull const info_in,
    const uint64_t create_options)
{
    const struct array *const metadata = &info_in->fields.metadata;
    const struct target_list targets = info_in->fields.targets;

    const struct tbd_metadata_info *info = metadata->data;
    const struct tbd_metadata_info *const end = metadata->data_end;

    if (info == end) {
        return 0;
    }

    const enum tbd_version version = info_in->version;
    enum tbd_metadata_type type = TBD_METADATA_TYPE_NONE;

    do {
        if (skip_metadata_if_needed(info, end, create_options, &info)) {
            return 1;
        }

        type = info->type;
        if (write_metadata_type(file, type)) {
            return 1;
        }

        switch (type) {
            case TBD_METADATA_TYPE_NONE:
                return 1;

            case TBD_METADATA_TYPE_PARENT_UMBRELLA: {
                const int result =
                    write_umbrella_list_with_full_targets(file,
                                                          info_in,
                                                          info,
                                                          end,
                                                          &info);

                if (result != 2) {
                    return result;
                }

                type = info->type;
                if (write_metadata_type(file, type)) {
                    return 1;
                }

                break;
            }

            case TBD_METADATA_TYPE_CLIENT:
            case TBD_METADATA_TYPE_REEXPORTED_LIBRARY:
                break;
        }

        uint64_t line_length = 0;
        if (write_full_targets(file, targets, version)) {
            return 1;
        }

        if (fprintf(file, "%-4slibraries:%-12s[ ", "", "") < 0) {
            return 1;
        }

        if (write_metadata_info(file, info)) {
            return 1;
        }

        line_length = line_length_initial + info->length;

        do {
            info++;
            if (info == end) {
                if (end_written_sequence(file)) {
                    return 1;
                }

                return 0;
            }

            const enum tbd_metadata_type inner_type = info->type;
            if (inner_type != type) {
                if (end_written_sequence(file)) {
                    return 1;
                }

                break;
            }

            /*
             * Write either a comma or a newline before writing the next
             * export to preserve a limit on line-lengths.
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

            if (write_metadata_info(file, info)) {
                return 1;
            }

            line_length += length;
        } while (true);
    } while (true);

    return 0;
}

static int
write_symbol_meta_type(FILE *__notnull const file,
                       const enum tbd_symbol_meta_type type)
{
    switch (type) {
        case TBD_SYMBOL_META_TYPE_NONE:
            return 1;

        case TBD_SYMBOL_META_TYPE_EXPORT:
            if (fputs("exports:\n", file) < 0) {
                return 1;
            }

            break;

        case TBD_SYMBOL_META_TYPE_REEXPORT:
            if (fputs("reexports:\n", file) < 0) {
                return 1;
            }

            break;

        case TBD_SYMBOL_META_TYPE_UNDEFINED:
            if (fputs("undefineds:\n", file) < 0) {
                return 1;
            }

            break;
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
            if (version != TBD_VERSION_V1) {
                if (fprintf(file, "%-4sallowable-clients:%-4s[ ", "", "") < 0) {
                    return 1;
                }
            } else {
                if (fprintf(file, "%-4sallowed-clients:%-6s[ ", "", "") < 0) {
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

static inline int
write_symbol_info(FILE *__notnull const file,
                  const struct tbd_symbol_info *__notnull const info)
{
    const bool needs_quotes =
        (info->flags & F_TBD_DATA_INFO_STRING_NEEDS_QUOTES);

    return write_yaml_string(file, info->string, info->length, needs_quotes);
}

static int
skip_syms_with_mtype(const struct tbd_symbol_info *__notnull sym,
                     const struct tbd_symbol_info *__notnull const end,
                     const enum tbd_symbol_meta_type type,
                     const uint64_t create_options,
                     const struct tbd_symbol_info **__notnull symbol_out)
{
    for (; sym != end; sym++) {
        switch (type) {
            case TBD_SYMBOL_META_TYPE_NONE:
                return 1;

            case TBD_SYMBOL_META_TYPE_EXPORT:
                if (create_options & O_TBD_CREATE_IGNORE_EXPORTS) {
                    break;
                }

                *symbol_out = sym;
                return 0;

            case TBD_SYMBOL_META_TYPE_REEXPORT:
                if (create_options & O_TBD_CREATE_IGNORE_REEXPORTS) {
                    break;
                }

                *symbol_out = sym;
                return 0;

            case TBD_SYMBOL_META_TYPE_UNDEFINED:
                if (create_options & O_TBD_CREATE_IGNORE_UNDEFINEDS) {
                    break;
                }

                *symbol_out = sym;
                return 0;
        }
    }

    return 1;
}

int
tbd_write_symbols_for_archs(FILE *__notnull const file,
                            const struct tbd_create_info *__notnull const info,
                            const uint64_t create_options)
{
    const struct array *const symbol_list = &info->fields.symbols;

    const struct tbd_symbol_info *sym = symbol_list->data;
    const struct tbd_symbol_info *const end = symbol_list->data_end;

    if (sym == end) {
        return 0;
    }

    /*
     * Store the length of the current-line written out (excluding the
     * beginning spaces).
     */

    const struct target_list targets = info->fields.targets;
    const enum tbd_version version = info->version;
    enum tbd_symbol_meta_type m_type = sym->meta_type;

    uint64_t line_length = 0;

    do {
    meta:
        if (skip_syms_with_mtype(sym, end, m_type, create_options, &sym)) {
            return 0;
        }

        m_type = sym->meta_type;
        if (write_symbol_meta_type(file, m_type)) {
            return 1;
        }

        do {
            const struct bit_list bits = sym->targets;
            if (write_archs_for_symbol_arrays(file, targets, bits)) {
                return 1;
            }

            enum tbd_symbol_type type = sym->type;
            if (write_symbol_type_key(file, type, version, true)) {
                return 1;
            }

            if (write_symbol_info(file, sym)) {
                return 1;
            }

            line_length = line_length_initial + sym->length;

            /*
             * Iterate over the rest of the sym-infos, writing all the
             * sym-type arrays until we reach different archs.
             */

            do {
                sym++;

                /*
                 * If we've written out all exports, simply end the current
                 * sym-type array and return.
                 */

                if (sym == end) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    return 0;
                }

                const enum tbd_symbol_meta_type inner_meta_type =
                    sym->meta_type;

                if (inner_meta_type != m_type) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    m_type = inner_meta_type;
                    goto meta;
                }

                /*
                 * If the current sym-info doesn't have matching archs to the
                 * previous ones, end the current sym-type array and break out.
                 */

                const struct bit_list inner_bits = sym->targets;
                const uint64_t inner_count = inner_bits.set_count;

                if (inner_count != bits.set_count) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    break;
                }

                if (!bit_list_equal_counts_is_equal(bits, inner_bits)) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    break;
                }

                /*
                 * If the current sym-info has a different type from the
                 * previous ones written out, end the current-sym-info array
                 * and create the new one of the current sym-info.
                 */

                const enum tbd_symbol_type in_type = sym->type;
                if (in_type != type) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    if (write_symbol_type_key(file, in_type, version, true)) {
                        return 1;
                    }

                    if (write_symbol_info(file, sym)) {
                        return 1;
                    }

                    /*
                     * Reset the line-length after ending the previous
                     * sym-type array.
                     */

                    line_length = line_length_initial + sym->length;
                    type = in_type;

                    continue;
                }

                /*
                 * Write either a comma or a newline before writing the next
                 * export to preserve a limit on line-lengths.
                 */

                const uint64_t length = sym->length;
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

                if (write_symbol_info(file, sym)) {
                    return 1;
                }

                line_length += length;
            } while (true);
        } while (true);
    } while (true);

    return 0;
}

int
tbd_write_symbols_for_targets(
    FILE *__notnull const file,
    const struct tbd_create_info *__notnull const info,
    const uint64_t create_options)
{
    const struct array *const symbol_list = &info->fields.symbols;

    const struct tbd_symbol_info *sym = symbol_list->data;
    const struct tbd_symbol_info *const end = symbol_list->data_end;

    if (sym == end) {
        return 0;
    }

    const struct target_list targets = info->fields.targets;
    const enum tbd_version version = info->version;

    enum tbd_symbol_meta_type m_type = sym->meta_type;
    uint64_t line_length = 0;

    do {
meta:
        if (skip_syms_with_mtype(sym, end, m_type, create_options, &sym)) {
            return 0;
        }

        m_type = sym->meta_type;
        if (write_symbol_meta_type(file, m_type)) {
            return 1;
        }

        do {
            const struct bit_list bits = sym->targets;
            if (write_targets_as_dict_key(file, targets, bits, version)) {
                return 1;
            }

            enum tbd_symbol_type type = sym->type;
            if (write_symbol_type_key(file, type, version, true)) {
                return 1;
            }

            if (write_symbol_info(file, sym)) {
                return 1;
            }

            line_length = line_length_initial + sym->length;

            /*
             * Iterate over the rest of the sym-infos, writing all the
             * sym-type arrays until we reach different archs.
             */

            do {
                sym++;

                /*
                 * If we've written out all symbols, simply end the current
                 * sym-type array and return.
                 */

                if (sym == end) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    return 0;
                }

                const enum tbd_symbol_meta_type inner_meta_type =
                    sym->meta_type;

                if (inner_meta_type != m_type) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    m_type = inner_meta_type;
                    goto meta;
                }

                /*
                 * If the current sym-info doesn't have matching archs to the
                 * previous ones, end the current sym-type array and break out.
                 */

                const struct bit_list inner_bits = sym->targets;
                const uint64_t inner_count = inner_bits.set_count;

                if (inner_count != bits.set_count) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    break;
                }

                if (!bit_list_equal_counts_is_equal(bits, inner_bits)) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    break;
                }

                /*
                 * If the current sym-info has a different type from the
                 * previous ones written out, end the current-sym-info array
                 * and create the new one of the current sym-info.
                 */

                const enum tbd_symbol_type in_type = sym->type;
                if (in_type != type) {
                    if (end_written_sequence(file)) {
                        return 1;
                    }

                    if (write_symbol_type_key(file, in_type, version, true)) {
                        return 1;
                    }

                    if (write_symbol_info(file, sym)) {
                        return 1;
                    }

                    /*
                     * Reset the line-length after ending the previous
                     * sym-type array.
                     */

                    line_length = line_length_initial + sym->length;
                    type = in_type;

                    continue;
                }

                /*
                 * Write either a comma or a newline before writing the next
                 * sym to preserve a limit on line-lengths.
                 */

                const uint64_t length = sym->length;
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

                if (write_symbol_info(file, sym)) {
                    return 1;
                }

                line_length += length;
            } while (true);
        } while (true);
    } while (true);

    return 0;
}

static
int write_full_archs(FILE *__notnull file, const struct target_list list) {
    if (list.set_count == 0) {
        return 1;
    }

    const struct arch_info *arch = NULL;
    enum tbd_platform platform = TBD_PLATFORM_NONE;

    target_list_get_target(&list, 0, &arch, &platform);

    if (fprintf(file, "  - targets:%-14s[ %s", "", arch->name) < 0) {
        return 1;
    }

    int counter = 1;
    for (int i = 1; i != list.set_count; i++) {
        target_list_get_target(&list, i, &arch, &platform);

        /*
         * Go to the next line after having printed two targets.
         */

        const bool write_comma = (counter != 0);
        if (write_comma) {
            if (fputs(", ", stdout) < 0) {
                return 1;
            }
        }

        if (fwrite(arch->name, arch->name_length, 1, file) < 0) {
            return 1;
        }

        if (counter == MAX_TARGET_ON_LINE && (i != list.set_count - 1)) {
            if (fprintf(file, ",\n%-11s", "") < 0) {
                return 1;
            }

            counter = 0;
        } else {
            counter++;
        }
    }

    /*
     * Write the end bracket for the target-list and return.
     */

    if (fputs(" ]\n", file) < 0) {
        return 1;
    }

    return 0;
}

int
tbd_write_symbols_with_full_archs(
    FILE *__notnull const file,
    const struct tbd_create_info *__notnull const info,
    const uint64_t create_options)
{
    const struct array *const symbol_list = &info->fields.symbols;

    const struct tbd_symbol_info *sym = symbol_list->data;
    const struct tbd_symbol_info *const end = symbol_list->data_end;

    if (sym == end) {
        return 0;
    }

    const struct target_list targets = info->fields.targets;
    enum tbd_symbol_meta_type m_type = sym->meta_type;

    do {
    meta:
        if (skip_syms_with_mtype(sym, end, m_type, create_options, &sym)) {
            return 0;
        }

        m_type = sym->meta_type;
        if (write_symbol_meta_type(file, m_type)) {
            return 1;
        }

        if (write_full_archs(file, targets)) {
            return 1;
        }

        const enum tbd_version version = info->version;
        enum tbd_symbol_type type = sym->type;

        if (write_symbol_type_key(file, type, version, true)) {
            return 1;
        }

        if (write_symbol_info(file, sym)) {
            return 1;
        }

        uint64_t line_length = line_length_initial + sym->length;

        do {
            sym++;

            /*
             * If we've written out all symbols, simply end the current
             * sym-type array and return.
             */

            if (sym == end) {
                if (end_written_sequence(file)) {
                    return 1;
                }

                return 0;
            }

            const enum tbd_symbol_meta_type inner_meta_type = sym->meta_type;
            if (inner_meta_type != m_type) {
                if (end_written_sequence(file)) {
                    return 1;
                }

                m_type = inner_meta_type;
                goto meta;
            }

            /*
             * If the current sym-info has a different type from the previous
             * ones written out, end the current-sym-info array and create the
             * new one of the current sym-info.
             */

            const enum tbd_symbol_type inner_type = sym->type;
            if (inner_type != type) {
                if (end_written_sequence(file)) {
                    return 1;
                }

                if (write_symbol_type_key(file, inner_type, version, true)) {
                    return 1;
                }

                if (write_symbol_info(file, sym)) {
                    return 1;
                }

                /*
                 * Reset the line-length after ending the previous sym-type
                 * array.
                 */

                line_length = line_length_initial + sym->length;
                type = inner_type;

                continue;
            }

            /*
             * Write either a comma or a newline before writing the next export
             * to preserve a limit on line-lengths.
             */

            const uint64_t length = sym->length;
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

            if (write_symbol_info(file, sym)) {
                return 1;
            }

            line_length += length;
        } while (true);
    } while (true);

    return 0;
}

int
tbd_write_symbols_with_full_targets(
    FILE *__notnull const file,
    const struct tbd_create_info *__notnull const info,
    const uint64_t create_options)
{
    const struct array *const symbol_list = &info->fields.symbols;

    const struct tbd_symbol_info *sym = symbol_list->data;
    const struct tbd_symbol_info *const end = symbol_list->data_end;

    if (sym == end) {
        return 0;
    }

    enum tbd_symbol_meta_type m_type = sym->meta_type;

    do {
    meta:
        if (skip_syms_with_mtype(sym, end, m_type, create_options, &sym)) {
            return 0;
        }

        m_type = sym->meta_type;
        if (write_symbol_meta_type(file, m_type)) {
            return 1;
        }

        const struct target_list targets = info->fields.targets;
        const enum tbd_version version = info->version;

        if (write_full_targets(file, targets, version)) {
            return 1;
        }

        enum tbd_symbol_type type = sym->type;
        if (write_symbol_type_key(file, type, version, true)) {
            return 1;
        }

        if (write_symbol_info(file, sym)) {
            return 1;
        }

        uint64_t line_length = line_length_initial + sym->length;

        do {
            sym++;

            /*
             * If we've written out all symbols, simply end the current
             * sym-type array and return.
             */

            if (sym == end) {
                if (end_written_sequence(file)) {
                    return 1;
                }

                return 0;
            }

            const enum tbd_symbol_meta_type inner_meta_type = sym->meta_type;
            if (inner_meta_type != m_type) {
                if (end_written_sequence(file)) {
                    return 1;
                }

                m_type = inner_meta_type;
                goto meta;
            }

            /*
             * If the current sym-info has a different type from the previous
             * ones written out, end the current-sym-info array and create the
             * new one of the current sym-info.
             */

            const enum tbd_symbol_type inner_type = sym->type;
            if (inner_type != type) {
                if (end_written_sequence(file)) {
                    return 1;
                }

                if (write_symbol_type_key(file, inner_type, version, true)) {
                    return 1;
                }

                if (write_symbol_info(file, sym)) {
                    return 1;
                }

                /*
                 * Reset the line-length after ending the previous sym-type
                 * array.
                 */

                line_length = line_length_initial + sym->length;
                type = inner_type;

                continue;
            }

            /*
             * Write either a comma or a newline before writing the next sym to
             * preserve a limit on line-lengths.
             */

            const uint64_t length = sym->length;
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

            if (write_symbol_info(file, sym)) {
                return 1;
            }

            line_length += length;
        } while (true);
    } while (true);

    return 0;
}
