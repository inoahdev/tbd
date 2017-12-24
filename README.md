# tbd
Convert Mach-O Libraries &amp; Frameworks to .tbd
```
Usage: tbd [-p file-paths] [-o/--output output-paths-or-stdout]
Main options:
    -h, --help,     Print this message
    -o, --output,   Path(s) to output file(s) to write converted tbd files. If provided file(s) already exists, contents will be overridden. Can also provide \"stdout\" to print to stdout
    -p, --path,     Path(s) to mach-o file(s) to convert to a tbd file. Can also provide \"stdin\" to use stdin
    -u, --usage,    Print this message

Path options:
Usage: tbd -p [-a/--arch architectures] [--archs architecture-overrides] [--platform platform] [-r/--recurse/ -r=once/all / --recurse=once/all] [-v/--version v1/v2] /path/to/macho/library
    -a, --arch,     Specify architecture(s) to output to tbd
        --archs,    Specify architecture(s) to use, instead of the ones in the provided mach-o file(s)
        --platform, Specify platform for all mach-o library files provided
    -r, --recurse,  Specify directory to recurse and find mach-o library files in
    -v, --version,  Specify version of tbd to convert to (default is v2)

Outputting options:
Usage: tbd -o [--maintain-directories] /path/to/output/file
        --maintain-directories, Maintain directories where mach-o library files were found in (subtracting the path provided)

Global options:
    -a, --arch,     Specify architecture(s) to output to tbd (where architectures were not already specified)
        --archs,    Specify architecture(s) to override architectures found in file (where default architecture-overrides were not already provided)
        --platform, Specify platform for all mach-o library files provided (applying to all mach-o library files where platform was not provided)
    -v, --version,  Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tbd-version was not provided)

Miscellaneous options:
        --dont-print-warnings,    Don't print any warnings (both path and global option)
        --only-dynamic-libraries, Option for `--list-macho-libraries` to only print dynamic-libraries
        --replace-path-extension, Replace path-extension on provided mach-o file(s) when creating an output-file (Replace instead of appending .tbd) (both path and global option)

Symbol options: (Both path and global options)
        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)
        --allow-private-normal-symbols, Allow all non-external symbols (of no type) (Not guaranteed to link at runtime)
        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)
        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars
        --allow-private-objc-classes,   Allow all non-external objc-classes
        --allow-private-objc-ivars,     Allow all non-external objc-ivars

tbd field options: (Both path and global options)
        --flags,                        Specify flags to add onto ones found in provided mach-o file(s)
        --ignore-missing-exports,       Ignore if no symbols or reexpors to output are found in provided mach-o file(s)
        --ignore-missing-uuids,         Ignore if uuids are not found in provided mach-o file(s)
        --ignore-non-unique-uuids,      Ignore if uuids found in provided mach-o file(s) not unique
        --objc-constraint,              Specify objc-constraint to use instead of one(s) found in provided mach-o file(s)
        --remove-current-version,       Remove current-version field from outputted tbds
        --remove-compatibility-version, Remove compatibility-version field from outputted tbds
        --remove-exports,               Remove exports field from outputted tbds
        --remove-flags,                 Remove flags field from outputted tbds
        --remove-objc-constraint,       Remove objc-constraint field from outputted tbds
        --remove-parent-umbrella,       Remove parent-umbrella field from outputted tbds
        --remove-swift-version,         Remove swift-version field from outputted tbds
        --remove-uuids,                 Remove uuids field from outputted tbds

List options:
        --list-architectures,    List all valid architectures for tbd files. Also able to list architectures of a provided mach-o file
        --list-tbd-flags,        List all valid flags for tbd files
        --list-macho-libraries,  List all valid mach-o libraries in current-directory (or at provided path(s))
        --list-objc-constraints, List all valid objc-constraint options for tbd files
        --list-platform,         List all valid platforms
        --list-recurse,          List all valid recurse options for parsing directories
        --list-versions,         List all valid versions for tbd files
```
