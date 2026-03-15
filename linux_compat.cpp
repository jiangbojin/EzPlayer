#ifndef _WIN32
extern "C" {
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>

int av_frame_get_channels(const AVFrame *frame) {
    return frame->ch_layout.nb_channels;
}

int avcodec_encode_video2(AVCodecContext *avctx, AVPacket *avpkt,
                          const AVFrame *frame, int *got_packet_ptr) {
    int ret = avcodec_send_frame(avctx, frame);
    if (ret < 0 && ret != AVERROR_EOF)  {
        *got_packet_ptr = 0;
        return ret;
    }
    ret = avcodec_receive_packet(avctx, avpkt);
    if (ret == 0) {
        *got_packet_ptr = 1;
    } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        *got_packet_ptr = 0;
        ret = 0;
    } else {
        *got_packet_ptr = 0;
    }
    return ret;
}

}
#endif
