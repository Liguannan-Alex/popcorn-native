#include "CameraCapture.h"
#include <iostream>

namespace popcorn {

CameraCapture::CameraCapture() = default;

CameraCapture::~CameraCapture() {
    shutdown();
}

bool CameraCapture::initialize(int deviceId, int width, int height) {
    std::cout << "[Camera] Opening device " << deviceId << "...\n";

    // 打开摄像头
    m_capture.open(deviceId);

    if (!m_capture.isOpened()) {
        std::cerr << "[Camera] Failed to open camera " << deviceId << "\n";
        return false;
    }

    // 设置分辨率
    m_capture.set(cv::CAP_PROP_FRAME_WIDTH, width);
    m_capture.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    m_capture.set(cv::CAP_PROP_FPS, 30);

    // 获取实际分辨率
    m_width = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_WIDTH));
    m_height = static_cast<int>(m_capture.get(cv::CAP_PROP_FRAME_HEIGHT));

    std::cout << "[Camera] Opened at " << m_width << "x" << m_height << "\n";

    m_isOpened = true;
    m_running = true;

    // 启动采集线程
    m_thread = std::thread(&CameraCapture::captureThread, this);

    return true;
}

void CameraCapture::shutdown() {
    m_running = false;

    if (m_thread.joinable()) {
        m_thread.join();
    }

    if (m_capture.isOpened()) {
        m_capture.release();
    }

    m_isOpened = false;
    std::cout << "[Camera] Shutdown complete\n";
}

void CameraCapture::captureThread() {
    std::cout << "[Camera] Capture thread started\n";

    cv::Mat frame;
    while (m_running) {
        if (m_capture.read(frame)) {
            std::lock_guard<std::mutex> lock(m_frameMutex);
            m_currentFrame = frame.clone();
        } else {
            // 读取失败，短暂休眠后重试
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    std::cout << "[Camera] Capture thread ended\n";
}

bool CameraCapture::getFrame(cv::Mat& frame) {
    std::lock_guard<std::mutex> lock(m_frameMutex);

    if (m_currentFrame.empty()) {
        return false;
    }

    frame = m_currentFrame.clone();
    return true;
}

} // namespace popcorn
