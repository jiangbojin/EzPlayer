#include "homewindow.h"
#include <QApplication>
#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP    // 初始化宏，有且只能使用一次

#undef main
int main(int argc, char *argv[])
{

//    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::Format, "%datetime %level %func(L%line) %msg");

    el::Configurations conf;
    conf.setToDefault();
    //设置日志输出格式,包括日期时间、日志级别、函数名称、行号和日志信息。
    conf.setGlobally(el::ConfigurationType::Format, "[%datetime | %level] %func(L%line) %msg");
    conf.setGlobally(el::ConfigurationType::Filename, "log_%datetime{%Y%M%d}.log");// 设置日志文件名,以当前日期命名。
    conf.setGlobally(el::ConfigurationType::Enabled, "true");//启用日志输出。
    conf.setGlobally(el::ConfigurationType::ToFile, "true");//输出日志到文件。
    el::Loggers::reconfigureAllLoggers(conf);//将配置应用到所有日志记录器。
    el::Loggers::reconfigureAllLoggers(el::ConfigurationType::ToStandardOutput, "true"); // 也输出一份到终端

/*    LOG(VERBOSE) << "logger test"; //该级别只能用宏VLOG而不能用宏 LOG(VERBOSE)
    LOG(TRACE) << " logger";
    LOG(DEBUG) << "logger test";
    LOG(INFO) << "logger test";
    LOG(WARNING) << "logger test";
    LOG(ERROR) << "logger test";
*/
    QApplication a(argc, argv);
    HomeWindow w;

    w.show();

    return a.exec();
}
