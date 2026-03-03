#!/usr/bin/env bash
# fetch_nats.sh — Vendor nats.c prebuilt static libraries for use with the NatsClient UE plugin.
#
# Run once from the repo root after cloning:
#   bash Plugins/NatsClient/Source/NatsClient/ThirdParty/fetch_nats.sh
#
# Prerequisites: cmake, git, make/ninja

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NATS_DIR="$SCRIPT_DIR/nats.c"
NATS_VERSION="v3.8.2"

echo "==> Cloning nats.c $NATS_VERSION ..."
if [ ! -d "$NATS_DIR/src" ]; then
  git clone --depth 1 --branch "$NATS_VERSION" https://github.com/nats-io/nats.c.git "$NATS_DIR/src"
fi

build_for() {
  local PLATFORM=$1
  local CMAKE_ARGS=$2
  local OUTLIB=$3
  local LIBNAME=$4

  local BUILD_DIR="$NATS_DIR/build/$PLATFORM"
  local LIB_DIR="$NATS_DIR/lib/$PLATFORM"
  mkdir -p "$BUILD_DIR" "$LIB_DIR"

  cmake -S "$NATS_DIR/src" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DNATS_BUILD_STATIC_LIBS=ON \
    -DNATS_BUILD_STREAMING=OFF \
    -DNATS_BUILD_WITH_TLS=OFF \
    $CMAKE_ARGS

  cmake --build "$BUILD_DIR" --config Release

  cp "$BUILD_DIR/$OUTLIB" "$LIB_DIR/$LIBNAME"
  echo "   -> $LIB_DIR/$LIBNAME"
}

# Copy headers
mkdir -p "$NATS_DIR/include"
cp -r "$NATS_DIR/src/include/." "$NATS_DIR/include/"

# macOS (current host)
build_for "Mac" "" "libnats_static.a" "libnats_static.a"

# Linux cross-compile (requires a Linux toolchain — skip on CI if unavailable)
# build_for "Linux" "-DCMAKE_TOOLCHAIN_FILE=..." "libnats_static.a" "libnats_static.a"

echo ""
echo "Done! Place the nats.c directory at:"
echo "  Plugins/NatsClient/Source/NatsClient/ThirdParty/nats.c/"
echo "Then regenerate project files and rebuild."
