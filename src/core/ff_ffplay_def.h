#ifndef FF_FFPLAY_DEF_H
#define FF_FFPLAY_DEF_H

#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
extern "C" {
#include "libavcodec/avfft.h"
#include "libavdevice/avdevice.h"
#include "libavformat/avformat.h"
#include "libavutil/avassert.h"
#include "libavutil/avstring.h"
#include "libavutil/channel_layout.h"
#include "libavutil/dict.h"
#include "libavutil/eval.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavutil/parseutils.h"
#include "libavutil/pixdesc.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libswresample/swresample.h"
#include "libswscale/swscale.h"
}
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <assert.h>

#include "ijksdl_timer.h"
//pkt队列最大容量
// 最大队列大小 (60 MB)
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)
// 最小帧数
#define MIN_FRAMES 25
// 外部时钟最小帧数
#define EXTERNAL_CLOCK_MIN_FRAMES 2
// 外部时钟最大帧数
#define EXTERNAL_CLOCK_MAX_FRAMES 10

// 视频图像帧队列大小
#define VIDEO_PICTURE_QUEUE_SIZE 3  // 图像帧缓存数量
#define VIDEO_PICTURE_QUEUE_SIZE_MIN (3)
#define VIDEO_PICTURE_QUEUE_SIZE_MAX (16)
#define VIDEO_PICTURE_QUEUE_SIZE_DEFAULT (VIDEO_PICTURE_QUEUE_SIZE_MIN)
// 字幕帧队列大小
#define SUBPICTURE_QUEUE_SIZE 16  // 字幕帧缓存数量
// 采样帧队列大小
#define SAMPLE_QUEUE_SIZE 9  // 采样帧缓存数量
// 帧队列大小 (取样本队列、视频队列和字幕队列中的最大值)
#define FRAME_QUEUE_SIZE \
    FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))

// 音量控制步长 (dB)
#define AUDIO_VOLUME_STEP (0.75)
#define SDL_VOLUME_STEP AUDIO_VOLUME_STEP

// 音视频同步阈值范围
// 小于最小阈值时不进行同步校正
#define AV_SYNC_THRESHOLD_MIN 0.04
// 大于最大阈值时进行同步校正
#define AV_SYNC_THRESHOLD_MAX 0.1
// 如果帧持续时间超过此阈值,将不会复制以补偿音视频同步
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1
// 错误过大时不进行同步校正
#define AV_NOSYNC_THRESHOLD 10.0

// 将时间戳转换为毫秒
#define fftime_to_milliseconds(ts) (av_rescale(ts, 1000, AV_TIME_BASE))
// 将毫秒转换为时间戳
#define milliseconds_to_fftime(ms) (av_rescale(ms, AV_TIME_BASE, 1000))
// FFTrackCacheStatistic 结构体
// 用于存储某个轨道的缓存统计信息
typedef struct FFTrackCacheStatistic {
    int64_t duration;  // 缓存数据的时长 单位毫秒ms
    int64_t bytes;     // 缓存数据的字节数
    int64_t packets;   // 缓存数据的数据包数
} FFTrackCacheStatistic;

// FFStatistic 结构体
// 用于存储媒体播放的各种统计信息
typedef struct FFStatistic {
    int64_t vdec_type;  // 视频解码器类型

    float vfps;        // 视频帧率
    float vdps;        // 视频显示帧率
    float avdelay;     // 音视频延迟
    float avdiff;      // 音视频差异
    int64_t bit_rate;  // 码率

    // 视频缓存统计信息
    FFTrackCacheStatistic video_cache;

    // 音频缓存统计信息
    FFTrackCacheStatistic audio_cache;

    int64_t buf_backwards;               // 后退缓冲区大小
    int64_t buf_forwards;                // 前进缓冲区大小
    int64_t buf_capacity;                // 缓冲区容量
    SDL_SpeedSampler2 tcp_read_sampler;  // TCP 读取速度采样器
    int64_t latest_seek_load_duration;   // 最近一次 seek 操作的加载时长
    int64_t byte_count;                  // 总字节数
    int64_t cache_physical_pos;          // 缓存的物理位置
    int64_t cache_file_forwards;         // 缓存文件的前进位置
    int64_t cache_file_pos;              // 缓存文件的位置
    int64_t cache_count_bytes;           // 缓存的字节数
    int64_t logical_file_size;           // 逻辑文件大小
    int drop_frame_count;                // 丢帧数
    int decode_frame_count;              // 解码帧数
    float drop_frame_rate;               // 丢帧率
} FFStatistic;

typedef struct MyAVPacketList {
    AVPacket pkt;                 //解封装后的数据
    struct MyAVPacketList* next;  //下一个节点
    int serial;                   //播放序列
} MyAVPacketList;

// ===========================================================================
// C++17 同步原语内联辅助函数 (替代 SDL_mutex / SDL_cond)
// ===========================================================================
static inline int ez_mutex_lock(std::mutex* mutex) {
    if (mutex) {
        mutex->lock();
        return 0;
    }
    return -1;
}

static inline int ez_mutex_unlock(std::mutex* mutex) {
    if (mutex) {
        mutex->unlock();
        return 0;
    }
    return -1;
}

static inline int ez_cond_signal(std::condition_variable* cond) {
    if (cond) {
        cond->notify_one();
        return 0;
    }
    return -1;
}

static inline int ez_cond_broadcast(std::condition_variable* cond) {
    if (cond) {
        cond->notify_all();
        return 0;
    }
    return -1;
}

static inline int ez_cond_wait(std::condition_variable* cond, std::mutex* mutex) {
    if (cond && mutex) {
        std::unique_lock<std::mutex> lock(*mutex, std::adopt_lock);
        cond->wait(lock);
        lock.release();
        return 0;
    }
    return -1;
}

static inline int ez_cond_wait_timeout(std::condition_variable* cond, std::mutex* mutex, int ms) {
    if (!cond || !mutex) {
        return -1;
    }
    std::unique_lock<std::mutex> lock(*mutex, std::adopt_lock);
    std::cv_status status = cond->wait_for(lock, std::chrono::milliseconds(ms));
    lock.release();
    return (status == std::cv_status::no_timeout) ? 0 : -1;
}

typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;  // 队首，队尾指针
    int nb_packets;                        // 包数量，也就是队列元素数量
    int size;                              // 队列所有元素的数据大小总和
    int64_t duration;                      // 队列所有元素的数据播放持续时间
    int abort_request;                     // 用户退出请求标志
    int serial;  // 播放序列号，和MyAVPacketList的serial作用相同，但改变的时序稍微有点不同
    std::mutex* mutex;              // 用于维持PacketQueue的多线程安全
    std::condition_variable* cond;  // 用于读、写线程相互通知

    //阈值单位ms
    int duration_cache_max   = 1000;
    int duration_cache_shake = 100;
} PacketQueue;

typedef struct AudioParams {
    int freq                  = 0;                   // 采样率
    int channels              = 0;                   // 通道数
    AVChannelLayout ch_layout = {};                  // FFmpeg 7.1 通道布局
    enum AVSampleFormat fmt   = AV_SAMPLE_FMT_NONE;  // 音频采样格式
    int frame_size            = 0;                   // 一个采样单元占用的字节数
    int bytes_per_sec         = 0;                   // 每秒字节数
} AudioParams;

static inline void ez_audio_params_uninit(AudioParams* params) {
    av_channel_layout_uninit(&params->ch_layout);
    params->channels = 0;
}

static inline int ez_audio_params_copy(AudioParams* dst, const AudioParams* src) {
    if (dst == src) {
        return 0;
    }
    av_channel_layout_uninit(&dst->ch_layout);
    const int ret = av_channel_layout_copy(&dst->ch_layout, &src->ch_layout);
    if (ret < 0) {
        return ret;
    }
    dst->freq          = src->freq;
    dst->channels      = src->channels;
    dst->fmt           = src->fmt;
    dst->frame_size    = src->frame_size;
    dst->bytes_per_sec = src->bytes_per_sec;
    return 0;
}

/* Common struct for handling all types of decoded data and allocated render buffers. */
// 用于缓存解码后的数据
typedef struct Frame {
    AVFrame* frame;   // 指向数据帧
    int serial;       // 帧序列，在seek的操作时serial会变化
    double pts;       // 时间戳，单位为秒
    double duration;  // 该帧持续时间，单位为秒
    int64_t pos;
    int width;   // 图像宽度
    int height;  // 图像高读
    int format;  // 对于图像为(enum AVPixelFormat)
    AVRational sar;
    int uploaded;
    int flip_v;
} Frame;

/* 这是一个循环队列，windex是指其中的首元素，rindex是指其中的尾部元素. */
typedef struct FrameQueue {
    Frame queue
        [FRAME_QUEUE_SIZE];  // FRAME_QUEUE_SIZE  最大size, 数字太大时会占用大量的内存，需要注意该值的设置
    int rindex;  // 读索引。待播放时读取此帧进行播放，播放后此帧成为上一帧
    int windex;    // 写索引
    int size;      // 当前总帧数
    int max_size;  // 可存储最大帧数
    int keep_last;
    int rindex_shown;
    std::mutex* mutex;              // 互斥量
    std::condition_variable* cond;  // 条件变量
    PacketQueue* pktq;              // 数据包缓冲队列
} FrameQueue;

// 这里讲的系统时钟 是通过av_gettime_relative()获取到的时钟，单位为微妙
typedef struct Clock {
    double pts;  // 时钟基础, 当前帧(待播放)显示时间戳，播放后，当前帧变成上一帧
    // 当前pts与当前系统时钟的差值, audio、video对于该值是独立的
    double pts_drift;  // clock base minus time at which we updated the clock
    // 当前时钟(如视频时钟)最后一次更新时间，也可称当前时钟时间
    double last_updated;  // 最后一次更新的系统时钟
    double speed;         // 时钟速度控制，用于控制播放速度
    // 播放序列，所谓播放序列就是一段连续的播放动作，一个seek操作会启动一段新的播放序列
    int serial;  // clock is based on a packet with this serial
    int paused;  // = 1 说明是暂停状态
    // 指向packet_serial
    int*
        queue_serial; /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

/**
 *音视频同步方式，缺省以音频为基准
 */
enum {
    AV_SYNC_UNKNOW_MASTER = -1,
    AV_SYNC_AUDIO_MASTER,  // 以音频为基准
    AV_SYNC_VIDEO_MASTER,  // 以视频为基准
    //    AV_SYNC_EXTERNAL_CLOCK,                 // 以外部时钟为基准，synchronize to an external clock */
};

extern AVPacket flush_pkt;
// 队列相关
int packet_queue_put(PacketQueue* q, AVPacket* pkt);
int packet_queue_put_nullpacket(PacketQueue* q, int stream_index);
int packet_queue_init(PacketQueue* q);
void packet_queue_flush(PacketQueue* q);
void packet_queue_destroy(PacketQueue* q);
void packet_queue_abort(PacketQueue* q);
void packet_queue_start(PacketQueue* q);
int packet_queue_get(PacketQueue* q, AVPacket* pkt, int block, int* serial);

/**
 * @brief 获取帧缓存的数据可以播放的时间长度
 * @param q 队列本身
 * @param time_base 用于计算packet的时间戳转换
 * @param packet_duration 单个包可以播放的时长
 * @return 返回时长以秒为单位
 */
double packet_queue_cache_duration(PacketQueue* q, AVRational time_base, double packet_duration);

/* 初始化FrameQueue，视频和音频keep_last设置为1，字幕设置为0 */
int frame_queue_init(FrameQueue* f, PacketQueue* pktq, int max_size, int keep_last);
void frame_queue_destory(FrameQueue* f);
void frame_queue_signal(FrameQueue* f);
/* 获取队列当前Frame, 在调用该函数前先调用frame_queue_nb_remaining确保有frame可读 */
Frame* frame_queue_peek(FrameQueue* f);

/* 获取当前Frame的下一Frame, 此时要确保queue里面至少有2个Frame */
// 不管你什么时候调用，返回来肯定不是 NULL
Frame* frame_queue_peek_next(FrameQueue* f);
/* 获取last Frame：
 */
Frame* frame_queue_peek_last(FrameQueue* f);
// 获取可写指针
Frame* frame_queue_peek_writable(FrameQueue* f);
// 获取可读
Frame* frame_queue_peek_readable(FrameQueue* f);
// 更新写指针
void frame_queue_push(FrameQueue* f);
/* 释放当前frame，并更新读索引rindex */
void frame_queue_next(FrameQueue* f);
int frame_queue_nb_remaining(FrameQueue* f);
int64_t frame_queue_last_pos(FrameQueue* f);

// 时钟相关
double get_clock(Clock* c);
void set_clock_at(Clock* c, double pts, int serial, double time);
void set_clock(Clock* c, double pts, int serial);
void init_clock(Clock* c, int* queue_serial);

void ffp_reset_statistic(FFStatistic* dcc);

#endif  // FF_FFPLAY_DEF_H
