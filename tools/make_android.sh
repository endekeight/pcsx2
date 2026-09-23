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
mkdir -p $LIBS_DIR
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

# CMAKE_INSTALL_PREFIX is what places the headers, so it points at the header root.
INSTALLDIR=$YAAPSECORE_LIBS

cmake   -DUSE_OPENGL=1 \
        -DUSE_VULKAN=1 \
        -DWEBP_ENABLE_SIMD=ON \
        -DENABLE_SETCAP=OFF \
        -DX11_API=0 \
        -DCMAKE_C_COMPILER=clang \
        -DCMAKE_CXX_COMPILER=clang++ \
        -DUSE_BACKTRACE=0 \
        `# RelWithDebInfo, not Debug: the shipping build compiles PCSX2's debug` \
        `# assertions out (A15). PCSX2_DEVBUILD, PCSX2_DEBUG and _DEBUG come only` \
        `# from $<$<CONFIG:Debug>:...> and change class layout - GSTexture gains a` \
        `# member and a virtual under PCSX2_DEVBUILD - so AndroidSX2 and yaapsecore` \
        `# must build RelWithDebInfo too. The flags repeat what Debug produced, with` \
        `# no -DNDEBUG; VIXL_DEBUG is passed here because vixl sets it only for Debug.` \
        `# Debug keeps its -O2 flags for debugging builds.` \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DCMAKE_C_FLAGS_RELWITHDEBINFO="-fno-limit-debug-info -g -O2 -fno-strict-aliasing" \
        -DCMAKE_CXX_FLAGS_RELWITHDEBINFO="-fno-limit-debug-info -g -O2 -fno-strict-aliasing -DVIXL_DEBUG" \
        -DCMAKE_C_FLAGS_DEBUG="-g -O2 -fno-strict-aliasing" \
        -DCMAKE_CXX_FLAGS_DEBUG="-g -O2 -fno-strict-aliasing" \
        -DQT_BUILD=OFF \
        -DCMAKE_PREFIX_PATH="$DIR/$DEPS" \
        -DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
        -DANDROID_PLATFORM=android-$ANDROID_SDK \
        -DANDROID_ABI=arm64-v8a \
        -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" \
        -B $BUILD_DIR -G Ninja

cmake --build $BUILD_DIR --parallel
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
