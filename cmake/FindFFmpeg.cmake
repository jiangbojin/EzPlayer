#[=======================================================================[.rst:
FindFFmpeg
----------

查找 FFmpeg 开发库（libavcodec, libavformat, libavutil 等）。

可配置的提示变量:
  FFMPEG_ROOT / FFMPEG_DIR : FFmpeg 安装根目录
  环境变量 FFMPEG7_DIR / FFMPEG_DIR

提供的导入目标 (Imported Targets):
  FFmpeg::FFmpeg        - 聚合所有找到的 FFmpeg 组件
  FFmpeg::<component>   - 具体组件目标，例如 FFmpeg::avcodec, FFmpeg::avformat

提供的输出变量:
  FFMPEG_FOUND          - 是否成功找到所有请求的组件
  FFMPEG_INCLUDE_DIRS   - FFmpeg 头文件目录
  FFMPEG_LIBRARIES      - FFmpeg 库文件列表
#]=======================================================================]

include(FindPackageHandleStandardArgs)

# 默认请求的核心组件
if(NOT FFmpeg_FIND_COMPONENTS)
    set(FFmpeg_FIND_COMPONENTS
        avformat
        avcodec
        avdevice
        avfilter
        avutil
        postproc
        swresample
        swscale
    )
endif()

# 收集可能的搜索路径
set(_FFMPEG_SEARCH_PATHS
    ${FFMPEG_ROOT}
    ${FFMPEG_DIR}
    $ENV{FFMPEG_ROOT}
    $ENV{FFMPEG_DIR}
    $ENV{FFMPEG7_DIR}
    /root/project/ffmpeg-7.1/dist
    /root/project/ffmpeg-7.1/ffmpeg/install
    /usr/local
    /usr
)

# 查找公共头文件（以 libavcodec/avcodec.h 为代表）
find_path(FFMPEG_INCLUDE_DIR
    NAMES libavcodec/avcodec.h libavformat/avformat.h
    HINTS ${_FFMPEG_SEARCH_PATHS}
    PATH_SUFFIXES include
)

set(FFMPEG_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
set(FFMPEG_LIBRARIES "")
set(_MISSING_COMPONENTS "")

foreach(_comp IN LISTS FFmpeg_FIND_COMPONENTS)
    # 头文件可能各组件独立或共用
    find_path(FFMPEG_${_comp}_INCLUDE_DIR
        NAMES "lib${_comp}/${_comp}.h"
        HINTS ${_FFMPEG_SEARCH_PATHS}
        PATH_SUFFIXES include
    )

    find_library(FFMPEG_${_comp}_LIBRARY
        NAMES ${_comp} "lib${_comp}"
        HINTS ${_FFMPEG_SEARCH_PATHS}
        PATH_SUFFIXES lib lib64 bin
    )

    if(FFMPEG_${_comp}_LIBRARY)
        set(FFmpeg_${_comp}_FOUND TRUE)
        set(FFMPEG_${_comp}_FOUND TRUE)
        list(APPEND FFMPEG_LIBRARIES ${FFMPEG_${_comp}_LIBRARY})
        
        # 创建现代 CMake 导入目标
        if(NOT TARGET FFmpeg::${_comp})
            add_library(FFmpeg::${_comp} UNKNOWN IMPORTED)
            set_target_properties(FFmpeg::${_comp} PROPERTIES
                IMPORTED_LOCATION "${FFMPEG_${_comp}_LIBRARY}"
            )
            if(FFMPEG_${_comp}_INCLUDE_DIR)
                set_target_properties(FFmpeg::${_comp} PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_${_comp}_INCLUDE_DIR}"
                )
            elseif(FFMPEG_INCLUDE_DIR)
                set_target_properties(FFmpeg::${_comp} PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}"
                )
            endif()
        endif()
    else()
        set(FFmpeg_${_comp}_FOUND FALSE)
        set(FFMPEG_${_comp}_FOUND FALSE)
        if(FFmpeg_FIND_REQUIRED_${_comp} OR FFmpeg_FIND_REQUIRED)
            list(APPEND _MISSING_COMPONENTS ${_comp})
        endif()
    endif()
endforeach()

# 校验并生成总体 FFMPEG_FOUND / FFmpeg_FOUND
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS FFMPEG_INCLUDE_DIR FFMPEG_LIBRARIES
    HANDLE_COMPONENTS
)

if(FFmpeg_FOUND OR FFMPEG_FOUND)
    set(FFMPEG_FOUND TRUE)
    set(FFmpeg_FOUND TRUE)
    if(NOT TARGET FFmpeg::FFmpeg)
        add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
        set_target_properties(FFmpeg::FFmpeg PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${FFMPEG_LIBRARIES}"
        )
    endif()
endif()

