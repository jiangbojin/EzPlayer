#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>

int av_frame_get_channels(const AVFrame *frame) {
#if LIBAVUTIL_VERSION_MAJOR >= 58
    return frame->ch_layout.nb_channels;
#else
    return frame->channels;
#endif
}

int avcodec_encode_video2(AVCodecContext *avctx, AVPacket *avpkt,
                          const AVFrame *frame, int *got_packet_ptr) {
    int ret;
    *got_packet_ptr = 0;

    ret = avcodec_send_frame(avctx, frame);
    if (ret < 0 && ret != AVERROR_EOF)
        return ret;

    ret = avcodec_receive_packet(avctx, avpkt);
    if (ret == 0) {
        *got_packet_ptr = 1;
    } else if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
        ret = 0;
    }
    return ret;
}
