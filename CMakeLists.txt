
cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

include(RezBuild)

set(LIB_NAME gt_base)

project(${LIB_NAME} VERSION $ENV{REZ_BUILD_PROJECT_VERSION} LANGUAGES CXX)

add_library(${LIB_NAME} SHARED
    # include/gt_base/base.h
    # include/gt_base/string.h
    source/string.cpp
    source/filesystem.cpp
    # filesystem.h
    source/functions.cpp
    # functions.h
    # Image.cpp
    # Image.h`
)

target_include_directories(${LIB_NAME}
    PRIVATE
        ./include
    # TODO: figure out how to use the packages / variants to resolve paths
    PUBLIC
        /mnt/rez/release/ext/openexr/3.1.5/platform-linux/arch-x86_64/os-centos-7/include
        /mnt/rez/release/ext/openexr/3.1.5/platform-linux/arch-x86_64/os-centos-7/include
        # ./include
)

# find_package(openexr REQUIRED)

# set()

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
