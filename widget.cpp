#include "widget.h"
#include "ui_widget.h"
#include <QKeyEvent>
#include <QDebug>
#include <math.h>
#include"log/easylogging++.h"
#include<ff_ffplay_def.h>
//#define VersionString ""
#define VersionString "#version 330 \n"
//#define VersionString "#version 330 core \n"
//#define VersionString "#version 330 compatibility \n"

//opengles的float/int等要手动指定精度
inline void initFragment(QStringList &list) {
    bool useOpenGLES = false;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    useOpenGLES = QCoreApplication::testAttribute(Qt::AA_UseOpenGLES);
#endif
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    useOpenGLES = false;
#endif
#ifdef Q_OS_LINUX
    useOpenGLES = false;
#endif
#ifdef __arm__
    useOpenGLES = true;
#endif
#ifdef Q_OS_ANDROID
    useOpenGLES = true;
#endif
    if (useOpenGLES) {
        list << "precision mediump int;";
        list << "precision mediump float;";
    }
}

Widget::~Widget()
{
    makeCurrent();
    if(format_ == NV12) {
        vbo.destroy();
    }
    doneCurrent();
    delete ui;
}

Widget::Widget(QWidget *parent) : QOpenGLWidget(parent) , ui(new Ui::Widget)
{
    ui->setupUi(this);

    win_width_ = width();
    win_height_ = height();
    //关联定时器读取文件
    connect(&timer, SIGNAL(timeout()), this, SLOT(read()));
}

int Widget::init(int format, int width, int height)
{
    this->initData();
    format_ = format;
    this->width_ = width;
    this->height_ = height;
    //GLSL3.0版本后废弃了attribute/varying对应用in/out作为前置关键字
    QStringList list;
    switch (format_) {
    case YUV420P:
        qDebug() << "inti yuv420p" ;
        list << "attribute vec4 vertexIn;";     // vec4 用于接收顶点输入数据
        list << "attribute vec2 textureIn;";    // vec2 用于接收纹理坐标输入数据
        list << "varying vec2 textureOut;"; // 用于将纹理坐标传递到片段着色器中

        list << "void main(void)";
        list << "{";
        list << "  gl_Position = vertexIn;";        // 用于确定顶点在裁剪空间中的位置
        list << "  textureOut = textureIn;";        // 实现传递纹理坐标的操作
        list << "}";
        shaderVert = list.join("");

        list.clear();
        initFragment(list);
        list << "varying mediump vec2 textureOut;"; // 用于接收从顶点着色器传递过来的纹理坐标
        // 同时定义了三个 uniform sampler2D 类型的变量
        // 它们分别对应 YUV420P 格式中的 Y、U、V 三个分量的纹理（后续在渲染时会将实际的纹理对象绑定到这些变量上）
        list << "uniform sampler2D textureY;";
        list << "uniform sampler2D textureU;";
        list << "uniform sampler2D textureV;";

        list << "void main(void)";
        list << "{";
        list << "  vec3 yuv;";
        list << "  vec3 rgb;";
        //可以自行注释xyz以及调整差值看效果(把yz注释画面变成黑白)
        // 通过 texture2D 函数对相应纹理按照传递过来的纹理坐标进行采样，获取 Y、U、V 分量的值，并进行一些简单的数值调整（比如 U、V 分量减去 0.5）。
        list << "  yuv.r = texture2D(textureY, textureOut).r;";
        list << "  yuv.g = texture2D(textureU, textureOut).r - 0.5;";
        list << "  yuv.b = texture2D(textureV, textureOut).r - 0.5;";
        // 然后使用一个 3x3 的矩阵乘法将 yuv 转换为 rgb 颜色空间（这里提供了多个不同的矩阵可供选择，
        // 通过注释不同行可以切换不同的转换矩阵，不同矩阵会带来不同的颜色转换效果）
      //list << "  rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.39465, 2.03211, 1.13983, -0.58060, 0.0) * yuv;";
      //list << "  rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.3455, 1.779, 1.4075, -0.7169, 0.0) * yuv;";
      //list << "  rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.34414, 1.772, 1.402, -0.71414, 0.0) * yuv;";
//        list << "  rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.138, 1.816, 1.540, -0.459, 0.0) * yuv;"; //这个公式偏灰
        list << "  rgb = mat3(1.0f, 1.0f, 1.0f, 0.0f, -0.1873f, 1.8556f, 1.5784f, -0.4681f, 0.0f) * yuv;"; //BT.709
        //得到的 rgb 颜色值组成一个 vec4 类型（增加透明度分量为 1.0）赋值给 gl_FragColor，
        // 这个变量决定了片段最终的颜色输出，也就是渲染出来的图像的像素颜色
        list << "  gl_FragColor = vec4(rgb, 1);";
        list << "}";
        shaderFrag = list.join("");

        yuyv = false;

        break;
    case NV12:
        qDebug() << "inti nv12" ;
        // 这里定义了 vertexIn 为 vec4 类型，通常用于接收顶点的坐标等相关信息
        list << "attribute vec4 vertexIn;";
        // 同时定义了 textureIn 也是 vec4 类型，这个变量一般是用于接收传入的纹理坐标相关数据（在 NV12 格式下，纹理坐标以 vec4 的形式传入
        list << "attribute vec4 textureIn;";
        // 定义了一个 varying 类型的变量 textureOut，它是 vec4 类型。varying 关键字用于声明那些需要从顶点着色器传递到片段着色器的数据，
        // 这里用于将处理后的纹理坐标信息传递给片段着色器，以便在片段着色器中基于这些纹理坐标进行纹理采样等操作。
        list << "varying vec4 textureOut;";

        list << "void main(void)";
        list << "{";
        // 首先将 vertexIn 直接赋值给 gl_Position。gl_Position 是一个内置的特殊变量，它在顶点着色器中用于指定顶点在裁剪空间
        //（Clip Space）中的位置，这个位置信息后续会经过一系列的 OpenGL 图形管线处理，最终决定顶点在屏幕上的显示位置。
        list << "  gl_Position = vertexIn;";
        // 将 textureIn 赋值给 textureOut，实现了将输入的纹理坐标数据传递给 textureOut 变量，以便传递到片段着色器中使用。
        list << "  textureOut = textureIn;";
        list << "}";
        shaderVert = list.join("");

        list.clear();
        initFragment(list);
        // 定义了一个 varying 类型的 vec4 变量 textureOut，用于接收从顶点着色器传递过来的纹理坐标信息
        list << "varying mediump vec4 textureOut;";
        // 定义了两个 uniform sampler2D 类型的变量 textureY 和 textureUV。
        list << "uniform sampler2D textureY;";
        list << "uniform sampler2D textureUV;";
        list << "void main(void)";
        list << "{";
        // 定义了两个 vec3 类型的变量 yuv 和 rgb，用于存储中间的颜色空间转换数据。
        list << "  vec3 yuv;";
        list << "  vec3 rgb;";
        list << "  yuv.r = texture2D(textureY, textureOut.st).r;";
        list << "  yuv.g = texture2D(textureUV, textureOut.st).r - 0.5;";
        list << "  yuv.b = texture2D(textureUV, textureOut.st).g - 0.5;";
        //list << "  rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.39465, 2.03211, 1.13983, -0.58060, 0.0) * yuv;";
//        list << "  rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.3455, 1.779, 1.4075, -0.7169, 0.0) * yuv;"; //偏灰色
        list << "  rgb = mat3(1.0, 1.0, 1.0, 0.0, -0.1873, 1.8556, 1.5784, -0.4681, 0.0) * yuv;"; //BT.709
        list << "  gl_FragColor = vec4(rgb, 1.0);";
        list << "}";
        shaderFrag = list.join("");
    default:
        break;
    }

    return 0;
}

int Widget::Draw(const Frame *f)
{
    static bool falg = true;
    if(falg){
        falg=false;
        init(YUV420P,f->frame->width,f->frame->height);
        LOG(INFO)<<f->frame->width<<"  "<<f->frame->height;
        //变动
        if(dataY)
            delete []dataY;

        dataY = new quint8[(width_ * height_ * 3) >> 1];
        dataU = dataY + (width_ * height_);
        dataV = dataU + ((width_ * height_) >> 2);

        // 初始化数据
        memset(dataY, 0, width_ * height_);
        memset(dataY + width_ * height_, 128, width_ * height_/2);
    }

    // {
    //     const AVFrame * frame = f->frame;
    //     // 如果 img_scaler_ 未初始化或需要调整大小
    //     if(!img_scaler_ || req_resize_) {
    //         // 如果 img_scaler_ 已经存在,先进行反初始化
    //         if(img_scaler_) {
    //             if(dst_video_frame_.data[0]) {
    //                 free(dst_video_frame_.data[0]);
    //                 dst_video_frame_.data[0] = NULL;
    //             }
    //             if(img_scaler_) {
    //                 delete img_scaler_;
    //                 img_scaler_ = NULL;
    //             }
    //         }

    //         // 获取当前窗口的宽高
    //         win_width_ = width();
    //         win_height_ = height();

    //         // 获取输入视频帧的宽高
    //         video_width = frame->width;
    //         video_height = frame->height;

    //         // 创建 ImageScaler 对象
    //         img_scaler_ = new ImageScaler();

    //         // 计算视频和窗口的宽高比
    //         double video_aspect_ratio = frame->width * 1.0 / frame->height;
    //         double win_aspect_ratio = win_width_ * 1.0 / win_height_;

    //         // 根据宽高比调整图像大小和起始位置
    //         if(win_aspect_ratio > video_aspect_ratio) {
    //             // 以高度为基准调整
    //             img_height = win_height_;
    //             img_height &= 0xfffc; // 确保高度为4的倍数
    //             img_width = img_height * video_aspect_ratio;
    //             img_width &= 0xfffc; // 确保宽度为4的倍数
    //             y_ = 0;
    //             x_ = (win_width_ - img_width) / 2;
    //         } else {
    //             // 以宽度为基准调整
    //             img_width = win_width_;
    //             img_width &= 0xfffc;
    //             img_height = img_width / video_aspect_ratio;
    //             img_height &= 0xfffc;
    //             x_ = 0;
    //             y_ = (win_height_ - img_height) / 2;
    //         }

    //         // 初始化 ImageScaler
    //         img_scaler_->Init(video_width, video_height, frame->format,
    //                           img_width, img_height, AV_PIX_FMT_RGB24);

    //         // 初始化 dst_video_frame_
    //         memset(&dst_video_frame_, 0, sizeof(VideoFrame));
    //         dst_video_frame_.width = img_width;
    //         dst_video_frame_.height = img_height;
    //         dst_video_frame_.format = AV_PIX_FMT_RGB24;
    //         dst_video_frame_.data[0] = (uint8_t*)malloc(img_width * img_height * 3);
    //         dst_video_frame_.linesize[0] = img_width * 3; // 每行的字节数

    //         // 标记不需要调整大小
    //         req_resize_ = false;
    //     }

    //     // 使用 ImageScaler 将输入帧缩放到 dst_video_frame_
    //     img_scaler_->Scale3(frame, &dst_video_frame_);
    // }



    // 将 f->frame->data[0/1/2] 拷贝到 dataY/dataU/dataV
    memcpy(dataY, f->frame->data[0], f->frame->linesize[0]);
    memcpy(dataU, f->frame->data[1], f->frame->linesize[1]);
    memcpy(dataV, f->frame->data[2], f->frame->linesize[2]);
    // {
    //     memcpy(dataY, f->frame->data[0], (width_ * height_));
    //     memcpy(dataU, f->frame->data[1], ((width_ * height_) >> 2));
    //     memcpy(dataV, f->frame->data[2], ((width_ * height_) >> 2));
    // }


    // updateFrame(f->frame->width,f->frame->height
    //             ,dataY, dataU, dataV
    //                 , f->frame->linesize[0]
    //                 , f->frame->linesize[1]
    //                  ,f->frame->linesize[2]);



    this->update();
    return 0;
}

void Widget::setYuyv(bool yuyv) {
    this->yuyv = yuyv;
}

void Widget::clear() {
    this->initData();
    this->update();
}

void Widget::setFrameSize(int width, int height) {
    this->width_ = width;
    this->height_ = height;
}

void Widget::updateTextures(quint8 *dataY, quint8 *dataU, quint8 *dataV, quint32 linesizeY, quint32 linesizeU,
                               quint32 linesizeV) {
    this->dataY = dataY;
    this->dataU = dataU;
    this->dataV = dataV;
    this->lineSizeY = linesizeY;
    this->lineSizeU = linesizeU;
    this->lineSizeV = linesizeV;
    this->update();
}

void Widget::updateTextures(quint8 *dataY, quint8 *dataUV, quint32 linesizeY, quint32 linesizeUV)
{
    this->dataY = dataY;
    this->dataUV = dataUV;
    this->lineSizeY = linesizeY;
    this->lineSizeUV = linesizeUV;
    this->update();
}

void Widget::updateFrame(int width, int height, quint8 *dataY, quint8 *dataU, quint8 *dataV
                         , quint32 linesizeY,
                            quint32 linesizeU, quint32 linesizeV) {
    this->setFrameSize(width, height);
    this->updateTextures(dataY, dataU, dataV, linesizeY, linesizeU, linesizeV);
    this->update();
}

// void Widget::updateFrame(int width, int height, quint8 *dataY, quint8 *dataUV, quint32 linesizeY, quint32 linesizeUV)
// {
//     this->setFrameSize(width, height);
//     this->updateTextures(dataY, dataUV, linesizeY, linesizeUV);
// }

void Widget::initializeGL() {
    qDebug() << "initializeGL";
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);

    if(format_ == YUV420P) {
        //传递顶点和纹理坐标
        // 第一个顶点坐标为 (-1.0f, -1.0f)，对应二维平面的左下角位置 第二个顶点坐标为 (1.0f, -1.0f)，代表右下角位置。
        // 第三个顶点坐标为 (-1.0f, 1.0f)，是左上角位置。 第四个顶点坐标为 (x 1.0f, 1.0f)，为右上角位置 。
        // 可以根据宽高 比例修改  顶点区域，以保持画面尺寸不变
       static GLfloat ver[] = {
           -1.0f, -1.0f
           , 1.0f, -1.0f
           , -1.0f, 1.0f, 1.0f, 1.0f
       };    //这里要设置静态变量
       //按比例显示对应的位置
       win_width_ = width();    //获取控件窗口的宽高
       win_height_ = height();
       video_aspect_ratio_ = width_ * 1.0 / height_;
       win_aspect_ratio_ = win_width_ * 1.0 / win_height_;
       img_height_ = 0;
       img_width_ = 0;

       if(win_aspect_ratio_ > video_aspect_ratio_) {
           //此时应该是调整x的起始位置，以高度为基准
           img_height_ = win_height_;
           img_height_ &= 0xfffc;
           img_width_ = img_height_ * video_aspect_ratio_;
           img_width_ &= 0xfffc;

       } else {
           //此时应该是调整y的起始位置，以宽度为基准
           img_width_ = win_width_;
           img_width_ &= 0xfffc;
           img_height_ = img_width_ / video_aspect_ratio_;
           img_height_ &= 0xfffc;
       }
       //计算坐标
       //计算左下角左边位置
       ver[0] = (img_width_/2.0)/ (win_width_/2.0) * -1;
       ver[1] = (img_height_/2.0)/ (win_height_/2.0) * -1;
       // 代表右下角位置
       ver[2] = (img_width_/2.0)/ (win_width_/2.0) ;
       ver[3] = (img_height_/2.0)/ (win_height_/2.0) * -1;

       //左上角位置
       ver[4] = (img_width_/2.0)/ (win_width_/2.0) * -1;
       ver[5] = (img_height_/2.0)/ (win_height_/2.0) * 1;

       //右上角位置
       ver[6] = (img_width_/2.0)/ (win_width_/2.0) * 1;
       ver[7] = (img_height_/2.0)/ (win_height_/2.0) * 1;

       // 纹理 即是我们要渲染的数据
       // 第一个纹理坐标为 (0.0f, 1.0f)，对应矩形左下角顶点 第二个纹理坐标为 (1.0f, 1.0f)，对应矩形右下角顶点
       // 第三个纹理坐标为 (0.0f, 0.0f)，对应矩形左上角顶点 第四个纹理坐标为 (1.0f, 0.0f)，对应矩形右上角顶点
       static const GLfloat tex[] = {
           0.0f, 1.0f,   1.0f, 1.0f,    0.0f, 0.0f,   1.0f, 0.0f};

       //设置顶点,纹理数组并启用
       //  设置顶点属性 ，告诉 OpenGL 如何解析顶点坐标数据
       glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, ver);
       // 调用这个函数来启用索引为 0 的顶点属性数组，使得 OpenGL 在后续的渲染过程中会使用通过 glVertexAttribPointer 设置好的顶点坐标数据。
       glEnableVertexAttribArray(0);
       // 与设置顶点坐标属性指针类似，这里是针对纹理坐标数据进行设置。参数 1 表示这个属性的索引为 1
       // （在着色器中通常会有对应的 in 关键字声明的变量来接收纹理坐标数据，例如 in vec2 textureCoord;
       glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, tex);
       // 启用索引为 1 的纹理坐标属性数组，让 OpenGL 在渲染时能够获取并使用设置好的纹理坐标数据，以便正确进行纹理映射操作，
       // 将纹理图像按照指定的坐标对应关系贴到对应的几何图形上。
       glEnableVertexAttribArray(1);
        setProjectionScale();
    } else if (format_ == NV12) {
//        //录制顶点坐标和纹理坐标
       static const GLfloat points[] = {
           //坐标             纹理
            -1.0f, 1.0f,      1.0f, 1.0f
            , 1.0f, -1.0f    , -1.0f, -1.0f
            , 0.0f, 0.0f    , 1.0f, 0.0f
            , 1.0f,1.0f      , 0.0f, 1.0f
       };

       //顶点缓冲对象初始化
       vbo.create();
       vbo.bind();
       vbo.allocate(points, sizeof(points));
        setProjectionScale();
    }

    //初始化shader
    this->initShader();
    //初始化textures
    this->initTextures();
    //初始化颜色
    this->initColor();
}

void Widget::paintGL() {

    if(win_width_ != width() || win_height_ != height()) {
        qDebug() << "adjust size"<<win_width_<< " "<<win_height_;
        setProjectionScale();
    }

    if (!dataY || width_ == 0 || height_ == 0) {
        this->initColor();
        return;
    }
    if(format_ == YUV420P) {
        yuv420pPaintGL();
    }else if (format_ == NV12) {
        nv12PaintGL();
    }
}

void Widget::yuv420pPaintGL()
{
//    qDebug() << "yuv420pPaintGL" ;

    glActiveTexture(GL_TEXTURE0);
    // 将一个名为 textureY 的纹理对象绑定到GL_TEXTURE_2D 目标上
    glBindTexture(GL_TEXTURE_2D, textureY);          // 创建纹理对象
    //设置像素存储参数，指定像素数据中原图一行 Y 分量占用的大小（通过 lineSizeY 参数指定），
    // 因为在某些情况下一行像素数据实际占用的字节数可能大于图像宽度（width_）所对应的字节数，比如存在对齐等情况，
    //通过这个函数调用可以让 OpenGL 正确解析像素数据。
    glPixelStorei(GL_UNPACK_ROW_LENGTH, lineSizeY);
    // 这里将 Y 分量的数据设置到绑定的纹理对象中。纹理级别设为 0（表示原始纹理图像）
    // 内部格式和像素格式都指定为 GL_LUMINANCE（表示亮度，适合存储 Y 分量这种灰度信息）
    glTexImage2D(GL_TEXTURE_2D,         //要操作的纹理目标
                 0,                     //指定多级渐远纹理的级别
                 GL_LUMINANCE,          //指定纹理内部的格式
                 width_,                //指定纹理的宽度和高度
                 height_,
                 0,                     //指定源图像数据的格
                 GL_LUMINANCE,
                 GL_UNSIGNED_BYTE,      //指定源图像数据的数据类型
                 dataY                  //指向包含源图像数据的内存地址
                 );    //处理Y
    // 将一个名为 textureUniformY 的统一变量（通常在着色器程序中用于接收纹理单元编号）设置为 0，
    // 这一步建立了着色器程序中纹理相关变量与当前激活并绑定的纹理单元（即 GL_TEXTURE0）之间的联系
    glUniform1i(textureUniformY, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureU);
    //类似处理 Y 分量时的操作，设置 U 分量像素数据每行占用的字节数相关参数，通过 lineSizeU 来指定具体大小，保证 OpenGL 正确解析数据。
    glPixelStorei(GL_UNPACK_ROW_LENGTH, lineSizeU);
    glTexImage2D(GL_TEXTURE_2D
                 , 0, GL_LUMINANCE
                 , width_ >> 1
                 , yuyv ? height_ : height_ >> 1
                 , 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, dataU);   //处理U
    glUniform1i(textureUniformU, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, textureV);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, lineSizeV);
    glTexImage2D(GL_TEXTURE_2D, 0
                 , GL_LUMINANCE
                 , width_ >> 1
                 , yuyv ? height_ : height_ >> 1
                 , 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE
                 , dataV);  //处理V
    glUniform1i(textureUniformV, 2);

    // 使用 GL_TRIANGLE_STRIP 绘制模式，从索引 0 开始，绘制 4 个顶点。具体绘制的内容应该是基于前面设置好的 Y、U、V 三个纹理所对应的数据来生成最终的图像
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); //如果是3，只是个三角形
}

void Widget::nv12PaintGL()      //注意调用的时机，没有装载yuv数据时，yuv都是0值，会显示绿屏
{
    qDebug() << "nv12PaintGL" ;

    //将对应的着色器程序绑定到当前的 OpenGL 上下文，使得后续对顶点属性、uniform 变量等的设置操作是针对这个绑定的着色器程序进行的。
    program.bind();
    // 分别启用名为 "vertexIn" 和 "textureIn" 的顶点属性数组。  初始化时定义的名字
    program.enableAttributeArray("vertexIn");
    program.enableAttributeArray("textureIn");
    // "vertexIn" 是要设置的顶点属性名称，与前面启用的属性数组对应。
    // GL_FLOAT 表示顶点属性数据的类型是单精度浮点数。
    // 0 表示数据在缓冲中的偏移量，这里是从缓冲起始位置开始（偏移量为 0 字节），意味着顶点坐标数据在缓冲数据的开头存放。
    // 2 表示每个顶点属性包含的分量数量，即每个顶点坐标用两个 GL_FLOAT 类型的数据表示（通常是 x 和 y 坐标，对应二维坐标情况）。
    // 2 * sizeof(GLfloat) 是步长（Stride），表示相邻两个顶点属性数据之间的间隔字节数，这里表示每个顶点的数据紧密排列，间隔为两个 GLfloat 类型数据占用的字节数（也就是两个单精度浮点数的字节数之和）。
    program.setAttributeBuffer("vertexIn", GL_FLOAT, 0, 2, 2 * sizeof(GLfloat));
    // 不同之处在于偏移量为 2 * 4 * sizeof(GLfloat)，这意味着纹理坐标数据在顶点坐标数据之后开始存放
    //（因为前面顶点坐标数据每个顶点占 2 * sizeof(GLfloat) 字节，总共 4 个顶点，所以纹理坐标数据的偏移量就是 2 * 4 * sizeof(GLfloat) 字节）
    program.setAttributeBuffer("textureIn", GL_FLOAT, 2 * 4 * sizeof(GLfloat), 2, 2 * sizeof(GLfloat));

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textureY);
    //字节对齐,网上很多代码都是少了这一步,导致有时候花屏
    glPixelStorei(GL_UNPACK_ROW_LENGTH, lineSizeY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width_, height_, 0, GL_RED, GL_UNSIGNED_BYTE, dataY);        //处理Y
    this->initParamete();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureUV);
    //字节对齐,网上很多代码都是少了这一步,导致有时候花屏
    glPixelStorei(GL_UNPACK_ROW_LENGTH, lineSizeUV >> 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, width_ >> 1, height_ >> 1, 0, GL_RG, GL_UNSIGNED_BYTE, dataUV);  //处理UV
    this->initParamete();

    //执行实际的绘制操作，使用 GL_QUADS 绘制模式，表示绘制四边形。从索引 0 开始，绘制 4 个顶点，
    // 结合前面设置的顶点坐标、纹理坐标以及绑定的纹理数据，将 NV12 格式的图像通过纹理映射等方式绘制出来呈现到屏幕上
    glDrawArrays(GL_QUADS, 0, 4);
    program.setUniformValue("textureY", 1);
    program.setUniformValue("textureUV", 0);
    program.disableAttributeArray("vertexIn");
    program.disableAttributeArray("textureIn");
    program.release();
}

void Widget::initData() {
    width_ = height_ = 0;
    dataY = dataU = dataV = dataUV = nullptr;
    lineSizeY = lineSizeU = lineSizeV  = lineSizeUV= 0;
}

void Widget::initColor() {
    //取画板背景颜色
    QColor color = palette().window().color();
    //设置背景清理色
//    glClearColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
    glClearColor(0, 0, 0, color.alphaF());
    //清理颜色背景
    glClear(GL_COLOR_BUFFER_BIT);
}

void Widget::initShader() {
    //加载顶点和片元脚本
    program.addShaderFromSourceCode(QOpenGLShader::Vertex, shaderVert);
    program.addShaderFromSourceCode(QOpenGLShader::Fragment, shaderFrag);

    if(format_ == YUV420P) {
        //设置顶点位置
        program.bindAttributeLocation("vertexIn", 0);
        //设置纹理位置
        program.bindAttributeLocation("textureIn", 1);
    }

    //编译shader
    program.link();
    program.bind();

    if(format_ == YUV420P) {
        //从shader获取地址
        textureUniformY = program.uniformLocation("textureY");
        textureUniformU = program.uniformLocation("textureU");
        textureUniformV = program.uniformLocation("textureV");
    }
}

void Widget::initTextures() {

    if(format_ == YUV420P) {
        //创建纹理
        glGenTextures(1, &textureY);
        glBindTexture(GL_TEXTURE_2D, textureY);
        this->initParamete();

        glGenTextures(1, &textureU);
        glBindTexture(GL_TEXTURE_2D, textureU);
        this->initParamete();

        glGenTextures(1, &textureV);
        glBindTexture(GL_TEXTURE_2D, textureV);
        this->initParamete();
    } else if (format_ == NV12) {
        glGenTextures(1, &textureY);
        glGenTextures(1, &textureUV);
    }
}

void Widget::initParamete() {
    //具体啥意思 https://blog.csdn.net/d04421024/article/details/5089641

    //纹理过滤
    //GL_TEXTURE_MAG_FILTER: 放大过滤
    //GL_TEXTURE_MIN_FILTER: 缩小过滤
    //GL_LINEAR: 线性插值过滤,获取坐标点附近4个像素的加权平均值
    //GL_NEAREST: 最临近过滤,获得最靠近纹理坐标点的像素
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  // GL_LINEAR  NV12
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    //纹理贴图
    //GL_TEXTURE_2D: 操作2D纹理
    //GL_TEXTURE_WRAP_S: S方向上的贴图模式
    //GL_TEXTURE_WRAP_T: T方向上的贴图模式
    //GL_CLAMP: 将纹理坐标限制在0.0,1.0的范围之内,如果超出了会如何呢,不会错误,只是会边缘拉伸填充
    //GL_CLAMP_TO_EDGE: 超出纹理范围的坐标被截取成0和1,形成纹理边缘延伸的效果
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);  // GL_CLAMP_TO_EDGE nv12
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

void Widget::deleteTextures() {
    glDeleteTextures(1, &textureY);
    glDeleteTextures(1, &textureU);
    glDeleteTextures(1, &textureV);
}
//重新设置宽高
void Widget::setProjectionScale()
{
    //按比例显示对应的位置
    win_width_ = width();    //获取控件窗口的宽高
    win_height_ = height();
    video_aspect_ratio_ = width_ * 1.0 / height_;
    win_aspect_ratio_ = win_width_ * 1.0 / win_height_;
    img_height_ = 0;
    img_width_ = 0;

    if(win_aspect_ratio_ > video_aspect_ratio_) {
        //此时应该是调整x的起始位置，以高度为基准
        img_height_ = win_height_;
        img_height_ &= 0xfffc;
        img_width_ = img_height_ * video_aspect_ratio_;
        img_width_ &= 0xfffc;

    } else {
        //此时应该是调整y的起始位置，以宽度为基准
        img_width_ = win_width_;
        img_width_ &= 0xfffc;
        img_height_ = img_width_ / video_aspect_ratio_;
        img_height_ &= 0xfffc;
    }
    if(format_ == YUV420P) {
       static GLfloat ver[] = {-1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f};
       //计算映射坐标 计算中心点坐标，然后再除以窗口wh一半得到-1 - 1  960/414
       //计算左下角左边位置
       ver[0] = (img_width_/2.0)  / (win_width_/2.0)  * -1;
       ver[1] = (img_height_/2.0) / (win_height_/2.0) * -1;
       // 代表右下角位置
       ver[2] = (img_width_/2.0)  / (win_width_/2.0) ;
       ver[3] = (img_height_/2.0) / (win_height_/2.0) * -1;

       //左上角位置
       ver[4] = (img_width_/2.0)  / (win_width_/2.0)  * -1;
       ver[5] = (img_height_/2.0) / (win_height_/2.0) * 1;

       //右上角位置
       ver[6] = (img_width_/2.0)  / (win_width_/2.0)  * 1;
       ver[7] = (img_height_/2.0) / (win_height_/2.0) * 1;

       // 纹理 即是我们要渲染的数据
       // 第一个纹理坐标为 (0.0f, 1.0f)，对应矩形左下角顶点 第二个纹理坐标为 (1.0f, 1.0f)，对应矩形右下角顶点
       // 第三个纹理坐标为 (0.0f, 0.0f)，对应矩形左上角顶点 第四个纹理坐标为 (1.0f, 0.0f)，对应矩形右上角顶点
       static const GLfloat tex[] = {
           0.0f, 1.0f,
           1.0f, 1.0f,
           0.0f, 0.0f,
           1.0f, 0.0f
       };

       //设置顶点,纹理数组并启用
       //  设置顶点属性 ，告诉 OpenGL 如何解析顶点坐标数据
       // 0：表示顶点属性的索引，在着色器程序中会通过这个索引来获取对应的顶点属性数据，这里索引为 0 的属性通常对应顶点坐标相关的数据
       // 2：指定每个顶点属性的分量数量，这里是 2，表示每个顶点使用两个 GL_FLOAT 类型的数据来表示坐标（也就是 x 和 y 坐标）。
       // GL_FLOAT：表明顶点坐标数据的类型是单精度浮点数类型。
       // 0：表示是否需要对数据进行归一化，这里 0 表示不需要归一化（因为我们提供的数据本身就是合适的浮点数格式，不需要额外处理）。
       // 0：步长（Stride）参数，这里设为 0 表示数据是紧密排列的，即下一个顶点的数据紧接着当前顶点数据存放，没有额外的间隔字节数。
       // ver：指向实际的顶点坐标数据数组的指针，也就是前面定义的 ver 数组。
       glVertexAttribPointer(0, 2, GL_FLOAT, 0, 0, ver);
       // 调用这个函数来启用索引为 0 的顶点属性数组，使得 OpenGL 在后续的渲染过程中会使用通过 glVertexAttribPointer 设置好的顶点坐标数据。
       glEnableVertexAttribArray(0);
       // 与设置顶点坐标属性指针类似，这里是针对纹理坐标数据进行设置。参数 1 表示这个属性的索引为 1
       // （在着色器中通常会有对应的 in 关键字声明的变量来接收纹理坐标数据，例如 in vec2 textureCoord;
       glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, tex);
       // 启用索引为 1 的纹理坐标属性数组，让 OpenGL 在渲染时能够获取并使用设置好的纹理坐标数据，以便正确进行纹理映射操作，
       // 将纹理图像按照指定的坐标对应关系贴到对应的几何图形上。
       glEnableVertexAttribArray(1);
    } else if(format_ == NV12) {
        if(vbo.isCreated()) {
            vbo.destroy();
        }
        //录制顶点坐标和纹理坐标

                                 // 左上          右上        右下          左下      和yuv420渲染时坐标有区别
        static  GLfloat points[] = {-1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, -1.0f, -1.0f,
                                    0.0f,  0.0f, 1.0f, 0.0f, 1.0f,  1.0f,  0.0f, 1.0f};
        //计算坐标
        //计算左下角左边位置
        points[6] = (img_width_/2.0)/ (win_width_/2.0) * -1;
        points[7] = (img_height_/2.0)/ (win_height_/2.0) * -1;
//        // 代表右下角位置
        points[4] = (img_width_/2.0)/ (win_width_/2.0) ;
        points[5] = (img_height_/2.0)/ (win_height_/2.0) * -1;

//        //左上角位置
        points[0] = (img_width_/2.0)/ (win_width_/2.0) * -1;
        points[1] = (img_height_/2.0)/ (win_height_/2.0) * 1;

//        //右上角位置
        points[2] = (img_width_/2.0)/ (win_width_/2.0) * 1;
        points[3] = (img_height_/2.0)/ (win_height_/2.0) * 1;
        //顶点缓冲对象初始化
        vbo.create();
        vbo.bind();
        vbo.allocate(points, sizeof(points));
    }
}

void Widget::read() {
    qint64 len = (width_ * height_ * 3) >> 1;
//     qDebug() << "read len = " << len;
    if (file.read((char *) dataY, len)) {
        this->update();
    } else {
        timer.stop();
        emit playFinish();
        qDebug() << "playFinish";
    }
}

void Widget::play(const QString &fileName, int frameRate) {
    qDebug() << "play " << fileName;
    //停止定时器并关闭文件
    if (timer.isActive()) {
        timer.stop();
    }
    if (file.isOpen()) {
        file.close();
    }

    file.setFileName(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    //初始化对应数据指针位置
    dataY = new quint8[(width_ * height_ * 3) >> 1];
    if(format_ == YUV420P) {
        dataU = dataY + (width_ * height_);
        dataV = dataU + ((width_ * height_) >> 2);
    }
    if(format_ == NV12)
        dataUV = dataY + (width_ * height_);
    memset(dataY, 0, width_ * height_);
    memset(dataY + width_ * height_, 128, width_ * height_/2);

    //启动定时器读取文件数据
    timer.start(1000 / frameRate);
}

void Widget::stop() {
    //停止定时器并关闭文件
    if (timer.isActive()) {
        timer.stop();
    }
    if (file.isOpen()) {
        file.close();
    }

    this->clear();
    emit playFinish();
}
