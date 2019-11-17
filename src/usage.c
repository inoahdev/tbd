//
// src/usage.c
// tbd
//
// Created by inoahdev on 11/18/18.
// Copyright © 2018 - 2019 inoahdev. All rights reserved.
//

#include <stdio.h>

void print_usage(void) {
    fputs("Usage: tbd [-p/--path] [path-options] [file-paths] [-o/--output] [output-options] [output-paths]\n", stdout);
    fputs("Main options:\n", stdout);
    fputs("    -h, --help,   Print this message\n", stdout);
    fputs("    -o, --output, Path to an output file (or directory for recursing/dyld_shared_cache files) to write converted tbd files.\n", stdout);
    fputs("                  If provided file(s) already exists, contents will be overridden.\n", stdout);
    fputs("                  Can also provide \"stdout\" to print to stdout\n", stdout);
    fputs("    -p, --path,   Path to a mach-o or dyld_shared_cache file to convert to a tbd file.\n", stdout);
    fputs("                  Can also provide \"stdin\" to use standard input.\n", stdout);
    fputs("    -u, --usage,  Print this message\n", stdout);

    fputc('\n', stdout);
    fputs("Path options:\n", stdout);
    fputs("Usage: tbd [-p] [options] path\n", stdout);
    fputs("    -r, --recurse, Specify directory to recurse and find all mach-o library and dyld_shared_cache images\n", stdout);
    fputs("                   Two modes exist for recursing:\n", stdout);
    fputs("                       once, Recurse only the top-level directory. This is the default mode for recursing\n", stdout);
    fputs("                       all,  Recurse both the top-level directory and over all sub-directories\n", stdout);

    fputc('\n', stdout);
    fputs("Write options:\n", stdout);
    fputs("Usage: tbd -o [options] path\n", stdout);
    fputs("        --preserve-subdirs,       Preserve the sub-directories of where files were found\n", stdout);
    fputs("                                  when recursing in relation to the actual provided recurse-path\n", stdout);
    fputs("        --no-overwrite,           Prevent overwriting of files when writing out\n", stdout);
    fputs("        --replace-path-extension, Replace the path-extension(s) of provided file(s) when\n", stdout);
    fputs("                                  writing out (Instead of simply appending .tbd)\n", stdout);
    fputs("        --combine-tbds,           Combine all tbds created (when recursing or with a dyld-shared-cache) into a\n", stdout);
    fputs("                                  single .tbd file\n", stdout);

    fputc('\n', stdout);
    fputs("Both local and global options:\n", stdout);
    fputs("        --macho,                         Specify that the file(s) provided should only be parsed\n", stdout);
    fputs("                                         if the file is a mach-o file.\n", stdout);
    fputs("                                         This option can be used to limit the filetypes parsed\n", stdout);
    fputs("                                         while recursing\n", stdout);
    fputs("        --dsc,                           Specify that the file(s) provided should only be parsed\n", stdout);
    fputs("                                         if it is a dyld-shared-cache file.\n", stdout);
    fputs("                                         Providing --macho or --dsc limits filetypes parsed when recursing\n", stdout);
    fputs("               --filter-image-directory, Specify a directory to filter dyld_shared_cache images from\n", stdout);
    fputs("               --filter-image-filename,  Specify a filename to filter dyld_shared_cache images from\n", stdout);
    fputs("               --filter-image-number,    Specify the number of an dyld_shared_cache image to parse out.\n", stdout);
    fputs("                                         To get the numbers of all available images, use the option --list-dsc-images\n", stdout);
    fputs("               --image-path,             Specify the path of an image to parse out.\n", stdout);
    fputs("                                         To get the paths of all available images, use the option --list-dsc-images\n", stdout);
    fputs("        -v, --version,                   Specify version of .tbd files to convert to (default is v2).\n", stdout);
    fputs("                                         This applies to all files where tbd-version was not explicitly set.\n", stdout);
    fputs("                                         To get a list of all available versions, look at the options below, or use\n", stdout);
    fputs("                                         the option --list-tbd-versions\n", stdout);
    fputs("        -v1,                             Set version of .tbd files to version v1.\n", stdout);
    fputs("        -v2,                             Set version of .tbd files to version v2. (This is the default .tbd version)\n", stdout);
    fputs("        -v3,                             Set version of .tbd files to version v3.\n", stdout);

    fputc('\n', stdout);
    fputs("Ignore options:\n", stdout);
    fputs("        --ignore-clients,               Ignore clients field\n", stdout);
    fputs("        --ignore-compatibility-version, Ignore compatibility-version field\n", stdout);
    fputs("        --ignore-current-version,       Ignore current-version field\n", stdout);
    fputs("        --ignore-objc-constraint,       Ignore objc-constraint field\n", stdout);
    fputs("        --ignore-parent-umbrella        Ignore parent-umbrella field\n", stdout);
    fputs("        --ignore-reexports,             Ignore re-expotrs field\n", stdout);
    fputs("        --ignore-swift-version,         Ignore swift-version field\n", stdout);
    fputs("        --ignore-undefineds,            Ignore undefineds field\n", stdout);
    fputs("        --ignore-uuids,                 Ignore uuids field\n", stdout);

    fputc('\n', stdout);
    fputs("General ignore options:\n", stdout);
    fputs("        --ignore-requests,          Ignore any and all user requests (both a path and global option)\n", stdout);
    fputs("        --ignore-warnings,          Ignore any warnings (both a path and global option)\n", stdout);
    fputs("        --ignore-wrong-filetype,    Ignore any warnings about a mach-o file having the wrong\n", stdout);
    fputs("                                    filetype (both a path and global option)\n", stdout);

    fputc('\n', stdout);
    fputs("Symbol options: (Both path and global options)\n", stdout);
    fputs("        --allow-private-objc-symbols,   Allow all non-external objc-symbols (classes, ivars, and ehtypes)\n", stdout);
    fputs("        --allow-private-objc-classes,   Allow all non-external objc-classes\n", stdout);
    fputs("        --allow-private-objc-ehtypes,   Allow all non-external objc-ehtypes.\n", stdout);
    fputs("                                        objc-ehtype symbols are only recognized for .tbd version v3\n", stdout);
    fputs("        --allow-private-objc-ivars,     Allow all non-external objc-ivars\n", stdout);
    fputs("        --use-symbol-table,             Use the symbol-table over the export-trie\n", stdout);

    fputc('\n', stdout);
    fputs("Field options: (Both path and global options)\n", stdout);
    fputs("        --replace-archs,           Provide a list of architectures to replace the list found in the provided input file(s)\n", stdout);
    fputs("                                   A list of architectures can be found by using option --list-architectures.\n", stdout);
    fputs("                                   Replacing the list of architectures will automatically remove the uuids field.\n", stdout);
    fputs("                                   In addition, each exported symbol will have the replaced list of architectures\n", stdout);
    fputs("        --replace-current-version, Provide a current-version to replace the one found in the provided input file(s)\n", stdout);
    fputs("        --replace-compat-version,  Provide a compatibility-version to replace the one found in the provided input file(s)\n", stdout);
    fputs("        --replace-flags,           Provide flag(s) to replace flags found for .tbd files.\n", stdout);
    fputs("                                   A list of flags can be found by using option --list-tbd-flags\n", stdout);
    fputs("        --replace-objc-constraint, Provide an objc-constraint to replace the one found in the provided input file(s)\n", stdout);
    fputs("        --replace-platform,        Provide a platform to replace the one found for in the provided input file(s)\n", stdout);
    fputs("        --replace-swift-version,   Provide a swift-version to replace the one found for in the provided input file(s)\n", stdout);

    fputc('\n', stdout);
    fputs("Ignore field warning options: (Both path and global options)\n", stdout);
    fputs("        --ignore-missing-exports,  Ignore error for when no symbols or reexpors to write out\n", stdout);
    fputs("                                   are found\n", stdout);
    fputs("        --ignore-missing-uuids,    Ignore error for when uuids are not found\n", stdout);
    fputs("        --ignore-non-unique-uuids, Ignore error for when uuids found are not unique among one another\n", stdout);

    fputc('\n', stdout);
    fputs("List options:\n", stdout);
    fputs("        --list-architectures,    List all valid architectures for .tbd files.\n", stdout);
    fputs("                                 Also able to list architectures of a single mach-o file from a provided path\n", stdout);
    fputs("        --list-dsc-images,       List all images of a dyld_shared_cache from a path to a single dyld_shared_cache file\n", stdout);
    fputs("                                 One option exists for listing dsc-images:\n", stdout);
    fputs("                                     --ordered\n", stdout);
    fputs("                                         Order image-paths alphabetically before printing them.\n", stdout);
    fputs("                                         An image-path's listed number from the ordered list should not be provided\n", stdout);
    fputs("                                         for option --filter-image-number\n", stdout);
    fputs("        --list-objc-constraints, List all valid objc-constraints\n", stdout);
    fputs("        --list-platform,         List all valid platforms\n", stdout);
    fputs("        --list-tbd-flags,        List all valid flags for .tbd files\n", stdout);
    fputs("        --list-tbd-versions,     List all valid versions for .tbd files\n", stdout);
}
