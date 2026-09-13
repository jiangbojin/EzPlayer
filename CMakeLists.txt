cmake_minimum_required(VERSION 3.20)

if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()

project(Ezplayer VERSION 1.0.0 LANGUAGES CXX C)

# ---------------------------------------------------------------------------
# 基础编译属性设置
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 模块搜索路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 启用 Qt 自动化工具链 (MOC / RCC / UIC)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 运行时 RPATH 保留
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ---------------------------------------------------------------------------
# 依赖包查找 (Qt5, SDL2, FFmpeg, spdlog, Threads)
# ---------------------------------------------------------------------------

# 0. 补充 Conan 2.x 生成器搜索路径
foreach(CONAN_GEN_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/build/${CMAKE_BUILD_TYPE}/generators"
    "${CMAKE_CURRENT_BINARY_DIR}/build/Release/generators"
    "${CMAKE_CURRENT_BINARY_DIR}/build/Debug/generators"
    "${CMAKE_CURRENT_BINARY_DIR}/generators"
)
    if(EXISTS "${CONAN_GEN_DIR}")
        list(PREPEND CMAKE_PREFIX_PATH "${CONAN_GEN_DIR}")
    endif()
endforeach()

# 1. 查找 Qt5
if(EXISTS "/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/cmake")
    list(APPEND CMAKE_PREFIX_PATH 
        "/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/cmake"
        "/root/project/qt-dev-tools/qt5/usr"
    )
endif()

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    OpenGL
)

# 2. SDL2 已彻底移除，无需查找 SDL2 (采用 C++17 原生线程与 WebRtcAvCapture 音频管道替代)


# 3. 查找 FFmpeg 7.1
find_package(FFmpeg REQUIRED COMPONENTS
    avformat
    avcodec
    avdevice
    avfilter
    avutil
    postproc
    swresample
    swscale
)

# 4. 查找 spdlog (由 Conan 2.x 管理)
find_package(spdlog REQUIRED)

# 5. 线程库
find_package(Threads REQUIRED)

# 5. 音视频采集与播放模块 (WebRTC M153 Fat Static Library)
option(ENABLE_WEBRTC_AV_CAPTURE "Enable WebRTC Audio/Video Capture module" ON)
if(ENABLE_WEBRTC_AV_CAPTURE)
    if(NOT DEFINED WEBRTC_AV_CAPTURE_DIR)
        set(WEBRTC_AV_CAPTURE_DIR "/root/project/webrtc-m153/knowledge-base/002-音频和视频采集/out/cmake")
    endif()
    if(EXISTS "${WEBRTC_AV_CAPTURE_DIR}/WebRtcAvCaptureConfig.cmake")
        include("${WEBRTC_AV_CAPTURE_DIR}/WebRtcAvCaptureConfig.cmake")
    else()
        message(WARNING "WebRtcAvCaptureConfig.cmake not found at ${WEBRTC_AV_CAPTURE_DIR}")
    endif()
endif()

# ---------------------------------------------------------------------------
# 主应用程序目标构建
# ---------------------------------------------------------------------------
add_executable(Ezplayer)

target_sources(Ezplayer PRIVATE
    # 程序入口与静态资源
    src/main.cpp
    resource.qrc
    resource.h

    # Core 模块 (播放核心引擎、解复用、同步与队列)
    src/core/commonlooper.cpp
    src/core/commonlooper.h
    src/core/ff_fferror.h
    src/core/ff_ffplay.cpp
    src/core/ff_ffplay.h
    src/core/ff_ffplay_def.cpp
    src/core/ff_ffplay_def.h
    src/core/ffmsg.h
    src/core/ijkmediaplayer.cpp
    src/core/ijkmediaplayer.h
    src/core/ijksdl_timer.cpp
    src/core/ijksdl_timer.h
    src/core/imagescaler.cpp
    src/core/imagescaler.h
    src/core/mediabase.cpp
    src/core/mediabase.h
    src/core/messagequeue.cpp
    src/core/messagequeue.h
    src/core/sonic.cpp
    src/core/sonic.h

    # Render 模块 (OpenGL GPU 硬件着色渲染 & 软件渲染窗口)
    src/render/displaywind.cpp
    src/render/displaywind.h
    src/render/displaywind.ui
    src/render/opengldisplaywidget.cpp
    src/render/opengldisplaywidget.h

    # UI 模块 (主窗口、播放列表、滑块与弹窗)
    src/ui/customslider.cpp
    src/ui/customslider.h
    src/ui/homewindow.cpp
    src/ui/homewindow.h
    src/ui/homewindow.ui
    src/ui/medialist.cpp
    src/ui/medialist.h
    src/ui/playlist.cpp
    src/ui/playlist.h
    src/ui/playlist.ui
    src/ui/screenshot.cpp
    src/ui/screenshot.h
    src/ui/toast.cpp
    src/ui/toast.h
    src/ui/urldialog.cpp
    src/ui/urldialog.h
    src/ui/urldialog.ui

    # Network 模块 (外部智能服务与网络流)
    src/network/deepseekclient.cpp
    src/network/deepseekclient.h
    src/network/deepseekclient.ui

    # Utils 模块 (工具库与日志系统)
    src/utils/globalhelper.cpp
    src/utils/globalhelper.h
    src/utils/log/logger.cpp
    src/utils/log/logger.h
    src/utils/log/easylogging++.h
)

if(WIN32)
    target_sources(Ezplayer PRIVATE player.ico)
endif()

# 包含路径定义 (支持模块相对路径与就近包含)
target_include_directories(Ezplayer PRIVATE
    src
    src/core
    src/render
    src/ui
    src/network
    src/utils
    src/utils/log
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 宏定义
target_compile_definitions(Ezplayer PRIVATE
    QT_DEPRECATED_WARNINGS
)

# 链接目标与库
target_link_libraries(Ezplayer PRIVATE
    Qt5::Widgets
    Qt5::Network
    Qt5::OpenGL
    Qt5::Gui
    Qt5::Core
    FFmpeg::FFmpeg
    spdlog::spdlog
    Threads::Threads
)

if(TARGET WebRtcAvCapture::WebRtcAvCapture)
    target_link_libraries(Ezplayer PRIVATE WebRtcAvCapture::WebRtcAvCapture)
    target_compile_definitions(Ezplayer PRIVATE HAVE_WEBRTC_AV_CAPTURE)
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(Ezplayer PRIVATE ${CMAKE_DL_LIBS})

    # 设置运行时 RPATH，保证加载自定义编译的 FFmpeg 动态库
    get_target_property(_FFMPEG_LIB FFmpeg::avcodec IMPORTED_LOCATION)
    if(_FFMPEG_LIB)
        get_filename_component(_FFMPEG_LIBDIR "${_FFMPEG_LIB}" DIRECTORY)
        set_target_properties(Ezplayer PROPERTIES
            BUILD_RPATH "${_FFMPEG_LIBDIR}"
            INSTALL_RPATH "${_FFMPEG_LIBDIR}"
        )
    endif()
elseif(WIN32)
    target_link_libraries(Ezplayer PRIVATE
        ws2_32
        Ole32
        WinMM
    )
endif()

# ---------------------------------------------------------------------------
# 自动化测试支持 (GoogleTest + CTest)
# ---------------------------------------------------------------------------
option(EZPLAYER_BUILD_TESTS "Build EzPlayer unit tests" ON)

if(EZPLAYER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# 安装规则
# ---------------------------------------------------------------------------
install(TARGETS Ezplayer
    RUNTIME DESTINATION bin
)
