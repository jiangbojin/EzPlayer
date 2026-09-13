#!/usr/bin/env bash
# ==============================================================================
#  visual_carrier.sh — EzPlayer 虚拟显示载体与可视化捕获控制脚本
#
#  功能：
#    在 Linux 无实体显示器 / CI / SSH 环境下，基于 Xvfb 提供隔离的 X11 虚拟显示环境，
#    驱动 Qt5 xcb 插件真实渲染 EzPlayer GUI 视窗，并通过 FFmpeg 捕获高保真首帧图像。
#
#  用法：
#    ./scripts/visual_carrier.sh capture [输出PNG路径]  # 一键拉起进程并捕获初始界面 (步骤 1+2)
#    ./scripts/visual_carrier.sh start                 # 启动后台持久化虚拟显示服务
#    ./scripts/visual_carrier.sh stop                  # 停止持久化虚拟显示服务
#    ./scripts/visual_carrier.sh status                # 检查虚拟显示状态
# ==============================================================================
set -euo pipefail

# 强制使用 /root 作为唯一用户主目录与 Conan 主目录
export HOME="/root"
export CONAN_HOME="/root/.conan2"

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

# ANSI 颜色控制
GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
BLUE="\033[0;34m"
CYAN="\033[0;36m"
BOLD="\033[1m"
RESET="\033[0m"

RUN_DIR="/tmp/ezplayer_virtual_display"
PID_FILE="$RUN_DIR/xvfb.pid"
APP_PID_FILE="$RUN_DIR/app.pid"
DISPLAY_FILE="$RUN_DIR/display.env"
DEFAULT_OUTPUT="$ROOT/test-image/initial_ui.png"

mkdir -p "$RUN_DIR"
mkdir -p "$ROOT/test-image"

# Qt 平台插件路径适配与 OpenGL 软件渲染加速
if [ -d "/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins" ]; then
    export QT_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins"
fi
if [ -d "/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms" ]; then
    export QT_QPA_PLATFORM_PLUGIN_PATH="/root/project/qt-dev-tools/qt5/usr/lib/x86_64-linux-gnu/qt5/plugins/platforms"
fi
export QT_QPA_PLATFORM=xcb
export LIBGL_ALWAYS_SOFTWARE=1

# 全局进程记录变量（防 trap 作用域丢失）
GLOBAL_APP_PID=""

# 检查依赖工具
check_deps() {
    local missing=()
    for cmd in Xvfb ffmpeg; do
        if ! command -v "$cmd" &>/dev/null; then
            missing+=("$cmd")
        fi
    done
    if [ ${#missing[@]} -ne 0 ]; then
        echo -e "${RED}[ERROR] 缺少必要工具: ${missing[*]}${RESET}" >&2
        exit 1
    fi
}

# 寻找空闲的 Display 端口 (从 120 开始向上探测以避免冲突)
find_free_display() {
    local disp=120
    while [ -e "/tmp/.X11-unix/X${disp}" ] || [ -f "/tmp/.X${disp}-lock" ]; do
        ((disp++))
    done
    echo "$disp"
}

# 内部子执行函数：在已分配的虚拟 DISPLAY 中运行并抓取
_inner_capture() {
    local target_output="${1:-$DEFAULT_OUTPUT}"
    local ezplayer_bin="$ROOT/build/Ezplayer"

    if [ ! -x "$ezplayer_bin" ]; then
        echo -e "${RED}[ERROR] 未找到可执行文件: $ezplayer_bin，请先执行 ./build.sh${RESET}" >&2
        exit 1
    fi

    echo -e "${CYAN}[步骤 1/2] 正在虚拟显示环境 (${DISPLAY}) 中启动 EzPlayer 进程...${RESET}"
    
    local app_log="$RUN_DIR/app.log"
    "$ezplayer_bin" >"$app_log" 2>&1 &
    GLOBAL_APP_PID=$!
    echo "$GLOBAL_APP_PID" > "$APP_PID_FILE"

    # 内部进程安全终止函数
    terminate_app() {
        set +e
        if [ -n "$GLOBAL_APP_PID" ] && kill -0 "$GLOBAL_APP_PID" 2>/dev/null; then
            echo -e "${CYAN}[清理] 正在平稳终止 EzPlayer 进程 (PID: $GLOBAL_APP_PID)...${RESET}"
            kill -TERM "$GLOBAL_APP_PID" 2>/dev/null || true
            local count=0
            while kill -0 "$GLOBAL_APP_PID" 2>/dev/null && [ $count -lt 30 ]; do
                sleep 0.1
                count=$((count + 1))
            done
            if kill -0 "$GLOBAL_APP_PID" 2>/dev/null; then
                kill -KILL "$GLOBAL_APP_PID" 2>/dev/null || true
            fi
        fi
        rm -f "$APP_PID_FILE"
        set -e
        return 0
    }

    # 注册异常退出 trap
    trap terminate_app INT TERM

    # 等待窗口映射与首帧渲染完成
    echo -e "${CYAN}[等待渲染] 探测主窗口初始化状态...${RESET}"
    local retries=30
    while [ $retries -gt 0 ]; do
        if ! kill -0 "$GLOBAL_APP_PID" 2>/dev/null; then
            echo -e "${RED}[ERROR] EzPlayer 进程提前退出！异常日志:${RESET}"
            cat "$app_log"
            exit 1
        fi
        if command -v xwininfo &>/dev/null; then
            if xwininfo -root -tree 2>/dev/null | grep -iE "Ez播放器|HomeWindow" >/dev/null; then
                echo -e "${GREEN}✓ 探测到 EzPlayer 主窗口已成功映射到虚拟显示屏${RESET}"
                break
            fi
        fi
        sleep 0.2
        ((retries--))
    done

    # 预留 1.5 秒确保 QSS 样式表与 OpenGL/软渲染视口完成重绘
    sleep 1.5

    echo -e "${CYAN}[步骤 2/2] 捕获当前虚拟帧缓冲图像 (1280x720)...${RESET}"
    mkdir -p "$(dirname "$target_output")"
    
    if ffmpeg -v error -f x11grab -video_size 1280x720 -i "${DISPLAY}.0" -vframes 1 -update 1 "$target_output" -y; then
        echo -e "${GREEN}✓ [PASS] 界面图像成功输出到: ${BOLD}$target_output${RESET}"
    else
        echo -e "${RED}✗ [FAIL] 帧缓冲抓取失败${RESET}"
        terminate_app
        exit 1
    fi

    # 验证输出文件有效性
    if [ ! -s "$target_output" ]; then
        echo -e "${RED}✗ [FAIL] 截图文件为空: $target_output${RESET}"
        terminate_app
        exit 1
    fi

    local fsize
    fsize=$(stat -c %s "$target_output" 2>/dev/null || stat -f %z "$target_output")
    local fsize_kb=$((fsize / 1024))
    echo -e "${GREEN}✓ [METRIC] 图像大小: ${fsize_kb} KB (${fsize} bytes)${RESET}"

    if [ "$fsize" -lt 10000 ]; then
        echo -e "${YELLOW}[WARN] 截图文件体积偏小 (<10KB)，请核验是否成功绘制完整窗口${RESET}"
    else
        echo -e "${GREEN}✓ [VERIFY] 截图有效，界面控件与渲染视口已成功绘制！${RESET}"
    fi

    # 主动平稳关闭进程并解除 trap
    terminate_app
    trap - INT TERM

    # 执行 L1 视觉认知与确定性断言分析
    if [ -f "$ROOT/scripts/visual_assert_l1.py" ]; then
        echo -e "\n${CYAN}[AI 断言] 正在执行 L1 视觉认知与确定性断言分析 (OpenCV + ONNX Runtime)...${RESET}"
        local py_bin="/root/miniconda3/envs/work/bin/python3"
        if [ ! -x "$py_bin" ]; then
            py_bin="$(command -v python3)"
        fi
        "$py_bin" "$ROOT/scripts/visual_assert_l1.py" \
            --image "$target_output" \
            --out-dir "$(dirname "$target_output")"
    fi

    return 0
}

# 一键模式：通过 xvfb-run 或动态分配执行隔离闭环
cmd_capture() {
    check_deps
    local output_path="${1:-$DEFAULT_OUTPUT}"

    echo -e "${BLUE}${BOLD}======================================================${RESET}"
    echo -e "${BLUE}${BOLD}     EzPlayer 虚拟显示载体 — 初始界面捕获流水线        ${RESET}"
    echo -e "${BLUE}${BOLD}======================================================${RESET}"
    echo -e "目标输出: ${CYAN}$output_path${RESET}"
    echo -e "分辨率:   ${CYAN}1280x720x24${RESET}"
    echo -e "平台插件: ${CYAN}Qt5 xcb (真机窗口渲染模式 + GLX 软件加速)${RESET}\n"

    # 若安装有 xvfb-run，使用 xvfb-run 自动隔离管理
    if command -v xvfb-run &>/dev/null; then
        xvfb-run -a -s "-screen 0 1280x720x24 +extension GLX +render -noreset -nolisten tcp" \
            bash -c "\"$ROOT/scripts/visual_carrier.sh\" _capture_direct \"$output_path\""
    else
        local disp
        disp=$(find_free_display)
        echo -e "${CYAN}[Xvfb] 启动虚拟显示服务 :$disp...${RESET}"
        Xvfb ":$disp" -screen 0 1280x720x24 +extension GLX +render -noreset -nolisten tcp &
        local xvfb_pid=$!
        sleep 1
        DISPLAY=":$disp" "$ROOT/scripts/visual_carrier.sh" _capture_direct "$output_path"
        kill "$xvfb_pid" 2>/dev/null || true
    fi

    echo -e "\n${BLUE}${BOLD}======================================================${RESET}"
    echo -e "${GREEN}${BOLD}✓ 初始界面 1 + 2 步骤执行闭环完成！${RESET}"
    echo -e "${BLUE}${BOLD}======================================================${RESET}"
}

# 启动后台持久化虚拟显示服务 (用于多步骤交互与点击测试)
cmd_start() {
    check_deps
    if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
        echo -e "${YELLOW}[WARN] 虚拟显示载体已在运行中 (PID: $(cat "$PID_FILE"), DISPLAY: $(cat "$DISPLAY_FILE"))${RESET}"
        return 0
    fi

    local disp
    disp=$(find_free_display)
    echo -e "${CYAN}正在启动持久化 Xvfb 虚拟显示服务 (DISPLAY=:$disp, 1280x720x24)...${RESET}"
    Xvfb ":$disp" -screen 0 1280x720x24 +extension GLX +render -noreset -nolisten tcp &
    local xvfb_pid=$!
    echo "$xvfb_pid" > "$PID_FILE"
    echo ":$disp" > "$DISPLAY_FILE"

    sleep 1
    if kill -0 "$xvfb_pid" 2>/dev/null; then
        echo -e "${GREEN}✓ 虚拟显示载体启动成功！${RESET}"
        echo -e "  DISPLAY: ${BOLD}:$disp${RESET}"
        echo -e "  PID:     ${BOLD}$xvfb_pid${RESET}"
    else
        echo -e "${RED}✗ 虚拟显示载体启动失败${RESET}"
        rm -f "$PID_FILE" "$DISPLAY_FILE"
        exit 1
    fi
}

# 停止后台持久化虚拟显示服务
cmd_stop() {
    if [ -f "$APP_PID_FILE" ]; then
        local app_pid
        app_pid=$(cat "$APP_PID_FILE")
        if kill -0 "$app_pid" 2>/dev/null; then
            echo -e "${CYAN}终止后台 EzPlayer 进程 (PID: $app_pid)...${RESET}"
            kill -TERM "$app_pid" 2>/dev/null || true
        fi
        rm -f "$APP_PID_FILE"
    fi

    if [ -f "$PID_FILE" ]; then
        local xvfb_pid
        xvfb_pid=$(cat "$PID_FILE")
        if kill -0 "$xvfb_pid" 2>/dev/null; then
            echo -e "${CYAN}停止 Xvfb 虚拟显示服务 (PID: $xvfb_pid)...${RESET}"
            kill "$xvfb_pid" 2>/dev/null || true
        fi
        rm -f "$PID_FILE" "$DISPLAY_FILE"
        echo -e "${GREEN}✓ 虚拟显示服务已安全停止${RESET}"
    else
        echo -e "${YELLOW}[INFO] 未发现运行中的虚拟显示服务${RESET}"
    fi
}

# 状态查询
cmd_status() {
    if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
        echo -e "${GREEN}● 虚拟显示服务正在运行${RESET}"
        echo -e "  PID:     $(cat "$PID_FILE")"
        echo -e "  DISPLAY: $(cat "$DISPLAY_FILE")"
        if [ -f "$APP_PID_FILE" ] && kill -0 "$(cat "$APP_PID_FILE")" 2>/dev/null; then
            echo -e "  APP PID: $(cat "$APP_PID_FILE") (EzPlayer)"
        fi
    else
        echo -e "${YELLOW}○ 虚拟显示服务未运行${RESET}"
    fi
}

# 命令行路由
ACTION="${1:-capture}"
case "$ACTION" in
    capture)
        cmd_capture "${2:-$DEFAULT_OUTPUT}"
        ;;
    _capture_direct)
        _inner_capture "${2:-$DEFAULT_OUTPUT}"
        ;;
    start)
        cmd_start
        ;;
    stop)
        cmd_stop
        ;;
    status)
        cmd_status
        ;;
    *)
        echo "用法: $0 {capture [输出PNG]|start|stop|status}"
        exit 1
        ;;
esac
