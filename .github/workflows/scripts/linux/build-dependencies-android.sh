#!/usr/bin/env bash

set -e

if [ "$#" -ne 1 ]; then
    echo "Syntax: $0 <output directory>"
    exit 1
fi

SCRIPTDIR=$(realpath $(dirname "${BASH_SOURCE[0]}"))
NPROCS="$(getconf _NPROCESSORS_ONLN)"
INSTALLDIR="$1"
if [ "${INSTALLDIR:0:1}" != "/" ]; then
	INSTALLDIR="$PWD/$INSTALLDIR"
fi

OS=$(uname)

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

ANDROID_SDK=33
NDK=28.0.12674087
NDK_TOOLCHAIN_PATH=$ANDROID_SDK_PATH/ndk/$NDK/build/cmake/android.toolchain.cmake
echo "NDK Toolchain Path: $NDK_TOOLCHAIN_PATH"

LIBBACKTRACE=ad106d5fdd5d960bd33fae1c48a351af567fd075
LIBJPEG=3.1.0
LIBPNG=1.6.44
LIBWEBP=1.4.0
LZ4=b8fd2d15309dd4e605070bd4486e26b6ef814e29
SDL=SDL3-3.2.10
ZSTD=1.5.6
PCAP=1.10.5
FREETYPE=2-13-3
CURL=8_11_1
PLUTOVG=1.1.0
PLUTOSVG=0.0.7
RAPIDYAML=0.12.1

SHADERC=2024.1
SHADERC_GLSLANG=142052fa30f9eca191aa9dcf65359fcaed09eeec
SHADERC_SPIRVHEADERS=5e3ad389ee56fca27c9705d093ae5387ce404df4
SHADERC_SPIRVTOOLS=dd4b663e13c07fea4fbb3f70c1c91c86731099f7

mkdir -p deps-build
cd deps-build

cat > SHASUMS <<EOF
fd6f417fe9e3a071cf1424a5152d926a34c4a3c5070745470be6cf12a404ed79  $LIBBACKTRACE.zip
60c4da1d5b7f0aa8d158da48e8f8afa9773c1c8baa5d21974df61f1886b8ce8e  libpng-$LIBPNG.tar.xz
61f873ec69e3be1b99535634340d5bde750b2e4447caa1db9f61be3fd49ab1e5  libwebp-$LIBWEBP.tar.gz
0728800155f3ed0a0c87e03addbd30ecbe374f7b080678bbca1506051d50dec3  $LZ4.tar.gz
f87be7b4dec66db4098e9c167b2aa34e2ca10aeb5443bdde95ae03185ed513e0  $SDL.tar.gz
8c29e06cf42aacc1eafc4077ae2ec6c6fcb96a626157e0593d5e82a34fd403c1  zstd-$ZSTD.tar.gz
eb3b5f0c16313d34f208d90c2fa1e588a23283eed63b101edd5422be6165d528  shaderc-$SHADERC.tar.gz
aa27e4454ce631c5a17924ce0624eac736da19fc6f5a2ab15a6c58da7b36950f  shaderc-glslang-$SHADERC_GLSLANG.tar.gz
5d866ce34a4b6908e262e5ebfffc0a5e11dd411640b5f24c85a80ad44c0d4697  shaderc-spirv-headers-$SHADERC_SPIRVHEADERS.tar.gz
03ee1a2c06f3b61008478f4abe9423454e53e580b9488b47c8071547c6a9db47  shaderc-spirv-tools-$SHADERC_SPIRVTOOLS.tar.gz
ba3c0152f14a504018de19c9f62250d8f3351525 libjpeg-turbo-$LIBJPEG.tar.gz
93eefc0f3e55432ee4050c3516f6b5ada6005e0b freetype-VER-$FREETYPE.tar.gz
1073f170ea71c834c0e346a20133d1dba2291b1f libpcap-$PCAP.tar.gz
f15749b82f2208007d563952b80c537be80b2408 curl-$CURL.tar.gz
f0abeb2f8e17db756f6ba5d0bebaaeeb96ae6aeaaeb9698d258afec05b9b77fa plutovg-$PLUTOVG.tar.gz
eadd5a3394a4f89d5cc9d3d3a0c43916964b4c7308d98d8ed2e6ef2f495147ae plutosvg-$PLUTOSVG.tar.gz
e9efcdd17f86287748793cf21d106e461fcad8d103a3e5a23632afe93828660d  rapidyaml-$RAPIDYAML-src.tgz
EOF

curl -L \
	-O "https://github.com/ianlancetaylor/libbacktrace/archive/$LIBBACKTRACE.zip" \
	-O "https://downloads.sourceforge.net/project/libpng/libpng16/$LIBPNG/libpng-$LIBPNG.tar.xz" \
	-O "https://storage.googleapis.com/downloads.webmproject.org/releases/webp/libwebp-$LIBWEBP.tar.gz" \
	-O "https://github.com/lz4/lz4/archive/$LZ4.tar.gz" \
	-O "https://libsdl.org/release/$SDL.tar.gz" \
	-O "https://github.com/facebook/zstd/releases/download/v$ZSTD/zstd-$ZSTD.tar.gz" \
	-O "https://github.com/libjpeg-turbo/libjpeg-turbo/releases/download/$LIBJPEG/libjpeg-turbo-$LIBJPEG.tar.gz" \
	-o "freetype-VER-$FREETYPE.tar.gz" "https://github.com/freetype/freetype/archive/refs/tags/VER-$FREETYPE.tar.gz" \
	-o "libpcap-$PCAP.tar.gz" "https://github.com/the-tcpdump-group/libpcap/archive/refs/tags/libpcap-$PCAP.tar.gz" \
	-o "curl-$CURL.tar.gz" "https://github.com/curl/curl/archive/refs/tags/curl-$CURL.tar.gz" \
	-o "plutovg-$PLUTOVG.tar.gz" "https://github.com/sammycage/plutovg/archive/refs/tags/v$PLUTOVG.tar.gz" \
	-o "plutosvg-$PLUTOSVG.tar.gz" "https://github.com/sammycage/plutosvg/archive/refs/tags/v$PLUTOSVG.tar.gz" \
	-O "https://github.com/biojppm/rapidyaml/releases/download/v$RAPIDYAML/rapidyaml-$RAPIDYAML-src.tgz" \
	-o "shaderc-$SHADERC.tar.gz" "https://github.com/google/shaderc/archive/refs/tags/v$SHADERC.tar.gz" \
	-o "shaderc-glslang-$SHADERC_GLSLANG.tar.gz" "https://github.com/KhronosGroup/glslang/archive/$SHADERC_GLSLANG.tar.gz" \
	-o "shaderc-spirv-headers-$SHADERC_SPIRVHEADERS.tar.gz" "https://github.com/KhronosGroup/SPIRV-Headers/archive/$SHADERC_SPIRVHEADERS.tar.gz" \
	-o "shaderc-spirv-tools-$SHADERC_SPIRVTOOLS.tar.gz" "https://github.com/KhronosGroup/SPIRV-Tools/archive/$SHADERC_SPIRVTOOLS.tar.gz"

shasum -a 256 --check SHASUMS

echo "Building libbacktrace..."
rm -fr "libbacktrace-$LIBBACKTRACE"
unzip "$LIBBACKTRACE.zip"
cd "libbacktrace-$LIBBACKTRACE"
./configure --prefix="$INSTALLDIR"
make
make install
cd ..

echo "Building libcurl..."
rm -rf "curl-$CURL"
tar xf "curl-$CURL.tar.gz"
cd "curl-curl-$CURL"
cmake -DCMAKE_BUILD_TYPE=Release \
	-DCURL_USE_OPENSSL=0 \
	-DCURL_STATICLIB=1 \
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_INSTALL_PREFIX="$INSTALLDIR" \
	-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
    -DANDROID_PLATFORM=android-$ANDROID_SDK \
    -DANDROID_ABI=arm64-v8a \
	-B build -G Ninja
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building libpcap..."
rm -rf "libpcap-$PCAP"
tar xf "libpcap-$PCAP.tar.gz"
cd "libpcap-libpcap-$PCAP"
cmake -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$INSTALLDIR" \
	-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
    -DANDROID_PLATFORM=android-$ANDROID_SDK \
    -DANDROID_ABI=arm64-v8a \
	-B build -G Ninja
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building freetype-VER-$FREETYPE..."
rm -rf "freetype-VER-$FREETYPE"
tar xf "freetype-VER-$FREETYPE.tar.gz"
cd "freetype-VER-$FREETYPE"
cmake -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$INSTALLDIR" \
	-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
    -DANDROID_PLATFORM=android-$ANDROID_SDK \
    -DANDROID_ABI=arm64-v8a \
	-B build -G Ninja
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building libpng..."
rm -fr "libpng-$LIBPNG"
tar xf "libpng-$LIBPNG.tar.xz"
cd "libpng-$LIBPNG"
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DBUILD_SHARED_LIBS=OFF \
		-DBUILD_SHARED_LIBS=ON -DPNG_TESTS=OFF -DPNG_STATIC=ON -DPNG_SHARED=ON -DPNG_TOOLS=OFF \
		-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
        -DANDROID_PLATFORM=android-$ANDROID_SDK \
        -DANDROID_ABI=arm64-v8a \
		-B build -G Ninja
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building libjpeg-turbo..."
rm -fr "libjpeg-turbo-$LIBJPEG"
tar xf "libjpeg-turbo-$LIBJPEG.tar.gz"
cd "libjpeg-turbo-$LIBJPEG"
cmake -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX="$INSTALLDIR" \
	-DANDROID_ABI=arm64-v8a \
  	-DANDROID_ARM_MODE=arm \
  	-DANDROID_PLATFORM=android-${ANDROID_SDK} \
  	-DANDROID_TOOLCHAIN=clang \
  	-DCMAKE_ASM_FLAGS="--target=aarch64-linux-android${ANDROID_SDK}" \
  	-DCMAKE_TOOLCHAIN_FILE=${NDK_TOOLCHAIN_PATH} \
  	-B build -G Ninja
cmake --build build
ninja -C build install
cd ..

echo "Building LZ4..."
rm -fr "lz4-$LZ4"
tar xf "$LZ4.tar.gz"
cd "lz4-$LZ4"
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DBUILD_SHARED_LIBS=OFF -DLZ4_BUILD_CLI=OFF -DLZ4_BUILD_LEGACY_LZ4C=OFF -DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
        -DANDROID_PLATFORM=android-$ANDROID_SDK \
        -DANDROID_ABI=arm64-v8a \
		 -B build-dir -G Ninja build/cmake
cmake --build build-dir --parallel
ninja -C build-dir install
cd ..

echo "Building Zstandard..."
rm -fr "zstd-$ZSTD"
tar xf "zstd-$ZSTD.tar.gz"
cd "zstd-$ZSTD"
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DBUILD_SHARED_LIBS=OFF -DZSTD_BUILD_SHARED=OFF -DZSTD_BUILD_STATIC=ON -DZSTD_BUILD_PROGRAMS=OFF -DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
        -DANDROID_PLATFORM=android-$ANDROID_SDK \
        -DANDROID_ABI=arm64-v8a \
		-B build -G Ninja build/cmake
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building WebP..."
rm -fr "libwebp-$LIBWEBP"
tar xf "libwebp-$LIBWEBP.tar.gz"
cd "libwebp-$LIBWEBP"
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -B build -G Ninja \
  -DWEBP_BUILD_ANIM_UTILS=OFF -DWEBP_BUILD_CWEBP=OFF -DWEBP_BUILD_DWEBP=OFF -DWEBP_BUILD_GIF2WEBP=OFF -DWEBP_BUILD_IMG2WEBP=OFF \
  -DWEBP_BUILD_VWEBP=OFF -DWEBP_BUILD_WEBPINFO=OFF -DWEBP_BUILD_WEBPMUX=OFF -DWEBP_BUILD_EXTRAS=OFF -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
  -DANDROID_PLATFORM=android-$ANDROID_SDK \
  -DANDROID_ABI=arm64-v8a
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building SDL..."
rm -fr "$SDL"
tar xf "$SDL.tar.gz"
cd "$SDL"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DBUILD_SHARED_LIBS=OFF \
	-DSDL_SHARED=OFF \
	-DSDL_STATIC=ON \
	-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
    -DANDROID_PLATFORM=android-$ANDROID_SDK \
    -DANDROID_ABI=arm64-v8a \
	-G Ninja
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building plutovg..."
rm -rf "plutovg-$PLUTOVG"
tar xf "plutovg-$PLUTOVG.tar.gz"
cd "plutovg-$PLUTOVG"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
    -DANDROID_PLATFORM=android-$ANDROID_SDK \
    -DANDROID_ABI=arm64-v8a \
	-G Ninja
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building plutosvg..."
rm -rf "plutosvg-$PLUTOSVG"
tar xf "plutosvg-$PLUTOSVG.tar.gz"
cd "plutosvg-$PLUTOSVG"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
    -DANDROID_PLATFORM=android-$ANDROID_SDK \
    -DANDROID_ABI=arm64-v8a \
	-G Ninja
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building RapidYAML..."
rm -fr "rapidyaml-$RAPIDYAML-src"
tar xf "rapidyaml-$RAPIDYAML-src.tgz"
cd "rapidyaml-$RAPIDYAML-src"
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$INSTALLDIR" -DCMAKE_INSTALL_PREFIX="$INSTALLDIR" -DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
    -DANDROID_PLATFORM=android-$ANDROID_SDK \
    -DANDROID_ABI=arm64-v8a \
	-G Ninja
cmake --build build --parallel
ninja -C build install
cd ..

echo "Building shaderc..."
rm -fr "shaderc-$SHADERC"
tar xf "shaderc-$SHADERC.tar.gz"
cd "shaderc-$SHADERC"
cd third_party
tar xf "../../shaderc-glslang-$SHADERC_GLSLANG.tar.gz"
mv "glslang-$SHADERC_GLSLANG" "glslang"
tar xf "../../shaderc-spirv-headers-$SHADERC_SPIRVHEADERS.tar.gz"
mv "SPIRV-Headers-$SHADERC_SPIRVHEADERS" "spirv-headers"
tar xf "../../shaderc-spirv-tools-$SHADERC_SPIRVTOOLS.tar.gz"
mv "SPIRV-Tools-$SHADERC_SPIRVTOOLS" "spirv-tools"
cd ..
#patch -p1 < "$SCRIPTDIR/../common/shaderc-changes.patch"
cmake -DCMAKE_BUILD_TYPE=Release \
	-DSHADERC_ENABLE_INSTALL=ON \
	-DBUILD_SHARED_LIBS=0 \
	-DCMAKE_PREFIX_PATH="$INSTALLDIR" \
	-DCMAKE_INSTALL_PREFIX="$INSTALLDIR" \
	-DSHADERC_SKIP_TESTS=ON \
	-DSHADERC_SKIP_EXAMPLES=ON \
	-DSHADERC_SKIP_COPYRIGHT_CHECK=ON \
	-DCMAKE_TOOLCHAIN_FILE=$NDK_TOOLCHAIN_PATH \
    -DANDROID_PLATFORM=android-35 \
    -DANDROID_ABI=arm64-v8a \
	-B build \
	-G Ninja

cmake --build build --parallel
ninja -C build install
cd ..

echo "Cleaning up..."
cd ..
rm -r deps-build
