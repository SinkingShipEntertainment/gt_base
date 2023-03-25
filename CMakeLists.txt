
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(RezBuild)

set(LIB_NAME gt_base)

project(${LIB_NAME} VERSION $ENV{REZ_BUILD_PROJECT_VERSION} LANGUAGES CXX)

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

add_compile_definitions(SSE_CENTOS7)

add_definitions(-D_GLIBCXX_USE_CXX11_ABI=0)

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
        /mnt/rez/release/ext/libjpeg/9.2/platform-linux/arch-x86_64/os-centos-7/include
        /mnt/rez/release/ext/libpng/1.6.29/platform-linux/arch-x86_64/os-centos-7/include
        /mnt/rez/release/ext/openexr/3.1.5/platform-linux/arch-x86_64/os-centos-7/include
        /mnt/rez/release/ext/openexr/3.1.5/platform-linux/arch-x86_64/os-centos-7/include/Imath
        /mnt/rez/release/ext/ocio/2.1.1/platform-linux/arch-x86_64/os-centos-7/python-3.9.7/include
        /mnt/rez/release/ext/ffmpeg/5.0.1/platform-linux/arch-x86_64/os-centos-7/include
)

# find_package(openexr REQUIRED)

# target_link_libraries(${LIB_NAME}
    # ar
#     sdf
# )

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
