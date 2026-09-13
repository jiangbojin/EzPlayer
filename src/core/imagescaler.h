#ifndef IMAGESCALER_H
#define IMAGESCALER_H
#include "easylogging++.h"
#include "ijkmediaplayer.h"
#include "mediabase.h"

/// @brief 用于图像缩放的工具
class ImageScaler {
   public:
    ImageScaler();
    ~ImageScaler();
    /// @brief 设置源和目标的图像参数,并创建一个 SwsContext 对象
    /// @param src_width
    /// @param src_height
    /// @param src_pix_fmt
    /// @param dst_width
    /// @param dst_height
    /// @param dst_pix_fmt
    /// @param en_alogrithm
    /// @return
    RET_CODE Init(uint32_t src_width, uint32_t src_height, int src_pix_fmt, uint32_t dst_width,
                  uint32_t dst_height, int dst_pix_fmt,
                  SwsAlogrithm en_alogrithm = SwsAlogrithm::SWS_SA_FAST_BILINEAR);
    void DeInit() {
        if (sws_ctx_) {
            sws_freeContext(sws_ctx_);
            sws_ctx_ = NULL;
        }
    }

    //图像缩放
    RET_CODE Scale(const AVFrame* src_frame, AVFrame* dst_frame);
    RET_CODE Scale2(const VideoFrame* src_frame, VideoFrame* dst_frame);
    RET_CODE Scale3(const Frame* src_frame, VideoFrame* dst_frame);

   private:
    SwsContext* sws_ctx_;                                     //SWS对象
    AVPixelFormat src_pix_fmt_;                               //源像素格式
    AVPixelFormat dst_pix_fmt_;                               //目标像素格式
    SwsAlogrithm en_alogrithm_ = SwsAlogrithm::SWS_SA_POINT;  //Resize算法

    int src_width_, src_height_;  //源图像宽高

    int dst_width_, dst_height_;  //目标图像宽高
};

#endif  // IMAGESCALER_H
