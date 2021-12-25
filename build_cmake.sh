#!/bin/bash

# Useful if you willing to stop on first error, also prints what is executed.
#set -ex

BUILDIR="${BUILDIR:-build}" # Default build dir.

# Define target platforms, feel free to comment out if you does not require some of it,
# or you can call this script with plaforms list you willing to build on the command line.
DEFAULT_PLATFORMS=(
	linux-amd64
	linux-aarch64
	linux-armhf
	linux-i686
	windows-x64
	windows-x86
	qvm
)
PLATFORMS=("${@:-${DEFAULT_PLATFORMS[@]}}")

# CMake option for bot support, CMake default used if not specified here.
BOT_SUPPORT="${BOT_SUPPORT:-}"
[ ! -z ${BOT_SUPPORT} ] && BOT_SUPPORT="-DBOT_SUPPORT=${BOT_SUPPORT}"

# If V variable is not empty then provide -v argument to cmake --build command (verbose output).
V="${V:-}"
[ ! -z ${V} ] && V="-v"

# Overwrite build type with B variable.
B="${B:-Release}"
[ ! -z ${B} ] && BUILD="-DCMAKE_BUILD_TYPE=${B}"

# The maximum number of concurrent processes to use when building.
export CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-8}"

# Use specified (with G variable) CMake generator or use default generator (most of the time its make) or ninja if found.
G="${G:-}"
[ -z "${G}" ] && hash ninja >/dev/null 2>&1 && G="Ninja"
[ ! -z "${G}" ] && GENERATOR=("-G" "${G}")

rm -rf ${BUILDIR}
mkdir -p ${BUILDIR}

# Build platforms one by one.
for name in "${PLATFORMS[@]}"; do
	P="${BUILDIR}/$name"
	mkdir -p "${P}"
	case "${name}" in
	"qvm" ) # Build QVM library.
		cmake -B "${P}" -S . ${BOT_SUPPORT} "${GENERATOR[@]}"
		cmake --build "${P}" --target qvm ${V}
	;;
	* ) # Build native library.
		cmake -B "${P}" -S . ${BOT_SUPPORT} ${BUILD} "${GENERATOR[@]}" -DCMAKE_TOOLCHAIN_FILE=tools/cross-cmake/${name}.cmake
		cmake --build "${P}" ${V}
	;;
	esac
done
