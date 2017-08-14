#!/bin/bash

function help {
    echo "Help:"
    echo "$0 build - build tbd"
    echo "$0 clean - clean project"
}

if [ $# -eq 0 ]; then
    help
    exit 0
fi

type=$1

rm -rf build >/dev/null 2>/dev/null
if [ "$type" == "build" ]; then
    build_flag="-Ofast"
    if [ $# -ge 1 ]; then
        if [[ $2 == "Debug" ]]; then
            build_flag="-O0 -g"
        fi
    fi

    should_log=false
    if [ $# -eq 3 ]; then
        should_log=$3
    fi

    mkdir -p build >/dev/null 2>/dev/null
    if [ $should_log = true ]; then
        clang++ -std=c++14 -stdlib=libc++ src/mach-o/architecture_info.cc src/mach-o/headers/cputype.cc src/mach-o/container.cc src/mach-o/file.cc src/mach-o/swap.cc src/misc/flags.cc src/tbd/tbd.cc src/main.cc $build_flag -o build/tbd
    else
        clang++ -std=c++14 -stdlib=libc++ src/mach-o/architecture_info.cc src/mach-o/header/cputype.cc src/mach-o/container.cc src/mach-o/file.cc src/mach-o/swap.cc src/misc/flags.cc src/tbd/tbd.cc src/main.cc $build_flag -o build/tbd >/dev/null 2>/dev/null
    fi
fi
