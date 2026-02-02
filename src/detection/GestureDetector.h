#pragma once

#include <vector>
#include <memory>
#include <string>

// 前向声明 MediaPipe 类型
namespace mediapipe::tasks::vision::hand_landmarker {
class HandLandmarker;
}

namespace cv {
class Mat;
}

namespace popcorn {

/**
 * 手部关键点索引 (MediaPipe Hand Landmarks)
 */
enum class HandLandmark : int {
    Wrist = 0,
    ThumbCmc = 1,
    ThumbMcp = 2,
    ThumbIp = 3,
    ThumbTip = 4,
    IndexMcp = 5,
    IndexPip = 6,
    IndexDip = 7,
    IndexTip = 8,
    MiddleMcp = 9,
    MiddlePip = 10,
    MiddleDip = 11,
    MiddleTip = 12,
    RingMcp = 13,
    RingPip = 14,
    RingDip = 15,
    RingTip = 16,
    PinkyMcp = 17,
    PinkyPip = 18,
    PinkyDip = 19,
    PinkyTip = 20
};

/**
 * 单手的手势检测结果
 */
struct HandGestureResult {
    bool detected{false};       // 是否检测到手
    bool isOkGesture{false};    // 是否为 OK 手势
    float x{0.0f};              // 手腕 x 坐标 (像素)
    float y{0.0f};              // 手腕 y 坐标 (像素)
    float confidence{0.0f};     // 检测置信度
};

/**
 * 完整的手势检测结果
 */
struct GestureResult {
    HandGestureResult leftHand;
    HandGestureResult rightHand;
    bool anyOkGesture() const {
        return leftHand.isOkGesture || rightHand.isOkGesture;
    }
};

/**
 * 手势检测器
 * 使用 MediaPipe HandLandmarker 检测手势（如 OK 手势）
 */
class GestureDetector {
public:
    GestureDetector();
    ~GestureDetector();

    // 禁止拷贝
    GestureDetector(const GestureDetector&) = delete;
    GestureDetector& operator=(const GestureDetector&) = delete;

    /**
     * 初始化手势检测器
     * @param modelPath 模型文件路径 (hand_landmarker.task)
     * @return 成功返回 true
     */
    bool initialize(const std::string& modelPath);

    /**
     * 释放资源
     */
    void shutdown();

    /**
     * 检测手势
     * @param frame 输入图像帧 (BGR 格式)
     * @return 手势检测结果
     */
    GestureResult detect(const cv::Mat& frame);

    /**
     * 是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * 获取上次检测耗时 (毫秒)
     */
    float getLastDetectionTime() const { return m_lastDetectionTime; }

private:
    /**
     * 检测 OK 手势
     * OK 手势特征：
     * 1. 拇指尖和食指尖距离很近（形成圆圈）
     * 2. 中指、无名指、小指伸直
     */
    bool isOkGesture(const std::vector<std::array<float, 3>>& landmarks);

    /**
     * 计算两点间的 3D 距离
     */
    float distance3D(const std::array<float, 3>& p1, const std::array<float, 3>& p2);

private:
    // MediaPipe 相关 (使用 PIMPL 模式隐藏实现细节)
    struct Impl;
    std::unique_ptr<Impl> m_impl;

    bool m_initialized{false};
    float m_lastDetectionTime{0.0f};

    // OK 手势检测参数
    float m_thumbIndexThreshold{0.08f};  // 拇指-食指距离阈值
    int m_minExtendedFingers{2};         // 最少伸直手指数
};

} // namespace popcorn
