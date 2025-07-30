set -e # Exit early if any commands fail

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

exec $(dirname $0)/build/http-server "$@"
