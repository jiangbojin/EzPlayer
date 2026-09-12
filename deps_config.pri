# ===== 依赖路径配置 =====
# 修改以下变量以适配您的开发环境，然后重新运行 qmake
# 路径分隔符请使用正斜杠 /

# ---------- 工具链 ----------
# MinGW 根目录（包含 bin/g++.exe、bin/mingw32-make.exe）
MINGW_DIR = D:/VS/Qt/Tools/mingw810_64

# Qt 安装目录（包含 bin/qmake.exe）
QT_DIR    = D:/VS/Qt/5.15.2/mingw81_64

# ---------- 第三方库 ----------
# FFmpeg 7.1 Windows x64 SDK 路径
FFMPEG_DIR = D:/VS/ffmpeg/ffmpeg-7.1

# SDL2 x64 SDK 路径
SDL2_DIR   = $$PWD/SDL2

# OpenSSL 头文件路径（仅供 Qt 网络模块配置使用）
OPENSSL_INCLUDE_DIR = D:/VS/ffmpeg/ffmpeg-7.1/include/openssl

# 日志库路径
LOG_DIR    = $$PWD/log
