#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="$ROOT/build-win"
DIST_DIR="$ROOT/dist"
JOBS="${JOBS:-2}"

command -v cmake >/dev/null 2>&1 || {
    echo "cmake is not in PATH" >&2
    echo 'Expected e.g.: export PATH="$HOME/tools/cmake/bin:$HOME/tools/llvm-mingw/bin:$PATH"' >&2
    exit 1
}

command -v x86_64-w64-mingw32-clang++ >/dev/null 2>&1 || {
    echo "x86_64-w64-mingw32-clang++ is not in PATH" >&2
    echo 'Expected e.g.: export PATH="$HOME/tools/cmake/bin:$HOME/tools/llvm-mingw/bin:$PATH"' >&2
    exit 1
}

cmake \
    -S "$ROOT" \
    -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$ROOT/cmake/windows-x64.cmake" \
    -DCMAKE_BUILD_TYPE=Release

cmake --build "$BUILD_DIR" -j"$JOBS"

rm -rf "$DIST_DIR/SpaceSim" "$DIST_DIR/SpaceSim-Windows-x64.zip"
mkdir -p "$DIST_DIR"

cmake --install "$BUILD_DIR" \
    --prefix "$DIST_DIR/SpaceSim" \
    --component SpaceSimRuntime

(
    cd "$DIST_DIR"
    cmake -E tar cf SpaceSim-Windows-x64.zip --format=zip SpaceSim
)

echo
echo "Windows build ready:"
echo "$DIST_DIR/SpaceSim-Windows-x64.zip"
