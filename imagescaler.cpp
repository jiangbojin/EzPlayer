#include"imagescaler.h"
ImageScaler::ImageScaler() {
    sws_ctx_ = NULL;
    src_pix_fmt_ = AV_PIX_FMT_NONE;
    dst_pix_fmt_ = AV_PIX_FMT_NONE;
    en_alogrithm_ = SWS_SA_FAST_BILINEAR;

    src_width_ = src_height_ = 0;
    dst_width_ = dst_height_ = 0;
}

ImageScaler::~ImageScaler() {
    DeInit();
}

RET_CODE ImageScaler::Init(uint32_t src_width, uint32_t src_height, int src_pix_fmt, uint32_t dst_width, uint32_t dst_height, int dst_pix_fmt, int en_alogrithm) {
    src_width_ = src_width;
    src_height_ = src_height;
    src_pix_fmt_ = (AVPixelFormat)src_pix_fmt;
    dst_width_ = dst_width;
    dst_height_ = dst_height;
    dst_pix_fmt_ = (AVPixelFormat)dst_pix_fmt;
    en_alogrithm_ = en_alogrithm;
    sws_ctx_ = sws_getContext(
                src_width_,
                src_height_,
                (AVPixelFormat)src_pix_fmt_,
                dst_width_,
                dst_height_,
                (AVPixelFormat)dst_pix_fmt_,
                SWS_FAST_BILINEAR,
                NULL,
                NULL,
                NULL);
    if (!sws_ctx_) {
        LOG(ERROR) << "Impossible to create scale context for the conversion fmt:"
                   << av_get_pix_fmt_name((enum AVPixelFormat)src_pix_fmt_)
                   << ", s:" <<  src_width_ << "x" << src_height_ << " -> fmt:" << av_get_pix_fmt_name(dst_pix_fmt_)
                   << ", s:" << dst_width_ << "x" << dst_height_;
        return RET_FAIL;
    }
    return RET_OK;
}

RET_CODE ImageScaler::Scale(const AVFrame *src_frame, AVFrame *dst_frame) {
    if(src_frame->width != src_width_
            || src_frame->height != src_height_
            || src_frame->format != src_pix_fmt_
            || dst_frame->width != dst_width_
            || dst_frame->height != dst_height_
            || dst_frame->format != dst_pix_fmt_
            || !sws_ctx_) {
        // 重新初始化
        DeInit();
        RET_CODE ret = Init(src_frame->width, src_frame->height, src_frame->format,
                            dst_frame->width, dst_frame->height, dst_frame->format,
                            en_alogrithm_);
        if(ret != RET_OK) {
            LOG(ERROR) << "Init failed: " << ret;
            return ret;
        }
    }

    int dst_slice_h = sws_scale(sws_ctx_, (const uint8_t **) src_frame->data, src_frame->linesize, 0, src_frame->height,
                                dst_frame->data, dst_frame->linesize);
    if(dst_slice_h>0)
        return RET_OK;
    else
        return RET_FAIL;
}

RET_CODE ImageScaler::Scale2(const VideoFrame *src_frame, VideoFrame *dst_frame) {
    if(src_frame->width != src_width_
            || src_frame->height != src_height_
            || src_frame->format != src_pix_fmt_
            || dst_frame->width != dst_width_
            || dst_frame->height != dst_height_
            || dst_frame->format != dst_pix_fmt_
            || !sws_ctx_) {
        DeInit();
        RET_CODE ret = Init(src_frame->width, src_frame->height, src_frame->format,
                            dst_frame->width, dst_frame->height, dst_frame->format,
                            en_alogrithm_);
        if(ret != RET_OK) {
            LOG(ERROR) << "Init failed: " << ret;
            return ret;
        }
    }
    int dst_slice_h = sws_scale(sws_ctx_,
                                (const uint8_t **)src_frame->data,
                                src_frame->linesize,
                                0,  // 起始位置
                                src_frame->height, //处理多少行
                                dst_frame->data,
                                dst_frame->linesize);
    if(dst_slice_h>0)
        return RET_OK;
    else
        return RET_FAIL;
}


RET_CODE ImageScaler::Scale3(const Frame *src_frame, VideoFrame *dst_frame) {
    LOG(INFO)<<"start";
    // 检查输入和输出是否发生改变,如果有变化则需要重新初始化
    if(src_frame->width != src_width_
            || src_frame->height != src_height_
            || src_frame->format != src_pix_fmt_
            || dst_frame->width != dst_width_
            || dst_frame->height != dst_height_
            || dst_frame->format != dst_pix_fmt_
            || !sws_ctx_) {
        // 如果有变化,先执行反初始化操作
        DeInit();

        // 重新初始化 ImageScaler
        RET_CODE ret = Init(src_frame->width, src_frame->height, src_frame->format,
                            dst_frame->width, dst_frame->height, dst_frame->format,
                            en_alogrithm_);
        if(ret != RET_OK) {
            LOG(ERROR) << "Init failed: " << ret;
            return ret;
        }
    }

    // 使用 sws_scale() 函数执行图像缩放
    int dst_slice_h = sws_scale(sws_ctx_,
                                (const uint8_t **)src_frame->frame->data,
                                src_frame->frame->linesize,
                                0,  // 起始位置
                                src_frame->height, //处理多少行
                                dst_frame->data,
                                dst_frame->linesize);
    LOG(INFO)<<"end";
    // 检查是否缩放成功
    if(dst_slice_h>0)
        return RET_OK;
    else
        return RET_FAIL;


}
