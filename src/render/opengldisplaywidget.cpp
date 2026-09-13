#include "opengldisplaywidget.h"
#include <QDebug>
#include <QMutexLocker>

// ============================================================================
// GLSL Shader 源码
// ============================================================================

// 顶点着色器（YUV420P 和 NV12 共用）
static const char* vertexShaderSource = R"(
    attribute vec4 vertexIn;
    attribute vec2 textureIn;
    varying vec2 textureOut;
    void main(void)
    {
        gl_Position = vertexIn;
        textureOut = textureIn;
    }
)";

// 片段着色器 - YUV420P（3 个独立平面：Y, U, V）
static const char* fragmentShaderYUV420P = R"(
    varying mediump vec2 textureOut;
    uniform sampler2D textureY;
    uniform sampler2D textureU;
    uniform sampler2D textureV;
    void main(void)
    {
        mediump float y = texture2D(textureY, textureOut).r;
        mediump float u = texture2D(textureU, textureOut).r - 0.5;
        mediump float v = texture2D(textureV, textureOut).r - 0.5;

        // BT.601 YUV -> RGB 转换矩阵
        mediump float r = y + 1.403 * v;
        mediump float g = y - 0.344 * u - 0.714 * v;
        mediump float b = y + 1.770 * u;

        gl_FragColor = vec4(r, g, b, 1.0);
    }
)";

// 片段着色器 - NV12（Y 平面 + 交错 UV 平面）
static const char* fragmentShaderNV12 = R"(
    varying mediump vec2 textureOut;
    uniform sampler2D textureY;
    uniform sampler2D textureUV;
    void main(void)
    {
        mediump float y = texture2D(textureY, textureOut).r;
        mediump vec2 uv = texture2D(textureUV, textureOut).ra - vec2(0.5, 0.5);

        // BT.601 YUV -> RGB 转换矩阵
        mediump float r = y + 1.403 * uv.y;
        mediump float g = y - 0.344 * uv.x - 0.714 * uv.y;
        mediump float b = y + 1.770 * uv.x;

        gl_FragColor = vec4(r, g, b, 1.0);
    }
)";

// 顶点数据：位置坐标 (x, y) + 纹理坐标 (s, t)
// OpenGL 坐标系 Y 轴向上，纹理坐标 t 轴需翻转以匹配视频数据方向
static const GLfloat vertexData[] = {
    // 位置            // 纹理坐标
    -1.0f, -1.0f, 0.0f, 1.0f,  // 左下
    1.0f,  -1.0f, 1.0f, 1.0f,  // 右下
    -1.0f, 1.0f,  0.0f, 0.0f,  // 左上
    1.0f,  1.0f,  1.0f, 0.0f,  // 右上
};

// ============================================================================
// 构造与析构
// ============================================================================

OpenGLDisplayWidget::OpenGLDisplayWidget(QWidget* parent)
    : QOpenGLWidget(parent), vbo_(QOpenGLBuffer::VertexBuffer) {
    play_state_ = 2;
}

OpenGLDisplayWidget::~OpenGLDisplayWidget() {
    makeCurrent();
    DeInit();
    doneCurrent();
}

// ============================================================================
// 公共接口
// ============================================================================

int OpenGLDisplayWidget::Draw(const Frame* frame) {
    if (!frame || !frame->frame) {
        return -1;
    }

    QMutexLocker locker(&mutex_);

    AVFrame* avframe = frame->frame;
    int w            = frame->width;
    int h            = frame->height;
    int fmt          = frame->format;

    // 检查是否需要重新分配缓冲区（分辨率或格式变化）
    bool need_realloc = (w != frame_width_ || h != frame_height_ || fmt != pixel_format_);

    if (need_realloc) {
        // 释放旧缓冲区
        if (y_data_) {
            free(y_data_);
            y_data_ = nullptr;
        }
        if (u_data_) {
            free(u_data_);
            u_data_ = nullptr;
        }
        if (v_data_) {
            free(v_data_);
            v_data_ = nullptr;
        }
        if (uv_data_) {
            free(uv_data_);
            uv_data_ = nullptr;
        }

        frame_width_  = w;
        frame_height_ = h;
        pixel_format_ = fmt;

        // 分配新缓冲区
        // YUV420P: Y = w*h, U = w/2 * h/2, V = w/2 * h/2
        // NV12:    Y = w*h, UV = w * h/2
        y_data_ = (uint8_t*)malloc(w * h);
        if (fmt == AV_PIX_FMT_YUV420P) {
            u_data_ = (uint8_t*)malloc((w / 2) * (h / 2));
            v_data_ = (uint8_t*)malloc((w / 2) * (h / 2));
        } else if (fmt == AV_PIX_FMT_NV12) {
            uv_data_ = (uint8_t*)malloc(w * (h / 2));
        }

        // 标记纹理需要重新创建
        textures_created_ = false;
    }

    // 拷贝 Y 平面数据（处理 linesize 对齐）
    if (avframe->linesize[0] == w) {
        memcpy(y_data_, avframe->data[0], w * h);
    } else {
        for (int i = 0; i < h; i++) {
            memcpy(y_data_ + i * w, avframe->data[0] + i * avframe->linesize[0], w);
        }
    }
    y_linesize_ = w;

    if (fmt == AV_PIX_FMT_YUV420P) {
        int half_w = w / 2;
        int half_h = h / 2;

        // 拷贝 U 平面
        if (avframe->linesize[1] == half_w) {
            memcpy(u_data_, avframe->data[1], half_w * half_h);
        } else {
            for (int i = 0; i < half_h; i++) {
                memcpy(u_data_ + i * half_w, avframe->data[1] + i * avframe->linesize[1], half_w);
            }
        }
        u_linesize_ = half_w;

        // 拷贝 V 平面
        if (avframe->linesize[2] == half_w) {
            memcpy(v_data_, avframe->data[2], half_w * half_h);
        } else {
            for (int i = 0; i < half_h; i++) {
                memcpy(v_data_ + i * half_w, avframe->data[2] + i * avframe->linesize[2], half_w);
            }
        }
        v_linesize_ = half_w;

    } else if (fmt == AV_PIX_FMT_NV12) {
        int half_h = h / 2;
        // 拷贝 UV 交错平面
        if (avframe->linesize[1] == w) {
            memcpy(uv_data_, avframe->data[1], w * half_h);
        } else {
            for (int i = 0; i < half_h; i++) {
                memcpy(uv_data_ + i * w, avframe->data[1] + i * avframe->linesize[1], w);
            }
        }
        uv_linesize_ = w;
    }

    has_new_frame_ = true;

    // 触发 GUI 线程重绘（线程安全）
    QMetaObject::invokeMethod(this, "update", Qt::QueuedConnection);

    return 0;
}

void OpenGLDisplayWidget::DeInit() {
    QMutexLocker locker(&mutex_);

    deleteTextures();

    if (vbo_.isCreated()) {
        vbo_.destroy();
    }

    if (shader_program_yuv420p_) {
        delete shader_program_yuv420p_;
        shader_program_yuv420p_ = nullptr;
    }
    if (shader_program_nv12_) {
        delete shader_program_nv12_;
        shader_program_nv12_ = nullptr;
    }

    if (y_data_) {
        free(y_data_);
        y_data_ = nullptr;
    }
    if (u_data_) {
        free(u_data_);
        u_data_ = nullptr;
    }
    if (v_data_) {
        free(v_data_);
        v_data_ = nullptr;
    }
    if (uv_data_) {
        free(uv_data_);
        uv_data_ = nullptr;
    }

    frame_width_      = 0;
    frame_height_     = 0;
    pixel_format_     = -1;
    has_new_frame_    = false;
    textures_created_ = false;
    gl_initialized_   = false;
}

void OpenGLDisplayWidget::StartPlay() {
    QMutexLocker locker(&mutex_);
    play_state_ = 1;
}

void OpenGLDisplayWidget::StopPlay() {
    QMutexLocker locker(&mutex_);
    play_state_    = 2;
    has_new_frame_ = false;
    update();
}

// ============================================================================
// OpenGL 生命周期回调
// ============================================================================

void OpenGLDisplayWidget::initializeGL() {
    initializeOpenGLFunctions();

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glDisable(GL_DEPTH_TEST);

    initShaders();
    initVertexBuffer();

    gl_initialized_ = true;
}

void OpenGLDisplayWidget::resizeGL(int w, int h) {
    QMutexLocker locker(&mutex_);
    calculateViewport(w, h);
}

void OpenGLDisplayWidget::paintGL() {
    QMutexLocker locker(&mutex_);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (play_state_ != 1 || !has_new_frame_ || frame_width_ == 0 || frame_height_ == 0) {
        return;
    }

    // 设置视口（等比缩放）
    glViewport(viewport_x_, viewport_y_, viewport_w_, viewport_h_);

    // 如果分辨率/格式变化过，重新创建纹理
    if (!textures_created_) {
        deleteTextures();
        initTextures();
        textures_created_ = true;
    }

    // 选择合适的 Shader 程序
    QOpenGLShaderProgram* program = nullptr;

    if (pixel_format_ == AV_PIX_FMT_YUV420P) {
        program = shader_program_yuv420p_;
        if (!program)
            return;
        program->bind();

        // 更新 Y 纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures_[0]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, y_linesize_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame_width_, frame_height_, GL_LUMINANCE,
                        GL_UNSIGNED_BYTE, y_data_);

        // 更新 U 纹理
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures_[1]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, u_linesize_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame_width_ / 2, frame_height_ / 2, GL_LUMINANCE,
                        GL_UNSIGNED_BYTE, u_data_);

        // 更新 V 纹理
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, textures_[2]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, v_linesize_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame_width_ / 2, frame_height_ / 2, GL_LUMINANCE,
                        GL_UNSIGNED_BYTE, v_data_);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        // 设置 uniform
        program->setUniformValue("textureY", 0);
        program->setUniformValue("textureU", 1);
        program->setUniformValue("textureV", 2);

    } else if (pixel_format_ == AV_PIX_FMT_NV12) {
        program = shader_program_nv12_;
        if (!program)
            return;
        program->bind();

        // 更新 Y 纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textures_[0]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, y_linesize_);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame_width_, frame_height_, GL_LUMINANCE,
                        GL_UNSIGNED_BYTE, y_data_);

        // 更新 UV 纹理
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textures_[1]);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, uv_linesize_ / 2);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, frame_width_ / 2, frame_height_ / 2,
                        GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, uv_data_);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        program->setUniformValue("textureY", 0);
        program->setUniformValue("textureUV", 1);
    } else {
        // 不支持的格式
        return;
    }

    // 绘制全屏四边形
    vbo_.bind();
    program->enableAttributeArray("vertexIn");
    program->enableAttributeArray("textureIn");
    program->setAttributeBuffer("vertexIn", GL_FLOAT, 0, 2, 4 * sizeof(GLfloat));
    program->setAttributeBuffer("textureIn", GL_FLOAT, 2 * sizeof(GLfloat), 2, 4 * sizeof(GLfloat));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    program->disableAttributeArray("vertexIn");
    program->disableAttributeArray("textureIn");
    vbo_.release();
    program->release();
}

// ============================================================================
// 私有辅助方法
// ============================================================================

void OpenGLDisplayWidget::initShaders() {
    // YUV420P Shader
    shader_program_yuv420p_ = new QOpenGLShaderProgram(this);
    shader_program_yuv420p_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shader_program_yuv420p_->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                                     fragmentShaderYUV420P);
    if (!shader_program_yuv420p_->link()) {
        qWarning() << "YUV420P Shader link failed:" << shader_program_yuv420p_->log();
    }

    // NV12 Shader
    shader_program_nv12_ = new QOpenGLShaderProgram(this);
    shader_program_nv12_->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    shader_program_nv12_->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderNV12);
    if (!shader_program_nv12_->link()) {
        qWarning() << "NV12 Shader link failed:" << shader_program_nv12_->log();
    }
}

void OpenGLDisplayWidget::initTextures() {
    if (frame_width_ == 0 || frame_height_ == 0)
        return;

    if (pixel_format_ == AV_PIX_FMT_YUV420P) {
        glGenTextures(3, textures_);

        // Y 纹理
        glBindTexture(GL_TEXTURE_2D, textures_[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width_, frame_height_, 0, GL_LUMINANCE,
                     GL_UNSIGNED_BYTE, nullptr);

        // U 纹理
        glBindTexture(GL_TEXTURE_2D, textures_[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width_ / 2, frame_height_ / 2, 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);

        // V 纹理
        glBindTexture(GL_TEXTURE_2D, textures_[2]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width_ / 2, frame_height_ / 2, 0,
                     GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);

    } else if (pixel_format_ == AV_PIX_FMT_NV12) {
        glGenTextures(2, textures_);

        // Y 纹理
        glBindTexture(GL_TEXTURE_2D, textures_[0]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, frame_width_, frame_height_, 0, GL_LUMINANCE,
                     GL_UNSIGNED_BYTE, nullptr);

        // UV 交错纹理（使用 LUMINANCE_ALPHA 格式：每个像素 2 字节）
        glBindTexture(GL_TEXTURE_2D, textures_[1]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, frame_width_ / 2, frame_height_ / 2, 0,
                     GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, nullptr);
    }
}

void OpenGLDisplayWidget::initVertexBuffer() {
    vbo_.create();
    vbo_.bind();
    vbo_.allocate(vertexData, sizeof(vertexData));
    vbo_.release();
}

void OpenGLDisplayWidget::deleteTextures() {
    if (textures_[0]) {
        int count = (pixel_format_ == AV_PIX_FMT_NV12) ? 2 : 3;
        glDeleteTextures(count, textures_);
        memset(textures_, 0, sizeof(textures_));
    }
    textures_created_ = false;
}

void OpenGLDisplayWidget::calculateViewport(int windowW, int windowH) {
    if (frame_width_ == 0 || frame_height_ == 0) {
        viewport_x_ = 0;
        viewport_y_ = 0;
        viewport_w_ = windowW;
        viewport_h_ = windowH;
        return;
    }

    double video_aspect  = (double)frame_width_ / frame_height_;
    double window_aspect = (double)windowW / windowH;

    if (window_aspect > video_aspect) {
        // 窗口更宽，以高度为基准
        viewport_h_ = windowH;
        viewport_w_ = (int)(windowH * video_aspect);
        viewport_x_ = (windowW - viewport_w_) / 2;
        viewport_y_ = 0;
    } else {
        // 窗口更高，以宽度为基准
        viewport_w_ = windowW;
        viewport_h_ = (int)(windowW / video_aspect);
        viewport_x_ = 0;
        viewport_y_ = (windowH - viewport_h_) / 2;
    }
}
