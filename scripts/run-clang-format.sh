#!/usr/bin/env bash
# ==============================================================================
#  run-clang-format.sh — 就地格式化 EzPlayer 源码与测试代码
# ==============================================================================
set -euo pipefail

CF="${CLANG_FORMAT:-clang-format}"
if ! command -v "$CF" >/dev/null 2>&1; then
    echo "[ERROR] 找不到 $CF。请先安装 clang-format 或指定 CLANG_FORMAT 环境变量。" >&2
    exit 1
fi

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# 查找 src/ 和 tests/ 源码文件，排除第三方日志库 src/utils/log 及历史/临时目录
mapfile -t files < <(
    find src tests \
        -path 'src/utils/log' -prune -o \
        \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.cc' -o -name '*.c' \) \
        -type f -print 2>/dev/null
)

if [ ${#files[@]} -eq 0 ]; then
    echo "[INFO] 未找到需要格式化的源文件。"
    exit 0
fi

echo "[INFO] 使用 $($CF --version | head -n 1)"
echo "[INFO] 正在格式化 ${#files[@]} 个源文件..."
"$CF" -i --style=file "${files[@]}"
echo "[SUCCESS] 格式化完成。"
