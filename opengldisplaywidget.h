#ifndef OPENGLDISPLAYWIDGET_H
#define OPENGLDISPLAYWIDGET_H

#include "ijkmediaplayer.h"
#include <QMutex>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLWidget>

/**
 * @brief 基于 Qt OpenGL 的视频渲染控件
 *
 * 使用 QOpenGLWidget + GLSL Shader 在 GPU 上完成 YUV→RGB 转换和渲染，
 * 替代原来基于 QPainter + ImageScaler 的 CPU 软件渲染方案。
 *
 * 支持的像素格式：
 * - AV_PIX_FMT_YUV420P (软解默认输出)
 * - AV_PIX_FMT_NV12    (硬解 av_hwframe_transfer_data 输出)
 */
class OpenGLDisplayWidget : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

public:
  explicit OpenGLDisplayWidget(QWidget *parent = nullptr);
  ~OpenGLDisplayWidget() override;

  /// @brief 接收解码帧并触发渲染（线程安全，可从非 GUI 线程调用）
  /// @param frame FFmpeg 解码输出的 Frame 指针
  /// @return 0 成功, -1 失败
  int Draw(const Frame *frame);

  /// @brief 释放 OpenGL 资源和内部缓冲区
  void DeInit();

  /// @brief 开始播放（设置播放状态）
  void StartPlay();

  /// @brief 停止播放（设置停止状态并清屏）
  void StopPlay();

protected:
  /// @brief 初始化 OpenGL 资源（Shader、纹理、VBO/VAO）
  void initializeGL() override;

  /// @brief 处理窗口大小变化
  void resizeGL(int w, int h) override;

  /// @brief 每帧渲染回调
  void paintGL() override;

private:
  /// @brief 编译并链接 GLSL Shader 程序
  void initShaders();

  /// @brief 创建 Y/U/V 纹理对象
  void initTextures();

  /// @brief 创建顶点缓冲对象 (VBO)
  void initVertexBuffer();

  /// @brief 更新 YUV420P 格式的纹理数据
  void updateYUV420PTextures();

  /// @brief 更新 NV12 格式的纹理数据
  void updateNV12Textures();

  /// @brief 释放纹理资源
  void deleteTextures();

  /// @brief 计算保持宽高比的 viewport
  void calculateViewport(int windowW, int windowH);

private:
  // Shader 程序
  QOpenGLShaderProgram *shader_program_yuv420p_ = nullptr;
  QOpenGLShaderProgram *shader_program_nv12_ = nullptr;

  // OpenGL 纹理 ID
  GLuint textures_[3] = {0, 0, 0}; // Y, U, V (YUV420P) 或 Y, UV (NV12)

  // 顶点缓冲
  QOpenGLBuffer vbo_;

  // 帧数据缓冲区（从解码线程拷贝到此，然后在 GUI 线程上传纹理）
  uint8_t *y_data_ = nullptr;
  uint8_t *u_data_ = nullptr;
  uint8_t *v_data_ = nullptr;
  uint8_t *uv_data_ = nullptr; // NV12 交错 UV 数据

  // 帧尺寸
  int frame_width_ = 0;
  int frame_height_ = 0;

  // linesize（FFmpeg 输出可能有 padding）
  int y_linesize_ = 0;
  int u_linesize_ = 0;
  int v_linesize_ = 0;
  int uv_linesize_ = 0;

  // 像素格式
  int pixel_format_ = -1; // AVPixelFormat

  // 视口参数（等比缩放后的绘制区域）
  int viewport_x_ = 0;
  int viewport_y_ = 0;
  int viewport_w_ = 0;
  int viewport_h_ = 0;

  // 状态标记
  bool has_new_frame_ = false;    // 是否有新帧待上传
  bool textures_created_ = false; // 纹理是否已创建
  bool gl_initialized_ = false;   // OpenGL 是否已初始化

  // 播放状态 (0 - 初始化, 1 - 播放, 2 - 停止)
  int play_state_ = 2;

  // 互斥锁（保护帧数据缓冲区的线程安全访问）
  QMutex mutex_;
};

#endif // OPENGLDISPLAYWIDGET_H
