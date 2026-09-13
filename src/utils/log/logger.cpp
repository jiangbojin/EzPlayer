#include "utils/log/logger.h"
#include <vector>
#include <filesystem>
#include <iostream>

namespace ezplayer::log {

void init(const std::string& log_dir) {
    try {
        // 创建日志目录（若不存在）
        std::filesystem::create_directories(log_dir);

        std::vector<spdlog::sink_ptr> sinks;

        // 1. 彩色终端输出 Sink
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::trace);
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] [%s:%#] %v");
        sinks.push_back(console_sink);

        // 2. 滚动文件 Sink（单文件最大 10MB，保留 5 个历史文件）
        std::string log_file = log_dir + "/ezplayer.log";
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 10 * 1024 * 1024, 5);
        file_sink->set_level(spdlog::level::trace);
        file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] [%s:%#] %v");
        sinks.push_back(file_sink);

        // 3. 构造多 Sink 的主 Logger
        auto combined_logger = std::make_shared<spdlog::logger>(
            "ezplayer", sinks.begin(), sinks.end());
        combined_logger->set_level(spdlog::level::trace);
        combined_logger->flush_on(spdlog::level::debug);

        // 注册为默认全局 Logger
        spdlog::set_default_logger(combined_logger);

        SPDLOG_INFO("EzPlayer spdlog 日志系统初始化完成，输出文件: {}", log_file);
    } catch (const std::exception& ex) {
        std::cerr << "[ERROR] 初始化 spdlog 失败: " << ex.what() << std::endl;
    }
}

void flush() {
    auto logger = spdlog::default_logger();
    if (logger) {
        logger->flush();
    }
}

}  // namespace ezplayer::log
