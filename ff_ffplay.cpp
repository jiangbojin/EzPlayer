#include "ff_ffplay.h"
#include <iostream>
#include <cmath>
#include <string.h>
#include "ffmsg.h"
#include "sonic.h"
#include "screenshot.h"

#include "easylogging++.h"
//#define LOG(INFO) std::cout
//#define LOG(ERROR) std::cout

/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
int infinite_buffer = 0;
static int decoder_reorder_pts = -1;
static int seek_by_bytes = -1;
void print_error(const char *filename, int err)
{
    char errbuf[128];
    const char *errbuf_ptr = errbuf;
    if (av_strerror(err, errbuf, sizeof(errbuf)) < 0) {
        errbuf_ptr = strerror(AVUNERROR(err));
    }
    av_log(NULL, AV_LOG_ERROR, "%s: %s\n", filename, errbuf_ptr);
}

FFPlayer::FFPlayer(std::shared_ptr<MessageQueue>m)
    :msg_queue_(m)
{
    pf_playback_rate = 1.0;
    // 初始化统计信息
    ffp_reset_statistic(&stat);
}


void FFPlayer::ffp_destroy()
{
    stream_close();
}
/**
 *  队列初始化 通过stream_open函数，开启read_thread读取线程以及视频刷新线程
 * @param file_name
 * @return
 */
int FFPlayer::ffp_prepare_async_l(char *file_name)
{
    //保存文件名
    input_filename_ =  strdup(file_name);
    int reval = stream_open(file_name);
    return reval;
}

// 开启播放 或者恢复播放
int FFPlayer::ffp_start_l()
{
    // 触发播放
    LOG(INFO) << "ffp_start_l";
    toggle_pause( 0);
    return 0;
}
///
/// 流退出 禁止再插入消息
/// \return
///
int FFPlayer::ffp_stop_l()
{
    abort_request = 1;  // 请求退出
    msg_queue_->msg_queue_abort();  // 禁止再插入消息
    return 0;
}

/*
 *  初始化SDL以允许⾳频输出；初始化帧Frame队列
    初始化包Packet队列
    初始化时钟Clock
    初始化⾳量
    创建解复⽤读取线程read_thread
    创建视频刷新线程video_refresh_thread
*/
/// \param file_name
/// \return
///
int FFPlayer::stream_open(const char *file_name)
{

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        av_log(NULL, AV_LOG_FATAL, "Could not initialize SDL - %s\n", SDL_GetError());
        av_log(NULL, AV_LOG_FATAL, "(Did you set the DISPLAY variable?)\n");
        return -1;
    }

    // 初始化Frame帧队列
    if (frame_queue_init(&pictq, &videoq, VIDEO_PICTURE_QUEUE_SIZE_DEFAULT, 1) < 0) {
        goto fail;
    }
    // 要注意最后一个值设置为1的重要性
    if (frame_queue_init(&sampq, &audioq, SAMPLE_QUEUE_SIZE, 1) < 0) {
        goto fail;
    }
    // 初始化Packet包队列
    if (packet_queue_init(&videoq) < 0 ||
            packet_queue_init(&audioq) < 0 ) {
        goto fail;
    }
    // 初始化时钟
    /*
     * 初始化时钟
     * 时钟序列->queue_serial，实际上指向的是videoq.serial
     */
    init_clock(&vidclk, &videoq.serial);
    init_clock(&audclk, &audioq.serial);
    audio_clock_serial = -1;
    // 初始化音量等
    startup_volume = av_clip(startup_volume, 0, 128);
    startup_volume = av_clip(SDL_MIX_MAXVOLUME *  startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
    audio_volume =  startup_volume;
    // 创建解复用器读数据线程 read_thread
    read_thread_ = new std::thread(&FFPlayer::read_thread, this);
    // 创建视频刷新线程video_refresh_thread
    video_refresh_thread_ = new std::thread(&FFPlayer::video_refresh_thread, this);
    return 0;
fail:
    stream_close();
    return -1;
}
/*
请求关闭read_thread线程
关闭解复⽤器
关闭stream，⽐如调⽤stream_component_close，该函数实现的时候涉及到：
    关闭解码线程
    释放解码器资源
    关闭对应的⾳频、视频设备
消耗包队列
消耗帧队列
释放其他资源
*/
void FFPlayer::stream_close()
{
    abort_request = 1; // 请求退出
    if(read_thread_ && read_thread_->joinable()) {
        read_thread_->join();       // 等待线程退出
    }
    /* close each stream */
    if (audio_stream >= 0) {
        stream_component_close(audio_stream);    // 解码器线程请求abort的时候有调用 packet_queue_abort
    }
    if (video_stream >= 0) {
        stream_component_close(video_stream);
    }
    // 关闭解复用器 avformat_close_input(&ic);
    // 释放packet队列
    packet_queue_destroy(&videoq);
    packet_queue_destroy(&audioq);
    // 释放frame队列
    frame_queue_destory(&pictq);
    frame_queue_destory(&sampq);
    if(input_filename_) {
        free(input_filename_);
        input_filename_ = NULL;
    }
}

// 如果想指定解码器怎么处理？
// 打开对应的解码器并做初始化
// 创建和启动解码线程
// 初始化⾳频或视频输出设备
int FFPlayer::stream_component_open(int stream_index)
{
    AVCodecContext *avctx;
    AVCodec *codec;
    int sample_rate;
    int nb_channels;
    int64_t channel_layout;
    int ret = 0;
    // 判断stream_index是否合法
    if (stream_index < 0 || stream_index >= ic->nb_streams) {
        return -1;
    }
    /*  为解码器分配一个编解码器上下文结构体 */
    avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
        return AVERROR(ENOMEM);
    }
    /* 将码流中的编解码器信息拷贝到新分配的编解码器上下文结构体 */
    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0) {
        goto fail;
    }
    // 设置pkt_timebase
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;
    /* 根据codec_id查找解码器 */
    codec = (AVCodec *)avcodec_find_decoder(avctx->codec_id);
    if (!codec) {
        av_log(NULL, AV_LOG_WARNING,
               "No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
        ret = AVERROR(EINVAL);
        goto fail;
    }
    if ((ret = avcodec_open2(avctx, codec, NULL)) < 0) {
        goto fail;
    }
    switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        //从avctx(即AVCodecContext)中获取音频格式参数
        sample_rate = avctx->sample_rate;;  // 采样率
        nb_channels = avctx->channels;;    // 通道数
        channel_layout = avctx->channel_layout;; // 通道布局
        /* prepare audio output 准备音频输出*/
        //调用audio_open打开sdl音频输出，实际打开的设备参数保存在audio_tgt，返回值表示输出设备的缓冲区大小
        if ((ret = audio_open( channel_layout, nb_channels, sample_rate, &audio_tgt)) < 0) {
            goto fail;
        }
        audio_hw_buf_size = ret;
        audio_src = audio_tgt;  //暂且将数据源参数等同于目标输出参数
        //初始化audio_buf相关参数
        audio_buf_size  = 0;
        audio_buf_index = 0;
        audio_stream = stream_index;    // 获取audio的stream索引
        audio_st = ic->streams[stream_index];  // 获取audio的stream指针
        // 初始化ffplay封装的音频解码器, 并将解码器上下文 avctx和Decoder绑定
        auddec.decoder_init(avctx, &audioq,this);
        // 启动音频解码线程
        auddec.decoder_start(AVMEDIA_TYPE_AUDIO, "audio_thread", this);
        // 允许音频输出
        //play audio
        SDL_PauseAudio(0);
        break;
    case AVMEDIA_TYPE_VIDEO:
        video_stream = stream_index;    // 获取video的stream索引
        video_st = ic->streams[stream_index];// 获取video的stream指针
        //        // 初始化ffplay封装的视频解码器
        viddec.decoder_init(avctx, &videoq,this); //
        //        // 启动视频频解码线程
        if ((ret = viddec.decoder_start(AVMEDIA_TYPE_VIDEO, "video_decoder", this)) < 0) {
            goto out;
        }
        break;
    default:
        break;
    }
    goto out;
fail:
    avcodec_free_context(&avctx);
out:
    return ret;
}
//关闭解码器线程
//释放解码器资源
//关闭⾳频或视频输出设备
void FFPlayer::stream_component_close(int stream_index)
{
    AVCodecParameters *codecpar;
    if (stream_index < 0 || stream_index >= ic->nb_streams) {
        return;
    }
    codecpar = ic->streams[stream_index]->codecpar;
    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        LOG(INFO) << "  AVMEDIA_TYPE_AUDIO\n";
        // 请求终止解码器线程
        auddec.decoder_abort(&sampq);
        // 关闭音频设备
        audio_close();
        // 销毁解码器
        auddec.decoder_destroy();
        // 释放重采样器
        swr_free(&swr_ctx);
        // 释放audio buf
        av_freep(&audio_buf1);
        audio_buf1_size = 0;
        audio_buf = NULL;
        break;
    case AVMEDIA_TYPE_VIDEO:
        // 请求退出视频画面刷新线程
        if(video_refresh_thread_ && video_refresh_thread_->joinable()) {
            video_refresh_thread_->join();  // 等待线程退出
        }
        LOG(INFO) <<  "  AVMEDIA_TYPE_VIDEO\n";
        // 请求终止解码器线程
        // 关闭音频设备
        // 销毁解码器
        viddec.decoder_abort(&pictq);
        viddec.decoder_destroy();
        break;
    default:
        break;
    }
    //    ic->streams[stream_index]->discard = AVDISCARD_ALL;  // 这个又有什么用?
    switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
        audio_st = NULL;
        audio_stream = -1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        video_st = NULL;
        video_stream = -1;
        break;
    default:
        break;
    }
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in audio_buf, with size in bytes given by the return
 * value.
 */
static int audio_decode_frame(FFPlayer *is)
{
    int data_size, resampled_data_size;
    int64_t dec_channel_layout;
    int wanted_nb_samples;
    Frame *af;
    int ret = 0;
    if(is->paused) {
        return -1;
    }
    // 读取一帧数据
    do {
        // 若队列头部可读，则由af指向可读帧
        if (!(af = frame_queue_peek_readable(&is->sampq))) {
            return -1;
        }
        frame_queue_next(&is->sampq);  // 不同序列的出队列
    } while (af->serial != is->audioq.serial); // 这里容易出现af->serial != audioq.serial 一直循环
    // 根据frame中指定的音频参数获取缓冲区的大小 af->frame->channels * af->frame->nb_samples * 2
    data_size = av_samples_get_buffer_size(NULL, av_frame_get_channels(af->frame),
                                           af->frame->nb_samples,
                                           (enum AVSampleFormat)af->frame->format, 1);
    // 获取声道布局
    dec_channel_layout =  (af->frame->channel_layout && af->frame->channels == av_get_channel_layout_nb_channels(af->frame->channel_layout)) ?
                af->frame->channel_layout : av_get_default_channel_layout(av_frame_get_channels(af->frame));
    if(dec_channel_layout == 0) {
        LOG(INFO) << af->frame->channel_layout << ", failed: " <<  av_get_default_channel_layout(af->frame->channels) ;
        dec_channel_layout = 3; // fixme
        return -1; // 这个是异常情况
    }
    // 获取样本数校正值：若同步时钟是音频，则不调整样本数；否则根据同步需要调整样本数
    //    wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);  // 目前不考虑非音视频同步的是情况
    wanted_nb_samples = af->frame->nb_samples;
    // audio_tgt是SDL可接受的音频帧数，是audio_open()中取得的参数
    // 在audio_open()函数中又有"audio_src = audio_tgt""
    // 此处表示：如果frame中的音频参数 == audio_src == audio_tgt，
    // 那音频重采样的过程就免了(因此时swr_ctr是NULL)
    // 否则使用frame(源)和audio_tgt(目标)中的音频参数来设置swr_ctx，
    // 并使用frame中的音频参数来赋值audio_src
    if (af->frame->format           != is->audio_src.fmt            || // 采样格式
            dec_channel_layout      != is->audio_src.channel_layout || // 通道布局
            af->frame->sample_rate  != is->audio_src.freq  ||        // 采样率
            (wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx) ) {
        swr_free(&is->swr_ctx);
        is->swr_ctx = swr_alloc_set_opts(NULL,
                                         is->audio_tgt.channel_layout,  // 目标输出
                                         is->audio_tgt.fmt,
                                         is->audio_tgt.freq,
                                         dec_channel_layout,            // 数据源
                                         (enum AVSampleFormat)af->frame->format,
                                         af->frame->sample_rate,
                                         0, NULL);
        int ret = 0;
        if (!is->swr_ctx || (ret = swr_init(is->swr_ctx)) < 0) {
            char errstr[256] = { 0 };
            av_strerror(ret, errstr, sizeof(errstr));
            LOG(INFO) << "swr_init failed:" << errstr ;
            sprintf(errstr, "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz %s %d channels!\n",
                    af->frame->sample_rate, av_get_sample_fmt_name((enum AVSampleFormat)af->frame->format), af->frame->channels,
                    is-> audio_tgt.freq, av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
            LOG(INFO) << errstr;
            swr_free(&is->swr_ctx);
            ret = -1;
            goto fail;
        }
        is->audio_src.channel_layout = dec_channel_layout;
        is->audio_src.channels       = af->frame->channels;
        is->audio_src.freq = af->frame->sample_rate;
        is->audio_src.fmt = (enum AVSampleFormat)af->frame->format;
    }
    if (is->swr_ctx) {
        // 重采样输入参数1：输入音频样本数是af->frame->nb_samples
        // 重采样输入参数2：输入音频缓冲区
        const uint8_t **in = (const uint8_t **)af->frame->extended_data; // data[0] data[1]
        // 重采样输出参数1：输出音频缓冲区
        uint8_t **out = &is->audio_buf1; //真正分配缓存audio_buf1，指向是用audio_buf
        // 重采样输出参数2：输出音频缓冲区尺寸， 高采样率往低采样率转换时得到更少的样本数量，比如 96k->48k, wanted_nb_samples=1024
        // 则wanted_nb_samples * audio_tgt.freq / af->frame->sample_rate 为1024*48000/96000 = 512
        // +256 的目的是重采样内部是有一定的缓存，就存在上一次的重采样还缓存数据和这一次重采样一起输出的情况，所以目的是多分配输出buffer
        int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate
                + 256;
        // 计算对应的样本数 对应的采样格式 以及通道数，需要多少buffer空间
        int out_size  = av_samples_get_buffer_size(NULL, is->audio_tgt.channels,
                                                   out_count, is->audio_tgt.fmt, 0);
        int len2;
        if (out_size < 0) {
            av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
            ret = -1;
            goto fail;
        }
        // if(audio_buf1_size < out_size) {重新分配out_size大小的缓存给audio_buf1, 并将audio_buf1_size设置为out_size }
        av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
        if (!is->audio_buf1) {
            ret = AVERROR(ENOMEM);
            goto fail;
        }
        // 音频重采样：len2返回值是重采样后得到的音频数据中单个声道的样本数
        len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
        if (len2 < 0) {
            av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
            ret = -1;
            goto fail;
        }
        if (len2 == out_count) { // 这里的意思是我已经多分配了buffer，实际输出的样本数不应该超过我多分配的数量
            av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
            if (swr_init(is->swr_ctx) < 0) {
                swr_free(&is->swr_ctx);
            }
        }
        // 重采样返回的一帧音频数据大小(以字节为单位)
        is->audio_buf = is->audio_buf1;
        resampled_data_size = len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
    } else {
        // 未经重采样，则将指针指向frame中的音频数据
        is->audio_buf = af->frame->data[0]; // s16交错模式data[0], fltp data[0] data[1]
        resampled_data_size = data_size;
    }
    if (!std::isnan(af->pts)) {
        is->audio_clock = af->pts + (double) af->frame->nb_samples / af->frame->sample_rate;
    } else {
        is->audio_clock = NAN;
    }
    is->audio_clock_serial = af->serial;    // 保存当前解码帧的serial
    ret = resampled_data_size;
fail:
    return ret;
}



/* prepare a new audio buffer */
/**
 * @brief sdl_audio_callback
 * @param opaque    指向user的数据
 * @param stream    拷贝PCM的地址
 * @param len       需要拷贝的长度
 */
static void sdl_audio_callback(void *opaque, Uint8 *stream, int len)
{
    // 2ch 2字节 1024 = 4096 -> 回调每次读取2帧数据
    FFPlayer *is = (FFPlayer *)opaque;
    int audio_size, len1;
    is->audio_callback_time = av_gettime_relative();
    while (len > 0) {   // 循环读取，直到读取到足够的数据
        /* (1)如果audio_buf_index < audio_buf_size则说明上次拷贝还剩余一些数据，
         * 先拷贝到stream再调用audio_decode_frame
         * (2)如果audio_buf消耗完了，则调用audio_decode_frame重新填充audio_buf
         */
        if (is->audio_buf_index >= is->audio_buf_size) {
            audio_size = audio_decode_frame(is);
            //            LOG(INFO) << "  audio_size: " << audio_size;
            if (audio_size < 0) {
                /* if error, just output silence */
                is->audio_buf = NULL;
                is->audio_buf_size = SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size
                        * is->audio_tgt.frame_size;
                is->audio_no_data  = 1;      // 没有数据可以读取
                if(is->eof) {
                    // 如果文件以及读取完毕，此时应该判断是否还有数据可以读取，如果没有就该发送通知ui停止播放
                    is->check_play_finish();
                }
            } else {
                is->audio_buf_size = audio_size; // 讲字节 多少字节
                is->audio_no_data = 0;
            }
            is->audio_buf_index = 0;
            // 2 是否需要做变速
            if(is->ffp_get_playback_rate_change()) {
                is->ffp_set_playback_rate_change(0);
                // 1.初始化
                if(is->audio_speed_convert) {
                    // 先释放
                    sonicDestroyStream(is->audio_speed_convert);
                }
                // 再创建
                is->audio_speed_convert = sonicCreateStream(is->get_target_frequency(),
                                                            is->get_target_channels());
                // 2.设置倍速 音调 语速系数
                sonicSetSpeed(is->audio_speed_convert, is->ffp_get_playback_rate());
                sonicSetPitch(is->audio_speed_convert, 1.0);
                sonicSetRate(is->audio_speed_convert, 1.0);
            }
            if(!is->is_normal_playback_rate() && is->audio_buf) {
                // 重新计算采样点 总字节/通道*采样点数
                int actual_out_samples = is->audio_buf_size /
                        (is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt));
                // 计算处理后的点数
                int out_ret = 0;
                int out_size = 0;
                int num_samples = 0;
                int sonic_samples = 0;
                //3.音频写入
                if(is->audio_tgt.fmt == AV_SAMPLE_FMT_FLT) {
                    out_ret = sonicWriteFloatToStream(is->audio_speed_convert,
                                                      (float *)is->audio_buf,
                                                      actual_out_samples);
                } else  if(is->audio_tgt.fmt == AV_SAMPLE_FMT_S16) {
                    out_ret = sonicWriteShortToStream(is->audio_speed_convert,
                                                      (short *)is->audio_buf,
                                                      actual_out_samples);
                } else {
                    av_log(NULL, AV_LOG_ERROR, "sonic unspport ......\n");
                }
                //重新计算buff
                num_samples =  sonicSamplesAvailable(is->audio_speed_convert);
                // 2通道  目前只支持2通道的
                out_size = (num_samples) * av_get_bytes_per_sample(is->audio_tgt.fmt) * is->audio_tgt.channels;
                av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
                if(out_ret) {
                    // 4.从流中读取处理好的数据
                    if(is->audio_tgt.fmt == AV_SAMPLE_FMT_FLT) {
                        sonic_samples = sonicReadFloatFromStream(is->audio_speed_convert,
                                                                 (float *)is->audio_buf1,
                                                                 num_samples);
                    } else  if(is->audio_tgt.fmt == AV_SAMPLE_FMT_S16) {
                        sonic_samples = sonicReadShortFromStream(is->audio_speed_convert,
                                                                 (short *)is->audio_buf1,
                                                                 num_samples);
                    } else {
                        LOG(ERROR) << "sonic unspport fmt: " << is->audio_tgt.fmt;
                    }
                    is->audio_buf = is->audio_buf1;
                    //                     LOG(INFO) << "mdy num_samples: " << num_samples;
                    //                     LOG(INFO) << "orig audio_buf_size: " << audio_buf_size;
                    is->audio_buf_size = out_size;
                    //                    LOG(INFO) << "mdy audio_buf_size: " << audio_buf_size;
                    is->audio_buf_index = 0;
                }
            }
        }
        if(is->audio_buf_size == 0) {
            continue;
        }
        //根据缓冲区剩余大小量力而行
        len1 = is->audio_buf_size - is->audio_buf_index;
        if (len1 > len) {
            len1 = len;
        }
        //判断是否为静音，以及当前音量的大小，如果音量为最大则直接拷贝数据
        if (!is->audio_muted_ && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME) {
            memcpy(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1);
        } else {
            memset(stream, 0, len1);
            if (!is->audio_muted_ &&  is->audio_buf) {
                SDL_MixAudio(stream, (uint8_t *)is->audio_buf + is->audio_buf_index, len1, is->audio_volume);
            }
        }
        /* 更新audio_buf_index，指向audio_buf中未被拷贝到stream的数据（剩余数据）的起始位置 */
        len -= len1;
        stream += len1;
        is->audio_buf_index += len1;
    }
    is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
    /* Let's assume the audio driver that is used by SDL has two periods. */
    if (!std::isnan(is->audio_clock)) {
        //   新时间点 = 原始时间点/播放速率  比如当播放速率位是0.5 则新时间点就要延迟 audio_clock变大 反之2.0  audio_clock变小
        double audio_clock = is->audio_clock / is->ffp_get_playback_rate();
        set_clock_at(&is->audclk,
                     audio_clock  - (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) / is->audio_tgt.bytes_per_sec,
                     is->audio_clock_serial,
                     is->audio_callback_time / 1000000.0);
    }
}


// 先参考我们之前讲的06-sdl-pcm范例
int FFPlayer::audio_open(int64_t wanted_channel_layout, int wanted_nb_channels, int wanted_sample_rate, AudioParams *audio_hw_params)
{
    SDL_AudioSpec wanted_spec;
    // 音频参数设置SDL_AudioSpec
    wanted_spec.freq = wanted_sample_rate;          // 采样频率
    wanted_spec.format = AUDIO_S16SYS; // 采样点格式
    wanted_spec.channels = wanted_nb_channels;          // 2通道
    wanted_spec.silence = 0;
    wanted_spec.samples = 2048;       // 23.2ms -> 46.4ms 每次读取的采样数量，多久产生一次回调和 samples
    wanted_spec.callback = sdl_audio_callback; // 回调函数
    wanted_spec.userdata = this;

    CoInitialize(NULL); //解决openaudio报错问题
    //    SDL_OpenAudioDevice
    //打开音频设备
    if(SDL_OpenAudio(&wanted_spec, NULL) != 0) {
        LOG(ERROR) << "Failed to open audio device, err: " <<  SDL_GetError();
        return -1;
    }

    // wanted_spec是期望的参数，spec是实际的参数，wanted_spec和spec都是SDL中的结构。
    // 此处audio_hw_params是FFmpeg中的参数，输出参数供上级函数使用
    // audio_hw_params保存的参数，就是在做重采样的时候要转成的格式。
    audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
    audio_hw_params->freq = wanted_spec.freq;
    audio_hw_params->channel_layout = wanted_channel_layout;
    audio_hw_params->channels =  wanted_spec.channels;
    if(audio_hw_params->channel_layout == 0) {
        audio_hw_params->channel_layout =
                av_get_default_channel_layout(audio_hw_params->channels);
        LOG(WARNING) << "layout is 0, force change to " << audio_hw_params->channel_layout;
    }
    /* audio_hw_params->frame_size这里只是计算一个采样点占用的字节数 */
    audio_hw_params->frame_size = av_samples_get_buffer_size(NULL, audio_hw_params->channels,
                                                             1,
                                                             audio_hw_params->fmt, 1);
    audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(NULL, audio_hw_params->channels,
                                                                audio_hw_params->freq,
                                                                audio_hw_params->fmt, 1);
    if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
        av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
        return -1;
    }
    // 比如2帧数据，一帧就是1024个采样点， 1024*2*2 * 2 = 8192字节
    return wanted_spec.size;	/* SDL内部缓存的数据字节, samples * channels *byte_per_sample */
}

void FFPlayer::audio_close()
{
    SDL_CloseAudio();  // SDL_CloseAudioDevice
}

long FFPlayer::ffp_get_duration_l()
{
    if(!ic) {
        return 0;
    }
    int64_t duration = fftime_to_milliseconds(ic->duration);
    if (duration < 0) {
        return 0;
    }
    return (long)duration;
}

// 当前播放的位置
long FFPlayer::ffp_get_current_position_l()
{
    if(!ic) {
        return 0;
    }
    int64_t start_time = ic->start_time;    // 起始时间 一般为0
    int64_t start_diff = 0;
    if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
        start_diff = fftime_to_milliseconds(start_time);    // 返回只需ms这个级别的
    }
    int64_t pos = 0;
    double pos_clock = get_master_clock();  // 获取当前时钟
    if (std::isnan(pos_clock)) {
        pos = fftime_to_milliseconds(seek_pos);
    } else {
        pos = pos_clock * 1000;     //转成msg
    }
    if (pos < 0 || pos < start_diff) {
        return 0;
    }
    int64_t adjust_pos = pos - start_diff;
    return (long)adjust_pos * pf_playback_rate; // 变速的系数
}

// 暂停的请求
int FFPlayer::ffp_pause_l()
{
    toggle_pause(1);
    return 0;
}

void FFPlayer::toggle_pause(int pause_on)
{
    toggle_pause_l(pause_on);
}
/// @brief 暂停1，0表示继续播放
/// @param pause_on 
void FFPlayer::toggle_pause_l(int pause_on)
{
    if (pause_req && !pause_on) {
        set_clock(&vidclk, get_clock(&vidclk), vidclk.serial);
        set_clock(&audclk, get_clock(&audclk), audclk.serial);
    }
    pause_req = pause_on;
    auto_resume = !pause_on;
    stream_update_pause_l();
    step = 0;
}

void FFPlayer::stream_update_pause_l()
{
    if (!step && (pause_req || buffering_on)) {
        stream_toggle_pause_l(1);
    } else {
        stream_toggle_pause_l(0);
    }
}

void FFPlayer::stream_toggle_pause_l(int pause_on)
{
    if (paused && !pause_on) { //继续播放
        //重新计算帧duration
        frame_timer += av_gettime_relative() / 1000000.0 - vidclk.last_updated;
        set_clock(&vidclk, get_clock(&vidclk), vidclk.serial);
        set_clock(&audclk, get_clock(&audclk), audclk.serial);
    } else {
    }
    //最后的决定暂停标志位
    if (step && (pause_req || buffering_on)) {
        paused = vidclk.paused = pause_on;
    } else {
        paused = audclk.paused = vidclk.paused =  pause_on;
        //        SDL_AoutPauseAudio(ffp->aout, pause_on);
    }
}

int FFPlayer::ffp_seek_to_l(long msec)
{
    int64_t start_time = 0;
    int64_t seek_pos = milliseconds_to_fftime(msec);  //ms
    int64_t duration = milliseconds_to_fftime(ffp_get_duration_l());
    if (duration > 0 && seek_pos >= duration) {
        msg_queue_.get()->notify_msg( FFP_MSG_SEEK_COMPLETE);        // 超出了范围
        return 0;
    }
    start_time =  ic->start_time;
    if (start_time > 0 && start_time != AV_NOPTS_VALUE) {
        seek_pos += start_time;
    }
    LOG(INFO) << "seek to:  " << seek_pos / 1000 ;
    stream_seek(seek_pos, 0, 0);
}

int FFPlayer::ffp_forward_to_l(long incr)
{
    ffp_forward_or_back_to_l(incr);
    return 0;
}

int FFPlayer::ffp_back_to_l(long incr)
{
    ffp_forward_or_back_to_l(incr);
    return 0;
}
/// @brief 
/// @param incr 快进10s
/// @return 
int FFPlayer::ffp_forward_or_back_to_l(long incr)
{
    double pos;
    if (seek_by_bytes) { //按字节seek
        pos = -1;
        
        // 方法1：以图像pos为基准
        if (pos < 0 &&  video_stream >= 0) {
            pos = frame_queue_last_pos(&pictq);
        }
        // 方法2：以音频pos为基准
        if (pos < 0 && audio_stream >= 0) {
            pos = frame_queue_last_pos(&sampq);
        }
        // 方法3：以文件pos为基准
        if (pos < 0) {
            pos = avio_tell(ic->pb);
        }
        //计算字节增量
        if (ic->bit_rate) {
            incr *= ic->bit_rate / 8.0;
        } else {
            incr *= 180000.0;
        }
        pos += incr;
        stream_seek(pos, incr, 1);
    } else {//按时间seek 最终也是以字节为单位的
        pos = get_master_clock();       // 单位是秒
        if (std::isnan(pos)) {
            pos = (double)seek_pos / AV_TIME_BASE;
        }
        pos += incr;   // 单位转成秒

        // 如果位置小于流的开始时间，调整到开始时间
        if (ic->start_time != AV_NOPTS_VALUE && pos < ic->start_time / (double)AV_TIME_BASE) {
            pos = ic->start_time / (double)AV_TIME_BASE;
        }
        //转成 AV_TIME_BASE
        stream_seek((int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE), 0);
    }
    return 0;
}
/// @brief 
/// @param pos 当前位置
/// @param rel 增量
/// @param seek_by_bytes 
void FFPlayer::stream_seek(int64_t pos, int64_t rel, int seek_by_bytes)
{
    if (!seek_req) {
        seek_pos = pos;
        seek_rel = rel;
        seek_flags &= ~AVSEEK_FLAG_BYTE;
        if (seek_by_bytes) {  //只实现了字节seek
            seek_flags |= AVSEEK_FLAG_BYTE;
        }
        seek_req = 1;
        //        SDL_CondSignal( continue_read_thread);
    }
}

int FFPlayer::ffp_screenshot_l(char *screen_path)
{
    // 存在视频的情况下才能截屏
    if(video_st && !req_screenshot_) {
        if(screen_path_) {
            free(screen_path_);
            screen_path_ = NULL;
        }
        screen_path_ = strdup(screen_path);
        req_screenshot_ = true;
    }
    return 0;
}

void FFPlayer::screenshot(AVFrame *frame)
{
    if(req_screenshot_) {
        ScreenShot shot;
        int ret = -1;
        if(frame) {
            ret = shot.SaveJpeg(frame, screen_path_, 70);
        }
        // 如果正常则ret = 0; 异常则为 < 0
        msg_queue_.get()->notify_msg(FFP_MSG_SCREENSHOT_COMPLETE, ret, 0, screen_path_, strlen(screen_path_) + 1);
        // 截屏完毕后允许再次截屏
        req_screenshot_ = false;
    }
}

int FFPlayer::get_target_frequency()
{
    return audio_tgt.freq;
}

int FFPlayer::get_target_channels()
{
    return audio_tgt.channels;
}

void FFPlayer::ffp_set_playback_rate(float rate)
{
    pf_playback_rate = rate;
    pf_playback_rate_changed = 1; //设置播放速率变化标志位
}

float FFPlayer::ffp_get_playback_rate()
{
    return pf_playback_rate;
}

bool FFPlayer::is_normal_playback_rate()
{
    if(pf_playback_rate > 0.99 && pf_playback_rate < 1.01) {
        return true;
    } else {
        return false;
    }
}

int FFPlayer::ffp_get_playback_rate_change()
{
    return pf_playback_rate_changed;
}

void FFPlayer::ffp_set_playback_rate_change(int change)
{
    pf_playback_rate_changed = change;
}
/**
 *  ffmpeg层设置音量
 * @param value
 */
void FFPlayer::ffp_set_playback_volume(int value)
{
    //最大128 最小0
    value = av_clip(value, 0, 100);
    value = av_clip(SDL_MIX_MAXVOLUME *  value / 100, 0, SDL_MIX_MAXVOLUME);
    audio_volume = value;
    LOG(INFO) << "audio_volume: " << audio_volume  ;
}

void FFPlayer::check_play_finish()
{
    //    LOG(INFO) << "eof: " << eof << ", audio_no_data: " << audio_no_data  ;
    if(eof == 1) { // 1. av_read_frame已经返回了AVERROR_EOF
        if(audio_stream >= 0 && video_stream >= 0) { // 2.1 音频、视频同时存在的场景
            if(audio_no_data == 1 && video_no_data == 1) {
                // 发送停止
                msg_queue_.get()->notify_msg(FFP_MSG_PLAY_FNISH);
            }
            return;
        }
        if(audio_stream >= 0) { // 2.2 只有音频存在
            if(audio_no_data == 1) {
                // 发送停止
                msg_queue_.get()->notify_msg(FFP_MSG_PLAY_FNISH);
            }
            return;
        }
        if(video_stream >= 0) { // 2.3 只有视频存在
            if(video_no_data == 1) {
                // 发送停止
                msg_queue_.get()->notify_msg(FFP_MSG_PLAY_FNISH);
            }
            return;
        }
    }
}
int64_t FFPlayer::ffp_get_property_int64(int id, int64_t default_value)
{
    switch (id) {
    case FFP_PROP_INT64_AUDIO_CACHED_DURATION:
        return  stat.audio_cache.duration;
    case FFP_PROP_INT64_VIDEO_CACHED_DURATION:
        return  stat.video_cache.duration;
    default:
        return default_value;
    }
}
void FFPlayer::ffp_track_statistic_l(AVStream * st, PacketQueue * q, FFTrackCacheStatistic * cache)
{
    if (q) {
        cache->bytes   = q->size;
        cache->packets = q->nb_packets;
    }
    if (q && st && st->time_base.den > 0 && st->time_base.num > 0) {
        cache->duration = q->duration * av_q2d(st->time_base) * 1000;  // 单位毫秒ms
    }
}
// 在audio_thread解码线程做统计
void FFPlayer::ffp_audio_statistic_l()
{
    ffp_track_statistic_l(audio_st, &audioq, &stat.audio_cache);
}
// 在audio_thread解码线程做统计
void FFPlayer::ffp_video_statistic_l()
{
    ffp_track_statistic_l(video_st, &videoq, &stat.video_cache);
}
int FFPlayer::stream_has_enough_packets(AVStream * st, int stream_id, PacketQueue * queue)
{
    return stream_id < 0 ||
            queue->abort_request ||
            (st->disposition & AV_DISPOSITION_ATTACHED_PIC) ||
            queue->nb_packets > MIN_FRAMES && (!queue->duration || av_q2d(st->time_base) * queue->duration > 1.0);
}


static int is_realtime(AVFormatContext * s)
{
    if(   !strcmp(s->iformat->name, "rtp")
          || !strcmp(s->iformat->name, "rtsp")
          || !strcmp(s->iformat->name, "sdp")
          ||  !strcmp(s->iformat->name, "rtmp")
          ) {
        return 1;
    }
    if(s->pb && (   !strncmp(s->filename, "rtp:", 4)
                    || !strncmp(s->filename, "udp:", 4)
                    )
            ) {
        return 1;
    }
    return 0;
}
///
/// 数据都由这里读取，主要功能是做解复用，从码流中分离音视频packet，并插入缓存队列
/// \return
///
int FFPlayer::read_thread()
{
    int err, i, ret;
    int st_index[AVMEDIA_TYPE_NB];      // AVMEDIA_TYPE_VIDEO/ AVMEDIA_TYPE_AUDIO 等，用来保存stream index
    AVPacket pkt1;
    AVPacket *pkt = &pkt1;  //
    // 初始化为-1,如果一直为-1说明没相应steam
    memset(st_index, -1, sizeof(st_index));
    video_stream = -1;
    audio_stream = -1;
    eof = 0;


    // 1. 创建上下文结构体，这个结构体是最上层的结构体，表示输入上下文
    ic = avformat_alloc_context();
    if (!ic) {
        av_log(NULL, AV_LOG_FATAL, "Could not allocate context.\n");
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    /* 2.设置中断回调函数，如果出错或者退出，就根据目前程序设置的状态选择继续check或者直接退出 */
    /* 当执行耗时操作时（一般是在执行while或者for循环的数据读取时），会调用interrupt_callback.callback
     * 回调函数中返回1则代表ffmpeg结束耗时操作退出当前函数的调用
     * 回调函数中返回0则代表ffmpeg内部继续执行耗时操作，直到完成既定的任务(比如读取到既定的数据包)
     */
    // ic->interrupt_callback.callback = decode_interrupt_cb;
    // ic->interrupt_callback.opaque = is;
    //特定选项处理
    // if (!av_dict_get(format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
    //     av_dict_set(&format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    //     scan_all_pmts_set = 1;
    // }
    /* 3.打开文件，主要是探测协议类型，如果是网络文件则创建网络链接等 */
    err = avformat_open_input(&ic, input_filename_, NULL, NULL);
    if (err < 0) {
        print_error(input_filename_, err);
        ret = -1;
        goto fail;
    }
    msg_queue_.get()->notify_msg(FFP_MSG_OPEN_INPUT);
    //LOG(INFO) << "read_thread FFP_MSG_OPEN_INPUT " << this ;
    if (seek_by_bytes < 0) {
        seek_by_bytes = !!(ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);
    }
    /*
     * 4.探测媒体类型，可得到当前文件的封装格式，音视频编码参数等信息
     * 调用该函数后得多的参数信息会比只调用avformat_open_input更为详细，
     * 其本质上是去做了decdoe packet获取信息的工作
     * codecpar, filled by libavformat on stream creation or
     * in avformat_find_stream_info()
     */
    err = avformat_find_stream_info(ic, NULL);
    if (err < 0) {
        av_log(NULL, AV_LOG_WARNING,
               "%s: could not find codec parameters\n", input_filename_);
        ret = -1;
        goto fail;
    }
    msg_queue_.get()->notify_msg( FFP_MSG_FIND_STREAM_INFO);
    LOG(INFO) << "read_thread FFP_MSG_FIND_STREAM_INFO " << this ;
    realtime = is_realtime(ic); //实时流
    av_dump_format(ic, 0, input_filename_, 0);
    // 6.2 利用av_find_best_stream选择流，
    st_index[AVMEDIA_TYPE_VIDEO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO,
                                st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);
    st_index[AVMEDIA_TYPE_AUDIO] =
            av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO,
                                st_index[AVMEDIA_TYPE_AUDIO],
                                st_index[AVMEDIA_TYPE_VIDEO],
                                NULL, 0);
    /* open the streams */
    /* 8. 打开视频、音频解码器。在此会打开相应解码器，并创建相应的解码线程。 */
    if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {// 如果有音频流则打开音频流
        stream_component_open(st_index[AVMEDIA_TYPE_AUDIO]);
    }
    ret = -1;
    if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) { // 如果有视频流则打开视频流
        ret = stream_component_open( st_index[AVMEDIA_TYPE_VIDEO]);
    }
    msg_queue_.get()->notify_msg(FFP_MSG_COMPONENT_OPEN);
    LOG(INFO) << "read_thread FFP_MSG_COMPONENT_OPEN " << this ;
    if (video_stream < 0 && audio_stream < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n",
               input_filename_);
        ret = -1;
        goto fail;
    }
    msg_queue_.get()->notify_msg(FFP_MSG_PREPARED);
    // LOG(INFO) << "read_thread FFP_MSG_PREPARED " << this ;

    /*
        * 二、For循环流程
     */
    while (1) {
        //        LOG(INFO) << "read_thread sleep, mp:" << this ;
        // 先模拟线程运行
        //        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if(abort_request) {
            break;
        }
        //  3 检测是否seek
        if (seek_req) {
            //计算 seek 的目标时间点  seek_min 和 seek_max 分别是 seek 操作的最小和最大时间点范围。 主要是为了修复由于四舍五入导致的精度问题
            int64_t seek_target = seek_pos;
            int64_t seek_min    = seek_rel > 0 ? seek_target - seek_rel + 2 : INT64_MIN;
            int64_t seek_max    =  seek_rel < 0 ? seek_target -  seek_rel - 2 : INT64_MAX;
            //执行 seek 操作
            ret = avformat_seek_file(ic, -1, seek_min, seek_target, seek_max,  seek_flags);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR,
                       "%s: error while seeking\n",  ic->filename);
            } else {
                //冲洗
                /* seek的时候，要把原先的数据情况，并重启解码器，put flush_pkt的目的是告知解码线程需要
                 * reset decoder
                 */
                if (audio_stream >= 0) {    //有audio流
                    packet_queue_flush(&audioq);
                    packet_queue_put(&audioq, &flush_pkt);
                }
                if (video_stream >= 0) { //有video流
                    packet_queue_flush(&videoq);
                    packet_queue_put(&videoq, &flush_pkt);
                }
            }
            //更新播放状态
            seek_req = 0;
            eof = 0;
            msg_queue_.get()->notify_msg(FFP_MSG_SEEK_COMPLETE);
        }
        /* if the queue are full, no need to read more */
        if (infinite_buffer < 1 &&
                (audioq.size + videoq.size  > MAX_QUEUE_SIZE
                 || (stream_has_enough_packets(audio_st, audio_stream, &audioq) &&
                     stream_has_enough_packets(video_st, video_stream, &videoq) ))) {
            /* wait 10 ms */
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }
        // 7.读取媒体数据，得到的是音视频分离后、解码前的数据
        ret = av_read_frame(ic, pkt); // 调用不会释放pkt的数据，需要我们自己去释放packet的数据
        if(ret < 0) { // 出错或者已经读取完毕了
            if ((ret == AVERROR_EOF || avio_feof(ic->pb)) && !eof) {        // 读取完毕了
                // 刷空包给队列
                if (video_stream >= 0) {
                    packet_queue_put_nullpacket(&videoq, video_stream);
                }
                if (audio_stream >= 0) {
                    packet_queue_put_nullpacket(&audioq, audio_stream);
                }
                eof = 1;
            }
            if (ic->pb && ic->pb->error) { // io异常 // 退出循环
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));     // 读取完数据了，这里可以使用timeout的方式休眠等待下一步的检测
            continue;		// 继续循环
        } else {
            eof = 0;
        }
        // 插入队列  先只处理音频包
        if (pkt->stream_index == audio_stream) {
            //            LOG(INFO) << "audio ===== pkt pts:" << pkt->pts << ", dts:" << pkt->dts;
            packet_queue_put(&audioq, pkt);
        } else if (pkt->stream_index == video_stream) {
            //            LOG(INFO) << "video ===== pkt pts:" << pkt->pts << ", dts:" << pkt->dts;
            packet_queue_put(&videoq, pkt);
        } else {
            av_packet_unref(pkt);// // 不入队列则直接释放数据
        }
    }
    LOG(INFO) << " leave";
fail:
    return 0;
}
/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01  // 每帧休眠10ms
int FFPlayer::video_refresh_thread()
{
    double remaining_time = 0.0;
    while (!abort_request) {
        if (remaining_time > 0.0) { //视频播放过快
            av_usleep((int)(int64_t)(remaining_time * 1000000.0));
        }
        remaining_time = REFRESH_RATE;
        if ( av_sync_type != AV_SYNC_UNKNOW_MASTER && // 显示模式不等于SHOW_MODE_NONE
             (!paused  // 非暂停状态
              || force_refresh) // 强制刷新状态
             )
            video_refresh(&remaining_time);
    }
    //LOG(INFO) <<  " leave" ;
    return 0;
}
///
/// 帧间隔
/// \param vp
/// \param nextvp
/// \return
///
double FFPlayer::vp_duration(  Frame * vp, Frame * nextvp)
{
    if (vp->serial == nextvp->serial) {
        double duration = nextvp->pts - vp->pts;
        if (std::isnan(duration) || duration <= 0 || duration >  max_frame_duration) {
            return vp->duration / pf_playback_rate;
        } else {
            return duration / pf_playback_rate;
        }
    } else {
        return 0.0;
    }
}
double FFPlayer::compute_target_delay(double delay)
{
    double sync_threshold, diff = 0;
    /* update delay to follow master synchronisation source */
    if (get_master_sync_type() != AV_SYNC_VIDEO_MASTER) {
        /* if video is slave, we try to correct big delays by
        duplicating or deleting a frame */
        diff = get_clock(&vidclk) - get_master_clock();
        /* skip or repeat frame. We take into account the
        delay to compute the threshold. I still don't know
        if it is the best guess */
        sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
        if (! std::isnan(diff) && fabs(diff) <  max_frame_duration) {
            if (diff <= -sync_threshold) {
                delay = FFMAX(0, delay + diff);
            } else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD) {
                delay = delay + diff;
            } else if (diff >= sync_threshold) {
                delay = 2 * delay;
            }
        }
    }
    av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n",
           delay, -diff);
    return delay;
}
void FFPlayer::update_video_pts(double pts, int64_t pos, int serial)
{
    /* update current video pts */
    set_clock(&vidclk, pts / pf_playback_rate, serial);
}
/*!
 * @brief   何时调用：非暂停或强制刷新的时候，循环调用video_refresh
 *          做了什么：显示一帧图像，重点是音视频同步规则。
 * lastvp vp nextvp 检查这三帧的合法性，目前播放lastvp 待播放vp
 * 检测1：lastvp和vp的播放时间是否符合当前时间，且满足当且队列与frame ser号一致
 * 检测2: vp与nextvp的合法性，合法注册显示否则drop帧。
 * 最后显示vp帧
 * @param         remaining_time
 * @attention
 */
void FFPlayer::video_refresh(double * remaining_time)
{
    Frame * vp = nullptr, *lastvp = nullptr;
    //目前我们先是只有队列里面有视频帧可以播放，就先播放出来
    //线上
    // if ( audio_st) {
    //     double time = av_gettime_relative() / 1000000.0;
    //     if (force_refresh || last_vis_time + rdftspeed < time) {
    //         goto display;
    //         last_vis_time = time;
    //     }
    //     *remaining_time = FFMIN(*remaining_time, last_vis_time + rdftspeed - time);
    // }

    // 判断有没有视频画面
    if (video_st) {
retry:
        if (frame_queue_nb_remaining(&pictq) == 0) {
            // nothing to do, no picture to display in the queue
            video_no_data = 1;  // 没有数据可读
            if(eof == 1) {
                check_play_finish();
            }
        } else {// 重点是音视频同步
            video_no_data = 0;  // 有数据可读
            double last_duration, duration, delay;
            // 从队列取出上一个Frame
            lastvp = frame_queue_peek_last(&pictq);
            screenshot(lastvp->frame); //截屏相关
            // 读取待显示的帧
            vp = frame_queue_peek(&pictq);
            // lastvp 上一帧(正在显示的帧)
            // vp 等待显示的帧
            if (vp->serial != videoq.serial) {// 如果不是最新的播放序列，则将其从队列，以尽快读取最新的播放序列的Frame
                frame_queue_next(&pictq);
                goto retry;
            }

            if (lastvp->serial != vp->serial) {
                // 新的播放序列重置当前时间
                frame_timer = av_gettime_relative() / 1000000.0;
            }
            if (paused) {
                goto display;
            }
            /* last_duration 计算上一帧应显示的时长 静态的 */
            last_duration = vp_duration(lastvp, vp);
            //             LOG(INFO) << "last_duration ......" << last_duration;
            /* 经过compute_target_delay方法，计算出待显示帧vp需要等待的时间 */
            // 如果以video同步，则delay直接等于last_duration。
            // 如果以audio或外部时钟同步，则需要比对主时钟调整待显示帧vp要等待的时间。
            delay = compute_target_delay(last_duration);
            //             LOG(INFO) << "delay ......" << delay;
            double time = av_gettime_relative() / 1000000.0;
            if (time <  frame_timer + delay) {//判断是否继续显示上一帧
                //                  LOG(INFO) << "(frame_timer + delay) - time " << frame_timer + delay - time;
                *remaining_time = FFMIN( frame_timer + delay - time, *remaining_time);
                goto display;
            }
            // 走到这一步，说明已经到了或过了该显示的时间，待显示帧vp的状态变更为当前要显示的帧

            frame_timer += delay;
            if (delay > 0 && time -  frame_timer > AV_SYNC_THRESHOLD_MAX) {
                frame_timer = time;
            }
            SDL_LockMutex(pictq.mutex);
            if (!std::isnan(vp->pts)) {
                update_video_pts(vp->pts, vp->pos, vp->serial);
            }
            SDL_UnlockMutex(pictq.mutex);
            //            LOG(INFO) << "debug " << __LINE__;

            //只有有nextvp才会分析是否有必要检测是否该丢帧
            if (frame_queue_nb_remaining(&pictq) > 1) {
                Frame *nextvp = frame_queue_peek_next(&pictq);
                duration = vp_duration(vp, nextvp);
                if (!step
                        && (framedrop > 0
                        || (framedrop && get_master_sync_type() != AV_SYNC_VIDEO_MASTER))
                        && time >  frame_timer + duration) {
                    frame_drops_late++;
                    //LOG(INFO) << "frame_drops_late  " << frame_drops_late;

                    frame_queue_next(&pictq);
                    goto retry;
                }
            }
            frame_queue_next(&pictq);
            force_refresh = 1;
            //            LOG(INFO) << "debug " << __LINE__;
            if (step && !paused)
                stream_toggle_pause(); // 逐帧的时候那继续进入暂停状态
        }
display:
        /* display picture */
        if (force_refresh &&  pictq.rindex_shown) {
            if(vp) {
                if(video_refresh_callback_) {
                    video_refresh_callback_(vp);
                }
            }
        }
    }
    force_refresh = 0;
}
/// @brief 暂停或恢复视频帧播放
void FFPlayer::stream_toggle_pause()
{
   //  if(paused) {
   //      /* 在恢复播放时，更新视频时钟（vidclk）的状态和时间。
   //           * 为什么要做这一步,因为默认是以音频为基准做同步，当暂停播放很长时间，视频时钟会增加
   //           * 虽然它做了动态frame_time 但是暂停时间很长，那动态frame_time值就很大 平均帧就很多
   //           * compute_target_delay 帧追赶调小，暂停长就能演示出bug，画面不动 音频在动。
   //   */
   //      frame_timer += av_gettime_relative() / 1000000.0 - vidclk.last_updated;

   //      set_clock(&vidclk, get_clock(&vidclk), vidclk.serial);
   //  }
   // // 切换 pause/resume 两种状态
   //  paused = audclk.paused = vidclk.paused = !paused;

}
void FFPlayer::AddVideoRefreshCallback(
        std::function<int (const Frame *)> callback)
{
    video_refresh_callback_ = callback;
}
int FFPlayer::get_master_sync_type()
{
    if (av_sync_type == AV_SYNC_VIDEO_MASTER) {
        if (video_st) {
            return AV_SYNC_VIDEO_MASTER;
        } else {
            return AV_SYNC_AUDIO_MASTER;    /* 如果没有视频成分则使用 audio master */
        }
    } else if (av_sync_type == AV_SYNC_AUDIO_MASTER) {
        if (audio_st) {
            return AV_SYNC_AUDIO_MASTER;
        } else if(video_st) {
            return AV_SYNC_VIDEO_MASTER;    // 只有音频的存在
        } else {
            return AV_SYNC_UNKNOW_MASTER;
        }
    }
}
double FFPlayer::get_master_clock()
{
    double val;
    switch (get_master_sync_type()) {
    case AV_SYNC_VIDEO_MASTER:
        val = get_clock(&vidclk);
        break;
    case AV_SYNC_AUDIO_MASTER:
        val = get_clock(&audclk);
        break;
    default:
        val = get_clock(&audclk);  // 这里我们不支持以外部时钟为基准的方式
        break;
    }
    return val;
}
Decoder::Decoder()
{
    av_init_packet(&pkt_);
}
Decoder::~Decoder()
{
}
void Decoder::decoder_init(AVCodecContext * avctx, PacketQueue * queue,void* is_)
{
    avctx_ = avctx;
    queue_ = queue;
    is=(FFPlayer*)is_;
}
int Decoder::decoder_start(AVMediaType codec_type, const char *thread_name, void *arg)
{
    // 启用包队列
    packet_queue_start(queue_);
    // 创建线程
    if(AVMEDIA_TYPE_VIDEO == codec_type) {
        decoder_thread_ = new std::thread(&Decoder::video_thread, this, arg);
    } else if (AVMEDIA_TYPE_AUDIO == codec_type) {
        decoder_thread_ = new std::thread(&Decoder::audio_thread, this, arg);
    } else {
        return -1;
    }
    return 0;
}
void Decoder::decoder_abort(FrameQueue * fq)
{
    packet_queue_abort(queue_);     // 请求退出包队列
    frame_queue_signal(fq);     // 唤醒阻塞的帧队列
    if(decoder_thread_ && decoder_thread_->joinable()) {
        decoder_thread_->join(); // 等待解码线程退出
        delete decoder_thread_;
        decoder_thread_ = NULL;
    }
    packet_queue_flush(queue_);  // 情况packet队列，并释放数据
}
void Decoder::decoder_destroy()
{
    av_packet_unref(&pkt_);
    avcodec_free_context(&avctx_);
}
// 音视频解码   接收（清空队列）->过滤（drop帧）->发送（发送给解码器）
// 返回值-1: 请求退出
//       0: 解码已经结束了，不再有数据可以读取
//       1: 获取到解码后的frame
int Decoder::decoder_decode_frame(AVFrame * frame)
{
    int ret = AVERROR(EAGAIN);
    for (;;) {
        AVPacket pkt;
        // 1. 流连续情况下获取解码后的帧
        if (queue_->serial == pkt_serial_) { // 1.1 先判断是否是同一播放序列的数据
            do {
                if (queue_->abort_request) {
                    return -1;    // 是否请求退出
                }
                // 1.2. 获取解码帧
                switch (avctx_->codec_type) {
                case AVMEDIA_TYPE_VIDEO:
                    ret = avcodec_receive_frame(avctx_, frame);
                    if (ret >= 0) {
                        if (decoder_reorder_pts == -1) {
                            frame->pts = frame->best_effort_timestamp;
                        } else if (!decoder_reorder_pts) {
                            frame->pts = frame->pkt_dts;
                        }
                        //                        LOG(INFO) << "video frame pts:" <<  frame->pts << ", dts:" << frame->pkt_dts;
                    }
                    break;
                case AVMEDIA_TYPE_AUDIO:
                    ret = avcodec_receive_frame(avctx_, frame);
                    if (ret >= 0) {
                        //                         LOG(INFO) << "audio frame pts:" <<  frame->pts << ", dts:" << frame->pkt_dts;
                        AVRational tb = {1, frame->sample_rate};    //
                        if (frame->pts != AV_NOPTS_VALUE) {
                            // 如果frame->pts正常则先将其从pkt_timebase转成{1, frame->sample_rate}
                            // pkt_timebase实质就是stream->time_base
                            frame->pts = av_rescale_q(frame->pts, avctx_->pkt_timebase, tb);
                        } else if (next_pts != AV_NOPTS_VALUE) {
                            // 如果frame->pts不正常则使用上一帧更新的next_pts和next_pts_tb
                            // 转成{1, frame->sample_rate}
                            frame->pts = av_rescale_q(next_pts, next_pts_tb, tb);
                        }
                        if (frame->pts != AV_NOPTS_VALUE) {
                            // 根据当前帧的pts和nb_samples预估下一帧的pts
                            next_pts = frame->pts + frame->nb_samples;
                            next_pts_tb = tb; // 设置timebase
                        }
                    }
                    break;
                }
                // 1.3. 检查解码是否已经结束，解码结束返回0
                if (ret == AVERROR_EOF) {
                    finished_ = pkt_serial_;
                    LOG(INFO) << "avcodec_flush_buffers pkt_serial:" << pkt_serial_;
                    avcodec_flush_buffers(avctx_);
                    return 0;
                }
                // 1.4. 正常解码返回1
                if (ret >= 0) {
                    return 1;
                }
            } while (ret != AVERROR(EAGAIN));   // 1.5 没帧可读时ret返回EAGIN，需要继续送packet
        }
        // 2 获取一个packet，如果播放序列不一致(数据不连续)则过滤掉“过时”的packet
        do {
            // 2.1 如果没有数据可读则唤醒read_thread, 实际是continue_read_thread SDL_cond
            //            if (queue_->nb_packets == 0)  // 没有数据可读
            //                SDL_CondSignal(empty_queue_cond);// 通知read_thread放入packet
            // 2.2 如果还有pending的packet则使用它
            if (packet_pending_) {
                av_packet_move_ref(&pkt, &pkt_);
                packet_pending_ = 0;
            } else {
                // 2.3 阻塞式读取packet
                if (packet_queue_get(queue_, &pkt, 1, &pkt_serial_) < 0) {
                    return -1;
                }
            }
            if(queue_->serial != pkt_serial_) {
                // darren自己的代码
                LOG(INFO) << "discontinue:queue->serial:" << queue_->serial << ", pkt_serial:" << pkt_serial_;
                av_packet_unref(&pkt); // fixed me? 释放要过滤的packet
            }
        } while (queue_->serial != pkt_serial_);// 如果不是同一播放序列(流不连续)则继续读取
        // 3 将packet送入解码器
        if (pkt.data == flush_pkt.data) {//
            // when seeking or when switching to a different stream
            avcodec_flush_buffers(avctx_); //清空里面的缓存帧
            finished_ = 0;        // 重置为0
            next_pts = start_pts;     // 主要用在了audio
            next_pts_tb = start_pts_tb;// 主要用在了audio
        } else {
            if (avctx_->codec_type == AVMEDIA_TYPE_SUBTITLE) {
                //                int got_frame = 0;
                //                ret = avcodec_decode_subtitle2(avctx_, sub, &got_frame, &pkt);
                //                if (ret < 0) {
                //                    ret = AVERROR(EAGAIN);
                //                } else {
                //                    if (got_frame && !pkt.data) {
                //                        packet_pending = 1;
                //                        av_packet_move_ref(&pkt, &pkt);
                //                    }
                //                    ret = got_frame ? 0 : (pkt.data ? AVERROR(EAGAIN) : AVERROR_EOF);
                //                }
            } else {
                if (avcodec_send_packet(avctx_, &pkt) == AVERROR(EAGAIN)) {
                    //                    av_log(avctx, AV_LOG_ERROR, "Receive_frame and send_packet both returned EAGAIN, which is an API violation.\n");
                    packet_pending_ = 1;
                    av_packet_move_ref(&pkt_, &pkt);
                }
            }
            av_packet_unref(&pkt);	// 一定要自己去释放音视频数据
        }
    }
}
int Decoder::get_video_frame(AVFrame * frame)
{
    int got_picture;

    // 1. 获取解码后的视频帧
    if ((got_picture = decoder_decode_frame(frame)) < 0) {
        return -1; // 返回-1意味着要退出解码线程, 所以要分析decoder_decode_frame什么情况下返回-1
    }
    /*-framedrop  当视频解码的速度跟不上实时播放的需求时,就会出现帧率下降的问题。为了保证视频播放的流畅性,可以采取丢弃一些帧的策略,即 frame dropping。
       default：使用默认的帧丢弃策略。
       all：始终丢弃所有不必要的帧。
       nokey：只丢弃非关键帧(non-keyframe)。
       vbf：只丢弃使用 B 帧预测的帧。
       aggressive：更积极地丢弃帧,以尽量保证播放的流畅性。

    */
    //2. 分析获取到的该帧是否要drop掉, 该机制的目的是在放入帧队列前先drop掉过时的视频帧
    if (got_picture) {

        // double dpts = NAN;

        // if (frame->pts != AV_NOPTS_VALUE)
        //     dpts = av_q2d(is->video_st->time_base) * frame->pts;    //计算出秒为单位的dpts
        // //更新视频帧的样本长宽比
        // frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(is->ic, is->video_st, frame);

        // if (is->av_sync_type != AV_SYNC_VIDEO_MASTER)//非视频同步模式
        // {
        //     //检查该帧的 PTS 值是否有效
        //     if (frame->pts != AV_NOPTS_VALUE) { // pts值有效
        //         double diff = dpts - is->get_master_clock();
        //         if (!isnan(diff) &&     // 差值有效
        //                 fabs(diff) < AV_NOSYNC_THRESHOLD && // 差值在可同步范围呢
        //                 is->viddec.pkt_serial_ == is->vidclk.serial && // 同一序列的包
        //                 is->videoq.nb_packets) { // 帧队列至少有1帧数据丢弃
        //             is->frame_drops_early++;
        //             LOG(ERROR)<<__FUNCTION__<<"pts"<<frame->pts
        //                      <<"diff:"<<diff
        //                     <<" drops:"<<is->frame_drops_early;

        //             av_frame_unref(frame);
        //             got_picture = 0; //表示该帧已丢弃。

        //         }
        //     }
        // }
    }
    return got_picture;
}
int Decoder::queue_picture(FrameQueue * fq, AVFrame * src_frame, double pts, double duration, int64_t pos, int serial)
{
    Frame *vp;
    if (!(vp = frame_queue_peek_writable(fq))) { // 检测队列是否有可写空间
        return -1;    // 请求退出则返回-1
    }
    // 执行到这步说已经获取到了可写入的Frame
    //    vp->sar = src_frame->sample_aspect_ratio;
    //    vp->uploaded = 0;
    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;
    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;
    vp->serial = serial;    // 设置serial
    av_frame_move_ref(vp->frame, src_frame); // 将src中所有数据转移到dst中，并复位src。
    frame_queue_push(fq);   // 更新写索引位置
    return 0;
}
int Decoder::audio_thread(void *arg)
{
    LOG(INFO) <<   " into " ;
    FFPlayer *is = (FFPlayer *)arg;
    AVFrame *frame = av_frame_alloc();  // 分配解码帧
    Frame *af;
    int got_frame = 0;  // 是否读取到帧
    AVRational tb;      // timebase
    int ret = 0;
    if (!frame) {
        return AVERROR(ENOMEM);
    }
    do {
        // 获取缓存情况
        is->ffp_audio_statistic_l();
        // 1. 读取解码帧
        if ((got_frame = decoder_decode_frame(frame)) < 0) { // 是否获取到一帧数据
            goto the_end;    // < =0 abort
        }
        //        LOG(INFO) << avctx_->codec->name << " packet size: " << queue_->size << " frame size: " << pictq.size << ", pts: " << frame->pts ;
        if (got_frame) {
            tb = AVRational {
                    1, frame->sample_rate
        };   // 设置为sample_rate为timebase
            // 2. 获取可写Frame
            if (!(af = frame_queue_peek_writable(&is->sampq))) { // 获取可写帧
                goto the_end;
            }
            // 3. 设置Frame并放入FrameQueue
            af->pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);  // 转换时间戳
            af->pos = frame->pkt_pos;
            af->serial = is->auddec.pkt_serial_;
            af->duration = av_q2d(AVRational {
                                      frame->nb_samples, frame->sample_rate
                                  });
            av_frame_move_ref(af->frame, frame);
            frame_queue_push(&is->sampq);  // 代表队列真正插入一帧数据
        }
    } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);
the_end:
    LOG(INFO) <<   " leave " ;
    av_frame_free(&frame);
    return ret;
}
int Decoder::video_thread(void *arg)
{
    LOG(INFO) <<   " into " ;
    FFPlayer *is = (FFPlayer *)arg;
    AVFrame *frame = av_frame_alloc();  // 分配解码帧
    double pts;                 // pts
    double duration;            // 帧持续时间
    int ret;
    //1 获取stream timebase
    AVRational tb = is->video_st->time_base; // 获取stream timebase
    //2 获取帧率，以便计算每帧picture的duration
    AVRational frame_rate = av_guess_frame_rate(is->ic, is->video_st, NULL);
    if (!frame) {
        return AVERROR(ENOMEM);
    }
    for (;;) {  // 循环取出视频解码的帧数据
        is->ffp_video_statistic_l();// 统计视频packet缓存
        // 3 获取解码后的视频帧
        ret = get_video_frame(frame);
        if (ret < 0) {
            goto the_end;    //解码结束, 什么时候会结束
        }
        if (!ret) {         //没有解码得到画面, 什么情况下会得不到解后的帧
            continue;
        }
        //        LOG(INFO) << avctx_->codec->name << " packet size: " << queue_->size << " frame size: " << pictq.size << ", pts: " << frame->pts ;
        //           1/25 = 0.04秒
        // 4 计算帧持续时间和换算pts值为秒
        // 1/帧率 = duration 单位秒, 没有帧率时则设置为0, 有帧率帧计算出帧间隔
        duration = (frame_rate.num && frame_rate.den ? av_q2d(AVRational {
                                                                  frame_rate.den, frame_rate.num
                                                              }) : 0);
        // 根据AVStream timebase计算出pts值, 单位为秒
        pts = (frame->pts == AV_NOPTS_VALUE) ? NAN : frame->pts * av_q2d(tb);  // 单位为秒
        // 5 将解码后的视频帧插入队列
        ret = queue_picture(&is->pictq, frame, pts, duration, frame->pkt_pos, is->viddec.pkt_serial_);
        // 6 释放frame对应的数据
        av_frame_unref(frame);
        if (ret < 0) { // 返回值小于0则退出线程
            goto the_end;
        }
    }
the_end:
    LOG(INFO) <<   " leave " ;
    av_frame_free(&frame);
    return 0;
}
