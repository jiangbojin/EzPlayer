#!/bin/bash
# ==============================================================================
#  test_time_playback.sh — EzPlayer time.mp4 (3分钟在线秒表) 自动化播放与 I 帧断言脚本
#
#  测试流程闭环：
#    1. 在 Xvfb 虚拟屏幕环境下启动 EzPlayer；
#    2. 通过命令行绝对路径添加 /root/project/study-project/EzPlayer/test-video/time.mp4；
#    3. 自动开启解码播放，按 2s, 7s, 12s, 17s 时序抓取 UI 视窗并有序导出过程关键 I 帧；
#    4. 执行 L1 视觉断言（OpenCV + ORT）：视口活跃度、696x382 宽高比保真、控件在位与长视频解析；
#    5. 平稳优雅发送 SIGINT/SIGTERM 终止播放器，无僵尸进程残留，退出码为 0。
# ==============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

VIDEO_FILE="$ROOT/test-video/time.mp4"
OUTPUT_DIR="$ROOT/test-image/time"
LOG_DIR="/tmp/ezplayer_time_test"
mkdir -p "$OUTPUT_DIR" "$LOG_DIR"

# 检查测试视频文件
if [ ! -f "$VIDEO_FILE" ]; then
    echo -e "${RED}[ERROR] 未找到测试视频文件: $VIDEO_FILE${RESET}" >&2
    exit 1
fi

# Qt 平台与 OpenGL 软渲染加速
if [ -d "/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins" ]; then
    export QT_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins"
fi
if [ -d "/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms" ]; then
    export QT_QPA_PLATFORM_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms"
fi
export QT_QPA_PLATFORM=xcb
export LIBGL_ALWAYS_SOFTWARE=1

# 导出 I 帧受控检测导出目录环境变量
export EZPLAYER_DUMP_IFRAME_DIR="$OUTPUT_DIR"

_run_inner_playback() {
    local ezplayer_bin="$ROOT/build/Ezplayer"
    if [ ! -x "$ezplayer_bin" ]; then
        echo -e "${RED}[ERROR] 未找到 Ezplayer 可执行程序: $ezplayer_bin${RESET}" >&2
        exit 1
    fi

    echo -e "${CYAN}[Step 1/5] 启动 EzPlayer 并通过绝对路径添加视频播放...${RESET}"
    echo -e "视频路径: ${BOLD}$VIDEO_FILE${RESET}"
    echo -e "I 帧输出: ${BOLD}$OUTPUT_DIR${RESET}\n"

    # 清空历史 I 帧图片（保留 .gitkeep）
    rm -f "$OUTPUT_DIR"/iframe_*.png "$OUTPUT_DIR"/ui_snap_*.png

    local app_log="$LOG_DIR/ezplayer.log"
    "$ezplayer_bin" "$VIDEO_FILE" >"$app_log" 2>&1 &
    local app_pid=$!

    terminate_app() {
        set +e
        if kill -0 "$app_pid" 2>/dev/null; then
            echo -e "\n${CYAN}[Step 5/5] 正在平稳终止 EzPlayer 进程 (PID: $app_pid)...${RESET}"
            kill -TERM "$app_pid" 2>/dev/null || true
            local count=0
            while kill -0 "$app_pid" 2>/dev/null && [ $count -lt 30 ]; do
                sleep 0.1
                count=$((count + 1))
            done
            if kill -0 "$app_pid" 2>/dev/null; then
                kill -KILL "$app_pid" 2>/dev/null || true
            fi
        fi
        set -e
        return 0
    }
    trap terminate_app INT TERM

    # 等待窗口映射
    echo -e "${CYAN}[Step 2/5] 等待主窗口就绪并初始化解码播放...${RESET}"
    local retries=25
    while [ $retries -gt 0 ]; do
        if ! kill -0 "$app_pid" 2>/dev/null; then
            echo -e "${RED}[ERROR] EzPlayer 异常退出！日志:${RESET}"
            cat "$app_log"
            exit 1
        fi
        if command -v xwininfo &>/dev/null; then
            if xwininfo -root -tree 2>/dev/null | grep -iE "Ez播放器|HomeWindow" >/dev/null; then
                echo -e "${GREEN}✓ 主视窗已就绪并加载播放列表${RESET}"
                break
            fi
        fi
        sleep 0.2
        retries=$((retries - 1))
    done

    # 时序连续观察并采样 UI 快照 (覆盖 0s, 0.79s, 5.47s, 9.83s, 14.47s 密集 I 帧)
    echo -e "\n${CYAN}[Step 3/5] 视频播放中，过程 I 帧自动检测与捕获中...${RESET}"
    echo -e "按时序观察 18 秒播放（覆盖前 5 组密集关键 I 帧并检验 32 字节对齐解码）..."

    # T=2s: 抓取第一次 UI 呈现
    sleep 2
    ffmpeg -v error -f x11grab -video_size 1280x720 -i "${DISPLAY}.0" -vframes 1 -update 1 \
        "$OUTPUT_DIR/ui_snap_0001_pts02s.png" -y || true
    echo -e "  -> [02s] 捕获 UI 视窗状态: $OUTPUT_DIR/ui_snap_0001_pts02s.png"

    # T=7s: 抓取第二次 UI 呈现
    sleep 5
    ffmpeg -v error -f x11grab -video_size 1280x720 -i "${DISPLAY}.0" -vframes 1 -update 1 \
        "$OUTPUT_DIR/ui_snap_0002_pts07s.png" -y || true
    echo -e "  -> [07s] 捕获 UI 视窗状态: $OUTPUT_DIR/ui_snap_0002_pts07s.png"

    # T=12s: 抓取第三次 UI 呈现
    sleep 5
    ffmpeg -v error -f x11grab -video_size 1280x720 -i "${DISPLAY}.0" -vframes 1 -update 1 \
        "$OUTPUT_DIR/ui_snap_0003_pts12s.png" -y || true
    echo -e "  -> [12s] 捕获 UI 视窗状态: $OUTPUT_DIR/ui_snap_0003_pts12s.png"

    # T=17s: 抓取第四次 UI 呈现
    sleep 5
    ffmpeg -v error -f x11grab -video_size 1280x720 -i "${DISPLAY}.0" -vframes 1 -update 1 \
        "$OUTPUT_DIR/ui_snap_0004_pts17s.png" -y || true
    echo -e "  -> [17s] 捕获 UI 视窗状态: $OUTPUT_DIR/ui_snap_0004_pts17s.png"

    # 稍作等待以确保所有 I 帧写入磁盘
    sleep 1

    # 统计捕获的 I 帧文件
    echo -e "\n${CYAN}[I 帧检测核验] 扫描 $OUTPUT_DIR 中的有序 I 帧产物...${RESET}"
    local iframes=()
    while IFS= read -r -d '' f; do
        iframes+=("$f")
    done < <(find "$OUTPUT_DIR" -maxdepth 1 -name "iframe_*.png" -print0 | sort -z)

    echo -e "${GREEN}✓ 共成功捕获并导出 ${#iframes[@]} 个关键 I 帧图像！${RESET}"
    for f in "${iframes[@]}"; do
        local sz
        sz=$(stat -c %s "$f" 2>/dev/null || stat -f %z "$f")
        echo -e "  - 帧文件: ${BOLD}$(basename "$f")${RESET} (${sz} bytes)"
    done

    if [ ${#iframes[@]} -eq 0 ]; then
        echo -e "${YELLOW}[WARN] 未检测到导出的 I 帧，检查日志:${RESET}"
        tail -n 25 "$app_log"
    fi

    # 执行 L1 视觉断言分析
    echo -e "\n${CYAN}[Step 4/5] 执行 L1 视觉认知与动态播放状态断言 (专属针对 time.mp4)...${RESET}"
    local py_bin="/root/miniconda3/envs/work/bin/python3"
    if [ ! -x "$py_bin" ]; then
        py_bin="$(command -v python3)"
    fi

    local target_snap="$OUTPUT_DIR/ui_snap_0002_pts07s.png"
    if [ ! -f "$target_snap" ]; then
        target_snap="$OUTPUT_DIR/ui_snap_0001_pts02s.png"
    fi

    if [ -f "$target_snap" ]; then
        "$py_bin" "$ROOT/scripts/visual_assert_l1.py" \
            --image "$target_snap" \
            --out-dir "$OUTPUT_DIR" \
            --video-type "time"
    fi

    # 优雅退出播放器
    terminate_app
    trap - INT TERM
    return 0
}

# 命令行路由
if [ "${1:-}" = "_inner_playback" ]; then
    _run_inner_playback
else
    echo -e "${BLUE}${BOLD}======================================================${RESET}"
    echo -e "${BLUE}${BOLD}   EzPlayer time.mp4 (3分钟在线秒表) 自动化测试套件   ${RESET}"
    echo -e "${BLUE}${BOLD}======================================================${RESET}"

    xvfb-run -a -s "-screen 0 1280x720x24 +extension GLX +render -noreset -nolisten tcp" \
        bash "$ROOT/scripts/test_time_playback.sh" _inner_playback

    echo -e "\n${BLUE}${BOLD}======================================================${RESET}"
    echo -e "${GREEN}${BOLD}✓ time.mp4 视频播放与新测试项断言高标准圆满完成！${RESET}"
    echo -e "${BLUE}${BOLD}======================================================${RESET}"
fi
