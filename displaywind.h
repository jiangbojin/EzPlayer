#ifndef DISPLAYWIND_H
#define DISPLAYWIND_H

#include <QWidget>
#include <QMutex>
#include "ijkmediaplayer.h"
#include "imagescaler.h"



namespace Ui {
class DisplayWind;
}

class DisplayWind : public QWidget
{
    Q_OBJECT

public:
    explicit DisplayWind(QWidget *parent = 0);
    ~DisplayWind();
    // 在显示区域绘制视频帧
    int Draw(const Frame *frame);

    // 执行清理和反初始化操作
    void DeInit();

    // 开始播放视频
    void StartPlay();

    // 停止播放视频
    void StopPlay();
protected:
    // 不要重写此事件,否则可能导致 paintEvent 不被触发
    void paintEvent(QPaintEvent *) override;

    // 处理窗口大小调整事件
    void resizeEvent(QResizeEvent *event);
private:
    Ui::DisplayWind *ui;


    // 记录上一次视频帧的宽高
    int m_nLastFrameWidth;
    int m_nLastFrameHeight;

    // 标记显示区域大小是否发生变化
    bool is_display_size_change_ = false;

    // 视频和窗口尺寸相关变量
    int x_, y_,
    //输入视频帧的宽高
    video_width, video_height,
    //最终图像
    img_width, img_height,
    //当前窗口的宽高
    win_width_, win_height_;

    // 标记是否需要调整大小
    bool req_resize_ = false;

    // 存储视频帧的 QImage
    QImage img;

    // 存储视频帧
    VideoFrame dst_video_frame_;

    // 用于同步的互斥锁
    QMutex m_mutex;

    // 指向 ImageScaler 对象的指针,用于缩放视频帧
    ImageScaler *img_scaler_ = NULL;

    // 记录当前播放状态 (0 - 初始化, 1 - 播放, 2 - 停止)
    int play_state_ = 0;

};

#endif // DISPLAYWIND_H
