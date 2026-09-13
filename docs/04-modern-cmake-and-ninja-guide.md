# 现代 CMake + Ninja 构建与测试指南

## 1. 迁移背景与架构演进

在早期版本中，EzPlayer 主要采用 Qt 传统的 `qmake` 构建系统（`Ezplayer.pro` + `deps_config.pri`）。随着项目规模扩大、`src/` 源码深度模块化以及测试驱动开发（TDD）规范的引入，旧体系暴露出多项短板：

| 对比维度 | 传统 qmake 构建体系 | 现代 CMake + Ninja 体系 (当前) |
| :--- | :--- | :--- |
| **构建隔离度** | 默认源码树内（In-source）混杂生成 `*.o`, `moc_*`, `ui_*` | 严格 Out-of-source 独立目录（`build/`），源码根目录零污染 |
| **构建速度** | 使用传统 `make`，依赖扫描慢，增量构建动辄数秒 | 深度结合 `Ninja`，全量高并发并行，增量检查仅需 **0.17 秒** |
| **目标抽象模型** | 依赖全局变量（`LIBS`、`INCLUDEPATH`、`DEFINES`） | **Target-First 现代模型**，通过目标属性（Target Properties）精准封装 |
| **依赖传递与管理** | 手动拼接静态路径，移植环境需改动多处 `.pri` | 基于 `find_package` 与标准导入目标（Imported Target），依赖路径自解析 |
| **模块化组织** | 所有源码平铺在根目录，界限模糊 | 清晰划分 `src/core`、`src/render`、`src/ui`、`src/network`、`src/utils` |
| **质量与测试支持** | 无官方自动化测试框架，依赖人工跑界面校验 | 集成 **GoogleTest + CTest** 工业级单元测试体系，支持自动化回归 |
| **IDE 与生态协同** | 局限于 Qt Creator 或手写 `.sln` | 原生输出 `compile_commands.json` 与 `CMakePresets.json`，无缝协同主流 IDE |

---

## 2. 核心设计与实现解析

### (1) 顶层目标化定义 ([CMakeLists.txt](../CMakeLists.txt))

- **版本与语言标准**：最低要求 CMake 3.20，启用 C++17 标准（`CMAKE_CXX_STANDARD 17`）。
- **自动化工具链集成**：
  ```cmake
  set(CMAKE_AUTOMOC ON)
  set(CMAKE_AUTORCC ON)
  set(CMAKE_AUTOUIC ON)
  ```
  自动调度 `moc`、`uic` 与 `rcc`，中间生成的元对象代码全部隔离在构建目录（如 `build/Ezplayer_autogen/`），彻底消除源码树污染。
- **模块化代码组织与包含路径**：
  源码按照领域驱动设计划分至 `src/` 各子目录：
  - `src/core`：解复用、时钟同步、解码调度、线程循环与队列
  - `src/render`：OpenGL 硬件着色渲染与软件绘制窗口
  - `src/utils`：全局工具、基于 Conan 管理的现代高性能 `spdlog` 日志库（保留向后兼容流式语法）
- **Conan 2.x 声明式包管理与工具链注入**：
  引入 `conanfile.txt` 声明依赖（`spdlog`、`gtest`），通过 `scripts/conan-install.sh` 生成 `conan_toolchain.cmake` 与 CMake 导入模块配置。`build.sh` 会自动检测并调用 Conan 完成依赖闭环安装。
- **运行期动态 RPATH 处理**：
  自动将外部编译的 FFmpeg 7.1 库目录注入生成的二进制目标的 `BUILD_RPATH` / `INSTALL_RPATH`。在 Linux 调试阶段，无需在每次启动前额外配置 `LD_LIBRARY_PATH`。

### (2) FFmpeg 模块化标准探测 ([cmake/FindFFmpeg.cmake](../cmake/FindFFmpeg.cmake))

编写了标准的 `FindFFmpeg` 查找模块，支持 `FFMPEG_ROOT`、环境变量 `FFMPEG7_DIR` 以及系统标准安装路径的多级自适应探测，并为 8 大核心组件构建了现代 CMake 导入目标：
- 聚合目标：`FFmpeg::FFmpeg`
- 具体组件：`FFmpeg::avformat`、`FFmpeg::avcodec`、`FFmpeg::avutil`、`FFmpeg::swscale`、`FFmpeg::swresample` 等

### (3) 标准化配置与测试预设 ([CMakePresets.json](../CMakePresets.json))

项目内置了标准 CMake Presets 配置（Schema Version 3），统一了开发、构建和测试流程：

- **configurePresets**：
  - `default`：Release 优化模式构建，启用 `CMAKE_EXPORT_COMPILE_COMMANDS`。
  - `debug`：生成完整调试符号，输出至 `build/debug`。
  - `release`：独立的 Release 专属构建目录 `build/release`。
- **buildPresets**：
  - 分别与 `default`、`debug`、`release` 配置预设完整一一对应，支持一键并发编译。
- **testPresets**：
  - 提供 `default`、`debug`、`release` 自动化测试预设；
  - 关联对应的 `configurePreset`；
  - 默认开启 `output.outputOnFailure: true`，在测试用例断言失败时自动展开详细现场调用栈与变量打印；
  - 默认设定 `execution.noTestsAction: "error"` 避免空跑测试。

---

## 3. 构建与运行操作规范

### 方式一：一键构建与测试脚本（推荐）

项目根目录提供了功能完备的自动化脚本 [`build.sh`](../build.sh)：

```bash
# 默认 Release 构建（使用 CMake + Ninja 并行编译）
./build.sh

# Debug 调试模式构建
./build.sh debug

# 构建并自动运行单元测试套件
./build.sh test

# Debug 模式构建并自动运行单元测试
./build.sh debug test

# 深度清理构建目录与历史生成文件
./build.sh clean

# 传统 qmake 兼容构建（保留回退通道）
./build.sh --legacy-qmake
```

### 方式二：标准 CMake Presets 命令行

```bash
# 1. 配置阶段
cmake --preset default

# 2. 构建阶段
cmake --build --preset default

# 3. 测试阶段（通过 testPresets 运行全量单元测试）
ctest --preset default
```

### 方式三：手动灵活配置

```bash
# 配置与生成 Ninja 构建文件
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 高并发编译主程序与测试套件
cmake --build build -j$(nproc)

# 执行测试并捕获失败现场
ctest --test-dir build --output-on-failure
```

---

## 4. 程序运行

构建生成的主程序位于 `build/Ezplayer`。运行根目录的 [`run.sh`](../run.sh) 脚本即可自动配置 Qt 平台插件环境并优先启动现代 CMake 产物：

```bash
# 正常带图形显示启动 (桌面或本地 X11)
./run.sh

# 无图形终端环境（如远程 SSH 服务器）离屏启动测试
./run.sh --offscreen
# 或通过环境变量注入
QT_QPA_PLATFORM=offscreen ./run.sh
```

---

## 5. GoogleTest 单元测试体系与 CTest 自动化测试

项目在 `tests/` 目录下建立了完整的单元测试套件，通过现代 CMake 的 `enable_testing()` 与 `GoogleTest` 模块实现标准化自动化测试。

### (1) 测试套件覆盖范围

测试二进制程序为 `ezplayer_tests`，涵盖了播放器关键底层核心逻辑的白盒测试（14 个用例 100% 通过）：

| 测试套件 | 测试用例 | 覆盖核心能力与校验点 |
| :--- | :--- | :--- |
| **MessageQueueTest** | `BasicPutAndGet` | 基础消息投递、队列存储、数值参数与取出还原 |
| **MessageQueueTest** | `ObjectPayload` | 结构体指针/自定义内存块在消息中的托管、深拷贝与释放函数回调 (`free_l`) |
| **MessageQueueTest** | `TimeoutBehavior` | 毫秒级超时等待机制，验证空队列下 `wait_for` 正确返回 0 |
| **MessageQueueTest** | `AbortInterruptsWait` | 播放器退出或重置时 `msg_queue_abort()` 强行打断阻塞等待，返回 -1 |
| **MessageQueueTest** | `RemoveSpecificMessage` | 按 `what` 消息类型精准过滤并批量移除特定未消费消息 |
| **SonicSpeedTest** | `CreateAndConfigure` | Sonic 倍速音频处理实例的通道数与采样率初始化配置 |
| **SonicSpeedTest** | `ProcessAudioSamples` | PCM 音频样本的变采样率写入、拉伸处理与输出读取 |
| **CommonLooperTest** | `StartAndStopLifecycle` | 异步事件循环线程的启动、状态标志管理与安全退出生命周期 |
| **SyncQueuesTest** | `PacketQueueLifecycle` | A/V 包缓冲队列 `PacketQueue` 的入队、出队、序列号 `serial` 维护与生命周期 |
| **SyncQueuesTest** | `PacketQueueAbortWakesBlocker` | 验证解复用线程挂起时 `packet_queue_abort` 的唤醒通知机制 |
| **SyncQueuesTest** | `FrameQueueLifecycle` | 视频帧环形缓冲队列 `FrameQueue` 的连续帧存取与回环安全性 |
| **WebRtcAvCaptureTest** | `DeviceManagerEnumeration` | WebRTC 音视频设备管理器初始化与摄像头、屏幕、音频录音/播放设备枚举 |
| **WebRtcAvCaptureTest** | `VideoCapturerLifecycle` | WebRTC 屏幕与摄像头采集器配置动态调整、生命周期启动与安全析构 |
| **WebRtcAvCaptureTest** | `AudioEngineLifecycle` | WebRTC 3A 音频引擎（AEC/ANS/AGC/高通滤波）配置更新与音量控制状态机 |

### (2) 大型静态库与 GTest 符号隔离实践

在链接第三方 Fat Archive（如 `libwebrtc_avcapture.a`）时，归档内可能混杂 Chromium 第三方工具链构建出的目标文件（例如 `nasm.o` 内含 `main` 符号）。若测试套件直接链接 `GTest::gtest_main`，链接器可能由于顺序优先提取了第三方归档包内的 `main` 导致测试入口劫持。

**工程解决方案**：
1. 显式编写 `tests/test_main.cpp` 源文件实现唯一的 `main()` 入口，并在启动时完成 `ezplayer::log::init("log")`；
2. 在 `tests/CMakeLists.txt` 中仅链接 `GTest::gtest`，彻底移除对 `GTest::gtest_main` 静态库的依赖，杜绝符号冲突。

### (3) CTest 动态发现机制

在 `tests/CMakeLists.txt` 中采用 `gtest_discover_tests(ezplayer_tests)`：
- 编译期间无需手动硬编码每个测试用例名；
- CTest 在构建后通过自省机制自动发现所有 `TEST()` 和 `TEST_F()` 宏定义的用例；
- 支持在 CTest 或 IDE 测试面板中按单个用例单独运行与统计耗时。

### (4) 多种测试执行方式

1. **一键自动化构建并测试**：
   ```bash
   ./build.sh test
   ```
2. **利用 CMake Presets 运行测试**：
   ```bash
   ctest --preset default
   # 调试版本测试预设
   ctest --preset debug
   ```
3. **一键端到端全闭环自检**：
   ```bash
   ./scripts/agent_verify.sh
   ```

---

## 6. 构建与测试实测表现

在开发测试宿主机环境下实测：
- **全量干净重编**：自动拉取 Conan 依赖并完整编译链接 Qt MOC、UIC、RCC、`src/` 核心子模块以及 `ezplayer_tests` 单元测试耗时约 **19 秒**。
- **亚秒级增量构建**：无代码修改时增量检查仅需 **0.18 秒**（`ninja: no work to do.`）。
- **单元测试耗时**：CTest 自动化运行全部 14 个核心单元测试全部通过（`100% tests passed out of 14`），耗时约 **5.8 秒**。
