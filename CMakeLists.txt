cmake_minimum_required(VERSION 3.20)
project(SoundFlowFFmpeg C)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TARGET_OS "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TARGET_OS "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(CMAKE_OSX_ARCHITECTURES MATCHES "arm64" OR IOS)
        set(TARGET_OS "ios")
    else()
        set(TARGET_OS "darwin")
    endif()
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TARGET_OS "android")
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(TARGET_OS "freebsd")
endif()

# Architecture detection
if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    set(TARGET_ARCH "x86_64")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i686")
    set(TARGET_ARCH "x86")
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
    set(TARGET_ARCH "aarch64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
    set(TARGET_ARCH "arm")
endif()

# Set output directory for .NET-style runtime folders
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/runtimes/${TARGET_OS}-${TARGET_ARCH}/native")
set(FFMPEG_INSTALL_DIR "${OUTPUT_DIR}")

# FFmpeg configuration
set(FFMPEG_VERSION "8.0")
set(FFMPEG_URL "http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz")

# Common FFmpeg flags
set(FFMPEG_COMMON_FLAGS
        --disable-shared
        --enable-static
        --enable-pic
        --disable-symver
        --disable-doc
        --disable-debug
        --disable-avdevice
        --disable-swscale
        --disable-programs
        --enable-ffmpeg
        --enable-ffprobe
        --disable-network
        --disable-zlib
        --disable-lzma
        --disable-bzlib
        --disable-iconv
        --disable-libxcb
        --disable-bsfs
        --disable-hwaccels
        --disable-nvenc
        --disable-videotoolbox
        --disable-audiotoolbox

        # Enabled components
        --enable-filter=aformat
        --enable-filter=anull
        --enable-filter=atrim
        --enable-filter=format
        --enable-filter=null
        --enable-filter=setpts
        --enable-filter=trim
        --enable-protocol=file
        --enable-protocol=pipe

        # Enabled demuxers
        --enable-demuxer=aac
        --enable-demuxer=ac3
        --enable-demuxer=aiff
        --enable-demuxer=ape
        --enable-demuxer=asf
        --enable-demuxer=au
        --enable-demuxer=avi
        --enable-demuxer=flac
        --enable-demuxer=flv
        --enable-demuxer=matroska
        --enable-demuxer=mov
        --enable-demuxer=m4v
        --enable-demuxer=mp3
        --enable-demuxer=ogg
        --enable-demuxer=wav
        --enable-demuxer=dsf

        # Enabled decoders
        --enable-decoder=aac
        --enable-decoder=aac_latm
        --enable-decoder=ac3
        --enable-decoder=alac
        --enable-decoder=ape
        --enable-decoder=eac3
        --enable-decoder=flac
        --enable-decoder=mp1
        --enable-decoder=mp2
        --enable-decoder=mp3
        --enable-decoder=opus
        --enable-decoder=vorbis
        --enable-decoder=wavpack
        --enable-decoder=pcm_alaw
        --enable-decoder=pcm_mulaw
        --enable-decoder=pcm_f32be
        --enable-decoder=pcm_f32le
        --enable-decoder=pcm_f64be
        --enable-decoder=pcm_f64le
        --enable-decoder=pcm_s16be
        --enable-decoder=pcm_s16le
        --enable-decoder=pcm_s24be
        --enable-decoder=pcm_s24le
        --enable-decoder=pcm_s32be
        --enable-decoder=pcm_s32le
        --enable-decoder=pcm_u8
        --enable-decoder=pcm_u16be
        --enable-decoder=pcm_u16le
        --enable-decoder=pcm_u24be
        --enable-decoder=pcm_u24le
        --enable-decoder=pcm_u32be
        --enable-decoder=pcm_u32le

        # Enabled encoders
        --enable-encoder=aac
        --enable-encoder=ac3
        --enable-encoder=alac
        --enable-encoder=eac3
        --enable-encoder=flac
        --enable-encoder=mp2
        --enable-encoder=opus
        --enable-encoder=vorbis
        --enable-encoder=wavpack
        --enable-encoder=pcm_alaw
        --enable-encoder=pcm_mulaw
        --enable-encoder=pcm_f32be
        --enable-encoder=pcm_f32le
        --enable-encoder=pcm_f64be
        --enable-encoder=pcm_f64le
        --enable-encoder=pcm_s16be
        --enable-encoder=pcm_s16le
        --enable-encoder=pcm_s24be
        --enable-encoder=pcm_s24le
        --enable-encoder=pcm_s32be
        --enable-encoder=pcm_s32le
        --enable-encoder=pcm_u8
        --enable-encoder=pcm_u16be
        --enable-encoder=pcm_u16le
        --enable-encoder=pcm_u24be
        --enable-encoder=pcm_u24le
        --enable-encoder=pcm_u32be
        --enable-encoder=pcm_u32le
)

# Platform-specific flags
if(TARGET_OS STREQUAL "windows")
    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=mingw32
            --arch=${TARGET_ARCH}
            --extra-cflags="-static -static-libgcc -static-libstdc++"
            --extra-ldflags="-static"
    )
    if(CMAKE_CROSSCOMPILING AND TARGET_ARCH STREQUAL "x86_64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=x86_64-w64-mingw32-)
    elseif(CMAKE_CROSSCOMPILING AND TARGET_ARCH STREQUAL "x86")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=i686-w64-mingw32-)
    elseif(CMAKE_CROSSCOMPILING AND TARGET_ARCH STREQUAL "aarch64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=aarch64-w64-mingw32-)
    endif()

elseif(TARGET_OS STREQUAL "darwin")
    if(TARGET_ARCH STREQUAL "x86_64")
        set(MACOS_TARGET "x86_64-apple-darwin")
    elseif(TARGET_ARCH STREQUAL "aarch64")
        set(MACOS_TARGET "arm64-apple-darwin")
    endif()

    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=darwin
            --arch=${TARGET_ARCH}
            --enable-cross-compile
            --extra-cflags="-target ${MACOS_TARGET}"
            --extra-ldflags="-target ${MACOS_TARGET}"
            --enable-runtime-cpudetect
    )

elseif(TARGET_OS STREQUAL "android")
    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=android
            --arch=${TARGET_ARCH}
            --enable-cross-compile
    )
    if(TARGET_ARCH STREQUAL "arm")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=arm-linux-androideabi-)
    elseif(TARGET_ARCH STREQUAL "aarch64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=aarch64-linux-android-)
    elseif(TARGET_ARCH STREQUAL "x86_64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=x86_64-linux-android-)
    endif()

elseif(TARGET_OS STREQUAL "ios")
    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=darwin
            --arch=arm64
            --enable-cross-compile
            --disable-asm
            --extra-cflags="-arch arm64 -isysroot ${CMAKE_OSX_SYSROOT}"
            --extra-ldflags="-arch arm64 -isysroot ${CMAKE_OSX_SYSROOT}"
    )

elseif(TARGET_OS STREQUAL "freebsd")
    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=freebsd
            --arch=${TARGET_ARCH}
    )
    if(CMAKE_CROSSCOMPILING AND TARGET_ARCH STREQUAL "x86_64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=x86_64-unknown-freebsd-)
    elseif(CMAKE_CROSSCOMPILING AND TARGET_ARCH STREQUAL "aarch64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=aarch64-unknown-freebsd-)
    endif()
endif()

# Find bash for configure script
find_program(BASH_EXECUTABLE bash REQUIRED)

# FFmpeg External Project
include(ExternalProject)

ExternalProject_Add(ffmpeg
        URL                 ${FFMPEG_URL}
        PREFIX              ${CMAKE_BINARY_DIR}/ffmpeg-build
        INSTALL_DIR         ${FFMPEG_INSTALL_DIR}
        BUILD_IN_SOURCE     1
        CONFIGURE_COMMAND   ${BASH_EXECUTABLE} <SOURCE_DIR>/configure
        --prefix=${FFMPEG_INSTALL_DIR}
        ${FFMPEG_COMMON_FLAGS}
        BUILD_COMMAND       ${CMAKE_MAKE_PROGRAM} -j${CMAKE_BUILD_PARALLEL_LEVEL}
        INSTALL_COMMAND     ${CMAKE_MAKE_PROGRAM} install
        LOG_DOWNLOAD       1
        LOG_CONFIGURE       1
        LOG_BUILD          1
        LOG_INSTALL        1
)

# Create a custom target to organize the output
add_custom_target(package_ffmpeg
        DEPENDS ffmpeg
        COMMENT "Packaging FFmpeg for ${TARGET_OS}-${TARGET_ARCH}"
)

# Install the built libraries
install(DIRECTORY ${FFMPEG_INSTALL_DIR}/
        DESTINATION runtimes/${TARGET_OS}-${TARGET_ARCH}/native
        USE_SOURCE_PERMISSIONS
)