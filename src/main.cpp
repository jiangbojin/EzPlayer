#include <QApplication>
#include "ui/homewindow.h"
#include "utils/log/logger.h"

#undef main
int main(int argc, char* argv[]) {
    // 初始化 spdlog 现代高性能日志系统
    ezplayer::log::init("log");

    QApplication a(argc, argv);
    HomeWindow w;

    w.show();

    return a.exec();
}
