#include <QApplication>
#include <QFileInfo>
#include <QTimer>
#include "easylogging++.h"
#include "ui/homewindow.h"
#include "utils/log/logger.h"

#undef main
int main(int argc, char* argv[]) {
    // 初始化 spdlog 现代高性能日志系统
    ezplayer::log::init("log");

    QApplication a(argc, argv);
    HomeWindow w;

    // 解析传入的视频绝对路径参数
    QString playTarget;
    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--play" && i + 1 < argc) {
            playTarget = QString::fromLocal8Bit(argv[++i]);
        } else if (!arg.startsWith("-")) {
            playTarget = arg;
        }
    }

    w.show();

    if (!playTarget.isEmpty()) {
        QFileInfo fi(playTarget);
        QString absPath = fi.absoluteFilePath();
        LOG(INFO) << "[Main] 启动时通过绝对路径触发添加与播放: " << absPath.toStdString();
        QTimer::singleShot(400, [&w, absPath]() { w.openPath(absPath); });
    }

    return a.exec();
}
