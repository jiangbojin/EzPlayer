#!/bin/bash
# ===== Ezplayer 一键构建脚本 (现代 CMake + Ninja) =====
set -e

# 强制使用 /root 作为唯一用户主目录
export HOME="/root"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_MODE="Release"
CLEAN_FIRST=0
USE_LEGACY_QMAKE=0
RUN_TESTS=0
BUILD_DIR="$SCRIPT_DIR/build"

# 参数解析
for arg in "$@"; do
    case "$arg" in
        clean)
            CLEAN_FIRST=1
            ;;
        debug|Debug)
            BUILD_MODE="Debug"
            ;;
        release|Release)
            BUILD_MODE="Release"
            ;;
        test|tests|check)
            RUN_TESTS=1
            ;;
        --legacy-qmake|qmake)
            USE_LEGACY_QMAKE=1
            ;;
        -h|--help)
            echo "用法: ./build.sh [clean] [debug|release] [test] [--legacy-qmake]"
            echo ""
            echo "选项:"
            echo "  clean           清理构建目录及历史临时文件"
            echo "  debug           以 Debug 模式构建"
            echo "  release         以 Release 模式构建 (默认)"
            echo "  test            构建完成后执行自动化单元测试 (CTest / GoogleTest)"
            echo "  --legacy-qmake  使用传统的 qmake + make 方式构建"
            exit 0
            ;;
        *)
            echo "[WARN] 未知参数: $arg (支持: clean, debug, release, test, --legacy-qmake)"
            ;;
    esac
done

# 清理操作
if [ "$CLEAN_FIRST" -eq 1 ]; then
    echo "[INFO] 正在执行深度清理..."
    rm -rf "$BUILD_DIR"
    # 清理根目录旧构建产物
    make clean 2>/dev/null || true
    rm -f Makefile Ezplayer .qmake.stash compile_commands.json
    rm -f *.o moc_*.cpp moc_*.o ui_*.h qrc_*.cpp qrc_*.o
    echo "[INFO] 清理完成。"
    if [ "$#" -eq 1 ] && [ "$1" == "clean" ]; then
        exit 0
    fi
fi

# 传统 qmake 构建分支
if [ "$USE_LEGACY_QMAKE" -eq 1 ]; then
    echo "[INFO] 模式: 传统 qmake + make 构建..."
    if ! command -v qmake >/dev/null 2>&1; then
        echo "[ERROR] 未检测到 qmake，请确认 Qt 5 开发环境已配置。"
        exit 1
    fi
    QMAKE_MODE=$(echo "$BUILD_MODE" | tr '[:upper:]' '[:lower:]')
    echo "[INFO] 运行 qmake (CONFIG+=$QMAKE_MODE)..."
    if [ -f "legacy/Ezplayer.pro" ]; then
        qmake legacy/Ezplayer.pro "CONFIG+=$QMAKE_MODE"
    else
        qmake Ezplayer.pro "CONFIG+=$QMAKE_MODE"
    fi
    NPROC=$(nproc 2>/dev/null || echo 4)
    echo "[INFO] 开始构建 (并行任务数: $NPROC)..."
    make -j"$NPROC"
    echo ""
    echo "[SUCCESS] 构建成功！可执行文件: $SCRIPT_DIR/Ezplayer"
    exit 0
fi

# 现代 CMake + Ninja 构建主分支
echo "[INFO] ==============================================="
echo "[INFO] 构建体系: Modern CMake + Ninja"
echo "[INFO] 构建类型: $BUILD_MODE"
echo "[INFO] 构建目录: $BUILD_DIR"
echo "[INFO] ==============================================="

# 检查 cmake
if ! command -v cmake >/dev/null 2>&1; then
    echo "[ERROR] 未检测到 cmake，请先安装 cmake (>= 3.20)。"
    exit 1
fi

# 检查 ninja
GENERATOR="Ninja"
if ! command -v ninja >/dev/null 2>&1; then
    echo "[WARN] 未检测到 ninja，将回退使用 Unix Makefiles 生成器。"
    GENERATOR="Unix Makefiles"
fi

# 自动定位 Qt5 路径（如存在专用目录）
EXTRA_CMAKE_ARGS=()
if [ -d "/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/cmake" ]; then
    EXTRA_CMAKE_ARGS+=("-DCMAKE_PREFIX_PATH=/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/cmake;/root/project/qt-dev-tools/qt5/usr")
fi

# 自动定位 FFmpeg 路径
if [ -z "$FFMPEG_ROOT" ] && [ -z "$FFMPEG_DIR" ]; then
    if [ -d "/root/project/ffmpeg-7.1/dist" ]; then
        EXTRA_CMAKE_ARGS+=("-DFFMPEG_ROOT=/root/project/ffmpeg-7.1/dist")
    fi
fi

# 自动注入 SDL2 头文件搜索路径以保障核心模块与单元测试目标编译
SDL_INC_FLAGS=""
if [ -d "$SCRIPT_DIR/SDL2/include" ]; then
    SDL_INC_FLAGS="-I$SCRIPT_DIR/SDL2/include"
fi
if [ -d "/usr/include/SDL2" ]; then
    SDL_INC_FLAGS="$SDL_INC_FLAGS -I/usr/include/SDL2"
fi
if [ -n "$SDL_INC_FLAGS" ]; then
    EXTRA_CMAKE_ARGS+=("-DCMAKE_CXX_FLAGS=$SDL_INC_FLAGS")
fi

# 配置阶段
echo "[INFO] 配置项目 (CMake -G \"$GENERATOR\")..."
cmake -S "$SCRIPT_DIR" -B "$BUILD_DIR" \
    -G "$GENERATOR" \
    -DCMAKE_BUILD_TYPE="$BUILD_MODE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    "${EXTRA_CMAKE_ARGS[@]}"

# 构建阶段
NPROC=$(nproc 2>/dev/null || echo 4)
echo "[INFO] 开始编译链接 (并行任务数: $NPROC)..."
cmake --build "$BUILD_DIR" -- -j"$NPROC"

# 软链接 compile_commands.json 方便 clangd / IDE 识别
if [ -f "$BUILD_DIR/compile_commands.json" ]; then
    ln -sf "$BUILD_DIR/compile_commands.json" "$SCRIPT_DIR/compile_commands.json"
fi

echo ""
echo "[SUCCESS] 构建成功！主程序位于: $BUILD_DIR/Ezplayer"

# 单元测试分支
if [ "$RUN_TESTS" -eq 1 ]; then
    echo ""
    echo "[INFO] ==============================================="
    echo "[INFO] 运行自动化单元测试 (CTest)..."
    echo "[INFO] ==============================================="
    if command -v ctest >/dev/null 2>&1; then
        ctest --test-dir "$BUILD_DIR" --output-on-failure
        echo "[SUCCESS] 自动化单元测试全部执行通过！"
    elif [ -x "$BUILD_DIR/tests/ezplayer_tests" ]; then
        echo "[INFO] 未检测到 ctest 命令，直接运行测试程序: $BUILD_DIR/tests/ezplayer_tests"
        "$BUILD_DIR/tests/ezplayer_tests"
        echo "[SUCCESS] 自动化单元测试全部执行通过！"
    else
        echo "[ERROR] 未找到测试二进制或 ctest 命令。"
        exit 1
    fi
fi

echo "[HINT] 可直接运行 ./run.sh 启动程序，或运行 ./build.sh test 执行单元测试"
