# EzPlayer - 基于 FFmpeg 的音视频播放器

[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Windows-blue.svg)](#)
[![Qt](https://img.shields.io/badge/Qt-5.15.2-green.svg)](https://www.qt.io/)
[![FFmpeg](https://img.shields.io/badge/FFmpeg-4.2.1-orange.svg)](https://ffmpeg.org/)
[![C++](https://img.shields.io/badge/C++-17-blue.svg)](#)

## 📖 项目简介

EzPlayer 是一个基于 **Qt 5.15** 和 **FFmpeg 4.2** 开发的 Windows 桌面音视频播放器。项目参考 ijkplayer 的架构设计，采用 **消息队列** 驱动的播放控制模型，支持本地文件与 RTMP 流媒体播放，并集成了 DeepSeek AI 助手。

### 🎥 演示视频

[▶ 音视频播放器展示 — Bilibili](https://www.bilibili.com/video/BV1PyiXYnEcy?vd_source=55dfba5031ed1a014c1ac576a7abd107)

### 📸 界面预览

![播放器界面](播放器界面.png)

## ✨ 主要特性

### 🎯 核心播放

| 功能 | 说明 |
|------|------|
| 多格式支持 | 基于 FFmpeg，支持 H.264 / H.265 / VP8 / VP9 等主流编解码格式 |
| 硬件解码 | 可选 NVIDIA CUVID / Intel QSV / AMD AMF 等 GPU 加速 |
| RTMP 流媒体 | 内置 `RTMPPlayer` 拉流模块，支持断线重连 |
| 音视频同步 | 基于 PTS 时钟的 A/V 同步策略 |
| 变速播放 | 集成 Sonic 库，支持变速不变调（0.5×–2.0×） |

### 🎮 播放控制

- **播放 / 暂停 / 停止** — 完整的播放状态机管理
- **快进 / 快退** — 可配置步长的 seek 操作
- **进度条拖动** — 精确到毫秒的进度定位
- **音量调节 & 一键静音** — 滑动条 + 按钮控制
- **上一集 / 下一集** — 播放列表联动

### 📋 高级功能

- **播放列表** — 支持本地文件添加、拖放导入、网络 URL 管理，持久化保存
- **截图** — 一键截取当前帧并保存为 JPEG
- **缓冲监控** — 实时显示音频 / 视频缓冲区时长
- **延迟追赶** — 直播流自动加速追赶延迟（可配置最大缓存 & 抖动区间）
- **AI 助手** — 集成 DeepSeek API，提供对话式交互
- **Toast 提示** — 播放状态、错误信息的分级弹框提示
- **日志系统** — 基于 EasyLogging++，支持多级别日志输出到文件 & 终端

## 🏗️ 技术架构

### 核心技术栈

| 组件 | 技术 | 版本 |
|------|------|------|
| GUI 框架 | Qt (Widgets + OpenGL + Network) | 5.15.2 |
| 音视频解码 | FFmpeg | 4.2.1 |
| 音频输出 | SDL2 | 2.0+ |
| 变速处理 | Sonic 库 | — |
| AI 集成 | DeepSeek API (HTTPS) | — |
| 日志 | EasyLogging++ | — |
| 构建工具 | qmake + MinGW 8.1 | — |
| 语言标准 | C++17 | — |

### 模块关系

```
┌──────────────────────────────────────────────────────────────────────┐
│                        HomeWindow (主窗口)                           │
│  ┌─────────┐  ┌──────────┐  ┌──────────┐  ┌───────────────────────┐ │
│  │Playlist │  │DisplayWind│  │DeepSeek  │  │  控制栏 / 设置面板    │ │
│  │播放列表  │  │视频渲染   │  │AI 助手   │  │  (进度/音量/变速…)    │ │
│  └────┬─────┘  └─────┬─────┘  └──────────┘  └───────────────────────┘ │
│       │              │                                               │
│  ┌────▼──────────────▼──────────────────────────────────────────┐    │
│  │              IjkMediaPlayer (播放器封装层)                     │    │
│  │  播放状态机 · seek · 音量 · 变速 · 截图 · 硬件解码选择          │    │
│  └──────────────────────┬───────────────────────────────────────┘    │
│                         │                                           │
│  ┌──────────────────────▼───────────────────────────────────────┐    │
│  │              FFPlayer (FFmpeg 播放引擎)                        │    │
│  │  解复用 · 解码 · 音视频同步 · 帧队列 · 硬件加速 · Sonic变速     │    │
│  └──────────┬──────────────────────────────┬────────────────────┘    │
│             │                              │                        │
│  ┌──────────▼──────────┐       ┌───────────▼────────────┐           │
│  │   RTMPPlayer        │       │   MessageQueue          │           │
│  │   RTMP 拉流/重连    │       │   线程安全消息队列       │           │
│  └─────────────────────┘       └────────────────────────┘           │
└──────────────────────────────────────────────────────────────────────┘
```

## 🚀 快速开始

### 环境要求

| 项目 | 要求 |
|------|------|
| 操作系统 | Windows 10 或更高 |
| 编译器 | MinGW 8.1 (32-bit)，项目内已配置路径 |
| Qt | 5.15.2 (MinGW 32-bit) |
| FFmpeg | 4.2.1（已随项目附带 `ffmpeg-4.2.1-win32-dev/`） |
| SDL2 | 随项目附带 `SDL2/` 目录 |

### 第一步：配置依赖路径

编辑项目根目录的 **`deps_config.pri`**，将路径修改为你本机的安装位置：

```ini
# MinGW 根目录
MINGW_DIR = D:/VS/Qt/Tools/mingw810_32

# Qt 安装目录
QT_DIR    = D:/VS/Qt/5.15.2/mingw81_32
```

> [!TIP]
> FFmpeg、SDL2、OpenSSL、EasyLogging++ 均已包含在项目目录中（使用 `$$PWD` 相对路径），通常无需额外修改。

### 第二步：编译

**方式一：使用一键构建脚本（推荐）**

```bat
:: Debug 构建（默认）
build.bat

:: Release 构建
build.bat release

:: 清理后重新构建
build.bat clean
```

**方式二：手动 qmake 构建**

```bat
:: 确保 MinGW 和 Qt 的 bin 目录已在 PATH 中
qmake Ezplayer.pro -spec win32-g++ "CONFIG+=debug"
mingw32-make -f Makefile.Debug -j%NUMBER_OF_PROCESSORS%
```

**方式三：Qt Creator**

用 Qt Creator 打开 `Ezplayer.pro`，选择 MinGW 32-bit Kit 后直接编译运行。

**方式四：Visual Studio**

打开 `Ezplayer.sln`，使用 Qt VS Tools 插件进行构建。

### 第三步：运行

```bat
:: Debug 版
debug\Ezplayer.exe

:: Release 版
release\Ezplayer.exe
```

> [!IMPORTANT]
> 运行前请确保 `dll/` 目录中的动态库（FFmpeg、SDL2 等）位于可执行文件同级目录或系统 PATH 中。

## 📁 项目结构

```
EzPlayer/
├── homewindow.cpp/h/ui       # 主窗口 —— UI 布局、播放控制逻辑、消息循环
├── ijkmediaplayer.cpp/h       # 播放器封装层 —— 状态机、API 接口
├── ff_ffplay.cpp/h            # FFmpeg 播放引擎 —— 解复用/解码/同步
├── ff_ffplay_def.cpp/h        # 播放引擎数据结构定义（帧队列、PacketQueue 等）
├── rtmpplayer.cpp/h           # RTMP 拉流模块
├── rtmpbase.cpp/h             # RTMP 底层封装
├── displaywind.cpp/h/ui       # 视频显示窗口（QPainter 绘制）
├── imagescaler.cpp/h          # 视频帧缩放（swscale）
├── playlist.cpp/h/ui          # 播放列表组件
├── medialist.cpp/h            # 播放列表数据管理
├── deepseekclient.cpp/h/ui    # DeepSeek AI 客户端
├── screenshot.cpp/h           # 截图功能（保存为 JPEG）
├── sonic.cpp/h                # Sonic 变速库
├── customslider.cpp/h         # 自定义滑动条控件
├── toast.cpp/h                # Toast 提示组件
├── messagequeue.cpp/h         # 线程安全消息队列
├── commonlooper.cpp/h         # 通用事件循环基类
├── globalhelper.cpp/h         # 全局工具函数、FFmpeg/SDL 头文件引入
├── urldialog.cpp/h/ui         # 网络 URL 输入对话框
├── mediabase.h                # 媒体基础数据结构
├── ffmsg.h                    # 播放器消息定义
├── ff_fferror.h               # 错误码定义
├── ijksdl_timer.cpp/h         # SDL 计时器封装
├── main.cpp                   # 程序入口、日志初始化
│
├── Ezplayer.pro               # qmake 工程文件
├── deps_config.pri            # 依赖路径配置（用户需修改）
├── build.bat                  # 一键构建脚本
├── resource.qrc               # Qt 资源文件
├── res/                       # 图标、样式表等静态资源
│   └── qss/                   # QSS 样式文件
├── log/                       # EasyLogging++ 源码
│   └── easylogging++.h/cc
│
├── ffmpeg-4.2.1-win32-dev/    # FFmpeg SDK（头文件 + 库）
├── SDL2/                      # SDL2 SDK
├── dll/                       # 运行时所需的动态库
├── 框架设计和分析/              # 架构设计文档 & Visio 图
└── assets/                    # 其他资源文件
```

## 🔧 可配置项

| 分类 | 参数 | 默认值 | 说明 |
|------|------|--------|------|
| 缓冲 | 最大缓存时长 | 1000 ms | 音视频队列缓存上限 |
| 缓冲 | 抖动区间 | 100 ms | 缓冲波动容忍范围 |
| 网络 | 连接超时 | 5 秒 | 流媒体连接最大等待时间 |
| 播放 | 加速因子 | 1.5× | 直播追赶时的加速倍率 |
| 硬件 | 解码类型 | 未选择 | 支持 CUVID / QSV / AMF |

以上参数均可在播放器的**设置面板**中实时调整。

## 📊 核心设计要点

- **消息驱动模型** — `MessageQueue` 负责线程间通信，`HomeWindow::Loop()` 在独立线程轮询消息并分发处理
- **IJK 架构复用** — `IjkMediaPlayer` → `FFPlayer` 的两层封装，上层管理状态机，底层专注解码与同步
- **A/V 同步** — 以音频时钟为基准，视频帧通过 delay/drop 策略对齐
- **RTMP 断线重连** — `RTMPPlayer` 继承 `CommonLooper`，在独立线程拉流，检测断流后自动重连
- **变速不变调** — 通过 Sonic 库对 PCM 数据进行时域拉伸/压缩

## 📝 更新日志

### v1.2.1 (2026年3月9日)

### v1.2.0 (2024-12-05)

- ✨ 新增 RTMP 流媒体支持及断线重连
- 🔧 优化网络连接超时和错误处理
- 🎯 新增硬件解码支持（CUVID / QSV / AMF）
- ⚡ 新增直播流延迟追赶机制

### v1.1.0 (2024-11-20)

- 🎵 基础音视频播放功能（FFmpeg + SDL2）
- 🎮 完整播放控制（播放/暂停/停止/快进/快退/seek）
- 📋 播放列表管理（添加/删除/持久化/拖放导入）
- 📸 一键截图功能
- 🤖 DeepSeek AI 助手集成
- 🎨 QSS 样式主题

## 📄 许可证

本项目采用 MIT 许可证 — 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- [FFmpeg](https://ffmpeg.org/) — 音视频解码与处理
- [Qt](https://www.qt.io/) — GUI 框架
- [SDL2](https://www.libsdl.org/) — 音频输出
- [Sonic](https://github.com/nicholasgasior/sonic) — 变速不变调算法
- [EasyLogging++](https://github.com/amrayn/easyloggingpp) — 轻量日志库
- [DeepSeek](https://www.deepseek.com/) — AI 助手 API
- [ijkplayer](https://github.com/bilibili/ijkplayer) — 架构设计参考

## 📞 联系方式

- **博客**: [C9程序猿 — CSDN](https://blog.csdn.net/weixin_50873490?type=blog)

---

⭐ 如果这个项目对你有帮助，欢迎点一个 Star！
