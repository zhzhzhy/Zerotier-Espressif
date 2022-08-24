#!/usr/bin/env bash


# -----------------------------------------------------------------------------
# | SYSTEM DISCOVERY AND CONFIGURATION                                        |
# -----------------------------------------------------------------------------

check_submodules()
{
    if [ "$(ls -A ext/lwip)" ] && [ "$(ls -A ext/lwip-contrib)" ] && [ "$(ls -A ext/ZeroTierOne)" ]; then
        :
    else
        echo "Submodules seem to be missing. Please run: git submodule update --init"
        exit 1
    fi
}

CLANG_FORMAT=clang-format-11

PYTHON=python3
PIP=pip3

libzt=$(pwd)

# Find and set cmake
CMAKE=cmake3
if [[ $(which $CMAKE) = "" ]];
then
    CMAKE=cmake # try this next
fi
if [[ $(which $CMAKE) = "" ]];
then
    echo "CMake (cmake) not found. Please install before continuing."
    exit
fi

#
if [[ ! $(which tree) = "" ]];
then
    TREE=tree
else
    TREE="du -a "
fi

# Determine operating system
OSNAME=$(uname | tr '[A-Z]' '[a-z]')
if [[ $OSNAME = *"darwin"* ]]; then
    SHARED_LIB_NAME="libzt.dylib"
    STATIC_LIB_NAME="libzt.a"
    HOST_PLATFORM="macos"
fi
if [[ $OSNAME = *"linux"* ]]; then
    SHARED_LIB_NAME="libzt.so"
    STATIC_LIB_NAME="libzt.a"
    HOST_PLATFORM="linux"
fi

# Determine and normalize machine type
HOST_MACHINE_TYPE=$(uname -m)
if [[ $HOST_MACHINE_TYPE = *"x86_64"* ]]; then
    HOST_MACHINE_TYPE="x64"
fi

# Determine number of cores. We'll tell CMake to use them all
if [[ $OSNAME = *"darwin"* ]]; then
    N_PROCESSORS=$(sysctl -n hw.ncpu)
fi
if [[ $OSNAME = *"linux"* ]]; then
    N_PROCESSORS=$(nproc --all)
fi

# How many processor cores CMake should use during builds,
# comment out the below line out if you don't want parallelism:
CMAKE_VERSION=$(cmake --version | head -n 1 | sed 's/[^0-9]*//')
function ver()
# Description: use for comparisons of version strings.
# $1  : a version string of form 1.2.3.4
# use: (( $(ver 1.2.3.4) >= $(ver 1.2.3.3) )) && echo "yes" || echo "no"
# Clever solution from https://stackoverflow.com/users/10682202/christopher
{
    printf "%02d%02d%02d%02d" ${1//./ }
}
if [[ (( $(ver $CMAKE_VERSION) > $(ver "3.12") )) ]]; then
    BUILD_CONCURRENCY="-j $N_PROCESSORS"
fi

# -----------------------------------------------------------------------------
# | PATHS                                                                     |
# -----------------------------------------------------------------------------

# Where we place all finished artifacts
BUILD_OUTPUT_DIR=$(pwd)/dist
# Where we tell CMake to place its build systems and their caches
BUILD_CACHE_DIR=$(pwd)/cache
# Where package projects, scripts, spec files, etc live
PKG_DIR=$(pwd)/pkg
# Default location for (host) libraries
DEFAULT_HOST_LIB_OUTPUT_DIR=$BUILD_OUTPUT_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE
# Default location for (host) binaries
DEFAULT_HOST_BIN_OUTPUT_DIR=$BUILD_OUTPUT_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE
# Default location for (host) packages
DEFAULT_HOST_PKG_OUTPUT_DIR=$BUILD_OUTPUT_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE
# Default location for CMake's caches (when building for host)
DEFAULT_HOST_BUILD_CACHE_DIR=$BUILD_CACHE_DIR/$HOST_PLATFORM-$HOST_MACHINE_TYPE

gethosttype()
{
    echo $HOST_PLATFORM-$HOST_MACHINE_TYPE
}


host()
{
    check_submodules
    ARTIFACT="host"
    # Default to release
    BUILD_TYPE=${1:-release}
    VARIANT="-DBUILD_HOST=True"
    CACHE_DIR=$DEFAULT_HOST_BUILD_CACHE_DIR-$ARTIFACT-$BUILD_TYPE
    TARGET_BUILD_DIR=$DEFAULT_HOST_BIN_OUTPUT_DIR-$ARTIFACT-$BUILD_TYPE
    rm -rf $TARGET_BUILD_DIR
    LIB_OUTPUT_DIR=$TARGET_BUILD_DIR/lib
    BIN_OUTPUT_DIR=$TARGET_BUILD_DIR/bin
    mkdir -p $LIB_OUTPUT_DIR
    mkdir -p $BIN_OUTPUT_DIR
    $CMAKE $VARIANT -H. -B$CACHE_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE
    $CMAKE --build $CACHE_DIR $BUILD_CONCURRENCY
    cp -f $CACHE_DIR/lib/libzt.* $LIB_OUTPUT_DIR
    cp -f $CACHE_DIR/bin/* $BIN_OUTPUT_DIR
    echo -e "\n - Build cache  : $CACHE_DIR\n - Build output : $BUILD_OUTPUT_DIR\n"
    $TREE $TARGET_BUILD_DIR
}

#buildingesp32(){
set -euo pipefail
PARAM=""

# Retrive the target from the current filename, if no target specified,
# the variable will be empty
TARGET=$(echo $0 | cut -s -f2 -d- | cut -s -f1 -d.)
if [[ -n $TARGET ]]
then
    # Target is not null, specify the build parameters
    PARAM="-DCMAKE_TOOLCHAIN_FILE=$IDF_PATH/tools/cmake/toolchain-${TARGET}.cmake -DTARGET=${TARGET} -GNinja"
fi

rm -rf build && mkdir build && cd build
cmake .. $PARAM
cmake --build .
#}
