#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include "core/commonlooper.h"

class DummyLooper : public CommonLooper {
   public:
    std::atomic<int> loop_count{0};

    virtual void Loop() override {
        while (!request_exit_) {
            loop_count++;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
};

TEST(CommonLooperTest, StartAndStopLifecycle) {
    DummyLooper looper;
    EXPECT_FALSE(looper.getRunning());

    RET_CODE ret = looper.Start();
    EXPECT_TRUE(looper.getRunning());

    // 运行一段时间，确认 Loop 在独立线程正常执行
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    EXPECT_GT(looper.loop_count.load(), 0);

    looper.Stop();
    EXPECT_FALSE(looper.getRunning());
}
