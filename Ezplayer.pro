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
    log/easylogging++.cc \
    widget.cpp

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
    log/easylogging++.h \
    widget.h

FORMS += \
    deepseekclient.ui \
        homewindow.ui \
    displaywind.ui \
    playlist.ui \
    urldialog.ui


win32 {
INCLUDEPATH += $$PWD/ffmpeg-4.2.1-win32-dev/include
INCLUDEPATH += $$PWD/openssl/win32/include
INCLUDEPATH += $$PWD/SDL2/include
INCLUDEPATH += $$PWD/log
LIBS += $$PWD/ffmpeg-4.2.1-win32-dev/lib/avformat.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avcodec.lib    \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avdevice.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avfilter.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/avutil.lib     \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/postproc.lib   \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/swresample.lib \
        $$PWD/ffmpeg-4.2.1-win32-dev/lib/swscale.lib    \
        $$PWD/SDL2/lib/x86/SDL2.lib \



LIBS += "D:\VS\Qt\Tools\mingw810_32\i686-w64-mingw32\lib\libws2_32.a"
LIBS += "C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22621.0\um\x86\WinMM.Lib"




LIBS += -lws2_32 -lOle32 -lWinMM
}


#解决ui改动未改变，使用缓存ui问题。
UI_DIR=$$PWD/

RESOURCES += \
    resource.qrc


QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO


