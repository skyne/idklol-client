#!/usr/bin/env bash
# fetch_nats.sh — Vendor nats.c prebuilt static libraries for use with the NatsClient UE plugin.
#
# Run once from the repo root after cloning:
#   bash Plugins/NatsClient/Source/NatsClient/ThirdParty/fetch_nats.sh
#
# Prerequisites: git, cmake, make/ninja toolchain

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NATS_DIR="$SCRIPT_DIR/nats.c"
NATS_VERSION="${NATS_VERSION:-v3.8.2}"
CLEAN=0
PLATFORMS=()

usage() {
  echo "Usage: ./fetch_nats.sh [--clean] [--platform Mac|Linux]..." >&2
}

normalize_platform() {
  case "$1" in
    Mac|mac|macOS|darwin)
      echo "Mac"
      ;;
    Linux|linux)
      echo "Linux"
      ;;
    *)
      echo ""
      ;;
  esac
}

detect_default_platform() {
  case "$(uname -s)" in
    Darwin)
      echo "Mac"
      ;;
    Linux)
      echo "Linux"
      ;;
    *)
      echo ""
      ;;
  esac
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --clean)
      CLEAN=1
      shift
      ;;
    --platform)
      if [[ $# -lt 2 ]]; then
        usage
        exit 1
      fi
      PLATFORM="$(normalize_platform "$2")"
      if [[ -z "$PLATFORM" ]]; then
        echo "ERROR: Unsupported platform '$2'. Expected Mac or Linux." >&2
        exit 1
      fi
      PLATFORMS+=("$PLATFORM")
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      usage
      exit 1
      ;;
  esac
done

require_cmd() {
  local CMD="$1"
  if ! command -v "$CMD" >/dev/null 2>&1; then
    echo "ERROR: Required command not found: $CMD" >&2
    echo "Please install '$CMD' and ensure it is in PATH." >&2
    exit 1
  fi
}

require_cmd git
require_cmd cmake

if [[ "${#PLATFORMS[@]}" -eq 0 ]]; then
  DEFAULT_PLATFORM="$(detect_default_platform)"
  if [[ -z "$DEFAULT_PLATFORM" ]]; then
    echo "ERROR: Could not determine a default platform for host OS '$(uname -s)'" >&2
    echo "Use --platform Mac or --platform Linux explicitly." >&2
    exit 1
  fi
  PLATFORMS=("$DEFAULT_PLATFORM")
fi

if [[ "$CLEAN" -eq 1 ]]; then
  echo "==> Cleaning generated nats artifacts ..."
  rm -rf "$NATS_DIR/build" "$NATS_DIR/include" "$NATS_DIR/lib"
fi

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
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_C_FLAGS=-fPIC \
    -DCMAKE_C_FLAGS_RELEASE=-fPIC \
    -DCMAKE_CXX_FLAGS=-fPIC \
    -DCMAKE_CXX_FLAGS_RELEASE=-fPIC \
    -DNATS_BUILD_LIB_STATIC=ON \
    -DNATS_BUILD_LIB_SHARED=OFF \
    -DNATS_BUILD_EXAMPLES=OFF \
    -DBUILD_TESTING=OFF \
    -DNATS_BUILD_STREAMING=OFF \
    -DNATS_BUILD_WITH_TLS=OFF \
    $CMAKE_ARGS

  cmake --build "$BUILD_DIR" --config Release --target nats_static

  local BUILT_LIB=""
  if [ -f "$BUILD_DIR/$OUTLIB" ]; then
    BUILT_LIB="$BUILD_DIR/$OUTLIB"
  elif [ -f "$BUILD_DIR/src/$OUTLIB" ]; then
    BUILT_LIB="$BUILD_DIR/src/$OUTLIB"
  else
    echo "ERROR: Built library not found. Checked:" >&2
    echo "  - $BUILD_DIR/$OUTLIB" >&2
    echo "  - $BUILD_DIR/src/$OUTLIB" >&2
    exit 1
  fi

  cp "$BUILT_LIB" "$LIB_DIR/$LIBNAME"
  echo "   -> $LIB_DIR/$LIBNAME"
}

# Copy headers
mkdir -p "$NATS_DIR/include/nats/adapters"

PUBLIC_HDR_SRC=""
if [ -f "$NATS_DIR/src/src/nats.h" ]; then
  PUBLIC_HDR_SRC="$NATS_DIR/src/src"
elif [ -f "$NATS_DIR/src/nats.h" ]; then
  PUBLIC_HDR_SRC="$NATS_DIR/src"
else
  echo "ERROR: Could not find nats public headers in expected paths:" >&2
  echo "  - $NATS_DIR/src/src/nats.h" >&2
  echo "  - $NATS_DIR/src/nats.h" >&2
  exit 1
fi

# Mirror upstream install layout from src/src/CMakeLists.txt:
#   include/nats.h (deprecated compatibility header)
#   include/nats/{nats.h,status.h,version.h}
#   include/nats/adapters/{libevent.h,libuv.h}
if [ -f "$PUBLIC_HDR_SRC/deprnats.h" ]; then
  cp "$PUBLIC_HDR_SRC/deprnats.h" "$NATS_DIR/include/nats.h"
else
  cp "$PUBLIC_HDR_SRC/nats.h" "$NATS_DIR/include/nats.h"
fi

cp "$PUBLIC_HDR_SRC/nats.h" "$NATS_DIR/include/nats/nats.h"
cp "$PUBLIC_HDR_SRC/status.h" "$NATS_DIR/include/nats/status.h"
cp "$PUBLIC_HDR_SRC/version.h" "$NATS_DIR/include/nats/version.h"

if [ -d "$PUBLIC_HDR_SRC/adapters" ]; then
  if [ -f "$PUBLIC_HDR_SRC/adapters/libevent.h" ]; then
    cp "$PUBLIC_HDR_SRC/adapters/libevent.h" "$NATS_DIR/include/nats/adapters/libevent.h"
  fi
  if [ -f "$PUBLIC_HDR_SRC/adapters/libuv.h" ]; then
    cp "$PUBLIC_HDR_SRC/adapters/libuv.h" "$NATS_DIR/include/nats/adapters/libuv.h"
  fi
fi

for PLATFORM in "${PLATFORMS[@]}"; do
  case "$PLATFORM" in
    Mac)
      build_for "Mac" "" "libnats_static.a" "libnats_static.a"
      ;;
    Linux)
      build_for "Linux" "" "libnats_static.a" "libnats_static.a"
      ;;
  esac
done

# Prevent UnrealBuildTool from picking up vendored nats.c source files under the
# module directory. Keep only include/ and lib/ artifacts in ThirdParty/nats.c.
echo "==> Pruning nats source/build trees (keeping include/ + lib/) ..."
rm -rf "$NATS_DIR/src" "$NATS_DIR/build"

echo ""
echo "Done! Place the nats.c directory at:"
echo "  Plugins/NatsClient/Source/NatsClient/ThirdParty/nats.c/"
echo "Then regenerate project files and rebuild."
