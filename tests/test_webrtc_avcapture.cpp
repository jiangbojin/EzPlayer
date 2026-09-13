#include <gtest/gtest.h>
#include <iostream>
#include "webrtc_av_capture/webrtc_av_capture.h"

// 验证 DeviceManager 实例化与多媒体硬件枚举功能
TEST(WebRtcAvCaptureTest, DeviceManagerEnumeration) {
    auto dev_mgr = webrtc_av::CreateDeviceManager();
    ASSERT_NE(dev_mgr, nullptr);

    // 1. 摄像头枚举测试
    auto cameras = dev_mgr->EnumerateCameras();
    std::cout << "[EzPlayer Test] EnumerateCameras count: " << cameras.size() << std::endl;
    for (const auto& cam : cameras) {
        std::cout << "  Camera: " << cam.GetName() << " (ID: " << cam.GetId() << ")" << std::endl;
        EXPECT_FALSE(cam.GetName().empty());
    }

    // 2. 屏幕枚举测试
    auto screens = dev_mgr->EnumerateScreens();
    std::cout << "[EzPlayer Test] EnumerateScreens count: " << screens.size() << std::endl;

    // 3. 音频录音设备枚举测试
    auto mics = dev_mgr->EnumerateAudioDevices(true);
    std::cout << "[EzPlayer Test] Audio recording devices count: " << mics.size() << std::endl;

    // 4. 音频播放设备枚举测试
    auto speakers = dev_mgr->EnumerateAudioDevices(false);
    std::cout << "[EzPlayer Test] Audio playout devices count: " << speakers.size() << std::endl;
}

// 模拟视频观察者
class MockVideoObserver : public webrtc_av::VideoCaptureObserver {
   public:
    int frame_count{0};
    void OnVideoFrame(const webrtc_av::VideoFrameI420& frame) override {
        frame_count++;
        EXPECT_GT(frame.width, 0);
        EXPECT_GT(frame.height, 0);
    }
};

// 验证 VideoCapturer 生命周期与工厂创建
TEST(WebRtcAvCaptureTest, VideoCapturerLifecycle) {
    MockVideoObserver observer;
    webrtc_av::VideoCaptureConfig config;
    config.source_type   = webrtc_av::VideoSourceType::kScreen;
    config.target_width  = 1280;
    config.target_height = 720;
    config.target_fps    = 30;

    auto capturer = webrtc_av::CreateVideoCapturer(config, &observer);
    ASSERT_NE(capturer, nullptr);

    // 验证更新配置
    config.target_fps = 15;
    capturer->UpdateConfig(config);

    // 在非 GUI/Headless 环境下，Start() 可能会根据 X11 状态返回 false，但必须不发生崩溃且安全析构
    bool started = capturer->Start();
    if (started) {
        EXPECT_TRUE(capturer->IsRunning());
        capturer->Stop();
        EXPECT_FALSE(capturer->IsRunning());
    }
}

// 模拟音频观察者
class MockAudioObserver : public webrtc_av::AudioCaptureObserver {
   public:
    int pcm_count{0};
    void OnAudioData(const webrtc_av::AudioPcmFrame& frame) override {
        pcm_count++;
        EXPECT_EQ(frame.sample_rate_hz, 48000);
        EXPECT_EQ(frame.num_channels, 2);
    }
};

// 验证 AudioEngine 3A 配置与生命周期
TEST(WebRtcAvCaptureTest, AudioEngineLifecycle) {
    MockAudioObserver observer;
    webrtc_av::ApmConfig apm_cfg;
    apm_cfg.enable_aec              = true;
    apm_cfg.enable_ans              = true;
    apm_cfg.enable_agc              = true;
    apm_cfg.enable_high_pass_filter = true;

    auto audio_engine = webrtc_av::CreateAudioEngine(apm_cfg, &observer);
    ASSERT_NE(audio_engine, nullptr);

    // 动态调控 APM 参数测试
    apm_cfg.enable_ans = false;
    audio_engine->UpdateApmConfig(apm_cfg);

    // 音量调节测试
    audio_engine->SetMicrophoneVolume(128);
    audio_engine->SetSpeakerVolume(200);

    // 验证录音初始状态，不调用硬件录音以避免虚拟无头环境下的 ALSA loopback 驱动阻塞
    EXPECT_FALSE(audio_engine->IsRecording());
}
