@echo off
REM ===== Ezplayer 一键构建脚本 =====
REM 从 deps_config.pri 读取工具链路径，自动设置 PATH 后构建

setlocal enabledelayedexpansion

REM --- 从 deps_config.pri 解析 MINGW_DIR 和 QT_DIR ---
set "CONFIG_FILE=%~dp0deps_config.pri"

if not exist "%CONFIG_FILE%" (
    echo [ERROR] 未找到 deps_config.pri，请先创建配置文件。
    exit /b 1
)

for /f "usebackq tokens=1,2,3 delims== " %%a in ("%CONFIG_FILE%") do (
    if "%%a"=="MINGW_DIR" set "MINGW_DIR=%%b"
    if "%%a"=="QT_DIR" set "QT_DIR=%%b"
)

REM 将正斜杠转为反斜杠
set "MINGW_DIR=%MINGW_DIR:/=\%"
set "QT_DIR=%QT_DIR:/=\%"

echo [INFO] MINGW_DIR = %MINGW_DIR%
echo [INFO] QT_DIR    = %QT_DIR%

REM --- 检查路径有效性 ---
if not exist "%MINGW_DIR%\bin\g++.exe" (
    echo [ERROR] 未找到 g++.exe，请检查 deps_config.pri 中的 MINGW_DIR
    exit /b 1
)
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [ERROR] 未找到 qmake.exe，请检查 deps_config.pri 中的 QT_DIR
    exit /b 1
)

REM --- 设置 PATH（MinGW 优先） ---
set "PATH=%MINGW_DIR%\bin;%QT_DIR%\bin;%PATH%"

REM --- 解析命令行参数 ---
set "BUILD_MODE=debug"
set "CLEAN_FIRST=0"

if "%1"=="release" set "BUILD_MODE=release"
if "%1"=="clean"   set "CLEAN_FIRST=1"

REM --- 清理（可选） ---
if "%CLEAN_FIRST%"=="1" (
    echo [INFO] 清理旧构建文件...
    if exist Makefile del /q Makefile Makefile.Debug Makefile.Release 2>nul
    if exist debug rd /s /q debug 2>nul
    if exist release rd /s /q release 2>nul
    echo [INFO] 清理完成。
)

REM --- 运行 qmake ---
echo [INFO] 正在运行 qmake...
"%QT_DIR%\bin\qmake.exe" Ezplayer.pro -spec win32-g++ "CONFIG+=%BUILD_MODE%" "CONFIG+=qml_debug"
if errorlevel 1 (
    echo [ERROR] qmake 执行失败！
    exit /b 1
)

REM --- 构建 ---
echo [INFO] 正在构建 (%BUILD_MODE%)...
"%MINGW_DIR%\bin\mingw32-make.exe" -f Makefile.%BUILD_MODE:d=D% -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo [ERROR] 构建失败！
    exit /b 1
)

echo.
echo [SUCCESS] 构建完成！输出: %BUILD_MODE%\Ezplayer.exe
