#pragma once

#include <string>
#include <vector>
#include <memory>
#include <opencv2/opencv.hpp>

// 前向声明 ONNX Runtime
namespace Ort {
    struct Session;
    struct Env;
    struct SessionOptions;
    struct MemoryInfo;
}

namespace popcorn {

/**
 * 手部/关键点位置
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
    HandPosition leftHand;      // 左手腕 (index 9)
    HandPosition rightHand;     // 右手腕 (index 10)
    HandPosition shoulder;      // 肩部中心
    HandPosition hip;           // 臀部中心
    HandPosition head;          // 鼻子 (index 0)
    HandPosition leftShoulder;  // 左肩 (index 5)
    HandPosition rightShoulder; // 右肩 (index 6)
    HandPosition leftElbow;     // 左肘 (index 7)
    HandPosition rightElbow;    // 右肘 (index 8)
};

/**
 * MoveNet 关键点索引
 * 0: nose, 1: left_eye, 2: right_eye, 3: left_ear, 4: right_ear
 * 5: left_shoulder, 6: right_shoulder, 7: left_elbow, 8: right_elbow
 * 9: left_wrist, 10: right_wrist, 11: left_hip, 12: right_hip
 * 13: left_knee, 14: right_knee, 15: left_ankle, 16: right_ankle
 */
enum class MoveNetKeypoint {
    Nose = 0,
    LeftEye = 1,
    RightEye = 2,
    LeftEar = 3,
    RightEar = 4,
    LeftShoulder = 5,
    RightShoulder = 6,
    LeftElbow = 7,
    RightElbow = 8,
    LeftWrist = 9,
    RightWrist = 10,
    LeftHip = 11,
    RightHip = 12,
    LeftKnee = 13,
    RightKnee = 14,
    LeftAnkle = 15,
    RightAnkle = 16
};

/**
 * 姿态检测器 (使用 ONNX Runtime + MoveNet)
 */
class PoseDetector {
public:
    PoseDetector();
    ~PoseDetector();

    /**
     * 初始化检测器
     * @param modelPath 模型文件路径 (.onnx)
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

    /**
     * 设置置信度阈值
     */
    void setConfidenceThreshold(float threshold) { m_confidenceThreshold = threshold; }

private:
    // 预处理图像
    cv::Mat preprocessImage(const cv::Mat& frame);

    // 解析输出
    DetectedPerson parseOutput(const float* output, int width, int height);

private:
    bool m_initialized{false};
    float m_lastDetectionTime{0.0f};
    float m_confidenceThreshold{0.3f};

    // 模型输入尺寸 (MoveNet Lightning: 192x192, Thunder: 256x256)
    int m_inputWidth{192};
    int m_inputHeight{192};

    // ONNX Runtime
    std::unique_ptr<Ort::Env> m_env;
    std::unique_ptr<Ort::Session> m_session;
    std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
    std::unique_ptr<Ort::MemoryInfo> m_memoryInfo;
};

} // namespace popcorn
