#ifndef SRC_UTILS_LOG_EASYLOGGINGPP_H_
#define SRC_UTILS_LOG_EASYLOGGINGPP_H_

// 向后兼容适配层：底层已切换为现代高性能日志引擎 spdlog
#include "utils/log/logger.h"

// 兼容旧版初始化宏（空操作）
#define INITIALIZE_EASYLOGGINGPP

#endif  // SRC_UTILS_LOG_EASYLOGGINGPP_H_
