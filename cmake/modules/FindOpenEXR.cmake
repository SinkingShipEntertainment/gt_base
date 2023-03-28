# Module to find OpenEXR.
#
# This module will set
#   OPENEXR_FOUND          true, if found
#   OPENEXR_INCLUDES       directory where headers are found
#   OPENEXR_LIBRARIES      libraries for OpenEXR + IlmBase
#   ILMBASE_LIBRARIES      libraries just IlmBase
#   OPENEXR_VERSION        OpenEXR version (accurate for >= 2.0.0,
#                              otherwise will just guess 1.6.1)
#
#

# Other standard issue macros
include (FindPackageHandleStandardArgs)
include (SelectLibraryConfigurations)

find_package (ZLIB REQUIRED)

# Link with pthreads if required
find_package (Threads)
if (CMAKE_USE_PTHREADS_INIT)
    set (ILMBASE_PTHREADS ${CMAKE_THREAD_LIBS_INIT})
endif ()

# Attempt to find OpenEXR with pkgconfig
find_package(PkgConfig)
if (PKG_CONFIG_FOUND)
    if (NOT Ilmbase_ROOT AND NOT ILMBASE_ROOT
        AND NOT DEFINED ENV{Ilmbase_ROOT} AND NOT DEFINED ENV{ILMBASE_ROOT})
        pkg_check_modules(_ILMBASE QUIET IlmBase>=2.0.0)
    endif ()
    if (NOT OpenEXR_ROOT AND NOT OPENEXR_ROOT
        AND NOT DEFINED ENV{OpenEXR_ROOT} AND NOT DEFINED ENV{OPENEXR_ROOT})
        pkg_check_modules(_OPENEXR QUIET OpenEXR>=2.0.0)
    endif ()
endif (PKG_CONFIG_FOUND)


# List of likely places to find the headers -- note priority override of
# ${OPENEXR_ROOT}/include.
# ILMBASE is needed in case ilmbase an openexr are installed in separate
# directories, like NixOS does
set (GENERIC_INCLUDE_PATHS
    ${OPENEXR_ROOT}/include
    $ENV{OPENEXR_ROOT}/include
    ${ILMBASE_ROOT}/include
    $ENV{ILMBASE_ROOT}/include
    ${_ILMBASE_INCLUDEDIR}
    ${_OPENEXR_INCLUDEDIR}
    /usr/local/include
    /usr/include
    /usr/include/${CMAKE_LIBRARY_ARCHITECTURE}
    /sw/include
    /opt/local/include )

# Find the include file locations.
find_path (ILMBASE_INCLUDE_PATH Imath/half.h
           HINTS ${ILMBASE_INCLUDE_DIR} ${OPENEXR_INCLUDE_DIR}
                 ${GENERIC_INCLUDE_PATHS} )
find_path (OPENEXR_INCLUDE_PATH OpenEXR/OpenEXRConfig.h
           HINTS ${OPENEXR_INCLUDE_DIR}
                 ${GENERIC_INCLUDE_PATHS} )

# Try to figure out version number
if (DEFINED _OPENEXR_VERSION AND NOT "${_OPENEXR_VERSION}" STREQUAL "")
    set (OPENEXR_VERSION "${_OPENEXR_VERSION}")
    string (REGEX REPLACE "([0-9]+)\\.[0-9\\.]+" "\\1" OPENEXR_VERSION_MAJOR "${_OPENEXR_VERSION}")
    string (REGEX REPLACE "[0-9]+\\.([0-9]+)(\\.[0-9]+)?" "\\1" OPENEXR_VERSION_MINOR "${_OPENEXR_VERSION}")
elseif (EXISTS "${OPENEXR_INCLUDE_PATH}/OpenEXR/ImfMultiPartInputFile.h")
    # Must be at least 2.0
    file(STRINGS "${OPENEXR_INCLUDE_PATH}/OpenEXR/OpenEXRConfig.h" TMP REGEX "^#define OPENEXR_VERSION_STRING .*$")
    string (REGEX MATCHALL "[0-9]+[.0-9]+" OPENEXR_VERSION ${TMP})
    file(STRINGS "${OPENEXR_INCLUDE_PATH}/OpenEXR/OpenEXRConfig.h" TMP REGEX "^#define OPENEXR_VERSION_MAJOR .*$")
    string (REGEX MATCHALL "[0-9]+" OPENEXR_VERSION_MAJOR ${TMP})
    file(STRINGS "${OPENEXR_INCLUDE_PATH}/OpenEXR/OpenEXRConfig.h" TMP REGEX "^#define OPENEXR_VERSION_MINOR .*$")
    string (REGEX MATCHALL "[0-9]+" OPENEXR_VERSION_MINOR ${TMP})
else ()
    # Assume an old one, predates 2.x that had versions
    set (OPENEXR_VERSION 1.6.1)
    set (OPENEXR_MAJOR 1)
    set (OPENEXR_MINOR 6)
endif ()

# List of likely places to find the libraries -- note priority override of
# ${OPENEXR_ROOT}/lib.
set (GENERIC_LIBRARY_PATHS
    ${OPENEXR_ROOT}/lib64
    ${ILMBASE_ROOT}/lib64
    ${OPENEXR_INCLUDE_PATH}/../lib64
    ${ILMBASE_INCLUDE_PATH}/../lib64
    ${_ILMBASE_LIBDIR}
    ${_OPENEXR_LIBDIR}
    /usr/local/lib
    /usr/local/lib/${CMAKE_LIBRARY_ARCHITECTURE}
    /usr/lib
    /usr/lib/${CMAKE_LIBRARY_ARCHITECTURE}
    /sw/lib
    /opt/local/lib
    $ENV{PROGRAM_FILES}/OpenEXR/lib/static )

# Handle request for static libs by altering CMAKE_FIND_LIBRARY_SUFFIXES.
# We will restore it at the end of this file.
set (_openexr_orig_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
if (OpenEXR_USE_STATIC_LIBS)
    if (WIN32)
        set (CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else ()
        set (CMAKE_FIND_LIBRARY_SUFFIXES .a)
    endif ()
endif ()

# Look for the libraries themselves, for all the components.
# This is complicated because the OpenEXR libraries may or may not be
# built with version numbers embedded.
set (_openexr_components IlmThread IlmImf Imath Iex Half)
foreach (COMPONENT ${_openexr_components})
    string (TOUPPER ${COMPONENT} UPPERCOMPONENT)
    # First try with the version embedded
    find_library (OPENEXR_${UPPERCOMPONENT}_LIBRARY
                  NAMES ${COMPONENT}-${OPENEXR_VERSION_MAJOR}_${OPENEXR_VERSION_MINOR}
                        ${COMPONENT}
                        ${COMPONENT}-${OPENEXR_VERSION_MAJOR}_${OPENEXR_VERSION_MINOR}_d
                        ${COMPONENT}_d
                  HINTS ${OPENEXR_LIBRARY_DIR} $ENV{OPENEXR_LIBRARY_DIR}
                        ${GENERIC_LIBRARY_PATHS} )
endforeach ()

message("ILMBASE_INCLUDE_PATH: ${ILMBASE_INCLUDE_PATH}")
message("OPENEXR_INCLUDE_PATH: ${OPENEXR_INCLUDE_PATH}")
message("OPENEXR_VERSION: ${OPENEXR_VERSION}")
message("OPENEXR_VERSION_MAJOR: ${OPENEXR_VERSION_MAJOR}")
message("OPENEXR_VERSION_MINOR: ${OPENEXR_VERSION_MINOR}")
message("OPENEXR_ILMIMF_LIBRARY: ${OPENEXR_ILMIMF_LIBRARY}")
message("OPENEXR_IMATH_LIBRARY: ${OPENEXR_IMATH_LIBRARY}")
message("OPENEXR_IEX_LIBRARY: ${OPENEXR_IEX_LIBRARY}")
message("OPENEXR_HALF_LIBRARY: ${OPENEXR_HALF_LIBRARY}")







find_path(OPENEXR_INCLUDE_DIR OpenEXR/half.h
HINTS
    "${OPENEXR_LOCATION}"
    "$ENV{OPENEXR_LOCATION}"
PATH_SUFFIXES
    include/
DOC
    "OpenEXR headers path"
)

if(OPENEXR_INCLUDE_DIR)
  set(openexr_config_file "${OPENEXR_INCLUDE_DIR}/OpenEXR/OpenEXRConfig.h")
  if(EXISTS ${openexr_config_file})
      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_STRING.*$")
      string(REGEX MATCHALL "[0-9.]+" OPENEXR_VERSION ${TMP})

      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_MAJOR.*$")
      string(REGEX MATCHALL "[0-9]" OPENEXR_MAJOR_VERSION ${TMP})

      file(STRINGS
           ${openexr_config_file}
           TMP
           REGEX "#define OPENEXR_VERSION_MINOR.*$")
      string(REGEX MATCHALL "[0-9]" OPENEXR_MINOR_VERSION ${TMP})
  endif()
endif()

foreach(OPENEXR_LIB
    Half
    Iex
    Imath
    IlmImf
    IlmThread
    IlmImfUtil
    IexMath
    )

    # OpenEXR libraries may be suffixed with the version number, so we search
    # using both versioned and unversioned names.
    find_library(OPENEXR_${OPENEXR_LIB}_LIBRARY
        NAMES
            ${OPENEXR_LIB}-${OPENEXR_MAJOR_VERSION}_${OPENEXR_MINOR_VERSION}
            ${OPENEXR_LIB}
        HINTS
            "${OPENEXR_LOCATION}"
            "$ENV{OPENEXR_LOCATION}"
        PATH_SUFFIXES
            lib/
        DOC
            "OPENEXR's ${OPENEXR_LIB} library path"
    )

    if(OPENEXR_${OPENEXR_LIB}_LIBRARY)
        list(APPEND OPENEXR_LIBRARIES ${OPENEXR_${OPENEXR_LIB}_LIBRARY})
    endif()
endforeach(OPENEXR_LIB)

# So #include <half.h> works
list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR})
list(APPEND OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR}/OpenEXR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenEXR
    REQUIRED_VARS
        OPENEXR_INCLUDE_DIRS
        OPENEXR_LIBRARIES
    VERSION_VAR
        OPENEXR_VERSION
)













