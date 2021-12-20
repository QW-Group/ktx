#!/bin/bash

# Useful if you willing to stop on first error, also print what is executed.
#set -ex

BUILDIR="${BUILDIR:-build}" # Default build dir.

# Default bot support, CMake already have it as ON,
# provided here just for possibility to turn it OFF without modification of CMake or this script file.
BOT_SUPPORT="${BOT_SUPPORT:-ON}"

# If V variable is not empty then provide -v argument to Ninja (verbose output).
V="${V:-}"
[ ! -z ${V} ] && V="-v"

rm -rf ${BUILDIR}
mkdir -p ${BUILDIR}

# Define target platforms, feel free to comment out if you does not require some of it.
BUILD_LIST=(
    linux-amd64
    linux-aarch64
    linux-armhf
    linux-i686
    windows-x64
    windows-x86
    qvm
)

# Build platforms one by one.
for name in "${BUILD_LIST[@]}"; do
    mkdir -p ${BUILDIR}/$name
    case "$name" in
    "qvm" ) # Build QVM library.
        cmake -B ${BUILDIR}/$name -S . -DBOT_SUPPORT=${BOT_SUPPORT} -G Ninja
        cmake --build ${BUILDIR}/$name --config Release --target qvm ${V}
    ;;
    * ) # Build native library.
        cmake -B ${BUILDIR}/$name -S . -DBOT_SUPPORT=${BOT_SUPPORT} -G Ninja -DCMAKE_TOOLCHAIN_FILE=tools/cross-cmake/$name.cmake
        cmake --build ${BUILDIR}/$name --config Release ${V}
    ;;
    esac
done
