#pragma once

#include <QMouseEvent>
#include <QSlider>

class CustomSlider : public QSlider {
    Q_OBJECT

   public:
    CustomSlider(QWidget* parent);
    ~CustomSlider();

   protected:
    void mousePressEvent(QMouseEvent* ev);  //重写QSlider的mousePressEvent事件
    void mouseReleaseEvent(QMouseEvent* ev);
    void mouseMoveEvent(QMouseEvent* ev);
   signals:
    void SigCustomSliderValueChanged(
        int value);  //自定义的鼠标单击信号，用于捕获并处理 音量调整信号
};
