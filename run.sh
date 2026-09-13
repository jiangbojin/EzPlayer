#!/bin/bash
# ===== Ezplayer Linux 运行脚本 =====
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# 支持 --offscreen 便捷参数注入 QT_QPA_PLATFORM
ARGS=()
for arg in "$@"; do
    if [ "$arg" == "--offscreen" ]; then
        export QT_QPA_PLATFORM="offscreen"
    else
        ARGS+=("$arg")
    fi
done

# Qt 平台插件路径适配
if [ -d "/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms" ]; then
    export QT_QPA_PLATFORM_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms"
fi

# 维持与注入 QT_QPA_PLATFORM 环境变量
if [ -n "$QT_QPA_PLATFORM" ]; then
    export QT_QPA_PLATFORM
    echo "[INFO] 使用指定的 Qt 平台插件: QT_QPA_PLATFORM=$QT_QPA_PLATFORM"
elif [ -z "$DISPLAY" ]; then
    echo "[WARN] 检测到当前无图形 DISPLAY 环境，若在无头服务器/SSH终端测试，可使用 QT_QPA_PLATFORM=offscreen ./run.sh 或 ./run.sh --offscreen"
fi

# 优先执行现代 CMake + Ninja 的 Out-of-source 构建产物 (build/Ezplayer)
if [ -f "$SCRIPT_DIR/build/Ezplayer" ]; then
    TARGET_BIN="$SCRIPT_DIR/build/Ezplayer"
elif [ -f "$SCRIPT_DIR/Ezplayer" ]; then
    TARGET_BIN="$SCRIPT_DIR/Ezplayer"
else
    echo "[ERROR] 未找到 Ezplayer 可执行程序，请先执行 ./build.sh 进行编译构建。"
    exit 1
fi

echo "[INFO] 正在启动 Ezplayer: $TARGET_BIN"
exec "$TARGET_BIN" "${ARGS[@]}"
