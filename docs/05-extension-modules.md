# 扩展模块与周边能力

## 1. DeepSeek AI 智能助手集成

EzPlayer 内置了基于 DeepSeek 大语言模型 API 的智能交互模块（[`src/network/deepseekclient.cpp`](../src/network/deepseekclient.cpp) / [`src/network/deepseekclient.h`](../src/network/deepseekclient.h)），为用户提供播放伴随式 AI 问答能力。

### 技术实现

- **网络通讯**：使用 `QNetworkAccessManager` 发起异步 HTTPS POST 请求，无缝配合 Qt 事件循环。
- **协议交互**：支持标准 OpenAI / DeepSeek 兼容格式的 JSON 请求与鉴权（`Authorization: Bearer <API_KEY>`）。
- **界面集成**：通过独立的对话面板实现消息气泡渲染与滚动交互。

---

## 2. Toast 弹窗通知组件

为提供优雅且轻量的状态反馈，EzPlayer 设计了非模态的悬浮提示组件（[`src/ui/toast.cpp`](../src/ui/toast.cpp) / [`src/ui/toast.h`](../src/ui/toast.h)）：

- **分级提示**：定义了 `Toast::Level` 枚举（`INFO`、`SUCCESS`、`WARN`、`ERROR`），配合不同主题色框高亮提示。
- **动态定时淡出**：内置 `QTimer`，展示 2~3 秒后自动淡出销毁，不阻碍用户常规播放控制。
- **典型应用场景**：截屏成功/失败通知、硬解加载状态反馈、播放结束提醒等。

---

## 3. spdlog 现代高性能日志系统迁移 (Conan 2.x)

项目已全面废弃旧有的简易单文件日志库 `EasyLogging++`，升级为现代工业级异步/多线程日志框架 **spdlog (v1.14.1)**（[`src/utils/log/logger.h`](../src/utils/log/logger.h) / [`src/utils/log/logger.cpp`](../src/utils/log/logger.cpp)），并通过 **Conan 2.x** 声明式包管理器进行依赖管理与构建集成。

### (1) 多 Sink 路由架构

日志引擎同时挂载两类专业 Sink：
1. **控制台彩色终端 Sink**（`spdlog::sinks::stdout_color_sink_mt`）：高亮区分 `TRACE`、`DEBUG`、`INFO`、`WARN`、`ERROR` 等日志级别；
2. **按大小自动滚动文件 Sink**（`spdlog::sinks::rotating_file_sink_mt`）：输出至 `log/ezplayer.log`，单文件上限 10MB，自动循环保留 5 份历史滚动备份。

### (2) 向后兼容适配层设计

为降低对全工程上万行既有代码的入侵，设计了轻量级 `LogStream` 包装器，并在 [`src/utils/log/easylogging++.h`](../src/utils/log/easylogging++.h) 中提供重定向层，实现 **100% 向后兼容**流式调用语法：

```cpp
// 既有代码完全无感直接使用：
LOG(INFO) << "初始化播放器完成, URL: " << url;
LOG(ERROR) << "网络拉流失败, 错误码: " << ret;
```

### (3) 程序入口统一初始化 ([main.cpp](../src/main.cpp))

```cpp
#include "utils/log/logger.h"

int main(int argc, char* argv[]) {
    // 全局初始化 spdlog（创建目录、配置彩色控制台与滚动日志）
    ezplayer::log::init("log");

    QApplication a(argc, argv);
    HomeWindow w;
    w.show();
    return a.exec();
}
```

---

## 4. WebRTC M153 音视频采集与底层音频引擎

为彻底根除传统 SDL2 音频驱动在多线程与跨平台环境下的死锁与性能瓶颈，EzPlayer 引入了基于现代 WebRTC M153 的音视频采集与底层音频驱动引擎（`WebRtcAvCapture`）：

### (1) 核心能力覆盖
- **专业 3A 音频信号处理**：动态调控回声消除（AEC）、噪声抑制（ANS）、自动增益控制（AGC）与高通滤波；
- **全平台硬件设备枚举**：自动探测与枚举系统麦克风、扬声器、摄像头及多显示器屏幕；
- **屏幕与摄像头捕获管线**：支持基于 PipeWire / X11 的屏幕流与视频帧观察者订阅推送；
- **高精度单调时钟驱动**：基于 C++17 `std::chrono` 与系统单调时钟驱动音频输出，彻底规避了主线程与底层音频设备回调之间的循环锁死。

---

## 5. RTMP 流媒体拉流模块

除了本地文件，播放引擎扩展了对 RTMP 直播流媒体协议的支持（[`src/network/rtmpplayer.cpp`](../src/network/rtmpplayer.cpp) / [`src/network/rtmpbase.cpp`](../src/network/rtmpbase.cpp)）：
- 继承 `CommonLooper` 运行在独立线程中执行拉流循环；
- 解析 FLV Tag（Audio / Video / Script Data）；
- 内置网络断流检测与自动重连机制。
