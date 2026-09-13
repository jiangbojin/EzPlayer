# 视频渲染与 OpenGL 硬件加速

## 1. 渲染架构概览

解码器输出的原始视频帧大多数为 **YUV420P** 格式。在常规的软件渲染中，必须先在 CPU 中通过 `sws_scale` 转换为 RGB 格式，再由 `QPainter` 逐像素绘制。这在大码率或 4K 高分辨率下会消耗大量的 CPU 资源。

为此，EzPlayer 提供了双渲染引擎：
1. **GPU 硬件渲染**（[`OpenGLDisplayWidget`](../src/render/opengldisplaywidget.h)）：基于 `QOpenGLWidget` 与 GLSL Shader，由显卡直接处理 YUV 到 RGB 的矩阵运算与纹理映射，CPU 占用率接近零。
2. **软件渲染备选**（[`DisplayWind`](../src/render/displaywind.h)）：基于 Qt 绘图管道的软件渲染控件，作为无 GPU 或远程无头环境下的降级兜底方案。

---

## 2. 基于 GLSL 的 YUV420P 硬件渲染

### 着色器逻辑

硬件渲染核心分为三个通道纹理管理：
- Y 分量纹理（全分辨率）
- U 分量纹理（1/2 宽，1/2 高）
- V 分量纹理（1/2 宽，1/2 高）

#### 片段着色器 (Fragment Shader) 核心公式：

```glsl
varying vec2 v_texCoord;
uniform sampler2D tex_y;
uniform sampler2D tex_u;
uniform sampler2D tex_v;

void main(void) {
    vec3 yuv;
    vec3 rgb;

    // 分别采样 Y、U、V 纹理通道
    yuv.x = texture2D(tex_y, v_texCoord).r;
    yuv.y = texture2D(tex_u, v_texCoord).r - 0.5;
    yuv.z = texture2D(tex_v, v_texCoord).r - 0.5;

    // BT.601 标准颜色空间转换矩阵
    rgb = mat3(
        1.0,     1.0,      1.0,
        0.0,    -0.39465,  2.03211,
        1.13983,-0.58060,  0.0
    ) * yuv;

    gl_FragColor = vec4(rgb, 1.0);
}
```

### 纹理数据流转

1. 在 `initializeGL()` 中编译着色器程序，分配纹理句柄（`glGenTextures`）。
2. 当解码器交付新的 `AVFrame` 时，触发 `updateFrame(const Frame *frame)`。
3. 在 `paintGL()` 中调用 `glTexSubImage2D` 将解码帧的三个 plane 直接上传到对应 GPU 纹理显存，随后执行绘制。

---

## 3. 渲染引擎热切换

主窗口控制栏提供了渲染模式一键切换功能（`on_renderSwitchBtn_clicked()`）：
- 可以在播放过程中无缝在 `OpenGL 硬件渲染` 与 `软件渲染` 之间进行热切换。
- 主窗口动态调整布局中的活动展示控件指针（`activeDisplay()`），保证播放状态与画面无缝延续。

---

## 4. 硬件解码 (GPU Decode) 扩展方案

除了渲染阶段的 GPU 加速，在解码阶段同样支持调用 GPU 专属硬件解码器（NVIDIA NVDEC/CUVID, Intel QSV, AMD AMF 等）：

![硬件解码方案](assets/image-20241205113604413.png)

- 通过 `av_hwdevice_ctx_create` 与 `avcodec_find_decoder_by_name` 创建硬件解码上下文。
- 解码出的硬件帧（如 `AV_PIX_FMT_CUDA`）可通过硬件映射直接送入渲染管线，实现端到端的零内存拷贝（Zero-Copy）极致播放性能。
