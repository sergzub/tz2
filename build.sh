#!/bin/bash -eu

BUILD_DIR='build'
IN_DIR="in"
OUT_DIR="out"
SWAP_DIR="swap"

CreateDir()
{
    test -d "${1}" || mkdir "${1}"
}

# ClearDir()
# {
#     rm -rf "${1}" && mkdir "${1}"
# }

PrepareDir()
{
    if [ -d "${1}" ]; then
        rm -rf "${1}"
    fi
    mkdir "${1}"
}

CreateInputFiles()
{
    PrepareDir "${IN_DIR}"

    local just1Mb=1048576
    # local size="$(( 15 * 1024 * ${just1Mb} / 10 ))" 
    local size="$(( 10 * 4096 ))" 
    local n=1 # 10

    # Create spare files -- the reading content will be zeros
    # For debug purposes these will be enough

    for((i=0; i<n; i++)); do
        local fn=''
        fn="$(printf "%02d\n" "${i}")"
        local f="${IN_DIR}/${fn}"
        
        # echo "Creating sparse input file '${f}'"
        # dd of="${f}" bs="${size}" seek=1 count=0 # 2>/dev/null
        
        echo "Creating input file '${f}' from /dev/urandom"
        dd of="${f}" bs="${size}" if=/dev/urandom count=1
    done
}

Main()
{
    CreateDir "${BUILD_DIR}"
    cmake -S . -B "${BUILD_DIR}"
    cmake --build "${BUILD_DIR}"

    PrepareDir "${OUT_DIR}"
    PrepareDir "${SWAP_DIR}"

    # Comment the line below to prevent generating input files
    # CreateInputFiles
}

Main "$@"
