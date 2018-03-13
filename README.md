# tbd
Convert Mach-O Libraries &amp; Frameworks to .tbd
```
Usage: tbd [-p file-paths] [-o/--output output-paths-or-stdout]
Main options:
    -h, --help,   Print this message
    -o, --output, Path(s) to output file(s) to write converted tbd files. If provided file(s) already exists, contents will be overridden. Can also provide \"stdout\" to print to stdout
    -p, --path,   Path(s) to mach-o file(s) to convert to a tbd file. Can also provide \"stdin\" to use stdin
    -u, --usage,  Print this message

Path options:
Usage: tbd -p [-a/--arch architectures] [--archs architecture-overrides] [--platform platform] [-r/--recurse/ -r=once/all / --recurse=once/all] [-v/--version v1/v2] /path/to/macho/library
        --platform, Specify platform for all mach-o library files provided
    -r, --recurse,  Specify directory to recurse and find mach-o library files in
    -v, --version,  Specify version of tbd to convert to (default is v2)

Outputting options:
Usage: tbd -o [--maintain-directories] /path/to/output/file
        --maintain-directories,   Maintain directories where mach-o library files were found in (subtracting the path provided)
        --no-overwrite,           Don't overwrite files when writing out (This results in entire files being skipped)
        --replace-path-extension, Replace path-extension on provided mach-o file(s) when creating an output-file (Replace instead of appending .tbd)

Global options:
        --platform, Specify platform for all mach-o library files provided (applying to all mach-o library files where platform was not provided)
    -v, --version,  Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tbd-version was not provided)

Miscellaneous options:
        --dont-print-warnings, Don't print any warnings (both path and global option)

Symbol options: (Both path and global options)
        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)
        --allow-private-normal-symbols, Allow all non-external symbols (of no type) (Not guaranteed to link at runtime)
        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)
        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars
        --allow-private-objc-classes,   Allow all non-external objc-classes
        --allow-private-objc-ivars,     Allow all non-external objc-ivars

tbd field replacement options: (Both path and global options)
        --replace-flags,           Specify flags to add onto ones found in provided mach-o file(s)
        --replace-objc-constraint, Specify objc-constraint to use instead of one(s) found in provided mach-o file(s)

tbd field remove options: (Both path and global options)
        --add-archs,                         Specify architecture(s) to add onto architectures found in file
        --remove-archs,                      Specify architecture(s) to remove from architectures found in file
        --replace-archs,                     Specify architecture(s) to replace architectures found in file
        --add-flags,                         Specify flag(s) to add onto flags found in file
        --remove-flags,                      Specify flag(s) to remove from flags found in file
        --remove-flags-field,                Remove flags field from written tbds
        --replace-flags,                     Specify flag(s) to replace flags found in file
        --remove-clients,                    Remove clients field from written tbds
        --remove-current-version,            Remove current-version field from written tbds
        --remove-compatibility-version,      Remove compatibility-version field from written tbds
        --remove-exports,                    Remove exports field from written tbds
        --remove-flags,                      Remove flags field from written tbds
        --remove-objc-constraint,            Remove objc-constraint field from written tbds
        --remove-parent-umbrella,            Remove parent-umbrella field from written tbds
        --remove-swift-version,              Remove swift-version field from written tbds
        --remove-uuids,                      Remove uuids field from written tbds
        --dont-remove-clients,               Don't Remove clients field from written tbds
        --dont-remove-current-version,       Don't remove current-version field from written tbds
        --dont-remove-compatibility-version, Don't remove compatibility-version field from written tbds
        --dont-remove-exports,               Don't remove exports field from written tbds
        --dont-remove-flags,                 Don't remove flags field from written tbds
        --dont-remove-objc-constraint,       Don't remove objc-constraint field from written tbds
        --dont-remove-parent-umbrella,       Don't remove parent-umbrella field from written tbds
        --dont-remove-swift-version,         Don't remove swift-version field from written tbds
        --dont-remove-uuids,                 Don't remove uuids field from written tbds

tbd field warning ignore options: (Both path and global options)
        --ignore-everything,            Ignore all tbd ignorable field warnings (sets the options below)
        --ignore-missing-exports,       Ignore if no symbols or reexpors to output are found in provided mach-o file(s)
        --ignore-missing-uuids,         Ignore if uuids are not found in provided mach-o file(s)
        --ignore-non-unique-uuids,      Ignore if uuids found in provided mach-o file(s) are not unique
        --dont-ignore-everything,       Reset all tbd field warning ignore options
        --dont-ignore-missing-exports,  Reset ignorning of missing symbols and reexports to output in provided mach-o file(s)
        --dont-ignore-missing-uuids,    Reset ignoring of missing uuids in provided mach-o file(s)
        --dont-ignore-non-unique-uuids, Reset ignoring of uuids found in provided mach-o file(s) not being unique

List options:
        --list-architectures,           List all valid architectures for tbd files. Also able to list architectures of a provided mach-o file
        --list-tbd-flags,               List all valid flags for tbd files
        --list-macho-dylibs,            List all valid mach-o dynamic-libraries in current-directory (or at provided path(s))
        --list-objc-constraints,        List all valid objc-constraint options for tbd files
        --list-platform,                List all valid platforms
        --list-recurse,                 List all valid recurse options for parsing directories
        --list-tbd-versions,            List all valid versions for tbd files

Validation options:
        --validate-macho-dylib, Check if file(s) at provided path(s) are valid mach-o dynamic-libraries
```
