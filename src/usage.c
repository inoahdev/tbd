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
    fputs("    -o, --output, Path(s) to output file(s) to write converted tbd files.\n", stdout);
    fputs("                  If provided file(s) already exists, contents will be overridden.\n", stdout);
    fputs("                  Can also provide \"stdout\" to print to stdout\n", stdout);
    fputs("    -p, --path,   Path(s) to mach-o file(s) to convert to a tbd file.\n", stdout);
    fputs("                  Can also provide \"stdin\" to use stdin\n", stdout);
    fputs("    -u, --usage,  Print this message\n", stdout);

    fputc('\n', stdout);
    fputs("Path options:\n", stdout);
    fputs("Usage: tbd [-p] [options] path\n", stdout);
    fputs("    -r, --recurse,     Specify directory to recurse and find mach-o library files in\n", stdout);
    fputs("        --dsc,         Specify that the file provided is actually a dyld_shared_cache file.\n", stdout);
    fputs("                       Note that dyld_shared_cache files are parsed by extracting their\n", stdout);
    fputs("                       images into tbds written in a provided folder.\n", stdout);
    fputs("                       This option can also be used when recursing, to indicate that only\n", stdout);
    fputs("                       dyld_shared_cache files should be parsed, and not any mach-o files\n", stdout);
    fputs("        --include-dsc, Specify that while recursing, dyld_shared_cache files should be parsed\n", stdout);
    fputs("                       in addition, to mach-o files\n", stdout);

    fputc('\n', stdout);
    fputs("Outputting options:\n", stdout);
    fputs("Usage: tbd -o [options] path\n", stdout);
    fputs("        --preserve-subdirs,       Preserve the sub-directories of where files were found in\n", stdout);
    fputs("                                  when recursing in relation to the actual provided recurse-path\n", stdout);
    fputs("        --no-overwrite,           Prevent overwriting of files when writing out.\n", stdout);
    fputs("                                  This may result in some files being skipped\n", stdout);
    fputs("        --replace-path-extension, Replace the path-extension(s) of provided file(s) when\n", stdout);
    fputs("                                  creating an output-file (Instead of simply appending .tbd)\n", stdout);

    fputc('\n', stdout);
    fputs("Both local and global options:\n", stdout);
    fputs("            --filter-image-directory, Specify a directory to filter images from\n", stdout);
    fputs("            --filter-image-filename,  Specify a filename to filter images from\n", stdout);
    fputs("            --filter-image-number,    Specify the number of an image to parse out.\n", stdout);
    fputs("                                      To get the numbers of all available images, Use the option --list-images\n", stdout);
    fputs("            --image-path,             Specify the path of an image to parse out.\n", stdout);
    fputs("                                      To get the paths of all available images, Use the option --list-images\n", stdout);
    fputs("        -v, --version,                Specify version of tbd to convert to (default is v2).\n", stdout);
    fputs("                                      This applies to all files where tbd-version was not explicitly set\n", stdout);

    fputc('\n', stdout);
    fputs("Ignore options:\n", stdout);
    fputs("        --ignore-clients,               Ignore clients field\n", stdout);
    fputs("        --ignore-compatibility-version, Ignore compatibility-version field\n", stdout);
    fputs("        --ignore-current-version,       Ignore current-version field\n", stdout);
    fputs("        --ignore-exports,               Ignore exports field\n", stdout);
    fputs("        --ignore-objc-constraint,       Ignore objc-constraint field\n", stdout);
    fputs("        --ignore-parent-umbrella        Ignore parent-umbrella field\n", stdout);
    fputs("        --ignore-reexports,             Ignore swift-version field\n", stdout);
    fputs("        --ignore-swift-version,         Ignore swift-version field\n", stdout);
    fputs("        --ignore-uuids,                 Ignore uuids field\n", stdout);

    fputc('\n', stdout);
    fputs("General ignore options:\n", stdout);
    fputs("        --ignore-requests,    Ignore requests of all kinds (both path and global option)\n", stdout);
    fputs("        --ignore-warnings,    Ignore any warnings (both path and global option)\n", stdout);

    fputc('\n', stdout);
    fputs("Symbol options: (Both path and global options)\n", stdout);
    fputs("        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-normal-symbols, Allow all non-external symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars\n", stdout);
    fputs("        --allow-private-objc-classes,   Allow all non-external objc-classes\n", stdout);
    fputs("        --allow-private-objc-ivars,     Allow all non-external objc-ivars\n", stdout);

    fputc('\n', stdout);
    fputs("Field options: (Both path and global options)\n", stdout);
    fputs("        --add-archs,     Provide architecture(s) to add onto architectures found in the provided mach-o(s)\n", stdout);
    fputs("        --remove-archs,  Provide architecture(s) to remove from architectures found in the provided mach-o(s)\n", stdout);
    fputs("        --replace-archs, Provide architecture(s) to replace architectures found in the provided mach-o(s)\n", stdout);
    fputs("        --add-flags,     Provide flag(s) to add onto flags found in the provided mach-o(s)\n", stdout);
    fputs("        --remove-flags,  Provide flag(s) to remove from flags found in the provided mach-o(s)\n", stdout);
    fputs("        --replace-flags, Provide flag(s) to replace flags found in the provided mach-o(s)\n", stdout);

    fputc('\n', stdout);
    fputs("Ignore field warning options: (Both path and global options)\n", stdout);
    fputs("        --ignore-missing-exports,  Ignore if no symbols or reexpors to output are found\n", stdout);
    fputs("                                   in the provided mach-o(s)\n", stdout);
    fputs("        --ignore-missing-uuids,    Ignore if uuids are not found in the provided mach-o(s)\n", stdout);
    fputs("        --ignore-non-unique-uuids, Ignore if uuids found in the provided mach-o(s)\n", stdout);

    fputc('\n', stdout);
    fputs("List options:\n", stdout);
    fputs("        --list-architectures,    List all valid architectures for tbd files.\n", stdout);
    fputs("                                 Also able to list architectures of the mach-o file from a provided path\n", stdout);
    fputs("        --list-dsc-images,       List all images of a dyld_shared_cache from a provided path\n", stdout);
    fputs("        --list-objc-constraints, List all valid objc-constraint options for tbd files\n", stdout);
    fputs("        --list-platform,         List all valid platforms\n", stdout);
    fputs("        --list-recurse,          List all valid recurse options for parsing directories\n", stdout);
    fputs("        --list-tbd-flags,        List all valid flags for tbd files\n", stdout);
    fputs("        --list-tbd-versions,     List all valid versions for tbd files\n", stdout);
}
