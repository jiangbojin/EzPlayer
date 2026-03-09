# ===== 依赖路径配置 =====
# 修改以下变量以适配您的开发环境，然后重新运行 qmake
# 路径分隔符请使用正斜杠 /

# ---------- 工具链 ----------
# MinGW 根目录（包含 bin/g++.exe、bin/mingw32-make.exe）
MINGW_DIR = D:/VS/Qt/Tools/mingw810_32

# Qt 安装目录（包含 bin/qmake.exe）
QT_DIR    = D:/VS/Qt/5.15.2/mingw81_32

# ---------- 第三方库 ----------
# FFmpeg SDK 路径
FFMPEG_DIR = $$PWD/ffmpeg-4.2.1-win32-dev

# SDL2 路径
SDL2_DIR   = $$PWD/SDL2

# OpenSSL 头文件路径
OPENSSL_INCLUDE_DIR = $$PWD/openssl/win32/include

# 日志库路径
LOG_DIR    = $$PWD/log
