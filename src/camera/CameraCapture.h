#pragma once

#include <opencv2/opencv.hpp>
#include <atomic>
#include <thread>
#include <mutex>

namespace popcorn {

/**
 * 摄像头采集类
 * 使用 OpenCV 进行跨平台摄像头采集
 */
class CameraCapture {
public:
    CameraCapture();
    ~CameraCapture();

    /**
     * 初始化摄像头
     * @param deviceId 设备 ID（通常为 0）
     * @param width 期望宽度
     * @param height 期望高度
     * @return 成功返回 true
     */
    bool initialize(int deviceId, int width, int height);

    /**
     * 关闭摄像头
     */
    void shutdown();

    /**
     * 获取当前帧
     * @param frame 输出帧
     * @return 成功返回 true
     */
    bool getFrame(cv::Mat& frame);

    /**
     * 获取实际分辨率
     */
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    /**
     * 摄像头是否打开
     */
    bool isOpened() const { return m_isOpened; }

private:
    // 采集线程函数
    void captureThread();

private:
    cv::VideoCapture m_capture;
    cv::Mat m_currentFrame;
    std::mutex m_frameMutex;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_isOpened{false};

    int m_width{0};
    int m_height{0};
};

} // namespace popcorn
