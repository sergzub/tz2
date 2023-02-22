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

    local just1M="$(( 1024 * 1024 ))"
    # local just1G="$(( 1024 * ${just1M} ))"
    local size="$(( 1500 * ${just1M} ))" 
    local n=3

    for((i=0; i<n; i++)); do
        local fn=''
        fn="$(printf "in_%02d\n" "${i}")"
        local f="${IN_DIR}/${fn}"
        
        # echo "Creating sparse input file '${f}'"
        # dd of="${f}" bs="${size}" seek=1 count=0 # 2>/dev/null
        
        # echo "Creating input file '${f}' from /dev/urandom"
        # dd of="${f}" bs="${size}" if=/dev/urandom count=1

        echo "Creating input file '${f}' (${size} bytes)..."
        cat /dev/urandom | base64 | head -c "${size}" > "${f}"
    done
}

Main()
{
    CreateDir "${BUILD_DIR}"
    cmake -S . -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Debug
    cmake --build "${BUILD_DIR}"

    PrepareDir "${OUT_DIR}"
    PrepareDir "${SWAP_DIR}"

    # Comment the line below to prevent generating input files
    CreateInputFiles
}

Main "$@"
