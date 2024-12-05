#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTimerEvent>
#include <QKeyEvent>
//#include <QtOpenGL/qgl.h>   //支持OpenGL
//#include <GL/gl.h>
//#include <GL/glu.h>
#include <QWidget>
#include <QMutex>
#include "ijkmediaplayer.h"
#include "imagescaler.h"


#include <QFile>
#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>

#define YUV420P 0
#define NV12    1


QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

#include"ff_ffplay_def.h"

class Widget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = 0);
    int init(int format, int width, int height);
    int Draw(const Frame *f);
    void StopPlay(){}
    void StartPlay(){}
    ~Widget();

public slots:

    //设置是否yuyv422格式
    void setYuyv(bool yuyv);

    //清空数据
    void clear();

    //设置图片尺寸
    void setFrameSize(int width, int height);

    //更新纹理数据
    void updateTextures(quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY, quint32 linesizeU,
                        quint32 linesizeV);
    void updateTextures(quint8 *dataY, quint8 *dataUV, quint32 linesizeY, quint32 linesizeUV);

    //统一一个函数
    void updateFrame(int width, int height, quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY,
                     quint32 linesizeU, quint32 linesizeV);

protected:
    //OpenGL部件初始化时被调用 设置OpenGL状态和初始化相关资源。
    void initializeGL();
    //重新绘制OpenGL部件时被调用,执行实际的渲染操作
    void paintGL();

private:
    //专门用于渲染YUV420P格式视频帧
    void yuv420pPaintGL();
    //专门用于渲染NV12格式视频帧
    void nv12PaintGL();

    void initData();
    void initColor();
    //初始化着色器程序
    void initShader();
    //初始化纹理对象
    void initTextures();
    //初始化一些渲染参数
    void initParamete();
    void deleteTextures();
    void setProjectionScale();  //根据窗口比例，图像帧比例调整映射关系

private:
    Ui::Widget *ui;
    //是否是yuyv422格式
    bool yuyv;
    //图片宽度高度
    int width_, height_;
    //YUV原数据
    quint8 *dataY; //yuv420p nv12共用
    quint8 *dataU, *dataV; //yuv420p使用
    quint8 *dataUV; //nv12使用
    //YUV数据尺寸
    quint32 lineSizeY, lineSizeU, lineSizeV, lineSizeUV;
    //顶点着色器代码+片段着色器代码
    QString shaderVert, shaderFrag;

    //着色器程序,编译链接着色器
    QOpenGLShaderProgram program;
    //YUV纹理,用于生成纹理贴图
    GLuint textureY, textureU, textureV, textureUV;
    //shader中YUV变量地址
    GLuint textureUniformY, textureUniformU, textureUniformV;

    int format_ = YUV420P;
    //顶点缓冲对象
    QOpenGLBuffer vbo;  //nv12使用

private:
    //直接读取文件
    QFile file;
    //读取文件内容定时器
    QTimer timer;

    int win_width_ ;    //获取控件窗口的宽高
    int win_height_;
    float video_aspect_ratio_ ;
    float win_aspect_ratio_;
    int img_height_ = 0;
    int img_width_ = 0;

    //输入视频帧的宽高
    int  x_, y_,
    video_width, video_height,
    //最终图像
    img_width, img_height;
    // 标记是否需要调整大小
    bool req_resize_ = false;
    // 存储视频帧
    VideoFrame dst_video_frame_;
    // 用于同步的互斥锁
    QMutex m_mutex;

    // 指向 ImageScaler 对象的指针,用于缩放视频帧
    ImageScaler *img_scaler_ = NULL;

private slots:

    //读取文件内容
    void read();

public slots:

    //播放本地文件
    void play(const QString &fileName, int frameRate);

    //停止播放
    void stop();

signals:

    //播放文件结束
    void playFinish();
};
#endif // WIDGET_H
