#ifndef IJKMEDIAPLAYER_H
#define IJKMEDIAPLAYER_H

#include  <mutex>
#include <thread>
#include <functional>
#include "ff_ffplay_def.h"
#include "ff_ffplay.h"
#include <messagequeue.h>
#include<commonlooper.h>

class MessageQueue;
class IjkMediaPlayer
{

    std::shared_ptr<MessageQueue> msg_queue_;
    
    // 真正的播放器
    std::shared_ptr<FFPlayer> ffplayer_ =nullptr;
    //函数指针, 指向创建的message_loop，即消息循环函数
    //    int (*msg_loop)(void*);
    // 互斥量
    std::mutex mutex_;

    //    SDL_Thread _msg_thread;
    //字符串，就是一个播放url
    char* data_source_;

    // 截屏请求
    char* file_path_ = NULL;
    //播放器状态，例如prepared,resumed,error,completed等
    int mp_state_;  // 播放状态

    int seek_req = 0;
    long seek_msec = 0;

public:
    IjkMediaPlayer(std::shared_ptr<MessageQueue>);
    ~IjkMediaPlayer();
    int ijk_init();
    int ijk_destroy();
    // 设置要播放的url
    int ijkmp_set_data_source(const char *url);
    // 准备播放
    int ijkmp_prepare_async();
    // 触发播放
    int ijkmp_start();
    // 停止
    int ijkmp_stop();
    // 暂停
    int ijkmp_pause();
    // seek到指定位置
    int ijkmp_seek_to(long msec);
    // 快进
    int ijkmp_forward_to(long incr);
    // 快退
    int ijkmp_back_to(long incr);
    //请求截屏
    int ijkmp_screenshot(char *file_path);
    // 获取播放状态
    int ijkmp_get_state();
    // 是不是播放中
    bool ijkmp_is_playing();
    // 当前播放位置
    long ijkmp_get_current_position();
    // 总长度
    long ijkmp_get_duration();
    // 已经播放的长度
    long ijkmp_get_playable_duration();
    // 设置循环播放
    void ijkmp_set_loop(int loop);
    // 获取是否循环播放
    int  ijkmp_get_loop();
    // 读取消息
    int ijkmp_get_msg(AVMessage *msg, int block);
    // 设置音量
    void ijkmp_set_playback_volume(int volume);
    // 设置一键静音音量
    void ijkmp_set_audio_muted(bool muted);




    void ijkmp_set_playback_rate(float rate);
    float ijkmp_get_playback_rate();
    void AddVideoRefreshCallback(std::function<int(const Frame *)> callback);

    // 获取状态值
    int64_t ijkmp_get_property_int64(int id, int64_t default_value);

    void ijkmp_change_state_l(int new_state);



    std::shared_ptr<FFPlayer> Get_ffplayer() const;
};

#endif // IJKMEDIAPLAYER_H
