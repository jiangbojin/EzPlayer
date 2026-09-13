#!/usr/bin/env bash
# ==============================================================================
#  check-clang-format.sh — 检查代码格式合规性（只读检查，不修改文件）
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
    echo "[INFO] 未找到可检查的源文件。"
    exit 0
fi

echo "[INFO] 使用 $($CF --version | head -n 1)"
echo "[INFO] 正在检查 ${#files[@]} 个源文件的代码格式..."

if "$CF" --dry-run --Werror --style=file "${files[@]}"; then
    echo "[SUCCESS] ✓ 所有文件代码格式符合规范。"
    exit 0
else
    echo "[ERROR] ✗ 存在不符合 .clang-format 规范的源文件。可运行 scripts/run-clang-format.sh 进行自动修复。" >&2
    exit 1
fi
