
# TODO: do this properly

# include (FindPackageHandleStandardArgs)
# include (SelectLibraryConfigurations)

# find_package(PkgConfig)
# if (PKG_CONFIG_FOUND)
#     if (NOT FFMPEG_ROOT  AND NOT DEFINED ENV{FFMPEG_ROOT})
#         pkg_check_modules(_FFMPEG QUIET)
#     endif ()
# endif (PKG_CONFIG_FOUND)

# set (GENERIC_INCLUDE_PATHS
#     ${FFMPEG_ROOT}/include)

# NOTE: cheating here until I know what this is supposed to be
set(FFMPEG_ROOT 
    /mnt/rez/release/ext/ffmpeg/5.0.1/platform-linux/arch-x86_64/os-centos-7)

find_path(FFMPEG_INCLUDE_DIR
        libavcodec/avcodec.h
    PATHS
        ${FFMPEG_ROOT}
        # /mnt/rez/release/ext/ffmpeg/5.0.1/platform-linux/arch-x86_64/os-centos-7 
    PATH_SUFFIXES
        include/
    DOC
        "FFMPEG_INCLUDE_DIR headers path"
)

find_package_handle_standard_args(FFMPEG
    REQUIRED_VARS
        FFMPEG_INCLUDE_DIR
    VERSION_VAR
        FFMPEG_VERSION
)
