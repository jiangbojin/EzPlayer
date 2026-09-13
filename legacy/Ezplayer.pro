# 加载依赖路径配置
include(deps_config.pri)

#-------------------------------------------------
#
# Project created by QtCreator 2023-04-27T17:30:38
#
#-------------------------------------------------

QT       += core gui
QT += opengl
QT       += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


TARGET = Ezplayer
TEMPLATE = app
CONFIG +=   c++17

DEFINES += QT_DEPRECATED_WARNINGS
#EasyLogging++兼容性，安全性。
DEFINES += ELPP_THREAD_SAFE \
           ELPP_FEATURE_CRASH_LOG \
           ELPP_STL_LOGGING


RC_ICONS = player.ico


SOURCES += \
    deepseekclient.cpp \
 homewindow.cpp \
    commonlooper.cpp \
    imagescaler.cpp \
        main.cpp \
    ff_ffplay.cpp \
    ff_ffplay_def.cpp \
    ijkmediaplayer.cpp \
    displaywind.cpp \
    opengldisplaywidget.cpp \
    globalhelper.cpp \
    mediabase.cpp \
    medialist.cpp \
    messagequeue.cpp \
    playlist.cpp \
    urldialog.cpp \
    customslider.cpp \
    sonic.cpp \
    screenshot.cpp \
    toast.cpp \
    ijksdl_timer.cpp \
    log/easylogging++.cc

HEADERS += \
    commonlooper.h \
    deepseekclient.h \
        homewindow.h \
    mediabase.h \
    medialist.h \
    ff_ffplay.h \
    ff_ffplay_def.h \
    ijkmediaplayer.h \
    displaywind.h \
    opengldisplaywidget.h \
    imagescaler.h \
    ff_fferror.h \
    ffmsg.h \
    globalhelper.h \
    messagequeue.h \
    playlist.h \
    urldialog.h \
    customslider.h \
    sonic.h \
    screenshot.h \
    toast.h \
    ijksdl_timer.h \
    log/easylogging++.h

FORMS += \
    deepseekclient.ui \
        homewindow.ui \
    displaywind.ui \
    playlist.ui \
    urldialog.ui


win32 {
    INCLUDEPATH += $$FFMPEG_DIR/include \
                   $$OPENSSL_INCLUDE_DIR \
                   $$SDL2_DIR/include \
                   $$LOG_DIR

    LIBS += $$FFMPEG_DIR/lib/avformat.lib   \
            $$FFMPEG_DIR/lib/avcodec.lib    \
            $$FFMPEG_DIR/lib/avdevice.lib   \
            $$FFMPEG_DIR/lib/avfilter.lib   \
            $$FFMPEG_DIR/lib/avutil.lib     \
            $$FFMPEG_DIR/lib/postproc.lib   \
            $$FFMPEG_DIR/lib/swresample.lib \
            $$FFMPEG_DIR/lib/swscale.lib    \
            $$SDL2_DIR/lib/x64/SDL2.lib

    LIBS += -lws2_32 -lOle32 -lWinMM
}

unix:!macx {
    INCLUDEPATH += $$FFMPEG_DIR/include \
                   $$SDL2_INCLUDE_DIR \
                   $$LOG_DIR

    LIBS += -L$$FFMPEG_DIR/lib \
            -lavformat \
            -lavcodec \
            -lavdevice \
            -lavfilter \
            -lavutil \
            -lpostproc \
            -lswresample \
            -lswscale \
            -lSDL2 \
            -ldl \
            -lpthread

    QMAKE_LFLAGS += -Wl,-rpath,$$FFMPEG_DIR/lib
}


#解决ui改动未改变，使用缓存ui问题。
UI_DIR=$$PWD/

RESOURCES += \
    resource.qrc


QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO


