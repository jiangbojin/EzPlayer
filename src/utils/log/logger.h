#ifndef SRC_UTILS_LOG_LOGGER_H_
#define SRC_UTILS_LOG_LOGGER_H_

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <sstream>
#include <string>
#include <memory>

namespace ezplayer::log {

/// @brief 初始化全局日志系统（控制台彩色输出 + 文件滚动存储）
/// @param log_dir 日志文件存储目录，默认为 "log"
void init(const std::string& log_dir = "log");

/// @brief 刷新所有日志缓冲区
void flush();

/// @brief 流式日志辅助类，兼容流式语法 LOG(LEVEL) << ...
class LogStream {
public:
    LogStream(spdlog::level::level_enum lvl, const char* file, int line, const char* func)
        : level_(lvl), file_(file), line_(line), func_(func) {}

    ~LogStream() {
        auto logger = spdlog::default_logger_raw();
        if (logger && logger->should_log(level_)) {
            logger->log(spdlog::source_loc{file_, line_, func_}, level_, oss_.str());
        }
    }

    template <typename T>
    LogStream& operator<<(const T& val) {
        oss_ << val;
        return *this;
    }

    // 支持 std::endl 等流操纵符
    LogStream& operator<<(std::ostream& (*)(std::ostream&)) {
        return *this;
    }

private:
    spdlog::level::level_enum level_;
    const char* file_;
    int line_;
    const char* func_;
    std::ostringstream oss_;
};

}  // namespace ezplayer::log

// 映射常规日志级别
#define LOG_LEVEL_TRACE    spdlog::level::trace
#define LOG_LEVEL_DEBUG    spdlog::level::debug
#define LOG_LEVEL_INFO     spdlog::level::info
#define LOG_LEVEL_WARNING  spdlog::level::warn
#define LOG_LEVEL_WARN     spdlog::level::warn
#define LOG_LEVEL_ERROR    spdlog::level::err
#define LOG_LEVEL_FATAL    spdlog::level::critical

// 兼容既有代码的 LOG(LEVEL) << ... 流式语法
#define LOG(LEVEL) ezplayer::log::LogStream(LOG_LEVEL_##LEVEL, __FILE__, __LINE__, static_cast<const char*>(__FUNCTION__))

#endif  // SRC_UTILS_LOG_LOGGER_H_
