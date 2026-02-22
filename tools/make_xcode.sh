#!/bin/bash

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

export PATH="/opt/homebrew/bin:/usr/local/bin:$PATH"

TARGET_ARCH="x86_64"

# Parse arguments
while [[ $# -gt 0 ]]; do
  case "$1" in
    --arch=*)
      TARGET_ARCH="${1#*=}"
      shift
      ;;
    -a|--arch)
      TARGET_ARCH="$2"
      shift 2
      ;;
    arm64|aarch64)
      TARGET_ARCH="arm64"
      shift
      ;;
    x86|x86_64|x64)
      TARGET_ARCH="x86_64"
      shift
      ;;
    -h|--help)
      echo "Usage: $0 [x86_64|arm64] [--arch=x86_64|arm64]"
      echo ""
      echo "Options:"
      echo "  x86_64, --arch=x86_64  Generate Xcode project for x86_64 (Default)"
      echo "  arm64, --arch=arm64    Generate Xcode project for ARM64"
      exit 0
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [x86_64|arm64] [--arch=x86_64|arm64]"
      exit 1
      ;;
  esac
done

# Normalize arch
if [[ "$TARGET_ARCH" == "x86" || "$TARGET_ARCH" == "x64" ]]; then
  TARGET_ARCH="x86_64"
elif [[ "$TARGET_ARCH" == "aarch64" ]]; then
  TARGET_ARCH="arm64"
fi

if [[ "$TARGET_ARCH" != "arm64" && "$TARGET_ARCH" != "x86_64" ]]; then
  echo "Error: Unsupported architecture '$TARGET_ARCH'. Choose 'arm64' or 'x86_64'."
  exit 1
fi

DEPS="deps/$TARGET_ARCH"

if [ -f "$DEPS/lib/cmake/ryml/rymlConfig.cmake" ]; then
    echo "The $DEPS folder contains the required dependencies. Skip building dependencies."
else
    echo "The $DEPS folder is missing required dependencies. Start building dependencies."
    if [ "$TARGET_ARCH" = "x86_64" ]; then
        arch -x86_64 .github/workflows/scripts/macos/build-dependencies.sh "$DEPS"
    else
        .github/workflows/scripts/macos/build-dependencies-universal.sh "$DEPS"
    fi
fi

BUILD_DIR="build/$TARGET_ARCH/xcode"

if [ -d "$BUILD_DIR" ]; then
  	echo "$BUILD_DIR does exist. Cleaning it up..."
	rm -rf $BUILD_DIR/*
else
	echo "Creating $BUILD_DIR"
	mkdir -p $BUILD_DIR
fi

CMAKE_EXTRA_ARGS=()
if [ "$TARGET_ARCH" = "x86_64" ] && [ "$(uname -m)" = "arm64" ]; then
    CMAKE_EXTRA_ARGS+=("-DIS_ROSETTA=1")
fi

cmake -B "$BUILD_DIR" \
	"${CMAKE_EXTRA_ARGS[@]}" \
	-DCMAKE_OSX_ARCHITECTURES="$TARGET_ARCH" \
	-DCMAKE_C_COMPILER=clang \
	-DCMAKE_CXX_COMPILER=clang++ \
	-DCMAKE_PREFIX_PATH="$DIR/$DEPS" -G Xcode .
