
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(RezBuild)

set(LIB_NAME gt_base)

project(${LIB_NAME} VERSION $ENV{REZ_BUILD_PROJECT_VERSION} LANGUAGES CXX)

LIST (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
LIST (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/macros")

include(utils)

# ------------------------------------------------------
# Find packages

find_package(JPEG)
if(JPEG_FOUND)
    message("JPEG found")
    # dump_cmake_variables("JPEG")
endif()

find_package(SPNG)
if(SPNG_FOUND)
    message("SPNG found")
endif()

find_package(TGA)
if(TGA_FOUND)
    message("TGA found")
endif()

find_package(OpenEXR)
if(OpenEXR_FOUND)
    message("OpenEXR found")
endif()

find_package(OpenColorIO)
if(OpenColorIO_FOUND)
    message("OCIO found")
    # dump_cmake_variables("OCIO")
endif()

find_package(Freetype)
if(FREETYPE_FOUND)
    message("FreeType found")
    # dump_cmake_variables("FREETYPE")
endif()

find_package(FFMPEG)
if(FFMPEG_FOUND)
    message("FFMPEG found")
    # dump_cmake_variables("FFMPEG")
endif()


add_library(${LIB_NAME} STATIC # SHARED
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
    # OpenColorIO_SKIP_IMPORTS
)

add_compile_options(-fPIC)
# SET(GCC_COVERAGE_COMPILE_FLAGS "-fPIC")

target_include_directories(${LIB_NAME}
    PRIVATE
        ./include
    PUBLIC
        ${JPEG_INCLUDE_DIR}
        ${SPNG_SPNG_INCLUDE_DIR}
        ${TGA_INCLUDE_DIR}
        ${ILMBASE_INCLUDE_PATH}
        ${OPENEXR_INCLUDE_PATH}
        ${OPENEXR_INCLUDE_PATH}/Imath
        ${OCIO_INCLUDE_DIR}
        ${FFMPEG_INCLUDE_DIR}
)

# target_link_directories(${LIB_NAME}
#     PUBLIC
#         ${OCIO_LIBRARIES}
#         # /home/tomsirdevan/rez/local/gt_base/0.1.0/platform-linux/arch-x86_64/os-centos-7
#         /mnt/rez/release/ext/libjpeg/9.2/platform-linux/arch-x86_64/os-centos-7/lib
#         /mnt/rez/release/ext/openexr/3.1.5/platform-linux/arch-x86_64/os-centos-7/lib64
#         /mnt/rez/release/ext/ffmpeg/5.0.1/platform-linux/arch-x86_64/os-centos-7/lib
#         /mnt/rez/release/ext/x264/stable.sse.1.0.1/platform-linux/arch-x86_64/os-centos-7/lib
# )


# target_compile_features(${LIB_NAME}
#     PUBLIC
#         cxx_auto_type
#         cxx_nullptr
#         cxx_range_for
# )

# set(libraryName ${LIB_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

set(libraryName ${LIB_NAME}${CMAKE_STATIC_LIBRARY_SUFFIX})

# Install library

install(
    DIRECTORY include/
    DESTINATION include
)


install(
    TARGETS ${LIB_NAME}
    LIBRARY DESTINATION .
)
