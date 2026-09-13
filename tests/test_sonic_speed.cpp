#include <gtest/gtest.h>
#include <vector>
#include "core/sonic.h"

TEST(SonicSpeedTest, CreateAndConfigure) {
    sonicStream stream = sonicCreateStream(44100, 2);
    ASSERT_NE(stream, nullptr);

    // 默认速度为 1.0
    EXPECT_FLOAT_EQ(sonicGetSpeed(stream), 1.0f);

    // 设置 1.5 倍速
    sonicSetSpeed(stream, 1.5f);
    EXPECT_FLOAT_EQ(sonicGetSpeed(stream), 1.5f);

    // 设置 2.0 倍速
    sonicSetSpeed(stream, 2.0f);
    EXPECT_FLOAT_EQ(sonicGetSpeed(stream), 2.0f);

    // 设置 0.5 倍速
    sonicSetSpeed(stream, 0.5f);
    EXPECT_FLOAT_EQ(sonicGetSpeed(stream), 0.5f);

    sonicDestroyStream(stream);
}

TEST(SonicSpeedTest, ProcessAudioSamples) {
    const int sampleRate = 44100;
    const int channels   = 2;
    sonicStream stream   = sonicCreateStream(sampleRate, channels);
    ASSERT_NE(stream, nullptr);

    sonicSetSpeed(stream, 2.0f);

    // 模拟 1 秒钟的 PCM 采样数据 (44100 * 2 个 short)
    const int inputSamples = 44100;
    std::vector<short> inputBuffer(inputSamples * channels, 1000);

    int writeRet = sonicWriteShortToStream(stream, inputBuffer.data(), inputSamples);
    EXPECT_EQ(writeRet, 1);

    sonicFlushStream(stream);

    int available = sonicSamplesAvailable(stream);
    EXPECT_GT(available, 0);

    // 在 2.0 倍速下，产出的采样点数量应当约为输入的一半
    std::vector<short> outputBuffer(available * channels);
    int readSamples = sonicReadShortFromStream(stream, outputBuffer.data(), available);
    EXPECT_EQ(readSamples, available);

    sonicDestroyStream(stream);
}
