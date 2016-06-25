# tbd
Convert Mach-O Libraries &amp; Frameworks to .tbd
```
Usage: tbd [path-to-file] [(-v/--version)=v2] [(-a/-archs)="armv7, arm64"], [-o/-output path-to-output]
Options:
    -a, --archs,   Specify Architecture to use, instead of the ones in the binary. Seperate architectures with ",". (ex. -archs="armv7, arm64")
    -o, --output,  Output converted .tbd to a file
	-u, --usage,   Print this message
	-v, --version, Set version of tbd to convert to. Run --versions to see a list of available versions. (ex. -v=v1) "v2" is the default version
```

Supports `FAT_MAGIC_64` & `FAT_CIGAM_64` (though not officially released)
