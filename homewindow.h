#ifndef HOMEWINDOW_H
#define HOMEWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "toast.h"
#include "ijkmediaplayer.h"
#include<messagequeue.h>
#include<commonlooper.h>
namespace Ui
{
class HomeWindow;
}

class HomeWindow : public QMainWindow , public CommonLooper
{
    Q_OBJECT
private:
    Ui::HomeWindow* ui;
    //消息队列
    std::shared_ptr<MessageQueue> msg_queue_ = nullptr;

    bool is_show_file_list_ = true;     // 是否显示文件列表，默认显示
    std::shared_ptr<IjkMediaPlayer> mp_ = nullptr;



    //播放相关的信息
    // 当前文件播放的总长度,单位为ms
    long total_duration_ = 0;
    long current_position_ = 0;
    int64_t pre_get_cur_pos_time_ = 0;

    
    std::unique_ptr<QTimer> play_timer_ = nullptr;

    int play_slider_max_value = 6000;
    bool req_seeking_ = false;     //当请求seek时，中间产生的播放速度不

    bool req_screenshot_ = false;


    // 缓存统计
    int max_cache_duration_ = 400;  // 默认200ms
    int network_jitter_duration_ = 100; // 默认100ms
    float accelerate_speed_factor_ = 1.5; //默认加速是1.2
    float normal_speed_factor_ = 1.0;     // 正常播放速度1.0
    bool  is_accelerate_speed_ = false;

    // 缓存长度
    int64_t audio_cache_duration = 0;
    int64_t video_cache_duration = 0;
    int64_t pre_get_cache_time_ = 0;
    int real_time_ = 1;  //值1变速播放 ，否则直播流

    // 码率
    int64_t audio_bitrate_duration = 0;
    int64_t video_bitrate_duration = 0;
    void initUi();
    int InitSignalsAndSlots();
public:
    explicit HomeWindow(QWidget *parent = 0);
    ~HomeWindow();


    int OutputVideo(const Frame *frame);

    void DeInit(){
        if(getRunning())
            this->Stop();
    }
    /**
     *  事件循环回调，从ijk取出消息做处理。调用方式：this.start 结束 this.stop
     * @return
     */
    virtual void Loop() override;
protected:
    virtual void  resizeEvent(QResizeEvent *event);
    void resizeUI();
signals:
    // 发送要显示的提示信息
    void sig_showTips(Toast::Level leve, QString tips);
    void sig_updateAudioCacheDuration(int64_t duration);
    //
    void sig_updateVideoCacheDuration(int64_t duration);
    //音量调整信号
    void sig_updateCurrentPosition(long position);
    //播放与暂停
    void sig_updatePlayOrPause(int state);

    void sig_stopped(); // 被动停止
private slots:
    //更新缓存ui
    void on_UpdateAudioCacheDuration(int64_t duration);
    void on_UpdateVideoCacheDuration(int64_t duration);
    // 打开文件
    void on_openFile();
    // 打开网络流，逻辑和vlc类似
    void on_openNetworkUrl();
    //文件列表
    void on_listBtn_clicked();
     // 播放暂停
    void on_playOrPauseBtn_clicked();
    void on_updatePlayOrPause(int state);
    //缓存阀值
    void on_updateDurationCacheMax(const QString&);
    void on_updateDurationCacheMin(const QString&);
    // 停止
    void on_stopBtn_clicked();
    /// @brief 消息处理机制开始点，视频文件播放
    /// @param url
    /// @return
    bool play(std::string url);
    bool stop();

    // 拖动进度条响应
    void on_updateCurrentPosition(long position);
    void onTimeOut();

    // 进度条和音量拖动触发
    void on_playSliderValueChanged(int value);
    void on_volumeSliderValueChanged(int value);
    void on_speedBtn_clicked();
    //截图
    void on_screenBtn_clicked();

    //红框显示
    void on_showTips(Toast::Level leve, QString tips);
    void on_bufDurationBox_currentIndexChanged(int index);

    void on_jitterBufBox_currentIndexChanged(int index);
    //上/下一集播放
    void on_prevBtn_clicked();
    void on_nextBtn_clicked();
    //前进和后退
    void on_forwardFastBtn_clicked();
    void on_backFastBtn_clicked();

    //一键禁音
    void on_audio_muted_clicked(bool checked);

private:

    void startTimer();
    void stopTimer();
    // pause->play
    bool resume();
    // play->pause
    bool pause();
    // play/pause->stop

    void resizeCtrlBar();
    void resizeDisplayAndFileList();
    int seek(int cur_valule);

    int fastForward(long inrc);
    int fastBack(long inrc);
    // 主动获取信息，并更新到ui
    void getTotalDuration();
public:
    // 定时器获取，每秒读取一次时间
    void reqUpdateCurrentPosition();
    void reqUpdateCacheDuration();



};

#endif // HOMEWINDOW_H
