
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(RezBuild)

set(LIB_NAME gt_base)

project(${LIB_NAME} VERSION $ENV{REZ_BUILD_PROJECT_VERSION} LANGUAGES CXX)

set(GT_BASE_ROOT $ENV{REZ_GT_BASE_ROOT}) # NOTE: REZ_GT_BASE_ROOT based on rez package name gt_base
message("GT_BASE_ROOT "${GT_BASE_ROOT})

set(JPEG_ROOT $ENV{REZ_LIBJPEG_ROOT})
message("JPEG_ROOT "${JPEG_ROOT})

set(SPNG_ROOT $ENV{REZ_LIBSPNG_ROOT})
message("SPNG_ROOT "${SPNG_ROOT})

set(TGA_ROOT $ENV{REZ_TGA_ROOT})
message("TGA_ROOT "${TGA_ROOT})

set(OPENEXR_ROOT $ENV{REZ_OPENEXR_ROOT})
message("OPENEXR_ROOT "${OPENEXR_ROOT})

set(OCIO_ROOT $ENV{REZ_OCIO_ROOT})
message("OCIO_ROOT "${OCIO_ROOT})

set(FREETYPE_ROOT $ENV{REZ_FREETYPE_ROOT})
message("FREETYPE_ROOT "${FREETYPE_ROOT})

set(FFMPEG_ROOT $ENV{REZ_FFMPEG_ROOT})
message("FFMPEG_ROOT "${FFMPEG_ROOT})

add_library(${LIB_NAME} STATIC
    source/types/f32x2.cpp
    source/types/f32x3.cpp
    source/types/f32x4.cpp
    source/string.cpp
    source/filesystem.cpp
    source/functions.cpp
    source/Logger.cpp
    source/Image.cpp
    source/imgUtils.cpp
    source/Text.cpp
    source/Video.cpp
)

add_compile_definitions(
    SSE_CENTOS7
    _GLIBCXX_USE_CXX11_ABI=0
)

# add_compile_options(-fPIC)
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

target_include_directories(${LIB_NAME}
    PRIVATE
        ./include
    PUBLIC
        ${GT_BASE_ROOT}/include
        ${JPEG_ROOT}/include
        ${SPNG_ROOT}/include
        ${TGA_ROOT}/include
        ${OPENEXR_ROOT}/include
        ${OPENEXR_ROOT}/include/Imath
        ${OCIO_ROOT}/include
        ${FREETYPE_ROOT}/include/freetype2
        ${FFMPEG_ROOT}/include
)

install(
    DIRECTORY include/
    DESTINATION include
)

install(
    TARGETS ${LIB_NAME}
    LIBRARY DESTINATION .
)
