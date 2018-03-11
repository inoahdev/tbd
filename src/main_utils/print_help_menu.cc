//
//  src/main_utils/print_help_menu.cc
//  tbd
//
//  Created by inoahdev on 1/25/18.
//  Copyright © 2018 inoahdev. All rights reserved.
//

#include <cstdio>
#include "print_help_menu.h"

/*
   tbd has several different types of options;
   - an option that is provided by itself (such as -h (--help) or -u (--usage)
   - an option that is used with other options to provide additional information.
     As they are within other options, and to differentiate from global options,
     they are referred to as local options
   - an option that is provided not from within another option but meant to
     apply to other options that don't manually supply them
   - an option that accepts local options in between the option and its main
     requested information (such as -p (--path) local-options /main/requested/path)
*/

namespace main_utils {
    void print_help_menu() noexcept {
        fputs("Usage: tbd [-p file-paths] [-o/--output output-paths-or-stdout]\n", stdout);
        fputs("Main options:\n", stdout);
        fputs("    -h, --help,   Print this message\n", stdout);
        fputs("    -o, --output, Path(s) to output file(s) to write converted tbd files. If provided file(s) already exists, contents will be overridden. Can also provide \"stdout\" to print to stdout\n", stdout);
        fputs("    -p, --path,   Path(s) to mach-o file(s) to convert to a tbd file. Can also provide \"stdin\" to use stdin\n", stdout);
        fputs("    -u, --usage,  Print this message\n", stdout);

        fputc('\n', stdout);
        fputs("Path options:\n", stdout);
        fputs("Usage: tbd -p [-a/--arch architectures] [--archs architecture-overrides] [--platform platform] [-r/--recurse/ -r=once/all / --recurse=once/all] [-v/--version v1/v2] /path/to/macho/library\n", stdout);
        fputs("    -a, --arch,     Specify architecture(s) to output to tbd\n", stdout);
        fputs("        --archs,    Specify architecture(s) to use, instead of the ones in the provided mach-o file(s)\n", stdout);
        fputs("        --platform, Specify platform for all mach-o library files provided\n", stdout);
        fputs("    -r, --recurse,  Specify directory to recurse and find mach-o library files in\n", stdout);
        fputs("    -v, --version,  Specify version of tbd to convert to (default is v2)\n", stdout);

        fputc('\n', stdout);
        fputs("Outputting options:\n", stdout);
        fputs("Usage: tbd -o [--maintain-directories] /path/to/output/file\n", stdout);
        fputs("        --maintain-directories,   Maintain directories where mach-o library files were found in (subtracting the path provided)\n", stdout);
        fputs("        --no-overwrite,           Don't overwrite files when writing out (This results in entire files being skipped)\n", stdout);
        fputs("        --replace-path-extension, Replace path-extension on provided mach-o file(s) when creating an output-file (Replace instead of appending .tbd)\n", stdout);

        fputc('\n', stdout);
        fputs("Global options:\n", stdout);
        fputs("    -a, --arch,     Specify architecture(s) to output to tbd (where architectures were not already specified)\n", stdout);
        fputs("        --archs,    Specify architecture(s) to override architectures found in file (where default architecture-overrides were not already provided)\n", stdout);
        fputs("        --platform, Specify platform for all mach-o library files provided (applying to all mach-o library files where platform was not provided)\n", stdout);
        fputs("    -v, --version,  Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tbd-version was not provided)\n", stdout);

        fputc('\n', stdout);
        fputs("Miscellaneous options:\n", stdout);
        fputs("        --dont-print-warnings, Don't print any warnings (both path and global option)\n", stdout);

        fputc('\n', stdout);
        fputs("Symbol options: (Both path and global options)\n", stdout);
        fputs("        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)\n", stdout);
        fputs("        --allow-private-normal-symbols, Allow all non-external symbols (of no type) (Not guaranteed to link at runtime)\n", stdout);
        fputs("        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)\n", stdout);
        fputs("        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars\n", stdout);
        fputs("        --allow-private-objc-classes,   Allow all non-external objc-classes\n", stdout);
        fputs("        --allow-private-objc-ivars,     Allow all non-external objc-ivars\n", stdout);

        fputc('\n', stdout);
        fputs("tbd field replacement options: (Both path and global options)\n", stdout);
        fputs("        --replace-flags,           Specify flags to add onto ones found in provided mach-o file(s)\n", stdout);
        fputs("        --replace-objc-constraint, Specify objc-constraint to use instead of one(s) found in provided mach-o file(s)\n", stdout);

        fputc('\n', stdout);
        fputs("tbd field remove options: (Both path and global options)\n", stdout);
        fputs("        --remove-current-version,            Remove current-version field from outputted tbds\n", stdout);
        fputs("        --remove-compatibility-version,      Remove compatibility-version field from outputted tbds\n", stdout);
        fputs("        --remove-exports,                    Remove exports field from outputted tbds\n", stdout);
        fputs("        --remove-flags,                      Remove flags field from outputted tbds\n", stdout);
        fputs("        --remove-objc-constraint,            Remove objc-constraint field from outputted tbds\n", stdout);
        fputs("        --remove-parent-umbrella,            Remove parent-umbrella field from outputted tbds\n", stdout);
        fputs("        --remove-swift-version,              Remove swift-version field from outputted tbds\n", stdout);
        fputs("        --remove-uuids,                      Remove uuids field from outputted tbds\n", stdout);
        fputs("        --dont-remove-current-version,       Don't remove current-version field from outputted tbds\n", stdout);
        fputs("        --dont-remove-compatibility-version, Don't remove compatibility-version field from outputted tbds\n", stdout);
        fputs("        --dont-remove-exports,               Don't remove exports field from outputted tbds\n", stdout);
        fputs("        --dont-remove-flags,                 Don't remove flags field from outputted tbds\n", stdout);
        fputs("        --dont-remove-objc-constraint,       Don't remove objc-constraint field from outputted tbds\n", stdout);
        fputs("        --dont-remove-parent-umbrella,       Don't remove parent-umbrella field from outputted tbds\n", stdout);
        fputs("        --dont-remove-swift-version,         Don't remove swift-version field from outputted tbds\n", stdout);
        fputs("        --dont-remove-uuids,                 Don't remove uuids field from outputted tbds\n", stdout);

        fputc('\n', stdout);
        fputs("tbd field warning ignore options: (Both path and global options)\n", stdout);
        fputs("        --ignore-everything,            Ignore all tbd ignorable field warnings (sets the options below)\n", stdout);
        fputs("        --ignore-missing-exports,       Ignore if no symbols or reexpors to output are found in provided mach-o file(s)\n", stdout);
        fputs("        --ignore-missing-uuids,         Ignore if uuids are not found in provided mach-o file(s)\n", stdout);
        fputs("        --ignore-non-unique-uuids,      Ignore if uuids found in provided mach-o file(s) are not unique\n", stdout);
        fputs("        --dont-ignore-everything,       Reset all tbd field warning ignore options\n", stdout);
        fputs("        --dont-ignore-missing-exports,  Reset ignorning of missing symbols and reexports to output in provided mach-o file(s)\n", stdout);
        fputs("        --dont-ignore-missing-uuids,    Reset ignoring of missing uuids in provided mach-o file(s)\n", stdout);
        fputs("        --dont-ignore-non-unique-uuids, Reset ignoring of uuids found in provided mach-o file(s) not being unique\n", stdout);

        fputc('\n', stdout);
        fputs("List options:\n", stdout);
        fputs("        --list-architectures,           List all valid architectures for tbd files. Also able to list architectures of a provided mach-o file\n", stdout);
        fputs("        --list-tbd-flags,               List all valid flags for tbd files\n", stdout);
        fputs("        --list-macho-dylibs,            List all valid mach-o dynamic-libraries in current-directory (or at provided path(s))\n", stdout);
        fputs("        --list-objc-constraints,        List all valid objc-constraint options for tbd files\n", stdout);
        fputs("        --list-platform,                List all valid platforms\n", stdout);
        fputs("        --list-recurse,                 List all valid recurse options for parsing directories\n", stdout);
        fputs("        --list-tbd-versions,            List all valid versions for tbd files\n", stdout);

        fputc('\n', stdout);
        fputs("Validation options:\n", stdout);
        fputs("        --validate-macho-dylib, Check if file(s) at provided path(s) are valid mach-o dynamic-libraries\n", stdout);
    }
}
