#pragma once

#include <string>
#include <vector>
#include <opencv2/opencv.hpp>

namespace popcorn {

/**
 * 手部位置
 */
struct HandPosition {
    float x{0.0f};
    float y{0.0f};
    float visibility{0.0f};
    bool valid{false};
};

/**
 * 检测到的人物
 */
struct DetectedPerson {
    int id{0};
    HandPosition leftHand;
    HandPosition rightHand;
    HandPosition shoulder;  // 肩部中心
    HandPosition hip;       // 臀部中心
    HandPosition head;      // 头部
};

/**
 * 姿态检测器
 *
 * TODO: 集成 MediaPipe C++ API
 * 当前为占位实现，后续替换为真实的 MediaPipe 检测
 */
class PoseDetector {
public:
    PoseDetector();
    ~PoseDetector();

    /**
     * 初始化检测器
     * @param modelPath 模型文件路径
     * @return 成功返回 true
     */
    bool initialize(const std::string& modelPath);

    /**
     * 关闭检测器
     */
    void shutdown();

    /**
     * 执行检测
     * @param frame 输入图像（BGR 格式）
     * @return 检测到的人物列表
     */
    std::vector<DetectedPerson> detect(const cv::Mat& frame);

    /**
     * 检测器是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * 获取上次检测耗时（毫秒）
     */
    float getLastDetectionTime() const { return m_lastDetectionTime; }

private:
    bool m_initialized{false};
    float m_lastDetectionTime{0.0f};

    // TODO: MediaPipe 相关成员
    // mediapipe::tasks::vision::PoseLandmarker* m_poseLandmarker{nullptr};
};

} // namespace popcorn
