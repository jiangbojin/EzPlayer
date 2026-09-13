#!/usr/bin/env bash
# ═══════════════════════════════════════════════════════════════════════════
#  conan-install.sh — 用项目 profile 安装依赖并生成 CMake toolchain
#  参考 /root/project/cc-modern-framework/ 规范
# ═══════════════════════════════════════════════════════════════════════════
set -euo pipefail

# 强制使用 /root 作为唯一用户主目录
export HOME="/root"
export CONAN_HOME="/root/.conan2"

BUILD_TYPE="${1:-Release}"
OUT="${2:-build}"
PROFILE="conan/profiles/linux-default"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

if ! command -v conan >/dev/null 2>&1; then
    echo "[ERROR] 未检测到 conan 命令，请先确认安装 conan 2.x。" >&2
    exit 1
fi

echo "[INFO] ==============================================="
echo "[INFO] Conan 依赖管理安装"
echo "[INFO] 工作主目录: $HOME"
echo "[INFO] Conan 主目录: $CONAN_HOME"
echo "[INFO] Profile: $PROFILE"
echo "[INFO] 构建类型: $BUILD_TYPE"
echo "[INFO] 输出目录: $OUT"
echo "[INFO] ==============================================="

conan install . \
    --output-folder="$OUT" \
    --build=missing \
    -pr:h="$PROFILE" \
    -pr:b="$PROFILE" \
    -s build_type="$BUILD_TYPE" \
    -c tools.cmake.cmaketoolchain:generator=Ninja \
    -c tools.cmake.cmaketoolchain:user_presets=""

echo ""
echo "[SUCCESS] Conan 依赖安装完成！"
echo "[HINT] 生成 toolchain 路径: $OUT/build/$BUILD_TYPE/generators/conan_toolchain.cmake"
