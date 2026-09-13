# EzPlayer Agent 工程规约与基础设施指南

本项目（EzPlayer）是一套面向 Linux / Windows 平台的现代化桌面音视频播放器。为了指导 AI Coding Agent 在本项目中高标准、无破坏地进行代码阅读、开发、重构与测试验证，特制定本规约。

---

## 1. 项目全景概览

### 1.1 项目定位与技术栈
EzPlayer 深度参考了 **ijkplayer** 的架构设计哲学，采用 **消息队列驱动** 的多线程异步播放控制模型。

- **编程语言标准**：C++17（严格遵循现代 C++17 范式）
- **GUI 视窗与网络框架**：Qt 5.15.2（Core, Gui, Widgets, Network, OpenGL）
- **音视频解复用与解码**：FFmpeg 7.1（avformat, avcodec, avutil, swscale, swresample, avfilter, avdevice, postproc）
- **音频底层驱动与输出**：C++17 原生高精度时钟驱动线程 + WebRTC M153 音频引擎（已完全剥离 SDL2 依赖）
- **音频变速变调引擎**：Sonic 算法库（支持 0.5× ~ 2.0× 变速不变调）
- **构建系统与生成器**：Modern CMake (>= 3.20) + Ninja
- **依赖包管理系统**：Conan 2.x（基于 Profile 声明式管理第三方现代 C++ 依赖）
- **日志引擎**：spdlog (v1.14.1) 现代高性能线程安全日志（多 Sink 彩色控制台 + 滚动文件），并提供 `LOG(LEVEL) << ...` 向后兼容层
- **单元测试框架**：GoogleTest (v1.14.0) + CTest
- **静态分析与代码格式**：clang-format (Google Style 微调), clang-tidy, clangd

### 1.2 关键环境路径与构建资产约定
为避免不同宿主机环境中的编译器或头文件冲突，本项目在 Linux 环境下约定如下底层依赖路径：
- **Qt5 本地依赖库**：`/root/project/qt-dev-tools/qt5/usr`（CMake 模块前缀路径包含 `/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/cmake`）
- **Qt5 平台插件**：`/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms`
- **FFmpeg 7.1 SDK**：`/root/project/ffmpeg-7.1/dist`（提供完整的 include 与 lib，已自动配置 rpath 动态加载）
- **构建输出目录**：`build/`（Out-of-source 隔离构建，主程序产物为 `build/Ezplayer`，测试产物为 `build/tests/ezplayer_tests`）
- **语言服务数据库**：`build/compile_commands.json`（构建后根目录自动建立软链接）

---

## 2. 架构拓扑地图 (src/ 目录架构)

项目源码经过现代化分层重构，严格划分为 5 大核心功能模块，各模块职责分明、依赖单向递进：

```
EzPlayer/src/
├── core/            # 核心播放引擎、音视频管道与底层线程机制
├── render/          # 画面渲染视口 (OpenGL GPU 硬件着色 & 软件渲染)
├── ui/              # GUI 视窗、控件、交互菜单与播放列表
├── network/         # 流媒体拉流 (RTMP) 与外部智能服务 (DeepSeek)
├── utils/           # 公共辅助类与统一日志系统
└── main.cpp         # 应用程序唯一入口与全局日志初始化
```

### 2.1 模块职责与拓扑边界

| 模块 | 核心源码文件 | 核心职责 | 依赖与边界约束 |
|---|---|---|---|
| **`core/`** | `ijkmediaplayer.cpp/.h`<br>`ff_ffplay.cpp/.h`<br>`ff_ffplay_def.cpp/.h`<br>`messagequeue.cpp/.h`<br>`commonlooper.cpp/.h`<br>`imagescaler.cpp/.h`<br>`sonic.cpp/.h`<br>`ijksdl_timer.cpp/.h` | **底层播放引擎与音视频管道**：<br>1. `IjkMediaPlayer`：对外高层播放器控制器与状态机管理（IDLE, PREPARED, PLAYING, PAUSED 等）；<br>2. `FFPlayer`：底层 FFmpeg 管道，管理解复用（read_thread）、视频解码（video_thread）、音频解码（audio_thread）、A/V 同步（PTS 时钟校准）；<br>3. `MessageQueue`：线程安全的环形复用消息队列；<br>4. `CommonLooper`：基于 C++ 线程的异步轮询事件基类；<br>5. `Sonic`：PCM 音频变速变调算法。 | **核心纯逻辑与管道层**：<br>• 严禁包含任何 `QtWidgets` 或 GUI 控件头文件；<br>• 跨层通知必须通过 `MessageQueue` 或回调函数向上传递；<br>• 允许依赖 FFmpeg、WebRTC、Sonic（已彻底移除 SDL2 依赖）。 |
| **`render/`** | `opengldisplaywidget.cpp/.h`<br>`displaywind.cpp/.h`<br>`displaywind.ui` | **画面呈现引擎**：<br>1. `OpenGLDisplayWidget`：继承自 `QOpenGLWidget` + `QOpenGLFunctions`，利用 GLSL 顶点/片元着色器实现 YUV420P 三纹理单元硬件渲染与 GPU 颜色空间转换；<br>2. `DisplayWind`：包含画面承载、双击全屏切换、支持在 OpenGL 硬件渲染与 QPainter 软件渲染间平滑热切换。 | **渲染与呈现层**：<br>• 依赖 `QOpenGLWidget`、`QPainter` 与 `core/` 中的图像帧结构；<br>• 专注图像呈现，不参与播放控制逻辑。 |
| **`ui/`** | `homewindow.cpp/.h`<br>`playlist.cpp/.h`<br>`customslider.cpp/.h`<br>`screenshot.cpp/.h`<br>`toast.cpp/.h`<br>`urldialog.cpp/.h` | **人机交互与用户界面**：<br>1. `HomeWindow`：主视窗，继承自 `CommonLooper` 接收 `MessageQueue` 消息更新 UI，持有控制栏、进度条、音量控件与视口；<br>2. `Playlist`：播放列表、文件导入与持久化；<br>3. `CustomSlider`：优化原生 QSlider 的点击跳转与拖拽平滑体验；<br>4. `Screenshot`：当前视频帧截取与导出；<br>5. `Toast` / `UrlDialog`：提示气泡与网络流地址输入。 | **视图与表现层**：<br>• 依赖 `core/`（调用高层播放器 API）与 `render/`（嵌入视频视口）；<br>• 严格运行于 Qt GUI 主线程，严禁在 UI 槽函数中执行耗时阻塞操作。 |
| **`network/`** | `deepseekclient.cpp/.h`<br>`deepseekclient.ui`<br>`rtmpplayer.cpp/.h`<br>`rtmpbase.cpp/.h` | **流媒体与外部网络服务**：<br>1. `RTMPPlayer` / `RTMPBase`：定制的 RTMP 网络拉流模块，支持流状态监控与断线重连；<br>2. `DeepSeekClient`：基于 `QNetworkAccessManager` 实现的 DeepSeek 大模型对话交互组件。 | **网络拓展层**：<br>• 依赖 `Qt5::Network` 与 `core/`；<br>• 所有网络 I/O 必须全异步处理，禁止阻塞。 |
| **`utils/`** | `globalhelper.cpp/.h`<br>`log/logger.cpp/.h`<br>`log/easylogging++.h` | **基础工具库与日志服务**：<br>1. `GlobalHelper`：QSS 样式表加载、时间格式转换、文件路径辅助；<br>2. `log/`：基于 Conan 管理的高性能 `spdlog` 日志封装，支持彩色控制台 + 滚动文件，且保留 `easylogging++.h` 向后兼容层。 | **底层公共支撑**：<br>• 全工程共享，被所有模块按需包含；<br>• 保持轻量与高内聚。 |

---

## 3. 开发与验证命令清单

Agent 在执行代码修改、功能新增或重构时，必须遵循以下标准命令链，严禁跳过验证环节：

### 3.1 一键自动化验证闭环（推荐 Agent 优先使用）
项目提供了全流程自动化验证脚本，串联构建、CTest 测试与 Qt 无头冒烟：
```bash
./scripts/agent_verify.sh
```
该脚本将依次执行：
1. `./build.sh` 现代 CMake + Ninja 编译构建；
2. `ctest` 执行全量单元测试（MessageQueue、SonicSpeed、CommonLooper）；
3. 设置 `QT_QPA_PLATFORM=offscreen` 并在独立子进程中进行 3 秒 GUI 冒烟验证；
4. 汇总输出清晰的 `PASS / FAIL` 状态矩阵。

### 3.2 编译构建命令
```bash
# 1. 默认构建 (Release 模式，使用 Ninja 亚秒级增量编译)
./build.sh

# 2. 构建 Debug 模式 (适合 GDB 调试或内存分析)
./build.sh debug

# 3. 彻底清理构建目录及历史产物
./build.sh clean

# 4. 手动 CMake 构建 (若需微调参数)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

### 3.3 自动化测试命令
```bash
# 运行全部 CTest 单元测试套件，失败时自动打印详细堆栈
ctest --test-dir build --output-on-failure

# 针对特定测试用例单独执行
ctest --test-dir build -R "MessageQueueTest.*" --output-on-failure
ctest --test-dir build -R "SonicSpeedTest.*" --output-on-failure
ctest --test-dir build -R "CommonLooperTest.*" --output-on-failure
```

### 3.4 格式化与静态检查命令
```bash
# 1. 检查全工程源码格式合规性（只读检查，发现问题以非零码退出）
./scripts/check-clang-format.sh

# 2. 自动就地格式化 src/ 与 tests/ 目录代码 (遵循 .clang-format)
./scripts/run-clang-format.sh
```

### 3.5 运行与冒烟命令
```bash
# 1. 本地常规运行（需要 X11/Wayland 桌面环境）
./run.sh

# 2. 自动化无头运行（在无桌面终端/CI 下测试二进制）
QT_QPA_PLATFORM_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms" \
QT_QPA_PLATFORM=offscreen timeout 3s ./build/Ezplayer
```

### 3.6 虚拟显示载体与视觉闭环测试 (Xvfb + FFmpeg)
```bash
# 1. 一键拉起进程并捕获初始界面图像 (步骤 1+2 闭环，输出至 test-image/initial_ui.png)
./scripts/visual_carrier.sh capture

# 2. 启动/停止/查看持久化虚拟显示服务 (用于多步骤交互与点击测试)
./scripts/visual_carrier.sh start
./scripts/visual_carrier.sh status
./scripts/visual_carrier.sh stop
```

### 3.7 端到端自动化播放与过程关键 I 帧测试
```bash
# 1. synctime.mp4 端到端测试 (40秒标准视频，捕获 4 个 I 帧并执行 L1 视觉断言)
./scripts/test_synctime_playback.sh

# 2. time.mp4 端到端测试 (3分钟在线秒表，捕获 5 个密集 I 帧、核验 696x382 宽高比与 03:00 时长)
./scripts/test_time_playback.sh

# 3. 单独执行 L1 确定性视觉认知断言引擎 (OpenCV + ORT)
python3 scripts/visual_assert_l1.py --image test-image/initial_ui.png
python3 scripts/visual_assert_l1.py --image test-image/time/ui_snap_0002_pts07s.png --video-type time
```

---

## 4. C++17 编码与 Qt 规范

为保证代码的鲁棒性、内存安全与多线程高并发性能，Agent 必须严格遵守以下规则：

### 4.1 RAII 与现代资源生命周期管理
1. **彻底杜绝裸 `new`/`delete` 与 `malloc`/`free`**：
   - 纯 C++ 业务对象优先使用值语义或 `std::unique_ptr` / `std::shared_ptr`；
   - Qt 视图控件必须挂载父对象（Parent-Child Object Tree），由 Qt 框架级联析构，避免手动管理生命周期；
2. **C 风格 SDK 资源必须封装智能删除器**：
   - 对于 FFmpeg 的堆内存对象，严禁直接使用裸指针传递：
     ```cpp
     // 推荐：为 AVFrame 绑定专属释放器
     std::unique_ptr<AVFrame, void(*)(AVFrame*)> frame(av_frame_alloc(), [](AVFrame* f) {
         av_frame_free(&f);
     });
     ```

### 4.2 GUI 主线程安全与跨线程通信准则
1. **GUI 线程绝对零阻塞**：
   - `HomeWindow` 及其所持有的 UI 控件必须始终保持对事件循环的及时响应；
   - 严禁在主线程中调用耗时操作，包括但不限于：`avformat_open_input`（网络探测可能阻塞数十秒）、文件密集 I/O、SDL 音频设备同步等待、大图像素转换；
2. **跨线程通信机制**：
   - 核心引擎向 UI 层上报事件，使用 `MessageQueue` 或 Qt 的信号槽（`emit`，默认 `Qt::QueuedConnection`）；
   - 严禁从子线程直接调用任何 QWidget 的成员函数（如 `setText`、`repaint`、`show` 等）。

### 4.3 日志与调试规范
1. **统一使用 EasyLogging++**：
   - 严禁在源码中引入 `printf`、`std::cout`、`std::cerr` 或裸 `qDebug`（生产代码中）；
   - 统一使用 `LOG(INFO)`、`LOG(DEBUG)`、`LOG(WARNING)`、`LOG(ERROR)`；
   - 涉及网络断连、编解码失败、队列溢出必须使用 `LOG(ERROR)` 记录上下文关键参数。

### 4.4 头文件组织与模块隔离
1. **Include Guard 规范**：
   - 所有头文件必须包含标准的宏防重包含保护（如 `#ifndef SRC_CORE_MESSAGEQUEUE_H ...`）；
2. **统一扁平化模块引用**：
   - CMake 中已通过 `target_include_directories` 导出 `src` 及其子目录；
   - 包含头文件应写为 `#include "core/messagequeue.h"`，严禁编写脆弱的深层跨目录相对路径（如 `#include "../../../src/core/messagequeue.h"`）。

---

## 5. Agent 避坑指南 (实战经验总结)

以下是 EzPlayer 项目开发与 CI 验证中极易踩坑的关键技术细节，Agent 在执行任务前必须仔细研读：

### 坑点 1：Qt 远程/CI 无头运行崩溃 (Offscreen Platform Plugin)
- **现象**：在 SSH 远程终端或自动化脚本中运行 `./build/Ezplayer` 或执行无头测试时，抛出错误并 Dump Core：
  ```text
  qt.qpa.plugin: Could not find the Qt platform plugin "offscreen" in ""
  This application failed to start because no Qt platform plugin could be initialized.
  ```
- **根因**：本项目在 Linux 下使用了独立部署的 Qt5 工具链（`/root/project/qt-dev-tools/qt5`），系统默认环境未配置 Qt 插件搜索路径，导致即使传入 `QT_QPA_PLATFORM=offscreen`，Qt 也无法定位到 `libqoffscreen.so`。
- **Agent 对策**：必须在执行任何 GUI 可执行文件前导出插件路径：
  ```bash
  export QT_QPA_PLATFORM_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms"
  export QT_QPA_PLATFORM=offscreen
  ```
  或者直接使用项目封装好的 `./scripts/agent_verify.sh` 进行全自动测试。

### 坑点 2：SDL2 音频回调与主线程死锁 (Audio Callback Deadlock)
- **现象**：播放器暂停、拖动进度条（Seek）或销毁播放器对象时程序彻底无响应卡死。
- **根因**：SDL2 的音频回调函数（`sdl_audio_callback`）是由底层独立的音频渲染线程定期触发的。如果在音频回调内部加锁试图访问 UI 共享状态，或者主线程持有某一互斥锁的同时调用 `SDL_CloseAudio` / `SDL_PauseAudio` 等待音频线程退出，会形成致命的双向循环锁死（Deadlock）。
- **Agent 对策**：
  1. 音频回调内部必须保持极端精简，仅执行重采样、Sonic 变速变调和内存数据复制（`memcpy`）；
  2. 在音频回调中严禁调用任何 Qt 对象的函数；
  3. 主线程需要操作音频队列时，使用小粒度的短临界区互斥锁或无锁原子操作。

### 坑点 3：MessageQueue 消息对象生命周期与内存泄漏 (AVMessage obj leak)
- **现象**：播放器长时间运行或频繁 seek 后内存线性持续增长。
- **根因**：`AVMessage` 结构设计支持携带任意堆内存对象（`void* obj`）与释放回调（`void (*free_l)(void* obj)`）。如果生产者通过 `notify_msg` 投递了动态分配的结构体，但消费者提取消息后忘记调用 `msg.free_l`，或者在队列提前退出（`msg_queue_abort`）及刷新（`msg_queue_flush`）时未遍历释放残留消息中的 `obj`，堆内存将彻底泄露。
- **Agent 对策**：
  1. 投递带 `obj` 消息必须显式提供自定义释放函数指针：
     ```cpp
     queue.notify_msg(MSG_CUSTOM, 0, 0, strdup("payload"), [](void* p) { free(p); });
     ```
  2. 提取消息后，始终执行安全释放块：
     ```cpp
     if (msg.free_l && msg.obj) {
         msg.free_l(msg.obj);
         msg.obj = nullptr;
     }
     ```

### 坑点 4：第三方 Fat 静态库 main 符号劫持 (Fat Archive `main()` Collision)
- **现象**：测试套件编译成功，但运行时报错 `nasm: unrecognized option '--gtest_list_tests'` 或 `nasm: fatal: no input file specified`。
- **根因**：大型多媒体静态库（如 `WebRtcAvCapture` 或 Chromium 第三方打包库）可能把包含 `main()` 的目标文件（如 `nasm.o`）直接打进了 fat 归档包。如果测试套件依赖 `GTest::gtest_main` 静态库，链接器可能由于顺序优先提取了第三方归档包内的 `main`。
- **Agent 对策**：
  1. 测试套件显式提供独立的测试入口源文件 `tests/test_main.cpp`；
  2. 在 `test_main.cpp` 中定义全局入口函数并初始化 `::testing::InitGoogleTest(&argc, argv)` 与 `ezplayer::log::init("log")`；
  3. `target_link_libraries` 仅链接 `GTest::gtest`，不链接 `GTest::gtest_main`。

### 坑点 5：FFmpeg 7.1 现代 API 迁移规约 (Deprecated APIs)
- **现象**：编译警告 `is deprecated` 或更新后符号缺失。
- **根因**：FFmpeg 7.1 已经全面移除了大量过时老旧 API（例如全局初始化函数 `av_register_all()`、`avcodec_register_all()` 已被完全删除，`AVFrame::pkt_pos` 等字段已被标记为 deprecated）。
- **Agent 对策**：
  1. 不得写出调用过时注册函数的代码；
  2. 解码操作统一使用推荐的 `avcodec_send_packet()` 与 `avcodec_receive_frame()` 发送/接收模型。

### 坑点 6：Conan 2.x 用户环境隔离与 HOME 路径约束 (Root Home Lockdown)
- **现象**：在多用户或非 root 终端调用 `conan install` 时找不到缓存、下载被权限拒绝，或污染普通用户主目录。
- **根因**：Conan 2.x 默认使用 `$HOME/.conan2` 存放 package cache 与 remotes。
- **Agent 对策**：
  1. 本工程在 Linux 主控环境下**一律严格使用 `/root` 作为 HOME**；
  2. 所有构建与自动化脚本开头必须强制导出：
     ```bash
     export HOME="/root"
     export CONAN_HOME="/root/.conan2"
     ```
  3. 严禁把普通用户主目录（如 `/home/<user>/.gemini`）用于业务构建上下文。

### 坑点 7：Qt XCB 与 Xvfb 虚拟显示下的 OpenGL 插件定位 (GLX Plugin Lockdown)
- **现象**：在 Xvfb 虚拟显示环境下运行 EzPlayer 时，报错 `QXcbIntegration: Cannot create platform OpenGL context, neither GLX nor EGL are enabled` 并引发崩溃。
- **根因**：若仅配置了 `QT_QPA_PLATFORM_PLUGIN_PATH` 指向 `platforms` 目录，Qt 将无法定位上一级 `plugins/xcbglintegrations/libqxcb-glx-integration.so` 插件，导致 OpenGL 视口初始化失败。
- **Agent 对策**：
  1. 必须同时显式导出 Qt 顶层插件搜索路径：
     ```bash
     export QT_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins"
     export QT_QPA_PLATFORM_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms"
     export LIBGL_ALWAYS_SOFTWARE=1
     ```
   2. 启动 Xvfb 时显式添加扩展标志：`Xvfb :DISPLAY -screen 0 1280x720x24 +extension GLX +render -noreset -nolisten tcp`。

### 坑点 8：非标准宽度下的 sws_scale SIMD 32 字节对齐踩堆 (SIMD Stride Alignment Heap Corruption)
- **现象**：播放特定视频（如 696x382 的 `time.mp4`）并在主线程或渲染回调中截取/转存关键帧图像时，程序抛出堆损坏异常：`malloc(): corrupted top size` 并 Core Dump 崩溃。
- **根因**：若使用 `QImage img(width, height, Format_RGB888)` 直接把 `img.bits()` 作为 `sws_scale` 的输出目标，`QImage` 的跨距仅为 4 字节对齐。对于非 16 或 32 倍数的宽度（如 $696 \times 3 = 2088$ 字节），FFmpeg 的 AVX2/SSE 向量汇编指令在写入行尾时会**写穿行缓冲区边界**，损坏 glibc 堆块头信息（Top Chunk Size）。
- **Agent 对策**：
  1. 涉及任何 FFmpeg 像素格式转换，目标内存一律使用 `av_image_alloc(..., 32)` 显式按 **32 字节对齐** 分配；
  2. 转换完成后，通过 `QImage(dst_data[0], width, height, dst_linesize[0], ...)` 包装并调用 `copy()` 深拷贝；
  3. 最后调用 `av_freep(&dst_data[0])` 安全释放堆内存。

---

## 6. 工具链与配置文件一览

| 配置文件/脚本 | 作用 |
|---|---|
| `AGENTS.md` | 本工程面向 AI Coding Agent 的核心规范、拓扑地图与避坑指南 |
| `CLAUDE.md` | 软链接指向 `AGENTS.md`，兼容不同 Agent 平台上下文读取习惯 |
| `conanfile.txt` | Conan 2.x 声明式包清单 (包含 spdlog, gtest) |
| `conan/profiles/linux-default` | Linux 平台 Conan Profile (GCC 11, C++17, libstdc++11, Ninja) |
| `.clang-format` | 适配 C++17 与 Qt 风格的代码排版格式规范 |
| `.clang-tidy` | 聚焦 `bugprone` 与 `clang-analyzer` 的静态代码质量分析配置 |
| `.clangd` | 配合 `build/compile_commands.json` 实现极致精准的 LSP 语法索引与补全 |
| `scripts/agent_verify.sh` | **核心闭环验证脚本**：一键执行构建、CTest 测试、无头冒烟与虚拟显示视觉抓图自检 |
| `scripts/visual_carrier.sh` | **虚拟显示载体控制脚本**：Xvfb 虚拟屏幕启动、停止与真机视窗首帧抓图 |
| `scripts/visual_assert_l1.py` | **L1 确定性视觉断言引擎**：基于 OpenCV+ORT 判定视口活跃度、宽高比、控件在位与缺陷标红 |
| `scripts/test_synctime_playback.sh` | **synctime 端到端测试脚本**：40 秒标准视频播放、4 组关键 I 帧捕获与断言判定 |
| `scripts/test_time_playback.sh` | **time 端到端测试脚本**：3 分钟在线秒表视频播放、5 组密集 I 帧捕获与 696x382 宽高比断言 |
| `scripts/conan-install.sh` | Conan 2.x 依赖自动下载安装与 CMake 生成器产物输出脚本 |
| `scripts/check-clang-format.sh` | 提交前只读检查格式规范（CI 守护） |
| `scripts/run-clang-format.sh` | 就地批量格式化 `src/` 与 `tests/` 下的所有源码文件 |
| `build.sh` | 顶层构建脚本，自动探测 Conan 依赖并调用 CMake + Ninja 实现自动化构建 |
| `run.sh` | 顶层启动脚本，封装了 Linux 平台插件与可执行程序探测 |
| `test-video/` | 端到端自动化测试视频样本目录 (`synctime.mp4`, `time.mp4`) |
| `test-image/` | 自动化视觉测试截屏存储与断言标红目录 (`synctime/`, `time/`) |
| `docs/06-visual-testing-and-headless-carrier.md` | **专题六**：虚拟显示载体、AI 视觉认知断言与端到端播放测试白皮书 |

