//
// src/usage.c
// tbd
//
// Created by inoahdev on 11/18/18.
// Copyright 2018 inoahdev. All rights reserved.
//

#include <stdio.h>

void print_usage(void) {
    fputs("Usage: tbd [-p/--path] [path-options] [file-paths] [-o/--output] [output-options] [[output-paths-or-stdout]\n", stdout);
    fputs("Main options:\n", stdout);
    fputs("    -h, --help,   Print this message\n", stdout);
    fputs("    -o, --output, Path(s) to output file(s) to write converted tbd files. If provided file(s) already exists, contents will be overridden. Can also provide \"stdout\" to print to stdout\n", stdout);
    fputs("    -p, --path,   Path(s) to mach-o file(s) to convert to a tbd file. Can also provide \"stdin\" to use stdin\n", stdout);
    fputs("    -u, --usage,  Print this message\n", stdout);

    fputc('\n', stdout);
    fputs("Path options:\n", stdout);
    fputs("Usage: tbd [-p] [options] /path/to/mach-o/library\n", stdout);
    fputs("    -r, --recurse, Specify directory to recurse and find mach-o library files in\n", stdout);

    fputc('\n', stdout);
    fputs("Outputting options:\n", stdout);
    fputs("Usage: tbd -o [options] /path/to/output/file\n", stdout);
    fputs("        --preserve-hierarchy,     Preserve directory hierarcy of where mach-o library files were found in when recursing\n", stdout);
    fputs("        --no-overwrite,           Prevent overwriting of files when writing out (This may result in some files being skipped)\n", stdout);
    fputs("        --replace-path-extension, Replace the path-extension(s) of provided mach-o file(s) when creating an output-file (Instead of simply appending .tbd)\n", stdout);

    fputc('\n', stdout);
    fputs("Both local and global options:\n", stdout);
    fputs("        -v, --version, Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tbd-version was not explicitly set)\n", stdout);

    fputc('\n', stdout);
    fputs("Ignore options:\n", stdout);
    fputs("        --ignore-clients,               Ignore clients field\n", stdout);
    fputs("        --ignore-compatibility-version, Ignore compatibility-version field\n", stdout);
    fputs("        --ignore-current-version,       Ignore current-version field\n", stdout);
    fputs("        --ignore-exports,               Ignore exports field\n", stdout);
    fputs("        --ignore-objc-constraint,       Ignore objc-constraint field\n", stdout);
    fputs("        --ignore-parent-umbrell         Ignore parent-umbrella field\n", stdout);
    fputs("        --ignore-reexports,             Ignore swift-version field\n", stdout);
    fputs("        --ignore-swift-version,         Ignore swift-version field\n", stdout);
    fputs("        --ignore-uuids,                 Ignore uuids field\n", stdout);
    fputs("        --ignore-warnings,              Ignore any warnings (both path and global option)\n", stdout);


    fputc('\n', stdout);
    fputs("Symbol options: (Both path and global options)\n", stdout);
    fputs("        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-normal-symbols, Allow all non-external symbols (of no type) (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)\n", stdout);
    fputs("        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars\n", stdout);
    fputs("        --allow-private-objc-classes,   Allow all non-external objc-classes\n", stdout);
    fputs("        --allow-private-objc-ivars,     Allow all non-external objc-ivars\n", stdout);

    fputc('\n', stdout);
    fputs("tbd field remove options: (Both path and global options)\n", stdout);
    fputs("        --add-archs,     Provide architecture(s) to add onto architectures found in provided mach-o file(s)\n", stdout);
    fputs("        --remove-archs,  Provide architecture(s) to remove from architectures found in provided mach-o file(s)\n", stdout);
    fputs("        --replace-archs, Provide architecture(s) to replace architectures found in provided mach-o file(s)\n", stdout);
    fputs("        --add-flags,     Provide flag(s) to add onto flags found in provided mach-o file(s)\n", stdout);
    fputs("        --remove-flags,  Provide flag(s) to remove from flags found in provided mach-o file(s)\n", stdout);
    fputs("        --replace-flags, Provide flag(s) to replace flags found in provided mach-o file(s)\n", stdout);

    fputc('\n', stdout);
    fputs("tbd field warning ignore options: (Both path and global options)\n", stdout);
    fputs("        --ignore-missing-exports,  Ignore if no symbols or reexpors to output are found in provided mach-o file(s)\n", stdout);
    fputs("        --ignore-missing-uuids,    Ignore if uuids are not found in provided mach-o file(s)\n", stdout);
    fputs("        --ignore-non-unique-uuids, Ignore if uuids found in provided mach-o file(s) are not unique\n", stdout);

    fputc('\n', stdout);
    fputs("List options:\n", stdout);
    fputs("        --list-architectures,    List all valid architectures for tbd files. Also able to list architectures of a mach-o file from a provided path\n", stdout);
    fputs("        --list-tbd-flags,        List all valid flags for tbd files\n", stdout);
    fputs("        --list-objc-constraints, List all valid objc-constraint options for tbd files\n", stdout);
    fputs("        --list-platform,         List all valid platforms\n", stdout);
    fputs("        --list-recurse,          List all valid recurse options for parsing directories\n", stdout);
    fputs("        --list-tbd-versions,     List all valid versions for tbd files\n", stdout);
}
