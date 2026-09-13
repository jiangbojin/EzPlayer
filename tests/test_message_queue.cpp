#include <gtest/gtest.h>
#include <chrono>
#include <thread>
#include "core/messagequeue.h"
#include "utils/log/logger.h"

TEST(MessageQueueTest, BasicPutAndGet) {
    MessageQueue queue;
    queue.msg_queue_start();

    // 投递普通数值消息
    queue.notify_msg(101, 42, 84);

    AVMessage msg;
    int ret = queue.msg_queue_get(&msg, 100);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(msg.what, 101);
    EXPECT_EQ(msg.arg1, 42);
    EXPECT_EQ(msg.arg2, 84);
    EXPECT_EQ(msg.obj, nullptr);
}

TEST(MessageQueueTest, ObjectPayload) {
    MessageQueue queue;
    queue.msg_queue_start();

    const char payload[] = "EzPlayer_Agent_Payload";
    int payload_len      = sizeof(payload);

    queue.notify_msg(202, 1, 2, (void*)payload, payload_len);

    AVMessage msg;
    int ret = queue.msg_queue_get(&msg, 100);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(msg.what, 202);
    ASSERT_NE(msg.obj, nullptr);
    EXPECT_STREQ((char*)msg.obj, payload);

    if (msg.free_l && msg.obj) {
        msg.free_l(msg.obj);
    }
}

TEST(MessageQueueTest, TimeoutBehavior) {
    MessageQueue queue;
    queue.msg_queue_start();

    AVMessage msg;
    // 队列为空，超时 50ms 后应返回 0
    auto start    = std::chrono::steady_clock::now();
    int ret       = queue.msg_queue_get(&msg, 50);
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now() - start)
                        .count();

    EXPECT_EQ(ret, 0);
    EXPECT_GE(duration, 40);
}

TEST(MessageQueueTest, AbortInterruptsWait) {
    MessageQueue queue;
    queue.msg_queue_start();

    std::thread worker([&queue]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        queue.msg_queue_abort();
    });

    AVMessage msg;
    int ret = queue.msg_queue_get(&msg, 1000);
    // abort 触发时应返回 -1
    EXPECT_EQ(ret, -1);

    worker.join();
}

TEST(MessageQueueTest, RemoveSpecificMessage) {
    MessageQueue queue;
    queue.msg_queue_start();

    queue.notify_msg(301, 1);
    queue.notify_msg(302, 2);
    queue.notify_msg(301, 3);

    // 移除所有 what == 301 的消息
    int removed = queue.msg_queue_remove(301);
    EXPECT_EQ(removed, 2);

    AVMessage msg;
    int ret = queue.msg_queue_get(&msg, 50);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(msg.what, 302);
}
