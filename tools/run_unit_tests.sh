#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Default to host architecture
HOST_ARCH="$(uname -m)"
if [[ "$HOST_ARCH" == "aarch64" ]]; then
  HOST_ARCH="arm64"
fi
TARGET_ARCH="$HOST_ARCH"
EXTRA_ARGS=()

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
      echo "Usage: $0 [arm64|x86_64] [--arch=arm64|x86_64] [gtest options...]"
      echo ""
      echo "Options:"
      echo "  arm64, --arch=arm64    Run unit tests targeting ARM64 (Native Apple Silicon)"
      echo "  x86_64, --arch=x86_64  Run unit tests targeting x86_64 (via Rosetta on Apple Silicon)"
      echo "  --gtest_filter=...     Filter tests by pattern"
      echo ""
      echo "Examples:"
      echo "  $0 arm64"
      echo "  $0 x86_64"
      echo "  $0 --arch=arm64 --gtest_filter='Path.*'"
      exit 0
      ;;
    *)
      EXTRA_ARGS+=("$1")
      shift
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

BUILD_DIR="$DIR/build/unittests/$TARGET_ARCH"
mkdir -p "$BUILD_DIR"

ARCH_FLAGS="-arch $TARGET_ARCH"

echo "=== Building & Running PCSX2 Unit Tests (Architecture: $TARGET_ARCH) ==="

# 1. Build common_test
echo ""
echo "[1/2] Compiling and running common_test [$TARGET_ARCH]..."
clang++ -std=c++20 $ARCH_FLAGS \
  -I"$DIR" \
  -I"$DIR/common" \
  -I"$DIR/pcsx2" \
  -I"$DIR/3rdparty/include" \
  -I"$DIR/3rdparty/fmt/include" \
  -I"$DIR/3rdparty/fast_float/include" \
  -I"$DIR/3rdparty/googletest/googletest/include" \
  -I"$DIR/3rdparty/googletest/googletest" \
  -I"$DIR/3rdparty/cpuinfo/include" \
  "$DIR/3rdparty/googletest/googletest/src/gtest-all.cc" \
  "$DIR/3rdparty/googletest/googletest/src/gtest_main.cc" \
  "$DIR/3rdparty/fmt/src/format.cc" \
  "$DIR/3rdparty/fmt/src/os.cc" \
  "$DIR/common/Assertions.cpp" \
  "$DIR/common/Console.cpp" \
  "$DIR/common/Error.cpp" \
  "$DIR/common/FileSystem.cpp" \
  "$DIR/common/HostSys.cpp" \
  "$DIR/common/CrashHandler.cpp" \
  "$DIR/common/MemoryInterface.cpp" \
  "$DIR/common/ProgressCallback.cpp" \
  "$DIR/common/SmallString.cpp" \
  "$DIR/common/StringUtil.cpp" \
  "$DIR/common/Timer.cpp" \
  "$DIR/common/Darwin/DarwinMisc.cpp" \
  "$DIR/common/Darwin/DarwinThreads.cpp" \
  "$DIR/tests/ctest/TestStubs.cpp" \
  "$DIR/tests/ctest/common/byteswap_tests.cpp" \
  "$DIR/tests/ctest/common/path_tests.cpp" \
  "$DIR/tests/ctest/common/small_string_tests.cpp" \
  "$DIR/tests/ctest/common/string_util_tests.cpp" \
  -framework Foundation -framework Cocoa -framework IOKit \
  -o "$BUILD_DIR/common_test"

"$BUILD_DIR/common_test" "${EXTRA_ARGS[@]}"

# 2. Build core patch_test
echo ""
echo "[2/2] Compiling and running core patch_tests [$TARGET_ARCH]..."
clang++ -std=c++20 $ARCH_FLAGS \
  -I"$DIR" \
  -I"$DIR/common" \
  -I"$DIR/pcsx2" \
  -I"$DIR/3rdparty/include" \
  -I"$DIR/3rdparty/fmt/include" \
  -I"$DIR/3rdparty/fast_float/include" \
  -I"$DIR/3rdparty/googletest/googletest/include" \
  -I"$DIR/3rdparty/googletest/googlemock/include" \
  -I"$DIR/3rdparty/googletest/googletest" \
  -I"$DIR/3rdparty/googletest/googlemock" \
  -I"$DIR/3rdparty/cpuinfo/include" \
  -I"$DIR/3rdparty/libzip/lib" \
  -I"$DIR/3rdparty/libzip/msvc" \
  -I"$DIR/3rdparty/imgui/include" \
  "$DIR/3rdparty/googletest/googletest/src/gtest-all.cc" \
  "$DIR/3rdparty/googletest/googlemock/src/gmock-all.cc" \
  "$DIR/3rdparty/googletest/googletest/src/gtest_main.cc" \
  "$DIR/3rdparty/fmt/src/format.cc" \
  "$DIR/3rdparty/fmt/src/os.cc" \
  "$DIR/common/Assertions.cpp" \
  "$DIR/common/Console.cpp" \
  "$DIR/common/Error.cpp" \
  "$DIR/common/FileSystem.cpp" \
  "$DIR/common/HostSys.cpp" \
  "$DIR/common/CrashHandler.cpp" \
  "$DIR/common/MemoryInterface.cpp" \
  "$DIR/common/ProgressCallback.cpp" \
  "$DIR/common/SmallString.cpp" \
  "$DIR/common/StringUtil.cpp" \
  "$DIR/common/Timer.cpp" \
  "$DIR/common/Darwin/DarwinMisc.cpp" \
  "$DIR/common/Darwin/DarwinThreads.cpp" \
  "$DIR/tests/ctest/TestStubs.cpp" \
  "$DIR/tests/ctest/core/StubHost.cpp" \
  "$DIR/pcsx2/Patch.cpp" \
  "$DIR/tests/ctest/core/patch_tests.cpp" \
  -framework Foundation -framework Cocoa -framework IOKit \
  -o "$BUILD_DIR/core_test"

"$BUILD_DIR/core_test" "${EXTRA_ARGS[@]}"

# 3. Build dynarec_test (only when targeting arm64)
if [[ "$TARGET_ARCH" == "arm64" ]]; then
  echo ""
  echo "[3/3] Compiling and running dynarec_test [$TARGET_ARCH]..."
  clang++ -std=c++20 $ARCH_FLAGS -Wno-deprecated-enum-enum-conversion \
    -DVIXL_INCLUDE_TARGET_AARCH64=1 \
    -DVIXL_CODE_BUFFER_MALLOC=1 \
    -I"$DIR" \
    -I"$DIR/common" \
    -I"$DIR/pcsx2" \
    -I"$DIR/../AndroidSX2/app/src/main/cpp" \
    -I"$DIR/3rdparty/include" \
    -I"$DIR/3rdparty/vixl/include" \
    -I"$DIR/3rdparty/vixl/include/vixl" \
    -I"$DIR/3rdparty/vixl/include/vixl/aarch64" \
    -I"$DIR/3rdparty/vixl/src" \
    -I"$DIR/3rdparty/vixl/src/aarch64" \
    -I"$DIR/3rdparty/fmt/include" \
    -I"$DIR/3rdparty/fast_float/include" \
    -I"$DIR/3rdparty/googletest/googletest/include" \
    -I"$DIR/3rdparty/googletest/googletest" \
    -I"$DIR/3rdparty/cpuinfo/include" \
    "$DIR/3rdparty/googletest/googletest/src/gtest-all.cc" \
    "$DIR/3rdparty/googletest/googletest/src/gtest_main.cc" \
    "$DIR/3rdparty/fmt/src/format.cc" \
    "$DIR/3rdparty/fmt/src/os.cc" \
    "$DIR/common/Assertions.cpp" \
    "$DIR/common/Console.cpp" \
    "$DIR/common/AlignedMalloc.cpp" \
    "$DIR/common/Error.cpp" \
    "$DIR/common/FileSystem.cpp" \
    "$DIR/common/HostSys.cpp" \
    "$DIR/common/CrashHandler.cpp" \
    "$DIR/common/MemoryInterface.cpp" \
    "$DIR/common/ProgressCallback.cpp" \
    "$DIR/common/SmallString.cpp" \
    "$DIR/common/StringUtil.cpp" \
    "$DIR/common/Timer.cpp" \
    "$DIR/common/Darwin/DarwinMisc.cpp" \
    "$DIR/common/Darwin/DarwinThreads.cpp" \
    "$DIR/tests/ctest/TestStubs.cpp" \
    "$DIR/pcsx2/arm64/AsmHelpers.cpp" \
    "$DIR/3rdparty/vixl/src/utils-vixl.cc" \
    "$DIR/3rdparty/vixl/src/code-buffer-vixl.cc" \
    "$DIR/3rdparty/vixl/src/compiler-intrinsics-vixl.cc" \
    "$DIR/3rdparty/vixl/src/cpu-features.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/assembler-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/cpu-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/decoder-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/disasm-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/instructions-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/logic-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/macro-assembler-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/operands-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/pointer-auth-aarch64.cc" \
    "$DIR/3rdparty/vixl/src/aarch64/registers-aarch64.cc" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/shared/BaseBlocks.cpp" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/shared/InstructionInfo.cpp" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/arm64/Arm64Emitter.cpp" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/arm64/Arm64RegAlloc.cpp" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/arm64/Arm64Dispatcher.cpp" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/arm64/Arm64VtlbBackpatch.cpp" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/ee/EEBlockManager.cpp" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/ee/EEConstProp.cpp" \
    "$DIR/../AndroidSX2/app/src/main/cpp/dynarec/iop/IOPBlockManager.cpp" \
    "$DIR/../tests/dynarec/patch_tests.cpp" \
    "$DIR/../tests/dynarec/regalloc_tests.cpp" \
    "$DIR/../tests/dynarec/ee_block_tests.cpp" \
    "$DIR/../tests/dynarec/iop_block_tests.cpp" \
    "$DIR/../tests/dynarec/alu_tests.cpp" \
    -framework Foundation -framework Cocoa -framework IOKit \
    -o "$BUILD_DIR/dynarec_test"

  "$BUILD_DIR/dynarec_test" "${EXTRA_ARGS[@]}"
fi

echo ""
echo "=== All Unit Tests Passed for [$TARGET_ARCH]! ==="
