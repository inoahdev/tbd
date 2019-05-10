# tbd
Convert Mach-O Libraries &amp; Frameworks to .tbd

```
Usage: tbd [-p/--path] [path-options] [file-paths] [-o/--output] [output-options] [output-paths]
Main options:
    -h, --help,   Print this message
    -o, --output, Path(s) to output file(s) to write converted tbd files.
                  If provided file(s) already exists, contents will be overridden.
                  Can also provide "stdout" to print to stdout
    -p, --path,   Path(s) to a mach-o or dyld_shared_cache file to convert to a tbd file.
                  Can also provide "stdin" to use stdin.
                  also be provided
    -u, --usage,  Print this message

Path options:
Usage: tbd [-p] [options] path
    -r, --recurse, Specify directory to recurse and find all mach-o library and dyld_shared_cache images
                   Two modes exist for recursing:
                       once, Recurse only the top-level directory. This is the default case for recursing
                       all,  Recurse both the top-level directory and sub-directories

        --macho,   Specify that the file provided should only be parsed if it is a mach-o file.
                   This option can be used to limit the filetypes parsed while recursing
        --dsc,     Specify that the file provided should only be parsed if it is a dyld-shared-cache file.
                   This option can be used to limit the filetypes parsed while recursing
                   Both --macho and --dsc may be provided to indicate that both filetypes should be parsed while recursing
                   This is however the default behavior, and therefore redundant.
                   Providing both --macho and --dsc when not recursing is supported for indicating the
                   filetype of the provided file

Outputting options:
Usage: tbd -o [options] path
        --preserve-subdirs,       Preserve the sub-directories of where files were found
                                  when recursing in relation to the actual provided recurse-path
        --no-overwrite,           Prevent overwriting of files when writing out
        --replace-path-extension, Replace the path-extension(s) of provided file(s) when
                                  writing out (Instead of simply appending .tbd)

Both local and global options:
            --filter-image-directory, Specify a directory to filter dyld_shared_cache images from
            --filter-image-filename,  Specify a filename to filter dyld_shared_cache images from
            --filter-image-number,    Specify the number of an dyld_shared_cache image to parse out.
                                      To get the numbers of all available images, use the option --list-dsc-images
            --image-path,             Specify the path of an image to parse out.
                                      To get the paths of all available images, use the option --list-dsc-images
        -v, --version,                Specify version of .tbd files to convert to (default is v2).
                                      This applies to all files where tbd-version was not explicitly set.
                                      To get a list of all available versions, use the option --list-tbd-versions

Ignore options:
        --ignore-clients,               Ignore clients field
        --ignore-compatibility-version, Ignore compatibility-version field
        --ignore-current-version,       Ignore current-version field
        --ignore-exports,               Ignore exports field
        --ignore-objc-constraint,       Ignore objc-constraint field
        --ignore-parent-umbrella        Ignore parent-umbrella field
        --ignore-reexports,             Ignore re-expotrs field
        --ignore-swift-version,         Ignore swift-version field
        --ignore-uuids,                 Ignore uuids field

General ignore options:
        --ignore-requests,    Ignore requests of all kinds (both path and global option)
        --ignore-warnings,    Ignore any warnings (both path and global option)

Symbol options: (Both path and global options)
        --allow-all-private-symbols,    Allow all non-external symbols (Not guaranteed to link at runtime)
        --allow-private-normal-symbols, Allow all non-external symbols (Not guaranteed to link at runtime)
        --allow-private-weak-symbols,   Allow all non-external weak symbols (Not guaranteed to link at runtime)
        --allow-private-objc-symbols,   Allow all non-external objc-classes and ivars
        --allow-private-objc-classes,   Allow all non-external objc-classes
        --allow-private-objc-ehtypes,   Allow all non-external objc eh-types
        --allow-private-objc-ivars,     Allow all non-external objc-ivars

Field options: (Both path and global options)
        --add-archs,     Provide architecture(s) to add onto architectures found for .tbd files
        --remove-archs,  Provide architecture(s) to remove from architectures found for .tbd files
        --replace-archs, Provide architecture(s) to replace architectures found for .tbd files
        --add-flags,     Provide flag(s) to add onto flags found for .tbd files
        --remove-flags,  Provide flag(s) to remove from flags found for .tbd files
        --replace-flags, Provide flag(s) to replace flags found for .tbd files

Ignore field warning options: (Both path and global options)
        --ignore-missing-exports,  Ignore error for when no symbols or reexpors to write out
                                   are found
        --ignore-missing-uuids,    Ignore error for when uuids are not found
        --ignore-non-unique-uuids, Ignore error for when uuids found are not unique among one another

List options:
        --list-architectures,    List all valid architectures for .tbd files.
                                 Also able to list architectures of the mach-o file from a provided path
        --list-dsc-images,       List all images of a dyld_shared_cache from a provided path
        --list-objc-constraints, List all valid objc-constraint options for .tbd files
        --list-platform,         List all valid platforms
        --list-recurse,          List all valid recurse options for parsing directories
        --list-tbd-flags,        List all valid flags for .tbd files
        --list-tbd-versions,     List all valid versions for .tbd files
```
