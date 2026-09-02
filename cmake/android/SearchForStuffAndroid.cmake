#-------------------------------------------------------------------------------
#                       Search all libraries on the system
#-------------------------------------------------------------------------------
find_package(Git)

# Require threads on all OSes.
find_package(Threads REQUIRED)
find_package(ZLIB REQUIRED)


# Dependency libraries.
# On macOS, Mono.framework contains an ancient version of libpng.  We don't want that.
# Avoid it by telling cmake to avoid finding frameworks while we search for libpng.
set(FIND_FRAMEWORK_BACKUP ${CMAKE_FIND_FRAMEWORK})
set(CMAKE_FIND_FRAMEWORK NEVER)

set(ANDROID_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(zstd_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(Freetype_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include/freetype2")
set(SDL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include/SDL2")

include_directories(${zstd_INCLUDE_DIR})
include_directories(${ANDROID_INCLUDE_DIR})
include_directories(${SDL_INCLUDE_DIR})

# -------
set(ZSTD_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(ZSTD_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libzstd.a")

add_library(Zstd::Zstd STATIC IMPORTED)
set_target_properties(Zstd::Zstd PROPERTIES
    IMPORTED_LOCATION "${ZSTD_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_INCLUDE_DIR}"
)

# -------
set(FREETYPE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(FREETYPE_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libfreetype.a")

add_library(Freetype::Freetype STATIC IMPORTED)
set_target_properties(Freetype::Freetype PROPERTIES
    IMPORTED_LOCATION "${FREETYPE_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIR}"
)
# -------

set(SDL2_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(SDL2_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libSDL2.a")

add_library(SDL2::SDL2 STATIC IMPORTED)
set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_LOCATION "${SDL2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIR}"
)
# -------
set(LZ4_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(LZ4_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/liblz4.a")

add_library(LZ4::LZ4 STATIC IMPORTED)
set_target_properties(LZ4::LZ4 PROPERTIES
    IMPORTED_LOCATION "${LZ4_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${LZ4_INCLUDE_DIR}"
)

# -------
set(JPEG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(JPEG_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libjpeg.a")

add_library(JPEG::JPEG STATIC IMPORTED)
set_target_properties(JPEG::JPEG PROPERTIES
    IMPORTED_LOCATION "${JPEG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${JPEG_INCLUDE_DIR}"
)
# -------
set(PNG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(PNG_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libpng.a")

add_library(PNG::PNG STATIC IMPORTED)
set_target_properties(PNG::PNG PROPERTIES
    IMPORTED_LOCATION "${PNG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PNG_INCLUDE_DIR}"
)
# -------
set(WebP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(WebP_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libwebp.a")

add_library(WebP::libwebp STATIC IMPORTED)
set_target_properties(WebP::libwebp PROPERTIES
    IMPORTED_LOCATION "${WebP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${WebP_INCLUDE_DIR}"
)
# -------
set(PCAP_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(PCAP_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libpcap.a")

add_library(PCAP::PCAP STATIC IMPORTED)
set_target_properties(PCAP::PCAP PROPERTIES
    IMPORTED_LOCATION "${PCAP_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PCAP_INCLUDE_DIR}"
)
# -------
set(CURL_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include")
set(CURL_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libcurl.a")

add_library(CURL::libcurl STATIC IMPORTED)
set_target_properties(CURL::libcurl PROPERTIES
    IMPORTED_LOCATION "${CURL_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${CURL_INCLUDE_DIR}"
)

# -------
set(PLUTOVG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include/plutovg")
set(PLUTOVG_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libplutovg.a")

add_library(plutovg::plutovg STATIC IMPORTED)
set_target_properties(plutovg::plutovg PROPERTIES
    IMPORTED_LOCATION "${PLUTOVG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PLUTOVG_INCLUDE_DIR}"
)

# -------
set(PLUTOSVG_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/deps/android/include/plutosvg")
set(PLUTOSVG_LIBRARY "${CMAKE_SOURCE_DIR}/deps/android/lib/libplutosvg.a")

add_library(plutosvg::plutosvg STATIC IMPORTED)
set_target_properties(plutosvg::plutosvg PROPERTIES
    IMPORTED_LOCATION "${PLUTOSVG_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${PLUTOSVG_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES plutovg::plutovg
)
# -------

# GSCapture is compiled on Android but never links ffmpeg: USE_LINKED_FFMPEG is OFF and
# the library is dlopen()ed at runtime, which no Android device ships. Only the headers
# are needed, and they must be the cross-compile ones - a find_package() here would
# happily hand us the build host's macOS/Linux ffmpeg. Upstream deleted the in-tree
# 3rdparty/ffmpeg/include copy in cf8d05f4a, so build-dependencies-android.sh installs
# the ffmpeg 7.1 headers into the Android deps prefix instead.
set(FFMPEG_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/deps/android/include")
if(NOT EXISTS "${FFMPEG_INCLUDE_DIRS}/libavcodec/avcodec.h")
	message(FATAL_ERROR "ffmpeg headers missing from ${FFMPEG_INCLUDE_DIRS}. Re-run .github/workflows/scripts/linux/build-dependencies-android.sh.")
endif()

include(CheckLib)
check_lib(EGL EGL EGL/egl.h)
set(CUBEB_API OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")

set(CMAKE_FIND_FRAMEWORK ${FIND_FRAMEWORK_BACKUP})

set(ryml_DIR "${CMAKE_SOURCE_DIR}/deps/android/lib/cmake/ryml")
find_package(ryml CONFIG REQUIRED)

add_subdirectory(3rdparty/fast_float EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/lzma EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/libchdr EXCLUDE_FROM_ALL)
disable_compiler_warnings_for_target(libchdr)
add_subdirectory(3rdparty/soundtouch EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/simpleini EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/imgui EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/cpuinfo EXCLUDE_FROM_ALL)
disable_compiler_warnings_for_target(cpuinfo)
add_subdirectory(3rdparty/libzip EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/rcheevos EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/rapidjson EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/discord-rpc EXCLUDE_FROM_ALL)
add_subdirectory(3rdparty/freesurround EXCLUDE_FROM_ALL)

if(USE_OPENGL)
	add_subdirectory(3rdparty/glad EXCLUDE_FROM_ALL)
endif()

if(USE_VULKAN)
	add_subdirectory(3rdparty/vulkan EXCLUDE_FROM_ALL)
endif()

add_subdirectory(3rdparty/cubeb EXCLUDE_FROM_ALL)
disable_compiler_warnings_for_target(cubeb)
disable_compiler_warnings_for_target(speex)

# Demangler for the debugger.
add_subdirectory(3rdparty/demangler EXCLUDE_FROM_ALL)

# Symbol table parser.
add_subdirectory(3rdparty/ccc EXCLUDE_FROM_ALL)

add_subdirectory(3rdparty/vixl EXCLUDE_FROM_ALL)

# Prevent fmt from being built with exceptions, or being thrown at call sites.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFMT_EXCEPTIONS=0")
add_subdirectory(3rdparty/fmt EXCLUDE_FROM_ALL)

# Deliberately at the end. We don't want to set the flag on third-party projects.
if(MSVC)
	# Don't warn about "deprecated" POSIX functions.
	add_definitions("-D_CRT_NONSTDC_NO_WARNINGS" "-D_CRT_SECURE_NO_WARNINGS" "-DCRT_SECURE_NO_DEPRECATE")
endif()
