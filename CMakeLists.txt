
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(RezBuild)

set(LIB_NAME gt_base)

project(${LIB_NAME} VERSION $ENV{REZ_BUILD_PROJECT_VERSION} LANGUAGES CXX)


# =======================================================
# Appending our own Cmake files
LIST (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
LIST (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

include(utils)

# ------------------------------------------------------
# Find packages

# OpenEXR
find_package(OpenEXR)
if(OpenEXR_FOUND)
    message("OpenEXR found")
endif()

# SPNG
include(FindSPNG)
if(SPNG_FOUND)
    message("SPNG found")
endif()

# TGA
find_package(TGA)
if(TGA_FOUND)
    message("TGA found")
endif()

# Freetype
find_package(Freetype)
if(FREETYPE_FOUND)
    message("FreeType found")
endif()

# libjpeg
find_package(JPEG)
if(LIBJPEG_FOUND)
    message("libjpeg found")
endif()

# OCIO
find_package(OpenColorIO)
if(OpenColorIO_FOUND)
    message("OCIO found")
endif()

# Print Cmake variables
# dump_cmake_variables()
dump_cmake_variables("OCIO")
dump_cmake_variables("SPNG")
dump_cmake_variables("TGA")
dump_cmake_variables("JPEG")
dump_cmake_variables("FFMPEG")
dump_cmake_variables("OPENEXR")
dump_cmake_variables("ILMBASE")



add_library(${LIB_NAME} SHARED
    source/types/f32x2.cpp
    source/types/f32x3.cpp
    source/types/f32x4.cpp
    source/string.cpp
    source/filesystem.cpp
    source/functions.cpp
    source/Logger.cpp
    source/Image.cpp
    source/imgUtils.cpp
    # source/Text.cpp
    source/Video.cpp
)

add_compile_definitions(
    SSE_CENTOS7
    _GLIBCXX_USE_CXX11_ABI=0
)


# add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

# set_target_properties(${LIB_NAME} PROPERTIES
#     CXX_STANDARD 14
#     CXX_STANDARD_REQUIRED ON
#     CXX_EXTENSIONS OFF
# )

# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

# add_compile_definitions(std=c++14)
# add_definitions(std=c++14) # std=c++11  std=gnu++11

target_include_directories(${LIB_NAME}
    PRIVATE
        ./include
    # TODO: figure out how to use the packages / variants to resolve paths
    PUBLIC
        ${ILMBASE_INCLUDE_PATH}
        ${OPENEXR_INCLUDE_PATH}
        ${OPENEXR_INCLUDE_PATH}/Imath
        ${OCIO_INCLUDE_DIR}
        ${SPNG_SPNG_INCLUDE_DIR}
        ${TGA_INCLUDE_DIR}
        /mnt/rez/release/ext/libjpeg/9.2/platform-linux/arch-x86_64/os-centos-7/include
        /mnt/rez/release/ext/ffmpeg/5.0.1/platform-linux/arch-x86_64/os-centos-7/include
)

# find_package(openexr REQUIRED)


target_link_directories(${LIB_NAME}
    PUBLIC
        ${OCIO_LIBRARIES}
        /home/tomsirdevan/rez/local/gt_base/0.1.0/platform-linux/arch-x86_64/os-centos-7
        /mnt/rez/release/ext/libjpeg/9.2/platform-linux/arch-x86_64/os-centos-7/lib
        /mnt/rez/release/ext/openexr/3.1.5/platform-linux/arch-x86_64/os-centos-7/lib64
        /mnt/rez/release/ext/ffmpeg/5.0.1/platform-linux/arch-x86_64/os-centos-7/lib
        /mnt/rez/release/ext/x264/stable.sse.1.0.1/platform-linux/arch-x86_64/os-centos-7/lib
)


target_compile_features(${LIB_NAME}
    PUBLIC
        cxx_auto_type
        cxx_nullptr
        cxx_range_for
)

set(libraryName ${LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

# Install library
install(
    TARGETS ${LIB_NAME}
    LIBRARY DESTINATION .
)
