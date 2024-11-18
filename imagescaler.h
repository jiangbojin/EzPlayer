#ifndef IMAGESCALER_H
#define IMAGESCALER_H
#include "ijkmediaplayer.h"

#include "easylogging++.h"
//Scale算法
enum SwsAlogrithm
{
    SWS_SA_FAST_BILINEAR    = 0x1,
    SWS_SA_BILINEAR            = 0x2,
    SWS_SA_BICUBIC            = 0x4,
    SWS_SA_X                = 0x8,
    SWS_SA_POINT            = 0x10,
    SWS_SA_AREA                = 0x20,
    SWS_SA_BICUBLIN            = 0x40,
    SWS_SA_GAUSS            = 0x80,
    SWS_SA_SINC                = 0x100,
    SWS_SA_LANCZOS            = 0x200,
    SWS_SA_SPLINE            = 0x400,
};

typedef struct VideoFrame
{
    uint8_t *data[8] = {NULL};         // 类似FFmpeg的buf, 如果是
    int32_t linesize[8] = {0};
    int32_t width;
    int32_t height;
    int format = AV_PIX_FMT_YUV420P;
}VideoFrame;
/// @brief 用于图像缩放的工具
class ImageScaler
{
public:
    ImageScaler();
    ~ImageScaler() ;
    /// @brief 设置源和目标的图像参数,并创建一个 SwsContext 对象
    /// @param src_width 
    /// @param src_height 
    /// @param src_pix_fmt 
    /// @param dst_width 
    /// @param dst_height 
    /// @param dst_pix_fmt 
    /// @param en_alogrithm 
    /// @return 
    RET_CODE Init(uint32_t src_width, uint32_t src_height, int src_pix_fmt,
                  uint32_t dst_width, uint32_t dst_height, int dst_pix_fmt,
                  int en_alogrithm = SWS_SA_FAST_BILINEAR);
    void DeInit( ) {
        if(sws_ctx_) {
            sws_freeContext(sws_ctx_);
            sws_ctx_ = NULL;
        }
    }

    //图像缩放
    RET_CODE Scale(const AVFrame *src_frame, AVFrame *dst_frame);
    RET_CODE Scale2(const VideoFrame *src_frame, VideoFrame *dst_frame) ;
    RET_CODE Scale3(const Frame *src_frame, VideoFrame *dst_frame);
private:

    SwsContext*	sws_ctx_;		//SWS对象
    AVPixelFormat src_pix_fmt_;			//源像素格式
    AVPixelFormat dst_pix_fmt_;			//目标像素格式
    int en_alogrithm_ = SWS_SA_POINT;		//Resize算法

    int src_width_, src_height_;			//源图像宽高

    int dst_width_, dst_height_;			//目标图像宽高

};

#endif // IMAGESCALER_H
