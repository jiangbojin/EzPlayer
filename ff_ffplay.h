#ifndef FF_FFPLAY_H
#define FF_FFPLAY_H
#include <thread>
#include <functional>
#include <messagequeue.h>
#include "ff_ffplay_def.h"
#include "sonic.h"
extern "C" {
#include "libavcodec/avcodec.h"
}
class FFPlayer;
class Decoder
{
public:
    int packet_pending_ = 0;
    AVPacket pkt_;
    FFPlayer* is;
    PacketQueue	*queue_;         // 数据包队列
    AVCodecContext	*avctx_;     // 解码器上下文
    int		pkt_serial_;         // 包序列
    int		finished_;           // =0，解码器处于工作状态；=非0，解码器处于空闲状态

    std::unique_ptr<std::thread>decoder_thread_;

    //使用了硬件加速解码



    int64_t start_pts;
    AVRational start_pts_tb;
    int64_t next_pts;
    AVRational next_pts_tb;
    Decoder();
    ~Decoder();
    void decoder_init(AVCodecContext *avctx, PacketQueue *queue,void* is);
    // 创建和启动线程
    int decoder_start(enum AVMediaType codec_type, const char *thread_name, void* arg);
    // 停止线程
    void decoder_abort(FrameQueue *fq);
    void decoder_destroy();
    int decoder_decode_frame(AVFrame *frame);
    int get_video_frame(AVFrame *frame);
    int queue_picture(FrameQueue *fq, AVFrame *src_frame, double pts,
                      double duration, int64_t pos, int serial);
    int audio_thread(void* arg);
    int video_thread(void* arg);


    ///硬解码,把gpu的数据放回到dst，建议直接通过显存渲染。
    ///
    /// \param dst 放回内存
    /// \param src gpu frame
    /// \return
    ///
    int hw_decoder_gpu_tocpu_copy(AVFrame* dst,AVFrame* src);
};
class FFPlayer
{
public:
    FFPlayer(std::shared_ptr<MessageQueue>);
    
    void ffp_destroy();
    int ffp_prepare_async_l(char *file_name);

    // 播放控制
    int       ffp_start_l();
    int       ffp_stop_l();
    int stream_open( const char *file_name);
    void stream_close();

    int stream_component_open(int stream_index);
    // 关闭指定stream的解码线程，释放解码器资源
    void stream_component_close(int stream_index);

    int audio_open(int64_t wanted_channel_layout,
                   int wanted_nb_channels, int wanted_sample_rate,
                   struct AudioParams *audio_hw_params);
    void audio_close();

    //获取播放时长
    long ffp_get_duration_l();
    long ffp_get_current_position_l();

    // 暂停恢复
    int ffp_pause_l();
    void toggle_pause(int pause_on);
    void toggle_pause_l(int pause_on);
    void stream_update_pause_l();
    void stream_toggle_pause_l(int pause_on);

    // seek相关
    int ffp_seek_to_l(long msec);
    // 单位是秒 整数
    int ffp_forward_to_l(long incr);
    // 单位是秒 负数
    int ffp_back_to_l(long incr);
    int ffp_forward_or_back_to_l(long incr);
    void stream_seek(int64_t pos, int64_t rel, int seek_by_bytes);

    // 截屏相关
    int ffp_screenshot_l(char *screen_path);
    void screenshot(AVFrame *frame);

    // 变速相关
    int get_target_frequency();
    int     get_target_channels();
    void ffp_set_playback_rate(float rate);
    float ffp_get_playback_rate();
    bool is_normal_playback_rate();
    int ffp_get_playback_rate_change();
    void ffp_set_playback_rate_change(int change);
    void sync_clock_to_slave(Clock *c, Clock *slave);
    //音量相关
    void ffp_set_playback_volume(int value);

    //frame队列报警机制
    int frameq_cache_flag = 1; //1 已使用中 0未使用 2 非直播流
    void ffp_frameq_cache(int value);

    //播放完毕相关判断 1. av_read_frame返回eof; 2. audio没有数据可以输出; 3.video没有数据可以输出
    void check_play_finish();   //如果已经结束则通知ui调用停止函数
    // 供外包获取信息
    int64_t ffp_get_property_int64(int id, int64_t default_value);
    void ffp_track_statistic_l(AVStream *st, PacketQueue *q, FFTrackCacheStatistic *cache);
    void ffp_audio_statistic_l();
    void ffp_video_statistic_l();

    std::shared_ptr<MessageQueue> msg_queue_ =nullptr;

    char *input_filename_;
    int realtime = 0;
    int  stream_has_enough_packets(AVStream *st, int stream_id, PacketQueue *queue);
    int read_thread();

    std::unique_ptr<std::thread> read_thread_;

    int video_refresh_thread();
    void video_refresh(double *remaining_time);
    void stream_toggle_pause();
    double vp_duration(  Frame *vp, Frame *nextvp);
    double compute_target_delay(double delay);
    void  update_video_pts(double pts, int64_t pos, int serial);
    // 视频画面输出相关

    std::unique_ptr<std::thread> video_refresh_thread_;
    std::function<int(const Frame *)> video_refresh_callback_ = NULL;
    void AddVideoRefreshCallback(std::function<int(const Frame *)> callback);

    int get_master_sync_type();
    double get_master_clock();

    //设置最大pkt缓存
    void ffp_set_pkt_queue_cache(bool type, int value);

    int av_sync_type = AV_SYNC_AUDIO_MASTER;           // 音视频同步类型, 默认audio master
    Clock	audclk;             // 音频时钟
    Clock	vidclk;             // 视频时钟
    //    Clock	extclk;

    double			audio_clock = 0;            // 当前音频帧的PTS+当前帧Duration
    int             audio_clock_serial;     // 播放序列，seek可改变此值, 解码后保存
    int64_t         audio_callback_time = 0;
    // 帧队列
    FrameQueue	pictq;          // 视频Frame队列
    FrameQueue	sampq;          // 采样Frame队列

    // 包队列
    PacketQueue		audioq;                 // 音频packet队列
    PacketQueue videoq;             // 视频队列
    int abort_request = 0;

    AVStream		*audio_st = NULL;              // 音频流
    AVStream		*video_st = NULL;              // 音频流
    int force_refresh = 0;
    int rdftspeed =0.03; //刷新频率
    int last_vis_time=0;
    double frame_timer = 0;

    int audio_stream = -1;
    int video_stream = -1;

    Decoder auddec;             // 音频解码器
    Decoder viddec;             // 视频解码器


    int eof = 0;
    int audio_no_data = 0;
    int video_no_data = 0;
    AVFormatContext *ic = NULL;

    int paused = 0;
    // 音频输出相关
    struct AudioParams audio_src;  // 保存最新解码的音频参数
    struct AudioParams audio_tgt;  // 保存SDL音频输出需要的参数
    struct SwrContext *swr_ctx = NULL;         // 音频重采样context
    int			audio_hw_buf_size = 0;          // SDL音频缓冲区的大小(字节为单位)
    // 指向待播放的一帧音频数据，指向的数据区将被拷入SDL音频缓冲区。若经过重采样则指向audio_buf1，
    // 否则指向frame中的音频
    uint8_t			*audio_buf = NULL;             // 指向需要重采样的数据
    uint8_t			*audio_buf1 = NULL;            // 指向重采样后的数据
    unsigned int		audio_buf_size = 0;     // 待播放的一帧音频数据(audio_buf指向)的大小
    unsigned int		audio_buf1_size = 0;    // 申请到的音频缓冲区audio_buf1的实际尺寸
    int			audio_buf_index = 0;            // 更新拷贝位置 当前音频帧中已拷入SDL音频缓冲区
    int audio_write_buf_size;
    int audio_volume = 50;   // 音量相关
    int startup_volume = 40; // 起始音量
    // seek相关
    int64_t seek_req = 0; //存储请求的寻址位置
    int64_t seek_rel = 0; //相对寻址的偏移量
    int64_t seek_flags = 0; //seek方式
    int64_t seek_pos = 0;  // seek的位置

    // 截屏相关
    bool req_screenshot_ = false;
    char *screen_path_ = NULL;

    //单步运行
    int step = 0;
    int framedrop = 1;
    int frame_drops_late = 0;

    int pause_req = 0;
    int auto_resume = 0;
    int buffering_on = 0;
    // 变速相关
    float       pf_playback_rate = 1.0;             // 播放速率
    int         pf_playback_rate_changed = 0;       // 播放速率改变标志位
    // 变速相关
    sonicStreamStruct *audio_speed_convert = nullptr;
    int max_frame_duration = 3600;
    int frame_drops_early = 0;  //yuv drop数量

    // 统计相关的操作
    FFStatistic         stat;

    //一键禁音
    bool audio_muted_ = false;


    //解码器类型
    std::string decode_type;


    //本机选择的硬件解码名称
    std::string hw_device_type;
    //硬加速的对应格式
    static enum AVPixelFormat hw_pix_fmt;
    //硬加速的上下文
    AVBufferRef *hw_device_ctx = NULL;
    // 记录是否使用硬件解码
    bool   m_isHw_device = false;
    //主要api接口
    ///
    ///初始化硬件解码器
    /// \param avctx
    /// \param codec
    /// \return -2传参有误，-1硬件有误 1成功返回
    ///
    int initHWDecoder(AVCodecContext*avctx, const AVCodec* codec);
    int DeinitHWDecoder();
   ///
    /// 硬件加速格式回调
    /// \param ctx
    /// \param pix_fmts
    /// \return
    ///
    static enum AVPixelFormat get_hw_format(AVCodecContext *ctx,const enum AVPixelFormat *pix_fmts);
};



#endif // FF_FFPLAY_H
