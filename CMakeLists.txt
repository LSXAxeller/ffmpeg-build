cmake_minimum_required(VERSION 3.20)
project(SoundFlowFFmpeg C)

cmake_policy(SET CMP0135 NEW)

# Platform detection and RID mapping
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TARGET_OS "win")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        set(TARGET_ARCH "x64")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "i686")
        set(TARGET_ARCH "x86")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
        set(TARGET_ARCH "arm64")
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TARGET_OS "linux")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
        set(TARGET_ARCH "x64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
        set(TARGET_ARCH "arm")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(TARGET_ARCH "arm64")
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(IOS)
        set(TARGET_OS "ios")
        set(TARGET_ARCH "arm64")
    else()
        set(TARGET_OS "osx")
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
            set(TARGET_ARCH "x64")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm64")
            set(TARGET_ARCH "arm64")
        endif()
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TARGET_OS "android")
    if(CMAKE_ANDROID_ARCH STREQUAL "x86_64")
        set(TARGET_ARCH "x64")
    elseif(CMAKE_ANDROID_ARCH MATCHES "^arm")
        set(TARGET_ARCH "arm")
    elseif(CMAKE_ANDROID_ARCH STREQUAL "arm64")
        set(TARGET_ARCH "arm64")
    endif()

elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    set(TARGET_OS "freebsd")
    if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
        set(TARGET_ARCH "x64")
    elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
        set(TARGET_ARCH "arm64")
    endif()
endif()

# Set .NET-style RID and output directory
set(RUNTIME_ID "${TARGET_OS}-${TARGET_ARCH}")
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/runtimes/${RUNTIME_ID}/native")
set(FFMPEG_INSTALL_DIR "${CMAKE_BINARY_DIR}/ffmpeg-install")

# FFmpeg configuration
set(FFMPEG_VERSION "8.0")
set(FFMPEG_URL "http://ffmpeg.org/releases/ffmpeg-${FFMPEG_VERSION}.tar.gz")

# Configure Flags
set(FFMPEG_COMMON_FLAGS
        # Build Type
        --disable-shared
        --enable-static
        --enable-pic
        --disable-symver

        # General Disables
        --disable-doc
        --disable-debug
        --disable-avdevice
        --disable-swscale
        --disable-programs
        --enable-ffmpeg
        --enable-ffprobe
        --disable-network
        --disable-muxers
        --disable-demuxers
        --disable-zlib
        --disable-lzma
        --disable-bzlib
        --disable-iconv
        --disable-libxcb
        --disable-bsfs
        --disable-filters
        --disable-parsers
        --disable-indevs
        --disable-outdevs
        --disable-encoders
        --disable-decoders
        --disable-hwaccels
        --disable-nvenc
        --disable-videotoolbox
        --disable-audiotoolbox

        # Enabled Filters
        --disable-filters
        --enable-filter=aformat
        --enable-filter=anull
        --enable-filter=atrim
        --enable-filter=format
        --enable-filter=null
        --enable-filter=setpts
        --enable-filter=trim

        # Enabled Protocols
        --disable-protocols
        --enable-protocol=file
        --enable-protocol=pipe

        # Enabled Demuxers
        --enable-demuxer=image2
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
        --enable-demuxer=mpc
        --enable-demuxer=mpc8
        --enable-demuxer=ogg
        --enable-demuxer=pcm_alaw
        --enable-demuxer=pcm_mulaw
        --enable-demuxer=pcm_f64be
        --enable-demuxer=pcm_f64le
        --enable-demuxer=pcm_f32be
        --enable-demuxer=pcm_f32le
        --enable-demuxer=pcm_s32be
        --enable-demuxer=pcm_s32le
        --enable-demuxer=pcm_s24be
        --enable-demuxer=pcm_s24le
        --enable-demuxer=pcm_s16be
        --enable-demuxer=pcm_s16le
        --enable-demuxer=pcm_s8
        --enable-demuxer=pcm_u32be
        --enable-demuxer=pcm_u32le
        --enable-demuxer=pcm_u24be
        --enable-demuxer=pcm_u24le
        --enable-demuxer=pcm_u16be
        --enable-demuxer=pcm_u16le
        --enable-demuxer=pcm_u8
        --enable-demuxer=rm
        --enable-demuxer=shorten
        --enable-demuxer=tak
        --enable-demuxer=tta
        --enable-demuxer=wav
        --enable-demuxer=wv
        --enable-demuxer=xwma
        --enable-demuxer=dsf

        # Enabled Decoders
        --enable-decoder=aac
        --enable-decoder=aac_latm
        --enable-decoder=ac3
        --enable-decoder=alac
        --enable-decoder=als
        --enable-decoder=ape
        --enable-decoder=atrac1
        --enable-decoder=atrac3
        --enable-decoder=eac3
        --enable-decoder=flac
        --enable-decoder=gsm
        --enable-decoder=gsm_ms
        --enable-decoder=mp1
        --enable-decoder=mp1float
        --enable-decoder=mp2
        --enable-decoder=mp2float
        --enable-decoder=mp3
        --enable-decoder=mp3adu
        --enable-decoder=mp3adufloat
        --enable-decoder=mp3float
        --enable-decoder=mp3on4
        --enable-decoder=mp3on4float
        --enable-decoder=mpc7
        --enable-decoder=mpc8
        --enable-decoder=opus
        --enable-decoder=ra_144
        --enable-decoder=ra_288
        --enable-decoder=ralf
        --enable-decoder=shorten
        --enable-decoder=tak
        --enable-decoder=tta
        --enable-decoder=vorbis
        --enable-decoder=wavpack
        --enable-decoder=wmalossless
        --enable-decoder=wmapro
        --enable-decoder=wmav1
        --enable-decoder=wmav2
        --enable-decoder=wmavoice
        --enable-decoder=pcm_alaw
        --enable-decoder=pcm_bluray
        --enable-decoder=pcm_dvd
        --enable-decoder=pcm_f32be
        --enable-decoder=pcm_f32le
        --enable-decoder=pcm_f64be
        --enable-decoder=pcm_f64le
        --enable-decoder=pcm_lxf
        --enable-decoder=pcm_mulaw
        --enable-decoder=pcm_s8
        --enable-decoder=pcm_s8_planar
        --enable-decoder=pcm_s16be
        --enable-decoder=pcm_s16be_planar
        --enable-decoder=pcm_s16le
        --enable-decoder=pcm_s16le_planar
        --enable-decoder=pcm_s24be
        --enable-decoder=pcm_s24daud
        --enable-decoder=pcm_s24le
        --enable-decoder=pcm_s24le_planar
        --enable-decoder=pcm_s32be
        --enable-decoder=pcm_s32le
        --enable-decoder=pcm_s32le_planar
        --enable-decoder=pcm_u8
        --enable-decoder=pcm_u16be
        --enable-decoder=pcm_u16le
        --enable-decoder=pcm_u24be
        --enable-decoder=pcm_u24le
        --enable-decoder=pcm_u32be
        --enable-decoder=pcm_u32le
        --enable-decoder=dsd_lsbf
        --enable-decoder=dsd_msbf
        --enable-decoder=dsd_lsbf_planar
        --enable-decoder=dsd_msbf_planar

        # Enabled Parsers
        --enable-parser=aac
        --enable-parser=aac_latm
        --enable-parser=ac3
        --enable-parser=cook
        --enable-parser=dca
        --enable-parser=flac
        --enable-parser=gsm
        --enable-parser=mpegaudio
        --enable-parser=tak
        --enable-parser=vorbis

        # Enabled Encoders
        # Lossy and Lossless encoders
        --enable-encoder=aac
        --enable-encoder=ac3
        --enable-encoder=alac
        --enable-encoder=aptx
        --enable-encoder=aptx_hd
        --enable-encoder=eac3
        --enable-encoder=flac
        --enable-encoder=mp2
        --enable-encoder=nellymoser
        --enable-encoder=opus
        --enable-encoder=sbc
        --enable-encoder=sonic
        --enable-encoder=tta
        --enable-encoder=vorbis
        --enable-encoder=wavpack
        --enable-encoder=wmav1
        --enable-encoder=wmav2

        # PCM (Uncompressed) encoders
        --enable-encoder=pcm_alaw
        --enable-encoder=pcm_mulaw
        --enable-encoder=pcm_f32be
        --enable-encoder=pcm_f32le
        --enable-encoder=pcm_f64be
        --enable-encoder=pcm_f64le
        --enable-encoder=pcm_s8
        --enable-encoder=pcm_s8_planar
        --enable-encoder=pcm_s16be
        --enable-encoder=pcm_s16be_planar
        --enable-encoder=pcm_s16le
        --enable-encoder=pcm_s16le_planar
        --enable-encoder=pcm_s24be
        --enable-encoder=pcm_s24daud
        --enable-encoder=pcm_s24le
        --enable-encoder=pcm_s24le_planar
        --enable-encoder=pcm_s32be
        --enable-encoder=pcm_s32le
        --enable-encoder=pcm_s32le_planar
        --enable-encoder=pcm_s64be
        --enable-encoder=pcm_s64le
        --enable-encoder=pcm_u8
        --enable-encoder=pcm_u16be
        --enable-encoder=pcm_u16le
        --enable-encoder=pcm_u24be
        --enable-encoder=pcm_u24le
        --enable-encoder=pcm_u32be
        --enable-encoder=pcm_u32le
)

# Platform-specific flags for FFmpeg
if(TARGET_OS STREQUAL "win")
    if(TARGET_ARCH STREQUAL "x64")
        set(FFMPEG_ARCH "x86_64")
    elseif(TARGET_ARCH STREQUAL "x86")
        set(FFMPEG_ARCH "x86")
    elseif(TARGET_ARCH STREQUAL "arm64")
        set(FFMPEG_ARCH "aarch64")
    endif()
    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=mingw32
            --arch=${FFMPEG_ARCH}
            "--extra-cflags=-O2 -static -static-libgcc -static-libstdc++"
            "--extra-ldflags=-static"
    )
    if(CMAKE_CROSSCOMPILING AND FFMPEG_ARCH STREQUAL "x86_64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=x86_64-w64-mingw32-)
    elseif(CMAKE_CROSSCOMPILING AND FFMPEG_ARCH STREQUAL "x86")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=i686-w64-mingw32-)
    elseif(CMAKE_CROSSCOMPILING AND FFMPEG_ARCH STREQUAL "aarch64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=aarch64-w64-mingw32-)
    endif()

elseif(TARGET_OS STREQUAL "osx")
    if(TARGET_ARCH STREQUAL "x64")
        set(FFMPEG_ARCH "x86_64")
        set(MACOS_TARGET "x86_64-apple-darwin")
    elseif(TARGET_ARCH STREQUAL "arm64")
        set(FFMPEG_ARCH "aarch64")
        set(MACOS_TARGET "arm64-apple-darwin")
    endif()

    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=darwin
            --arch=${FFMPEG_ARCH}
            --enable-cross-compile
            "--extra-cflags=-target ${MACOS_TARGET}"
            "--extra-ldflags=-target ${MACOS_TARGET}"
            --enable-runtime-cpudetect
    )

elseif(TARGET_OS STREQUAL "android")
    if(TARGET_ARCH STREQUAL "x64")
        set(FFMPEG_ARCH "x86_64")
    elseif(TARGET_ARCH STREQUAL "arm")
        set(FFMPEG_ARCH "arm")
    elseif(TARGET_ARCH STREQUAL "arm64")
        set(FFMPEG_ARCH "aarch64")
    endif()
    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=android
            --arch=${FFMPEG_ARCH}
            --enable-cross-compile
    )
    if(FFMPEG_ARCH STREQUAL "arm")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=arm-linux-androideabi-)
    elseif(FFMPEG_ARCH STREQUAL "aarch64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=aarch64-linux-android-)
    elseif(FFMPEG_ARCH STREQUAL "x86_64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=x86_64-linux-android-)
    endif()

elseif(TARGET_OS STREQUAL "ios")
    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=darwin
            --arch=arm64
            --enable-cross-compile
            --disable-asm
            "--extra-cflags=-arch arm64 -isysroot ${CMAKE_OSX_SYSROOT}"
            "--extra-ldflags=-arch arm64 -isysroot ${CMAKE_OSX_SYSROOT}"
    )

elseif(TARGET_OS STREQUAL "freebsd")
    if(TARGET_ARCH STREQUAL "x64")
        set(FFMPEG_ARCH "x86_64")
    elseif(TARGET_ARCH STREQUAL "arm64")
        set(FFMPEG_ARCH "aarch64")
    endif()
    list(APPEND FFMPEG_COMMON_FLAGS
            --target-os=freebsd
            --arch=${FFMPEG_ARCH}
    )
    if(CMAKE_CROSSCOMPILING AND FFMPEG_ARCH STREQUAL "x86_64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=x86_64-unknown-freebsd-)
    elseif(CMAKE_CROSSCOMPILING AND FFMPEG_ARCH STREQUAL "aarch64")
        list(APPEND FFMPEG_COMMON_FLAGS --cross-prefix=aarch64-unknown-freebsd-)
    endif()
endif()

# Find bash for configure script
find_program(BASH_EXECUTABLE bash REQUIRED)

# FFmpeg External Project
include(ExternalProject)

ExternalProject_Add(ffmpeg_dependency
        URL                 ${FFMPEG_URL}
        PREFIX              ${CMAKE_BINARY_DIR}/ffmpeg
        INSTALL_DIR         ${FFMPEG_INSTALL_DIR}
        BUILD_IN_SOURCE     1
        CONFIGURE_COMMAND   ${BASH_EXECUTABLE} <SOURCE_DIR>/configure
        --prefix=${FFMPEG_INSTALL_DIR}
        ${FFMPEG_COMMON_FLAGS}
        BUILD_COMMAND       ${CMAKE_MAKE_PROGRAM} -j${CMAKE_BUILD_PARALLEL_LEVEL}
        INSTALL_COMMAND     ${CMAKE_MAKE_PROGRAM} install
        LOG_DOWNLOAD       1
        LOG_CONFIGURE      1
        LOG_BUILD          1
        LOG_INSTALL        1
)

# Now build your wrapper library
add_library(soundflow_ffmpeg SHARED
        wrapper_src/soundflow_ffmpeg.c
        wrapper_src/soundflow_ffmpeg.h
)

# Depend on FFmpeg build
add_dependencies(soundflow_ffmpeg ffmpeg_dependency)

# Set output properties for .NET-style naming
if(TARGET_OS STREQUAL "win")
    set_target_properties(soundflow_ffmpeg PROPERTIES
            PREFIX ""
            SUFFIX ".dll"
            OUTPUT_NAME "soundflow_ffmpeg"
            RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    )
elseif(TARGET_OS STREQUAL "linux" OR TARGET_OS STREQUAL "freebsd" OR TARGET_OS STREQUAL "android")
    set_target_properties(soundflow_ffmpeg PROPERTIES
            PREFIX "lib"
            SUFFIX ".so"
            OUTPUT_NAME "soundflow_ffmpeg"
            RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    )
elseif(TARGET_OS STREQUAL "osx")
    set_target_properties(soundflow_ffmpeg PROPERTIES
            PREFIX "lib"
            SUFFIX ".dylib"
            OUTPUT_NAME "soundflow_ffmpeg"
            RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    )
elseif(TARGET_OS STREQUAL "ios")
    set_target_properties(soundflow_ffmpeg PROPERTIES
            FRAMEWORK TRUE
            FRAMEWORK_VERSION A
            MACOSX_FRAMEWORK_IDENTIFIER com.soundflow.ffmpeg
            OUTPUT_NAME "soundflow_ffmpeg"
            RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
            ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
    )
endif()

# Include directories
target_include_directories(soundflow_ffmpeg PRIVATE
        ${FFMPEG_INSTALL_DIR}/include
        wrapper_src/
)

# Link directories and libraries
target_link_directories(soundflow_ffmpeg PRIVATE ${FFMPEG_INSTALL_DIR}/lib)

# Link against static FFmpeg libraries
target_link_libraries(soundflow_ffmpeg PRIVATE
        avformat
        avcodec
        swresample
        avutil
)

# Platform-specific system libraries
if(TARGET_OS STREQUAL "win")
    target_link_libraries(soundflow_ffmpeg PRIVATE
            bcrypt
            secur32
            ws2_32
            iphlpapi
    )
    # Static linking for Windows
    target_link_options(soundflow_ffmpeg PRIVATE -static -static-libgcc -static-libstdc++)
else()
    target_link_libraries(soundflow_ffmpeg PRIVATE
            m
            pthread
    )
endif()

# Install the wrapper library to the output directory
install(TARGETS soundflow_ffmpeg
        LIBRARY DESTINATION runtimes/${RUNTIME_ID}/native
        ARCHIVE DESTINATION runtimes/${RUNTIME_ID}/native
        RUNTIME DESTINATION runtimes/${RUNTIME_ID}/native
)

# Custom target to ensure proper output structure
add_custom_target(package_all
        DEPENDS soundflow_ffmpeg
        COMMENT "Packaging soundflow_ffmpeg for ${RUNTIME_ID}"
)

# Create a post-build step to verify the output
add_custom_command(TARGET soundflow_ffmpeg POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E echo "Built soundflow_ffmpeg for ${RUNTIME_ID} in ${OUTPUT_DIR}"
        COMMENT "Finalizing output for ${RUNTIME_ID}"
)