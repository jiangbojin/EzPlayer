# AGENTS.md

## Cursor Cloud specific instructions

### Project overview

EzPlayer is a Windows desktop media player built with Qt 5.15 + FFmpeg 4.2.1 + SDL2, using C++17 and qmake. See `README.md` for full documentation.

### Linux cross-compilation notes

The project targets Windows (MinGW 32-bit), but can be built and run on Linux with the following workarounds:

- **qmake invocation** — The `.pro` file only has `win32 { }` blocks for INCLUDEPATH/LIBS. On Linux, pass them on the command line:
  ```
  qmake Ezplayer.pro -spec linux-g++ "CONFIG+=debug" \
    "INCLUDEPATH += /workspace/ffmpeg-4.2.1-win32-dev/include /usr/include/SDL2 /workspace/log" \
    "LIBS += -lavformat -lavcodec -lavdevice -lavfilter -lavutil -lpostproc -lswresample -lswscale -lSDL2" \
    "QMAKE_CXXFLAGS += -include /workspace/linux_compat.h"
  ```
- **Case-sensitive header** — `homewindow.cpp` includes `<HomeWindow.h>` but the file is `homewindow.h`. A symlink `HomeWindow.h -> homewindow.h` is needed on Linux (already created).
- **Compat shim** — `linux_compat.h` and `linux_compat.cpp` provide stubs for Windows-only APIs (`CoInitialize`, `memcpy_s`) and FFmpeg 4.x functions removed in FFmpeg 6.x (`av_frame_get_channels`, `avcodec_encode_video2`). The `.cpp` must be compiled and linked manually alongside the main Makefile targets.
- **Missing resource** — `res/fullscreen.png` is referenced in `resource.qrc` but missing from the repo; a placeholder copy of `screenBtn.png` is used.

### Lint

Run `cppcheck` for static analysis:
```
cppcheck --enable=warning,style,performance --std=c++17 \
  -I ffmpeg-4.2.1-win32-dev/include -I /usr/include/SDL2 -I log \
  --suppress=missingIncludeSystem --suppress=unknownMacro \
  *.cpp *.h
```

### Build (Linux)

After running qmake (see above), compile with:
```
make -j$(nproc)
```
Then link with the compat shim:
```
g++ -c -pipe -g -std=gnu++1z -fPIC -I/usr/include/x86_64-linux-gnu -o linux_compat.o linux_compat.cpp
# Re-run the linker command from make output, adding linux_compat.o
```

### Run

```
./Ezplayer
```
The app requires a display (X11/Wayland). On headless environments, use `DISPLAY=:1` or Xvfb.

### No automated tests

This project has no automated test suite. Validation is manual via the GUI.
