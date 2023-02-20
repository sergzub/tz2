#!/bin/bash -eu

BUILD_DIR='build'
IN_DIR="in"
OUT_DIR="out"

CreateDir()
{
    test -d "${1}" || mkdir "${1}"
}

DropInputFiles()
{
    rm -rf "${IN_DIR}" && mkdir "${IN_DIR}"
}

CreateInputFiles()
{
    DropInputFiles

    local just1Mb=1048576
    # local size="$(( 15 * 1024 * ${just1Mb} / 10 ))" 
    local size="$(( 3 * 4096 ))" 
    local n=3 # 10

    # Create spare files -- the reading content will be zeros
    # For debug purposes these will be enough

    for((i=0; i<n; i++)); do
        local fn="$(printf "%02d\n" "${i}")"
        local f="${IN_DIR}/${fn}"
        echo "Preparing sparse input file '${f}'"
        dd of="${f}" bs="${size}" seek=1 count=0 2>/dev/null
    done
}

Main()
{
    CreateDir "${BUILD_DIR}"

    cmake -S . -B "${BUILD_DIR}"
    cmake --build "${BUILD_DIR}"

    CreateDir "${IN_DIR}"
    CreateDir "${OUT_DIR}"

    CreateInputFiles
}

Main "$@"
