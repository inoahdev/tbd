# tbd
Convert Mach-O Libraries &amp; Frameworks to .tbd
```
Usage: tbd [-p file-paths] [-o/-output output-paths-or-stdout]
Main options:
    -h, --help,     Print this message
    -o, --output,   Path(s) to output file(s) to write converted .tbd. If provided file(s) already exists, contents will be overriden. Can also provide "stdout" to print to stdout
    -p, --path,     Path(s) to mach-o file(s) to convert to a .tbd
    -u, --usage,    Print this message

Path options:
Usage: tbd -p [-a/--arch architectures] [--archs architecture-overrides] [--platform platform] [-r/--recurse/ -r=once/all / --recurse=once/all] [-v/--version v1/v2] /path/to/macho/library
    -a, --arch,     Specify architecture(s) to output as tbd
        --archs,    Specify architecture(s) to use, instead of the ones in the provieded mach-o file(s)
        --platform, Specify platform for all mach-o library files provided
    -r, --recurse,  Specify directory to recurse and find mach-o library files in
    -v, --version,  Specify version of tbd to convert to (default is v2)

Outputting options:
Usage: tbd -o [--maintain-directories] /path/to/output/file
        --maintain-directories, Maintain directories where mach-o library files were found in (subtracting the path provided)

Global options:
    -a, --arch,     Specify architecture(s) to output to tbd (where architectures were not already specified)
        --archs,    Specify architecture(s) to use, replacing default architectures (where default architectures were not already provided)
        --platform, Specify platform for all mach-o library files provided (applying to all mach-o library files where platform was not provided)
    -v, --version,  Specify version of tbd to convert to (default is v2) (applying to all mach-o library files where tnd-version was not provided)

Miscellaneous options:
        --dont-print-warnings, Don't print any warnings (both path and global option)

Symbol options: (Both path and global options)
        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)
        --allow-private-normal-symbols, Allow all non-external symbols (of no type) (Not guaranteed to link at runtime)
        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)
        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars
        --allow-private-objc-classes,   Allow all non-external objc-classes
        --allow-private-objc-ivars,     Allow all non-external objc-ivars

List options:
        --list-architectures,   List all valid architectures for tbd-files
        --list-macho-libraries, List all valid mach-o libraries in current-directory (or at provided path(s))
        --list-platform,        List all valid platforms
        --list-recurse,         List all valid recurse options for parsing directories
        --list-versions,        List all valid versions for tbd-files
```
