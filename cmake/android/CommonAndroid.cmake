

# x86emitter sources
target_sources(common PRIVATE
	AlignedMalloc.cpp
	Assertions.cpp
	Console.cpp
	CrashHandler.cpp
	DynamicLibrary.cpp
	Error.cpp
	FastJmp.cpp
	FileSystem.cpp
	HostSys.cpp
	Image.cpp
	HTTPDownloader.cpp
	MemorySettingsInterface.cpp
	MD5Digest.cpp
	PrecompiledHeader.cpp
	Perf.cpp
	ProgressCallback.cpp
	ReadbackSpinManager.cpp
	Semaphore.cpp
	SettingsWrapper.cpp
	SmallString.cpp
	StringUtil.cpp
	TextureDecompress.cpp
	Timer.cpp
	WAVWriter.cpp
	WindowInfo.cpp
	MemoryInterface.cpp
	YAML.cpp
)

# x86emitter headers
target_sources(common PRIVATE
	AlignedMalloc.h
	Assertions.h
	boost_spsc_queue.hpp
	BitUtils.h
	ByteSwap.h
	Console.h
	CrashHandler.h
	DynamicLibrary.h
	Easing.h
	EnumOps.h
	Error.h
	FPControl.h
	FastJmp.h
	FileSystem.h
	HashCombine.h
	HostSys.h
	HeterogeneousContainers.h
	Image.h
	LRUCache.h
	HeapArray.h
	HTTPDownloader.h
	MemoryInterface.h
	MemorySettingsInterface.h
	MD5Digest.h
	MRCHelpers.h
	Path.h
	PrecompiledHeader.h
	ProgressCallback.h
	ReadbackSpinManager.h
	RedtapeWilCom.h
	RedtapeWindows.h
	ScopedGuard.h
	SettingsInterface.h
	SettingsWrapper.h
	SingleRegisterTypes.h
	SmallString.h
	StringUtil.h
	Timer.h
	TextureDecompress.h
	Threading.h
	VectorIntrin.h
	WAVWriter.h
	WindowInfo.h
	WrappedMemCopy.h
	YAML.h
)

target_include_directories(common PUBLIC "${ANDROID_INCLUDE_DIR}")

if(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
    target_link_libraries(common PRIVATE cpuinfo)
endif()

# Add the include directory for libjpeg
include_directories(${LIBJPEG_INCLUDE_DIR})
message(STATUS "jpeglib LIBJPEG_INCLUDE_DIR: ${LIBJPEG_INCLUDE_DIR}")
include_directories(${WEBP_INCLUDE_DIR})
message(STATUS "webp WEBP_INCLUDE_DIR: ${WEBP_INCLUDE_DIR}")

message(STATUS "CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
if (CMAKE_CXX_STANDARD EQUAL 20)
  check_cxx_compiler_flag(-std=c++20 has_std_20_flag)
  check_cxx_compiler_flag(-std=c++2a has_std_2a_flag)
  message(STATUS "has_std_20_flag: ${has_std_20_flag}")
  if (has_std_20_flag)
    set(CXX_STANDARD_FLAG -std=c++20)
  elseif (has_std_2a_flag)
    set(CXX_STANDARD_FLAG -std=c++2a)
  endif ()
endif()
message(STATUS ">> CXX_STANDARD_FLAG: ${CXX_STANDARD_FLAG}")
set(CMAKE_REQUIRED_FLAGS ${CXX_STANDARD_FLAG})


set_source_files_properties(PrecompiledHeader.cpp PROPERTIES HEADER_FILE_ONLY TRUE)

if(USE_VTUNE)
	target_link_libraries(common PUBLIC Vtune::Vtune)
endif()

if (USE_GCC AND CMAKE_INTERPROCEDURAL_OPTIMIZATION)
	# GCC LTO doesn't work with asm statements
	set_source_files_properties(FastJmp.cpp PROPERTIES COMPILE_FLAGS -fno-lto)
endif()

if(NOT WIN32)
	# libcurl-based HTTPDownloader
	target_sources(common PRIVATE
		HTTPDownloaderCurl.cpp
		HTTPDownloaderCurl.h
	)
	target_link_libraries(common PRIVATE
		CURL::libcurl
	)
endif()

target_link_libraries(common PRIVATE
	${LIBC_LIBRARIES}
	JPEG::JPEG
	PNG::PNG
	WebP::libwebp
	cpuinfo
)

target_link_libraries(common PUBLIC
	fmt::fmt
	fast_float
)

fixup_file_properties(common)
target_compile_features(common PUBLIC cxx_std_17)
target_include_directories(common PUBLIC ../3rdparty/include ../)
target_compile_definitions(common PUBLIC "${PCSX2_DEFS}")
target_compile_options(common PRIVATE "${PCSX2_WARNINGS}")

if(COMMAND target_precompile_headers)
	target_precompile_headers(common PRIVATE PrecompiledHeader.h)
endif()

INSTALL (
    DIRECTORY ${CMAKE_SOURCE_DIR}/common/
    DESTINATION ${CMAKE_BINARY_DIR}/include/common
    FILES_MATCHING PATTERN "*.h*")