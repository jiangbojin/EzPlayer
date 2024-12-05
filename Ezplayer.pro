#-------------------------------------------------
#
# Project created by QtCreator 2023-04-27T17:30:38
#
#-------------------------------------------------

QT       += core gui
QT += opengl
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


TARGET = Ezplayer
TEMPLATE = app
CONFIG +=   c++17
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

RC_ICONS = player.ico

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
 homewindow.cpp \
    commonlooper.cpp \
    imagescaler.cpp \
    librtmp/amf.c \
    librtmp/hashswf.c \
    librtmp/log.c \
    librtmp/parseurl.c \
    librtmp/rtmp.c \
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
    rtmpbase.cpp \
    rtmpplayer.cpp \
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
        homewindow.h \
    librtmp/amf.h \
    librtmp/bytes.h \
    librtmp/dh.h \
    librtmp/dhgroups.h \
    librtmp/handshake.h \
    librtmp/http.h \
    librtmp/librtmp.3 \
    librtmp/log.h \
    librtmp/rtmp.h \
    librtmp/rtmp_sys.h \
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
    rtmpbase.h \
    rtmpplayer.h \
    urldialog.h \
    customslider.h \
    sonic.h \
    screenshot.h \
    toast.h \
    ijksdl_timer.h \
    log/easylogging++.h \
    widget.h

FORMS += \
        homewindow.ui \
    displaywind.ui \
    playlist.ui \
    urldialog.ui


win32 {
INCLUDEPATH += $$PWD/ffmpeg-4.2.1-win32-dev/include
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
LIBS += -lOle32
}

UI_DIR=$$PWD/
RESOURCES += \
    resource.qrc


QMAKE_CXXFLAGS_RELEASE = $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
QMAKE_LFLAGS_RELEASE = $$QMAKE_LFLAGS_RELEASE_WITH_DEBUGINFO

DISTFILES += \
    fragment.fsh \
    librtmp/COPYING \
    librtmp/Makefile \
    librtmp/librtmp.3.html \
    librtmp/librtmp.pc.in \
    vertex.vsh
