//
//  src/macho_file_parse_export_trie.c
//  tbd
//
//  Created by inoahdev on 11/3/19.
//  Copyright © 2019 inoahdev. All rights reserved.
//

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "guard_overflow.h"
#include "likely.h"
#include "macho_file.h"
#include "macho_file_parse_export_trie.h"
#include "our_io.h"

const uint8_t *
read_uleb128(const uint8_t *iter,
             const uint8_t *const end,
             uint64_t *const result_out)
{
    uint64_t shift = 0;
    uint64_t result = 0;

    /*
     * uleb128 format is as follows:
     * Every byte with the MSB set to 1 indicates that the byte and the next
     * byte are part of the uleb128 format.
     *
     * The uleb128's integer contents are stored in the 7 LSBs, and are combined
     * into a single integer by placing every 7 bits right to the left (MSB) of
     * the previous 7 bits stored.
     *
     * Ex:
     *     10000110 10010100 00101000
     *
     * In the example above, the first two bytes have the MSB set, meaning that
     * the same byte, and the byte after, are in that uleb128.
     *
     * In this case, all three bytes are used.
     *
     * The lower 7 bits of the integer are all combined together as described
     * earlier.
     *
     * Parsing into an integer should result in the following:
     *     0101000 0010100 0000110
     *
     * Here, the 7 bits are stored in the order opposite to how they were found.
     *
     * The first byte's 7 bits are stored in the 3rd component, the second
     * byte's 7 bits are in the second component, and the third byte's 7 bits
     * are stored in the 1st component.
     */

    do {
        if (iter == end) {
            return NULL;
        }

        /*
         * Get the lower 7 bits.
         */

        const uint64_t bits = (*iter & 0x7f);

        /*
         * Make sure we actually have the bit-space on a 64-bit integer to store
         * the lower 7-bits.
         */

        const uint64_t shifted_bits = (bits << shift);
        const uint64_t shifted_back_bits = (shifted_bits >> shift);

        if (bits != shifted_back_bits) {
            return NULL;
        }

        result |= (bits << shift);
        shift += 7;
    } while (*iter++ & 0x80);

    *result_out = result;
    return iter;
}

const uint8_t *skip_uleb128(const uint8_t *iter, const uint8_t *const end) {
    uint64_t shift = 0;

    do {
        if (iter == end || shift == 63) {
            return NULL;
        }

        shift += 7;
    } while (*iter++ & 0x80);

    return iter;
}

static bool
has_overlapping_range(struct array *__notnull const list,
                      const struct range range)
{
    const struct range *list_range = list->data;
    const struct range *const end = list->data_end;

    for (; list_range != end; list_range++) {
        if (ranges_overlap(*list_range, range)) {
            return true;
        }
    }

    return false;
}

/*
 * Apple's mach-o loader.h header doesn't seem to have this export-kind.
 */

const uint64_t EXPORT_SYMBOL_FLAGS_KIND_ABSOLUTE = 0x02;

/*
 * The export-trie is a compressed tree designed to store symbols and other info
 * in an efficient fashion.
 *
 * To better understand the forat, let's create a trie of the symbols "_symbol",
 * "__symcol", and "_abcdef", "_abcghi", and work backwrds.
 *
 * Here's a tree showing how the compressed trie would look like:
 *              "_"
 *            /     \
 *       "sym"       "abc"
 *       /   \        /  \
 *  "bol"    "col" "def"  "ghi"
 *
 * As you can see, the symbols are parsed to find common prefixes, and then
 * placed in a tree where they can later be reconstructed back into full-fledged
 * symbols.
 *
 * To store them in binary, each node on the tree is stored in a tree-node.
 *
 * Every tree-node contains two properties - its structure (terminal) size and
 * the children-count. The structure size and children-count are both stored in
 * the uleb128 format described above.
 *
 * So in C, a basic tree-node may look something like the following:
 *
 * struct tree_node {
 *     uleb128 terminal_size; // structure-size
 *     uleb128 children_count;
 * }
 *
 * If the tree-node's terminal-size is zero, the tree-node simply contains the
 * children-count, and an array of children mini-nodes that follow directly
 * after the tree-node.
 *
 * Every tree-node is pointed to by a child (mini-node) belonging to another
 * tree-node, except the first tree-node. Until the terminal-size is a non-zero
 * value, indicating that tree-node has reached its end, creating a full symbol,
 * the tree of nodes will keep extending down.
 *
 * Each child (mini-node) contains a label (the string, as shown in the tree
 * above), and a uleb128 index to the next node in that path. The uleb128 index
 * is relative to the start of the export-trie, and not relative to the start of
 * the current tree-node.
 *
 * So for the tree provided above, the top-level node would have one child,
 * whose label is '_', and an index to another tree-node, which provides info
 * for the '_' label.
 *
 * That tree-node would have two children, for both the "sym" and the "abc"
 * components, and these two children would each have their two children for the
 * "bol", "col", "def", "ghi" components.
 *
 * However, if the terminal-size is non-zero, the previous node encountered gave
 * the last bit of string we needed to create the symbol.
 *
 * In this case, the present node would be the final node to parse, and would
 * contain information on the symbol.
 *
 * The final export-node structure stores a uleb128 flags field.
 *
 * The flags field stores the 'kind' of the symbol, whether its a normal symbol,
 * weakly-defined, or a thread-local symbol.
 *
 * The flags field also specifies whether the export is a re-export or not, and
 * as such what format the fields after the flags field are in.
 *
 * For a re-export export-node, a dylib-ordinal uleb128 immediately follows the
 * flags field.
 *
 * The dylib-ordinal is the number (starting from 1) of the dylibs imported via
 * the LC_REEXPORT_DYLIB and other load-commands. The ordinal describes where
 * the symbol is re-exported from.
 *
 * Following the dylib-ordinal is the re-export string. If the string is not
 * empty, the string describes what symbol the export-node's is re-exported
 * from.
 *
 * Re-exports are used to provide additional symbols, to one that already exist,
 * for example, the export-node _bzero is re-exported from _platform_bzero in
 * another library file.
 *
 * On the other hand, a non-reexport export-node has an address uleb128 field
 * follow it.
 *
 * If the export is non-reexport, as well as a stub-resolver, the address of the
 * stub-resolver's own stub-resolver is given in another uleb128.
 *
 * Basically, the structures can be imagined in the following:
 *
 * struct final_export_node_reexport {
 *     uleb128 terminal_size;
 *     uleb128 flags;
 *     uleb128 dylib_ordinal;
 *     char reexport_symbol[]; // may be empty.
 * };
 *
 * struct final_export_node_non_reexport {
 *     uleb128 terminal_size;
 *     uleb128 flags;
 *     uleb128 address;
 * };
 *
 * struct final_export_node_non_reexport_stub_resolver {
 *     uleb128 terminal_size;
 *     uleb128 flags;
 *     uleb128 address;
 *     uleb128 address_of_self_stub_resolver;
 * };
 */

enum macho_file_parse_result
parse_trie_node(struct tbd_create_info *__notnull info_in,
                const uint64_t arch_bit,
                const uint8_t *__notnull const start,
                const uint64_t offset,
                const uint8_t *__notnull const end,
                struct array *__notnull const node_ranges,
                const uint64_t export_size,
                struct string_buffer *__notnull sb_buffer,
                const uint64_t options)
{
    const uint8_t *iter = start + offset;
    uint64_t iter_size = *iter;

    if (iter_size > 127) {
        /*
         * The iter-size is stored in a uleb128.
         */

        if ((iter = read_uleb128(iter, end, &iter_size)) == NULL) {
            return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
        }
    } else {
        iter++;
    }

    const uint8_t *const node_start = iter;
    const uint8_t *const children = iter + iter_size;

    if (unlikely(children > end)) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    const struct range export_range = {
        .begin = offset,
        .end = offset + iter_size
    };

    if (unlikely(has_overlapping_range(node_ranges, export_range))) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    const enum array_result add_range_result =
        array_add_item(node_ranges,
                       sizeof(export_range),
                       &export_range,
                       NULL);

    if (unlikely(add_range_result != E_ARRAY_OK)) {
        return E_MACHO_FILE_PARSE_ARRAY_FAIL;
    }

    const bool is_export_info = (iter_size != 0);
    if (is_export_info) {
        /*
         * This should only occur if the first tree-node is an export-node, but
         * check anyways as this is invalid behavior.
         */

        if (sb_buffer->length == 0) {
            return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
        }

        uint64_t flags = 0;
        if ((iter = read_uleb128(iter, end, &flags)) == NULL) {
            return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
        }

        const uint64_t kind = (flags & EXPORT_SYMBOL_FLAGS_KIND_MASK);
        if (flags != 0) {
            if (kind != EXPORT_SYMBOL_FLAGS_KIND_REGULAR &&
                kind != EXPORT_SYMBOL_FLAGS_KIND_ABSOLUTE &&
                kind != EXPORT_SYMBOL_FLAGS_KIND_THREAD_LOCAL)
            {
                return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
            }
        }

        if (flags & EXPORT_SYMBOL_FLAGS_REEXPORT) {
            if ((iter = skip_uleb128(iter, end)) == NULL) {
                return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
            }

            if (unlikely(*iter != '\0')) {
                iter++;

                /*
                 * Pass the length-calculation of the re-export's install-name
                 * to strnlen in the hopes of better performance.
                 */

                const uint64_t max_length = (uint64_t)(end - iter);
                const uint64_t length = strnlen((char *)iter, max_length);

                /*
                 * We can't have a re-export whose install-name reaches the end
                 * of the export-trie.
                 */

                if (unlikely(length == max_length)) {
                    return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
                }

                /*
                 * Skip past the null-terminator.
                 */

                iter += (length + 1);
            } else {
                iter++;
            }
        } else {
            if ((iter = skip_uleb128(iter, end)) == NULL) {
                return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
            }

            if (flags & EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER) {
                if ((iter = skip_uleb128(iter, end)) == NULL) {
                    return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
                }
            }
        }

        const uint8_t *const expected_end = node_start + iter_size;
        if (unlikely(iter != expected_end)) {
            return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
        }

        enum tbd_symbol_type predefined_type = TBD_SYMBOL_TYPE_NONE;
        switch (kind) {
            case EXPORT_SYMBOL_FLAGS_KIND_REGULAR:
                if (flags & EXPORT_SYMBOL_FLAGS_WEAK_DEFINITION) {
                    predefined_type = TBD_SYMBOL_TYPE_WEAK_DEF;
                }

                break;

            case EXPORT_SYMBOL_FLAGS_KIND_THREAD_LOCAL:
                predefined_type = TBD_SYMBOL_TYPE_THREAD_LOCAL;
                break;

            default:
                break;
        }

        const enum tbd_ci_add_symbol_result add_symbol_result =
            tbd_ci_add_symbol_with_info_and_len(info_in,
                                                sb_buffer->data,
                                                sb_buffer->length,
                                                arch_bit,
                                                predefined_type,
                                                true,
                                                false,
                                                options);

        if (add_symbol_result != E_TBD_CI_ADD_SYMBOL_OK) {
            return E_MACHO_FILE_PARSE_CREATE_SYMBOLS_FAIL;
        }
    }

    const uint8_t children_count = *iter;
    if (children_count == 0) {
        return E_MACHO_FILE_PARSE_OK;
    }

    iter++;
    if (unlikely(iter == end)) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    /*
     * Every child shares only the same symbol-prefix, and only the same
     * node-ranges, both of which we need to restore to its original amount
     * after every loop.
     */

    const uint64_t orig_buff_length = sb_buffer->length;
    const uint64_t orig_node_ranges_count = node_ranges->item_count;

    for (uint8_t i = 0; i != children_count; i++) {
        /*
         * Pass the length-calculation of the string to strnlen in the hopes of
         * better performance.
         */

        const uint64_t max_length = (uint64_t)(end - iter);
        const uint64_t length = strnlen((char *)iter, max_length);

        /*
         * We can't have the string reach the end of the export-trie.
         */

        if (unlikely(length == max_length)) {
            return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
        }

        const enum string_buffer_result add_c_str_result =
            sb_add_c_str(sb_buffer, (char *)iter, length);

        if (unlikely(add_c_str_result != E_STRING_BUFFER_OK)) {
            return E_MACHO_FILE_PARSE_ALLOC_FAIL;
        }

        /*
         * Skip past the null-terminator.
         */

        iter += (length + 1);

        uint64_t next = *iter;
        if (next > 127) {
            if ((iter = read_uleb128(iter, end, &next)) == NULL) {
                return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
            }
        } else {
            iter++;
        }

        if (unlikely(next >= export_size)) {
            return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
        }

        /*
         * From dyld, don't parse an export-trie that gets too deep.
         */

        if (unlikely(node_ranges->item_count > 128)) {
            return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
        }

        const enum macho_file_parse_result parse_export_result =
            parse_trie_node(info_in,
                            arch_bit,
                            start,
                            next,
                            end,
                            node_ranges,
                            export_size,
                            sb_buffer,
                            options);

        if (unlikely(parse_export_result != E_MACHO_FILE_PARSE_OK)) {
            return parse_export_result;
        }

        array_trim_to_item_count(node_ranges,
                                 sizeof(struct range),
                                 orig_node_ranges_count);

        sb_buffer->length = orig_buff_length;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_export_trie_from_file(
    const struct macho_file_parse_export_trie_args args,
    const int fd,
    const uint64_t base_offset)
{
    /*
     * Validate the dyld-info exports-range.
     */

    if (args.dyld_info.export_size < 2) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    uint64_t export_end = args.dyld_info.export_off;
    if (guard_overflow_add(&export_end, args.dyld_info.export_size)) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    uint64_t full_export_off = base_offset;
    if (guard_overflow_add(&full_export_off, args.dyld_info.export_off)) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    uint64_t full_export_end = base_offset;
    if (guard_overflow_add(&full_export_end, export_end)) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    const struct range full_export_range = {
        .begin = full_export_off,
        .end = full_export_end
    };

    if (!range_contains_other(args.available_range, full_export_range)) {
        return E_MACHO_FILE_PARSE_INVALID_RANGE;
    }

    if (our_lseek(fd, full_export_off, SEEK_SET) < 0) {
        return E_MACHO_FILE_PARSE_SEEK_FAIL;
    }

    uint8_t *const export_trie = calloc(1, args.dyld_info.export_size);
    if (export_trie == NULL) {
        return E_MACHO_FILE_PARSE_ALLOC_FAIL;
    }

    if (our_read(fd, export_trie, args.dyld_info.export_size) < 0) {
        free(export_trie);
        return E_MACHO_FILE_PARSE_READ_FAIL;
    }

    struct array node_ranges = {};
    const uint8_t *const end = export_trie + args.dyld_info.export_size;

    const enum macho_file_parse_result parse_node_result =
        parse_trie_node(args.info_in,
                        args.arch_bit,
                        export_trie,
                        0,
                        end,
                        &node_ranges,
                        args.dyld_info.export_size,
                        args.sb_buffer,
                        args.tbd_options);

    array_destroy(&node_ranges);
    free(export_trie);

    if (parse_node_result != E_MACHO_FILE_PARSE_OK) {
        return parse_node_result;
    }

    return E_MACHO_FILE_PARSE_OK;
}

enum macho_file_parse_result
macho_file_parse_export_trie_from_map(
    const struct macho_file_parse_export_trie_args args,
    const uint8_t *__notnull map)
{
    /*
     * Validate the dyld-info exports-range.
     */

    if (args.dyld_info.export_size < 2) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    uint64_t export_end = args.dyld_info.export_off;
    if (guard_overflow_add(&export_end, args.dyld_info.export_size)) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    const struct range export_range = {
        .begin = args.dyld_info.export_off,
        .end = export_end
    };

    if (!range_contains_other(args.available_range, export_range)) {
        return E_MACHO_FILE_PARSE_INVALID_EXPORTS_TRIE;
    }

    const uint8_t *const export_trie = map + args.dyld_info.export_off;
    const uint8_t *const end = export_trie + args.dyld_info.export_size;

    struct array node_ranges = {};
    const enum macho_file_parse_result parse_node_result =
        parse_trie_node(args.info_in,
                        args.arch_bit,
                        export_trie,
                        0,
                        end,
                        &node_ranges,
                        args.dyld_info.export_size,
                        args.sb_buffer,
                        args.tbd_options);

    array_destroy(&node_ranges);

    if (parse_node_result != E_MACHO_FILE_PARSE_OK) {
        return parse_node_result;
    }

    return E_MACHO_FILE_PARSE_OK;
}
