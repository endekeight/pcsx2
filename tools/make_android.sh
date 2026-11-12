#!/bin/bash

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$DIR"

ANDROID_PROJECT=AndroidSX2
ANDROID_LIBS_DIR=../$ANDROID_PROJECT/app/libs

# This build produces two payloads with different lifetimes, so they go to different
# places:
#
#   headers  (~19 MB) -> yaapsecore. yaapsecore COMPILES against them; it is an OBJECT
#                        library and never links libpcsx2.a. It is also the permanent
#                        asset - frontends come and go, so the frontend must not own the
#                        headers the core needs to build.
#   archives (~767 MB) -> the frontend. Only the application links them.
#
# Every consumer then points its include path at YAAPSECORE_LIBS/include: yaapsecore's own
# host test suite, AndroidSX2, and any later frontend. That keeps INTEGRATION.md's rule -
# exactly one source of pcsx2 headers, no fallback search paths - true by construction.
#
# Both are overridable so a different layout needs no edit here.
YAAPSECORE_ROOT=${YAAPSECORE_ROOT:-../yaapsecore}
YAAPSECORE_LIBS=${YAAPSECORE_LIBS:-$YAAPSECORE_ROOT/libs}
LIBS_INCLUDE=$YAAPSECORE_LIBS/include

LIBS_ROOT=../$ANDROID_PROJECT/app/src/main/cpp/libs
LIBS_DIR=$LIBS_ROOT/arm64-v8a

BUILD_DIR=build/arm64/android
BUILD_DIR_16K=build/arm64/android-16k
DEPS=deps/android
OS=$(uname)
NDK=28.0.12674087
ANDROID_SDK=33
SDL_VERSION=3.2.10

if [ -f "$DEPS/lib/cmake/ryml/rymlConfig.cmake" ]; then
    echo "The $DEPS folder contains the required dependencies. Skip building dependencies."
else
    echo "The $DEPS folder is missing required dependencies. Start building dependencies."
    .github/workflows/scripts/linux/build-dependencies-android.sh $DEPS
fi

if [ -d "$BUILD_DIR" ]; then
  	echo "$BUILD_DIR does exist. Clean it up"
	rm -rf $BUILD_DIR/*
else
	mkdir -p $BUILD_DIR
fi

if [ -d "$LIBS_ROOT" ]; then
  	echo "$LIBS_ROOT does exist. Clean it up"
	rm -rf $LIBS_ROOT/*
else
	mkdir -p $LIBS_ROOT
fi

# The header tree is regenerated wholesale, so a stale header from a previous revision
# cannot survive into the new one.
if [ -d "$YAAPSECORE_LIBS" ]; then
	echo "$YAAPSECORE_LIBS does exist. Clean it up"
	rm -rf $YAAPSECORE_LIBS/*
else
	mkdir -p $YAAPSECORE_LIBS
fi
mkdir -p $LIBS_INCLUDE

if [ "$OS" = "Linux" ]; then
    echo "Building on Linux"
    ANDROID_SDK_PATH=~/Android/Sdk
elif [ "$OS" = "Darwin" ]; then
    echo "Building on macOS"
    ANDROID_SDK_PATH=~/Library/Android/sdk
else
    echo "Unsupported platform"
    exit 1
fi

NDK_TOOLCHAIN_PATH=$ANDROID_SDK_PATH/ndk/$NDK/build/cmake/android.toolchain.cmake

# Diagnostic only (A20): YAAPSE_ASAN=1 builds the archives with upstream's USE_ASAN
# switch and a shared C++ runtime, as the NDK's ASan guide requires, and leaves a
# USE_ASAN marker beside them. The frontend refuses to link an ASan archive set
# without its own ASan build, or the reverse: a one-sided sanitizer build links and
# corrupts memory silently. Unset, nothing below changes.
ASAN_ARGS=()
if [ "${YAAPSE_ASAN:-0}" = "1" ]; then
	echo "YAAPSE_ASAN=1: building with -DUSE_ASAN=ON"
	# Precompiled headers are off: with -fsanitize=address, NDK 28's clang
	# rejects the PCH's va_list against libc++'s ("non-const lvalue reference
	# to type '__builtin_va_list' cannot bind to a value of unrelated type
	# 'va_list'"); the same file compiles without the PCH.
	ASAN_ARGS=(-DUSE_ASAN=ON -DANDROID_STL=c++_shared -DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON)
fi

# C128: shipping build options (assertions out, LTO, PGO). The shipping build is
# upstream's release build since C128 (arm B3: assertions out + LTO + profile use).
# Defaults: YAAPSE_ASSERTS_OFF=1, YAAPSE_LTO=1, YAAPSE_PGO=use.
# YAAPSE_ASSERTS_OFF=0, YAAPSE_LTO=0 and YAAPSE_PGO=off turn each part off;
# YAAPSE_ASSERTS_OFF=0 YAAPSE_LTO=0 YAAPSE_PGO=off gives the pre-C128 build.
# The profile lives outside git at .bridge/pgo/yaapse.profdata, recorded by
# tools/pgo_profile.sh, regenerated before a release build and whenever a merged
# task changes GS or yaapsecore sources.
# With YAAPSE_ASAN=1, LTO and PGO default to off instead.
YAAPSE_ASSERTS_OFF="${YAAPSE_ASSERTS_OFF:-1}"
if [ "${YAAPSE_ASAN:-0}" = "1" ]; then
	YAAPSE_LTO="${YAAPSE_LTO:-0}"
	YAAPSE_PGO="${YAAPSE_PGO:-off}"
else
	YAAPSE_LTO="${YAAPSE_LTO:-1}"
	YAAPSE_PGO="${YAAPSE_PGO:-use}"
fi
YAAPSE_PGO_PROFILE="${YAAPSE_PGO_PROFILE:-}"

if [ "${YAAPSE_ASAN:-0}" = "1" ]; then
	if [ "$YAAPSE_LTO" = "1" ] || [ "$YAAPSE_PGO" = "generate" ] || [ "$YAAPSE_PGO" = "use" ]; then
		echo "make_android.sh: YAAPSE_ASAN=1 cannot be combined with YAAPSE_LTO=1 or YAAPSE_PGO"
		exit 1
	fi
fi

if [ "$YAAPSE_PGO" != "off" ] && [ "$YAAPSE_PGO" != "generate" ] && [ "$YAAPSE_PGO" != "use" ]; then
	echo "make_android.sh: invalid YAAPSE_PGO value '$YAAPSE_PGO' (expected 'off', 'generate' or 'use')"
	exit 1
fi

if [ "$YAAPSE_ASSERTS_OFF" = "1" ]; then
	RELWITHDEBINFO_C_FLAGS="-fno-limit-debug-info -g -O2 -fno-strict-aliasing -DNDEBUG"
	RELWITHDEBINFO_CXX_FLAGS="-fno-limit-debug-info -g -O2 -fno-strict-aliasing -DNDEBUG"
	ASSERTS_OFF_CMAKE="ON"
else
	RELWITHDEBINFO_C_FLAGS="-fno-limit-debug-info -g -O2 -fno-strict-aliasing"
	RELWITHDEBINFO_CXX_FLAGS="-fno-limit-debug-info -g -O2 -fno-strict-aliasing -DVIXL_DEBUG"
	ASSERTS_OFF_CMAKE="OFF"
fi

LTO_ARGS=()
if [ "$YAAPSE_LTO" = "1" ]; then
	LTO_ARGS=(-DLTO_PCSX2_CORE=ON)
	LTO_CMAKE="ON"
else
	LTO_CMAKE="OFF"
fi

PGO_MODE="off"
PGO_FLAGS=""
PGO_PROFILE_PATH=""
PGO_PROFILE_ID=""

if [ "$YAAPSE_PGO" = "generate" ]; then
	PGO_MODE="generate"
	PGO_FLAGS="-fprofile-generate"
elif [ "$YAAPSE_PGO" = "use" ]; then
	if [ -n "$YAAPSE_PGO_PROFILE" ]; then
		if [[ "$YAAPSE_PGO_PROFILE" = /* ]]; then
			PGO_PROFILE_ABS="$YAAPSE_PGO_PROFILE"
		else
			PGO_PROFILE_DIR="$(dirname "$YAAPSE_PGO_PROFILE")"
			if [ -d "$PGO_PROFILE_DIR" ]; then
				PGO_PROFILE_ABS="$(cd "$PGO_PROFILE_DIR" && pwd)/$(basename "$YAAPSE_PGO_PROFILE")"
			else
				PGO_PROFILE_ABS="$(pwd)/$YAAPSE_PGO_PROFILE"
			fi
		fi
	else
		PGO_PROFILE_ABS="$(cd "$DIR/.." && pwd)/.bridge/pgo/yaapse.profdata"
	fi

	if [ ! -f "$PGO_PROFILE_ABS" ]; then
		echo "make_android.sh: no profile at $PGO_PROFILE_ABS; building without a profile (link-time optimisation and release definitions still apply). Record one with tools/pgo_profile.sh"
		PGO_MODE="off"
		PGO_FLAGS=""
		PGO_PROFILE_PATH=""
		PGO_PROFILE_ID=""
	else
		PGO_MODE="use"
		PGO_FLAGS="-fprofile-use=$PGO_PROFILE_ABS -Wno-profile-instr-unprofiled -Wno-profile-instr-out-of-date -Wno-backend-plugin"
		PGO_PROFILE_PATH="$PGO_PROFILE_ABS"
		PGO_PROFILE_ID="$(shasum -a 256 "$PGO_PROFILE_ABS" | cut -c1-16)"
	fi
fi

mkdir -p $LIBS_DIR

cat > "$LIBS_DIR/BUILD_OPTIONS.cmake" <<EOF
# Written by pcsx2/tools/make_android.sh on every run. Do not edit.
set(YAAPSE_ARCHIVES_ASSERTS_OFF $ASSERTS_OFF_CMAKE)
set(YAAPSE_ARCHIVES_LTO $LTO_CMAKE)
set(YAAPSE_ARCHIVES_PGO "$PGO_MODE")
set(YAAPSE_ARCHIVES_PGO_PROFILE "$PGO_PROFILE_PATH")
set(YAAPSE_ARCHIVES_PGO_PROFILE_ID "$PGO_PROFILE_ID")
EOF

echo "make_android.sh: build options asserts_off=$ASSERTS_OFF_CMAKE lto=$LTO_CMAKE pgo=$PGO_MODE profile=${PGO_PROFILE_PATH:-none} id=${PGO_PROFILE_ID:-none}"

# CMAKE_INSTALL_PREFIX is what places the headers, so it points at the header root.
INSTALLDIR=$YAAPSECORE_LIBS

# Builds the tree once for one host page size. The value is compiled in and sets how much
# compiled EE code one write fault discards, so the app carries one library per page size.
configure_and_build() {
local build_dir=$1
local page_size_flag=-DOVERRIDE_HOST_PAGE_SIZE=$2

cmake   -DUSE_OPENGL=1 \
        -DUSE_VULKAN=1 \
        -DWEBP_ENABLE_SIMD=ON \
        -DENABLE_SETCAP=OFF \
        -DX11_API=0 \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DUSE_BACKTRACE=0 \
        `# RelWithDebInfo, not Debug: PCSX2_DEVBUILD, PCSX2_DEBUG and _DEBUG come` \
        `# only from $<$<CONFIG:Debug>:...> and change class layout - GSTexture gains a` \
        `# member and a virtual under PCSX2_DEVBUILD - so AndroidSX2 and yaapsecore` \
        `# must build RelWithDebInfo too. The default shipping build (YAAPSE_ASSERTS_OFF=1)` \
        `# builds upstream's release definitions (NDEBUG, no VIXL_DEBUG).` \
        `# For the YAAPSE_ASSERTS_OFF=0 build, the flags repeat what Debug produced,` \
        `# with no -DNDEBUG; VIXL_DEBUG is passed here because vixl sets it only for Debug.` \
        `# Debug keeps its -O2 flags for debugging builds.` \
        `# The frontend and yaapsecore read the same choice from BUILD_OPTIONS.cmake,` \
        `# because all three libraries must agree.` \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_C_FLAGS_RELWITHDEBINFO="$RELWITHDEBINFO_C_FLAGS" \
        -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="$RELWITHDEBINFO_CXX_FLAGS" \
        -DCMAKE_C_FLAGS_DEBUG="-g -O2 -fno-strict-aliasing" \
        -DCMAKE_CXX_FLAGS_DEBUG="-g -O2 -fno-strict-aliasing" \
        -DQT_BUILD=OFF \
        -DCMAKE_C_FLAGS="$page_size_flag${PGO_FLAGS:+ $PGO_FLAGS}" \
        -DCMAKE_CXX_FLAGS="$page_size_flag${PGO_FLAGS:+ $PGO_FLAGS}" \
        -DCMAKE_PREFIX_PATH="$DIR/$DEPS" \
        -DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
        -DANDROID_PLATFORM=android-$ANDROID_SDK \
        -DANDROID_ABI=arm64-v8a \
        -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" \
        "${ASAN_ARGS[@]}" \
        "${LTO_ARGS[@]}" \
        -B "$build_dir" -G Ninja

cmake --build "$build_dir" --parallel
}

configure_and_build "$BUILD_DIR" 0x1000
ninja -C $BUILD_DIR install

cp -f $BUILD_DIR/pcsx2/libpcsx2.a $LIBS_DIR/libpcsx2.a
cp -f $BUILD_DIR/common/libcommon.a $LIBS_DIR/libcommon.a

cp -f $BUILD_DIR/3rdparty/ccc/libccc.a $LIBS_DIR/libccc.a
cp -f $BUILD_DIR/3rdparty/cpuinfo/libcpuinfo.a $LIBS_DIR/libcpuinfo.a
cp -f $BUILD_DIR/3rdparty/cubeb/libcubeb.a $LIBS_DIR/libcubeb.a
# fmt appends a 'd' only for Debug builds, so accept either name and install it
# under the name the Android CMakeLists links against.
if [ -f "$BUILD_DIR/3rdparty/fmt/libfmtd.a" ]; then
  cp -f "$BUILD_DIR/3rdparty/fmt/libfmtd.a" "$LIBS_DIR/libfmtd.a"
else
  cp -f "$BUILD_DIR/3rdparty/fmt/libfmt.a" "$LIBS_DIR/libfmtd.a"
fi
cp -f $BUILD_DIR/3rdparty/glad/libglad.a $LIBS_DIR/libglad.a
cp -f $BUILD_DIR/3rdparty/imgui/libimgui.a $LIBS_DIR/libimgui.a
cp -f $BUILD_DIR/3rdparty/libzip/libzip.a $LIBS_DIR/libzip.a
cp -f $BUILD_DIR/3rdparty/lzma/libpcsx2-lzma.a $LIBS_DIR/libpcsx2-lzma.a
cp -f $BUILD_DIR/3rdparty/rcheevos/librcheevos.a $LIBS_DIR/librcheevos.a
cp -f $BUILD_DIR/3rdparty/simpleini/libsimpleini.a $LIBS_DIR/libsimpleini.a
cp -f $BUILD_DIR/3rdparty/soundtouch/libpcsx2-soundtouch.a $LIBS_DIR/libpcsx2-soundtouch.a
cp -f $BUILD_DIR/3rdparty/discord-rpc/libdiscord-rpc.a $LIBS_DIR/libdiscord-rpc.a
cp -f $BUILD_DIR/3rdparty/libchdr/liblibchdr.a $LIBS_DIR/liblibchdr.a
cp -f $BUILD_DIR/3rdparty/vixl/libvixl.a $LIBS_DIR/libvixl.a
cp -f $BUILD_DIR/3rdparty/freesurround/libfreesurround.a $LIBS_DIR/libfreesurround.a
cp -f $BUILD_DIR/3rdparty/demangler/libdemanglegnu.a $LIBS_DIR/libdemanglegnu.a

if [ "$YAAPSE_LTO" = "1" ]; then
	cp -f $BUILD_DIR/pcsx2/libPCSX2_LTO.a $LIBS_DIR/libpcsx2_lto.a
fi

# The 16 KB library. Only pcsx2 and common compile against the page size, so these are the
# only archives that differ; both libraries share every third-party archive.
rm -rf "$BUILD_DIR_16K"
configure_and_build "$BUILD_DIR_16K" 0x4000
mkdir -p $LIBS_DIR/16k
cp -f $BUILD_DIR_16K/pcsx2/libpcsx2.a $LIBS_DIR/16k/libpcsx2.a
cp -f $BUILD_DIR_16K/common/libcommon.a $LIBS_DIR/16k/libcommon.a

if [ "$YAAPSE_LTO" = "1" ]; then
	cp -f $BUILD_DIR_16K/pcsx2/libPCSX2_LTO.a $LIBS_DIR/16k/libpcsx2_lto.a
fi

if [ "${YAAPSE_ASAN:-0}" = "1" ]; then
	touch $LIBS_DIR/USE_ASAN
fi

# ======== Deps

cp -f $DEPS/lib/libwebp.a $LIBS_DIR/libwebp.a
cp -f $DEPS/lib/libzstd.a $LIBS_DIR/libzstd.a
cp -f $DEPS/lib/libjpeg.a $LIBS_DIR/libjpeg.a
cp -f $DEPS/lib/libpng.a $LIBS_DIR/libpng.a
cp -f $DEPS/lib/liblz4.a $LIBS_DIR/liblz4.a
cp -f $DEPS/lib/libfreetype.a $LIBS_DIR/libfreetype.a
cp -f $DEPS/lib/libSDL3.a $LIBS_DIR/libSDL3.a
cp -f $DEPS/lib/libsharpyuv.a $LIBS_DIR/libsharpyuv.a
cp -f $DEPS/lib/libpcap.a $LIBS_DIR/libpcap.a
cp -f $DEPS/lib/libcurl.a $LIBS_DIR/libcurl.a
cp -f $DEPS/lib/libshaderc_combined.a $LIBS_DIR/libshaderc_combined.a
cp -f $DEPS/lib/libplutovg.a $LIBS_DIR/libplutovg.a
cp -f $DEPS/lib/libplutosvg.a $LIBS_DIR/libplutosvg.a
cp -f $DEPS/lib/libryml.a $LIBS_DIR/libryml.a
cp -f $DEPS/lib/libc4core.a $LIBS_DIR/libc4core.a
cp -f $DEPS/share/java/SDL3/SDL3-$SDL_VERSION.jar $ANDROID_LIBS_DIR/SDL3.jar
