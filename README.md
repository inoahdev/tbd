# tbd
Convert Mach-O Libraries &amp; Frameworks to .tbd
```
Usage: tbd [-p file-paths] [-v/--version v2] [-a/--archs architectures] [-o/-output output-paths-or-stdout]
Main options:
    -a, --archs,    Specify Architecture(s) to use, instead of the ones in the provieded mach-o file(s)
    -h, --help,     Print this message
    -o, --output,   Path(s) to output file(s) to write converted .tbd. If provided file(s) already exists, contents will get overrided. Can also provide "stdout" to print to stdout
    -p, --path,     Path(s) to mach-o file(s) to convert to a .tbd
    -u, --usage,    Print this message
    -v, --version,  Set version of tbd to convert to (default is v2)

Extra options:
        --platform, Specify platform for all mach-o files provided
    -r, --recurse,  Specify directory to recurse and find mach-o files in. Use in conjunction with -p (ex. -p -r /path/to/directory)
        --versions, Print a list of all valid tbd-versions
```
