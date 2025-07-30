#!/bin/sh
#
# Use this script to run your program LOCALLY.
#
# Note: Changing this script WILL NOT affect how CodeCrafters runs your program.
#
# Learn more: https://codecrafters.io/program-interface

set -e # Exit early if any commands fail

# Copied from .codecrafters/compile.sh
#
# - Edit this to change how your program compiles locally
# - Edit .codecrafters/compile.sh to change how your program compiles remotely

# ensure VCPKG_ROOT is set (or default to ~/vcpkg)
export VCPKG_ROOT=${VCPKG_ROOT:-"$HOME/vcpkg"}

# sanity‐check the toolchain file
if [ ! -f "$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" ]; then
  echo "Error: vcpkg toolchain not found at $VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
  exit 1
fi

# select a generator (you can install ninja-build or use Unix Makefiles)
cmake \
  -B build \
  -S . \
  -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"


(
  cd "$(dirname "$0")" # Ensure compile steps are run within the repository directory
  cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
  cmake --build ./build
)

# Copied from .codecrafters/run.sh
#
# - Edit this to change how your program runs locally
# - Edit .codecrafters/run.sh to change how your program runs remotely
exec $(dirname $0)/build/http-server "$@"
