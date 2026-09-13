#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "ff_ffplay_def.h"

class SyncQueuesTest : public ::testing::Test {
   protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(SyncQueuesTest, PacketQueueLifecycle) {
    PacketQueue q;
    int ret = packet_queue_init(&q);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(q.mutex, nullptr);
    EXPECT_NE(q.cond, nullptr);
    EXPECT_EQ(q.nb_packets, 0);
    EXPECT_EQ(q.abort_request, 1);

    packet_queue_start(&q);
    EXPECT_EQ(q.abort_request, 0);
    // start 放入了一个 flush_pkt
    EXPECT_EQ(q.nb_packets, 1);

    // 测试放入自定义包
    AVPacket pkt;
    av_init_packet(&pkt);
    pkt.data = static_cast<uint8_t*>(av_malloc(64));
    pkt.size = 64;
    pkt.pts  = 1000;
    pkt.dts  = 1000;

    ret = packet_queue_put(&q, &pkt);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(q.nb_packets, 2);

    // 取出 flush_pkt
    AVPacket out_pkt;
    int serial = -1;
    ret        = packet_queue_get(&q, &out_pkt, 1, &serial);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(out_pkt.data, flush_pkt.data);
    av_packet_unref(&out_pkt);

    // 取出第2个包
    ret = packet_queue_get(&q, &out_pkt, 1, &serial);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(out_pkt.size, 64);
    av_packet_unref(&out_pkt);

    // 此时队列为空，非阻塞 get 返回 0
    ret = packet_queue_get(&q, &out_pkt, 0, &serial);
    EXPECT_EQ(ret, 0);

    packet_queue_destroy(&q);
    EXPECT_EQ(q.mutex, nullptr);
    EXPECT_EQ(q.cond, nullptr);
}

TEST_F(SyncQueuesTest, PacketQueueAbortWakesBlocker) {
    PacketQueue q;
    packet_queue_init(&q);
    packet_queue_start(&q);

    // 取出初始 flush_pkt
    AVPacket out_pkt;
    int serial = -1;
    packet_queue_get(&q, &out_pkt, 1, &serial);
    av_packet_unref(&out_pkt);

    std::atomic<bool> thread_started{false};
    std::atomic<int> get_result{0};

    std::thread reader([&]() {
        thread_started = true;
        AVPacket p;
        get_result = packet_queue_get(&q, &p, 1, nullptr);
    });

    while (!thread_started) {
        std::this_thread::yield();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    packet_queue_abort(&q);
    reader.join();

    EXPECT_EQ(get_result.load(), -1);
    packet_queue_destroy(&q);
}

TEST_F(SyncQueuesTest, FrameQueueLifecycle) {
    PacketQueue pktq;
    packet_queue_init(&pktq);
    packet_queue_start(&pktq);

    FrameQueue fq;
    int ret = frame_queue_init(&fq, &pktq, 4, 1);
    EXPECT_EQ(ret, 0);
    EXPECT_NE(fq.mutex, nullptr);
    EXPECT_NE(fq.cond, nullptr);
    EXPECT_EQ(fq.max_size, 4);
    EXPECT_EQ(frame_queue_nb_remaining(&fq), 0);

    // 获取可写指针并推送
    Frame* vp = frame_queue_peek_writable(&fq);
    EXPECT_NE(vp, nullptr);
    vp->pts = 1.234;
    frame_queue_push(&fq);

    EXPECT_EQ(frame_queue_nb_remaining(&fq), 1);

    // 读取帧
    Frame* read_vp = frame_queue_peek_readable(&fq);
    EXPECT_NE(read_vp, nullptr);
    EXPECT_DOUBLE_EQ(read_vp->pts, 1.234);

    frame_queue_next(&fq);
    // keep_last = 1 时第一次 next 会将 rindex_shown 置为 1
    EXPECT_EQ(fq.rindex_shown, 1);

    frame_queue_destory(&fq);
    EXPECT_EQ(fq.mutex, nullptr);
    EXPECT_EQ(fq.cond, nullptr);

    packet_queue_destroy(&pktq);
}
