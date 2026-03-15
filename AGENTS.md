# AGENTS.md

## Cursor Cloud specific instructions

### Project Overview

EzPlayer is a C++17/Qt 5.15 desktop media player originally targeting Windows (MinGW 8.1 32-bit). It uses FFmpeg 4.2.1, SDL2, and OpenGL for rendering, with an integrated DeepSeek AI assistant. See `README.md` for full architecture and feature details.

### Linux Build (Cloud Agent Environment)

The project is designed for Windows but can be cross-built on Linux with the following adaptations:

**Build command:**
```bash
# Create case-sensitivity symlink (Linux filesystems are case-sensitive)
ln -sf homewindow.h HomeWindow.h

# Build FFmpeg compat shim (bridges 4.2.1 API to system FFmpeg 6.x)
gcc -c -fPIC -I/usr/include/x86_64-linux-gnu .compat/ffmpeg_compat.c -o .compat/ffmpeg_compat.o

# Generate Makefile with Linux-specific paths
qmake Ezplayer.pro -spec linux-g++ "CONFIG+=debug" \
  "INCLUDEPATH += /workspace/ffmpeg-4.2.1-win32-dev/include /usr/include/SDL2 /workspace/log" \
  "LIBS += /workspace/.compat/ffmpeg_compat.o -lavformat -lavcodec -lavdevice -lavfilter -lavutil -lpostproc -lswresample -lswscale -lSDL2" \
  "QMAKE_CXXFLAGS += -include /workspace/.compat/win_compat.h"

# Build
make -j$(nproc)
```

**Run command:**
```bash
DISPLAY=:1 QT_QPA_PLATFORM=xcb ./Ezplayer
```

### Key Gotchas

- **Case-sensitivity**: `homewindow.cpp` includes `<HomeWindow.h>` — must create symlink `HomeWindow.h -> homewindow.h` before building.
- **FFmpeg API mismatch**: The bundled FFmpeg 4.2.1 headers (in `ffmpeg-4.2.1-win32-dev/include/`) are used for compilation, while system FFmpeg 6.x `.so` files are used for linking. The `.compat/ffmpeg_compat.c` shim provides removed functions (`av_frame_get_channels`, `avcodec_encode_video2`).
- **Windows COM API**: `CoInitialize`/`CoUninitialize` calls are no-oped via `.compat/win_compat.h`.
- **`memcpy_s`**: Microsoft-specific function shimmed to `memcpy` via `.compat/win_compat.h`.
- **`#pragma execution_character_set`**: MSVC-only pragma; GCC warns but compiles fine.
- **Missing resource**: `res/fullscreen.png` is not committed (`.gitignore` excludes `*.png`); a placeholder is generated during setup.
- **No automated tests**: The project has no test suite. Validation is manual (launch the application, test playback).
- **No lint tool configured**: No linter config files exist in the project.

### System Dependencies (pre-installed by update script)

`qtbase5-dev`, `qt5-qmake`, `qttools5-dev-tools`, `libqt5opengl5-dev`, `libqt5network5`, FFmpeg dev packages (`libavformat-dev`, `libavcodec-dev`, etc.), `libsdl2-dev`, `libssl-dev`, `libgl1-mesa-dev`.
