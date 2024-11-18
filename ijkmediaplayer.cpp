#include "ijkmediaplayer.h"
#include <iostream>
#include <string.h>
#include<ff_ffplay.h>
#include "ffmsg.h"
#include<messagequeue.h>
#include<thread>
#include<homewindow.h>

#include "easylogging++.h"
std::shared_ptr<FFPlayer> IjkMediaPlayer::Get_ffplayer() const
{
    return ffplayer_;
}

void IjkMediaPlayer::ijkmp_set_HW_DecodeType(const std::string &data)
{
    if(data.empty())
        return;
    ffplayer_->m_isHw_device = true;
    ffplayer_->hw_device_type = data;

}


IjkMediaPlayer::IjkMediaPlayer(std::shared_ptr<MessageQueue> msg_queue)
    :msg_queue_(msg_queue){
    LOG(INFO) << " IjkMediaPlayer()\n ";
}

IjkMediaPlayer::~IjkMediaPlayer()
{
    LOG(INFO) << " ~IjkMediaPlayer()\n ";
    // 此时需要停止线程
}
///
/// 创建ffplay对象
/// \return
///
int IjkMediaPlayer::ijk_init()
{
    ffplayer_ =std::make_shared<FFPlayer>(msg_queue_);
    if(!ffplayer_) {
        LOG(INFO) << " new FFPlayer() failed\n ";
        return -1;
    }



    return 0;
}

int IjkMediaPlayer::ijk_destroy()
{
    ffplayer_->stream_close();
    return 0;
}
// 这个方法的设计来源于Android mediaplayer, 其本意是
//int IjkMediaPlayer::ijkmp_set_data_source(Uri uri)
int IjkMediaPlayer::ijkmp_set_data_source(const char *url)
{
    if(!url) {
        return -1;
    }
    data_source_ = strdup(url); // 分配内存+ 拷贝字符串
    return 0;
}
/**
 *  音视频同步准备工作
 * @return
 */
int IjkMediaPlayer::ijkmp_prepare_async()
{
    // 判断mp的状态
    // 正在准备中
    mp_state_ = MP_STATE_ASYNC_PREPARING;
    msg_queue_->msg_queue_start();
    // 调用ffplayer
    int ret = ffplayer_.get()->ffp_prepare_async_l(data_source_);
    if(ret < 0) {
        mp_state_ = MP_STATE_ERROR;
        return -1;
    }
    return 0;
}

int IjkMediaPlayer::ijkmp_start()
{
    msg_queue_.get()->notify_msg(FFP_REQ_START);
    return 0;
}
///
/// 流退出 禁止再插入消息
/// \return
///
int IjkMediaPlayer::ijkmp_stop()
{
    int retval = ffplayer_->ffp_stop_l();
    if (retval < 0) {
        return retval;
    }
    return 0;
}

int IjkMediaPlayer::ijkmp_pause()
{
    // 发送暂停的操作命令
    msg_queue_.get()->msg_queue_remove( FFP_REQ_START);
    msg_queue_.get()->msg_queue_remove(FFP_REQ_PAUSE);
    msg_queue_.get()->notify_msg(FFP_REQ_PAUSE);
    return 0;
}

int IjkMediaPlayer::ijkmp_seek_to(long msec)
{
    seek_req = 1;
    seek_msec = msec;
    msg_queue_.get()->msg_queue_remove(FFP_REQ_SEEK);
    msg_queue_.get()->notify_msg(FFP_REQ_SEEK, (int)msec);
    return 0;
}

int IjkMediaPlayer::ijkmp_forward_to(long incr)
{
    seek_req = 1;
    msg_queue_.get()->msg_queue_remove(FFP_REQ_FORWARD);
    msg_queue_.get()->notify_msg(FFP_REQ_FORWARD, (int)incr);
    return 0;
}

int IjkMediaPlayer::ijkmp_back_to(long incr)
{
    seek_req = 1;
    msg_queue_.get()->msg_queue_remove( FFP_REQ_FORWARD);
    msg_queue_.get()->notify_msg(FFP_REQ_FORWARD, (int)incr);
    return 0;
}


// 请求截屏
int IjkMediaPlayer::ijkmp_screenshot(char *file_path)
{
    msg_queue_.get()->msg_queue_remove( FFP_REQ_SCREENSHOT);
    msg_queue_.get()->notify_msg( FFP_REQ_SCREENSHOT, 0, 0, file_path, strlen(file_path) + 1);
    return 0;
}

int IjkMediaPlayer::ijkmp_get_state()
{
    return mp_state_;
}

long IjkMediaPlayer::ijkmp_get_current_position()
{
    return ffplayer_->ffp_get_current_position_l();
}

long IjkMediaPlayer::ijkmp_get_duration()
{
    return ffplayer_->ffp_get_duration_l();
}
/**
 *  从ffmpeg层取出一个消息，根据 continue_wait_next_msg = 1表示不往上层用户ui层传递 否则继续返回给上层调用
 * @param msg
 * @param block 阻塞调用方式
 * @return
 */

int IjkMediaPlayer::ijkmp_get_msg(AVMessage *msg, int block,void* is_)
{

    HomeWindow* is = (HomeWindow *)is_;
    int pause_ret = 0;
    while (1) {
        int continue_wait_next_msg = 0;
        //取消息，没有消息则根据block值 =1阻塞，=0不阻塞。
        int retval = msg_queue_.get()->msg_queue_get(msg, block);
        if (retval <= 0) {      // -1 abort, 0 没有消息
            return retval;
        }


        switch (msg->what) {

        case FFP_REQ_START:
            LOG(INFO) <<  " FFP_REQ_START" ;
            continue_wait_next_msg = 1;
            retval = ffplayer_->ffp_start_l();
            if (retval == 0) {
                ijkmp_change_state_l(MP_STATE_STARTED);
            }
            break;
        case FFP_REQ_PAUSE:
            continue_wait_next_msg = 1;
            pause_ret = ffplayer_->ffp_pause_l();
            if(pause_ret == 0) {
                //设置为暂停暂停
                ijkmp_change_state_l(MP_STATE_PAUSED);  // 暂停后怎么恢复？
            }
            break;

        case FFP_REQ_SEEK:
            LOG(INFO) << "ijkmp_get_msg: FFP_REQ_SEEK\n";
            continue_wait_next_msg = 1;
            ffplayer_->ffp_seek_to_l(msg->arg1);
            break;
        case FFP_REQ_FORWARD:
            LOG(INFO) << "ijkmp_get_msg: FFP_REQ_FORWARD\n";
            continue_wait_next_msg = 1;
            ffplayer_->ffp_forward_to_l(msg->arg1);
            break;
        case FFP_REQ_BACK:
            LOG(INFO) << "ijkmp_get_msg: FFP_REQ_BACK\n";
            continue_wait_next_msg = 1;
            ffplayer_->ffp_back_to_l(msg->arg1);
            break;
        case FFP_REQ_SCREENSHOT:
            LOG(INFO) << "ijkmp_get_msg: FFP_REQ_SCREENSHOT: " << (char *)msg->obj ;
            continue_wait_next_msg = 1;
            ffplayer_->ffp_screenshot_l((char *)msg->obj);
            break;
        case FFP_MSG_PREPARED:
            LOG(INFO) <<  " FFP_MSG_PREPARED" ;
            //            ijkmp_change_state_l(MP_STATE_PREPARED);
            break;
        case FFP_MSG_SEEK_COMPLETE:
            LOG(INFO) << "ijkmp_get_msg: FFP_MSG_SEEK_COMPLETE\n";
            seek_req = 0;
            seek_msec = 0;
            break;
        case FFP_MSG_SPEED_SUB_DOUBLE:
            ffplayer_->ffp_set_playback_rate(-0.5);
            break;
        case FFP_MSG_SPEED_ADD_DOUBLE:
            ffplayer_->ffp_set_playback_rate(+0.5);
            break;
        case FFP_MSG_FRAMEQ_CACHE_SPEED:
            ffplayer_->pf_playback_rate_changed = 1;
            ffplayer_->pf_playback_rate=2;
            break;
        case FFP_MSG_FRAMEQ_CACHE_REGAIN:
            ffplayer_->pf_playback_rate_changed = 1;
            ffplayer_->pf_playback_rate=1;
            break;
        default:
            //LOG(INFO) <<  " default " << msg->what ;
            break;
        }


        if(continue_wait_next_msg){ //不再传递
            if (msg->obj)
                msg->free_l(msg->obj);
            continue;
        }

        return retval;
    }
    return -1;
}
/**
 *  设置播放器音量
 * @param volume
 */
void IjkMediaPlayer::ijkmp_set_playback_volume(int volume)
{
    ffplayer_->ffp_set_playback_volume(volume);
}

void IjkMediaPlayer::ijkmp_set_audio_muted(bool muted)
{
    ffplayer_->audio_muted_ = muted;
}

void IjkMediaPlayer::ijkmp_set_pkt_queue_cache(bool type, int value)
{
    ffplayer_->ffp_set_pkt_queue_cache(type,value);
}



void IjkMediaPlayer::ijkmp_set_playback_rate(float rate)
{
    msg_queue_->notify_msg(FFP_MSG_SPEED_ADD_DOUBLE);
}

float IjkMediaPlayer::ijkmp_get_playback_rate()
{
    return ffplayer_->ffp_get_playback_rate();
}

void IjkMediaPlayer::AddVideoRefreshCallback(
        std::function<int (const Frame *)> callback)
{
    ffplayer_->AddVideoRefreshCallback(callback);
}

int64_t IjkMediaPlayer::ijkmp_get_property_int64(int id, int64_t default_value)
{
    return  ffplayer_->ffp_get_property_int64(id, default_value);
}

void IjkMediaPlayer::ijkmp_change_state_l(int new_state)
{
    mp_state_ = new_state;
    msg_queue_.get()->notify_msg(FFP_MSG_PLAYBACK_STATE_CHANGED);
}








