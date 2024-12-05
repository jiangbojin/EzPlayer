#ifndef TOAST_H
#define TOAST_H
/**
 * @brief The Toast class
 *     源码参考:Qt实现Toast提示消息 https://blog.csdn.net/wwwlyj123321/article/details/112391884
 */


#include <QObject>
#include <QRect>
class ToastDlg;

///
/// 用于显示提示信息
///
class Toast : public QObject
{
public:
    // 提示信息的级别枚举
    enum Level
    {
        INFO,   // 信息提示
        WARN,   // 警告提示
        ERROR   // 错误提示
    };

private:
    // 私有构造函数,防止外部创建对象
    Toast();

public:
    // 获取Toast单例实例
    static Toast& instance();

public:
    // 显示提示信息
    // @param level 提示信息的级别
    // @param text 提示信息的文本
    void show(Level level, const QString& text);

private:
    // 定时器事件处理函数,用于隐藏提示对话框
    void timerEvent(QTimerEvent *event) override;

private:
    // 提示对话框对象
    ToastDlg* dlg_;
    // 定时器ID
    int timer_id_{0};
    // 提示对话框的几何信息
    QRect geometry_;
};
#endif // TOAST_H
