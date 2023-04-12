


set(FFMPEG_ROOT $ENV{REZ_FFMPEG_ROOT})

find_path(FFMPEG_INCLUDE_DIR
        libavcodec/avcodec.h
    PATHS
        ${FFMPEG_ROOT}
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
