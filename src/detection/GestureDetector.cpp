#include "GestureDetector.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <fstream>

// OpenCV
#include <opencv2/opencv.hpp>

// ONNX Runtime (条件编译)
#ifdef HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace popcorn {

/**
 * GestureDetector 内部实现
 * 使用 ONNX Runtime 加载手部关键点检测模型
 */
struct GestureDetector::Impl {
#ifdef HAS_ONNXRUNTIME
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::SessionOptions> sessionOptions;
    std::unique_ptr<Ort::Session> session;
    std::unique_ptr<Ort::MemoryInfo> memoryInfo;
#endif

    int inputWidth{224};
    int inputHeight{224};
    bool hasModel{false};
};

GestureDetector::GestureDetector() : m_impl(std::make_unique<Impl>()) {}

GestureDetector::~GestureDetector() {
    shutdown();
}

bool GestureDetector::initialize(const std::string& modelPath) {
    std::cout << "[GestureDetector] Initializing with model: " << modelPath << "\n";

    // 检查模型文件是否存在
    std::ifstream file(modelPath);
    if (!file.good()) {
        std::cerr << "[GestureDetector] Model file not found: " << modelPath << "\n";
        std::cout << "[GestureDetector] Running in simulation mode (skin color detection)\n";
        // 即使没有模型也标记为初始化，使用模拟模式
        m_initialized = true;
        return true;
    }
    file.close();

#ifdef HAS_ONNXRUNTIME
    try {
        // 创建 ONNX Runtime 环境
        m_impl->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "GestureDetector");

        // 会话选项
        m_impl->sessionOptions = std::make_unique<Ort::SessionOptions>();
        m_impl->sessionOptions->SetIntraOpNumThreads(2);
        m_impl->sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

        // 创建会话
#ifdef _WIN32
        std::wstring wpath(modelPath.begin(), modelPath.end());
        m_impl->session = std::make_unique<Ort::Session>(*m_impl->env, wpath.c_str(), *m_impl->sessionOptions);
#else
        m_impl->session = std::make_unique<Ort::Session>(*m_impl->env, modelPath.c_str(), *m_impl->sessionOptions);
#endif

        // 内存信息
        m_impl->memoryInfo = std::make_unique<Ort::MemoryInfo>(
            Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)
        );

        // 获取输入信息
        Ort::AllocatorWithDefaultOptions allocator;
        auto inputShape = m_impl->session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();

        if (inputShape.size() >= 4) {
            m_impl->inputHeight = static_cast<int>(inputShape[1]);
            m_impl->inputWidth = static_cast<int>(inputShape[2]);
        }

        m_impl->hasModel = true;
        m_initialized = true;
        std::cout << "[GestureDetector] Initialized with ONNX model! Input size: "
                  << m_impl->inputWidth << "x" << m_impl->inputHeight << "\n";
        return true;

    } catch (const Ort::Exception& e) {
        std::cerr << "[GestureDetector] ONNX Runtime error: " << e.what() << "\n";
        std::cout << "[GestureDetector] Falling back to simulation mode\n";
        m_initialized = true;  // 模拟模式
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[GestureDetector] Error: " << e.what() << "\n";
        std::cout << "[GestureDetector] Falling back to simulation mode\n";
        m_initialized = true;  // 模拟模式
        return true;
    }
#else
    std::cout << "[GestureDetector] ONNX Runtime not available, using simulation mode\n";
    m_initialized = true;
    return true;
#endif
}

void GestureDetector::shutdown() {
    if (m_impl) {
#ifdef HAS_ONNXRUNTIME
        m_impl->session.reset();
        m_impl->sessionOptions.reset();
        m_impl->memoryInfo.reset();
        m_impl->env.reset();
#endif
        m_impl->hasModel = false;
    }
    m_initialized = false;
    std::cout << "[GestureDetector] Shutdown complete\n";
}

GestureResult GestureDetector::detect(const cv::Mat& frame) {
    GestureResult result;

    if (!m_initialized || frame.empty()) {
        return result;
    }

    auto startTime = std::chrono::steady_clock::now();

    // 如果没有实际模型，使用模拟模式
    // 检测用户是否在做 OK 手势的简化方法：
    // 使用皮肤颜色检测来识别手的位置
    if (!m_impl->hasModel) {
        // 模拟模式：使用颜色检测找手
        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        // 皮肤颜色范围 (HSV)
        cv::Mat skinMask;
        cv::inRange(hsv, cv::Scalar(0, 20, 70), cv::Scalar(20, 255, 255), skinMask);

        // 形态学操作
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
        cv::morphologyEx(skinMask, skinMask, cv::MORPH_OPEN, kernel);
        cv::morphologyEx(skinMask, skinMask, cv::MORPH_CLOSE, kernel);

        // 找轮廓
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(skinMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        // 找最大的两个轮廓作为手
        std::vector<std::pair<double, int>> contourAreas;
        for (size_t i = 0; i < contours.size(); ++i) {
            double area = cv::contourArea(contours[i]);
            if (area > 5000) {  // 最小面积阈值
                contourAreas.push_back({area, static_cast<int>(i)});
            }
        }

        std::sort(contourAreas.begin(), contourAreas.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        int handCount = 0;
        for (const auto& [area, idx] : contourAreas) {
            if (handCount >= 2) break;

            cv::Rect boundRect = cv::boundingRect(contours[idx]);
            float centerX = boundRect.x + boundRect.width / 2.0f;
            float centerY = boundRect.y + boundRect.height / 2.0f;

            // 使用轮廓的凸包缺陷来检测 OK 手势
            std::vector<cv::Point> hull;
            std::vector<int> hullIndices;
            cv::convexHull(contours[idx], hull);
            cv::convexHull(contours[idx], hullIndices);

            // 计算凸包缺陷
            std::vector<cv::Vec4i> defects;
            if (hullIndices.size() > 3) {
                cv::convexityDefects(contours[idx], hullIndices, defects);
            }

            // 简化的 OK 手势检测：
            // OK 手势通常有一个明显的圆形洞（拇指和食指形成）
            // 这会在凸包缺陷中表现为深度较大的缺陷
            int deepDefects = 0;
            for (const auto& d : defects) {
                float depth = d[3] / 256.0f;  // 缺陷深度
                if (depth > 20) {  // 深度阈值
                    deepDefects++;
                }
            }

            // 检测圆形（OK 手势的特征）
            double circularity = 4 * CV_PI * area / (boundRect.width * boundRect.height * 4);

            // 简单判断：有 1-2 个深缺陷，且整体较紧凑
            bool isOk = (deepDefects >= 1 && deepDefects <= 3) &&
                        (static_cast<float>(boundRect.width) / boundRect.height > 0.5f) &&
                        (static_cast<float>(boundRect.width) / boundRect.height < 2.0f);

            // 根据位置分配到左手或右手
            if (centerX < frame.cols / 2.0f) {
                // 画面左侧 = 用户右手（镜像）
                result.rightHand.detected = true;
                result.rightHand.x = centerX;
                result.rightHand.y = centerY;
                result.rightHand.isOkGesture = isOk;
                result.rightHand.confidence = 0.7f;
            } else {
                // 画面右侧 = 用户左手（镜像）
                result.leftHand.detected = true;
                result.leftHand.x = centerX;
                result.leftHand.y = centerY;
                result.leftHand.isOkGesture = isOk;
                result.leftHand.confidence = 0.7f;
            }

            handCount++;
        }
    } else {
        // 真实模型推理模式 (当有手部关键点模型时)
        // TODO: 实现完整的 MediaPipe HandLandmarker 推理
    }

    auto endTime = std::chrono::steady_clock::now();
    m_lastDetectionTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    return result;
}

bool GestureDetector::isOkGesture(const std::vector<std::array<float, 3>>& landmarks) {
    if (landmarks.size() < 21) {
        return false;
    }

    // 获取关键点
    auto thumbTip = landmarks[static_cast<int>(HandLandmark::ThumbTip)];
    auto indexTip = landmarks[static_cast<int>(HandLandmark::IndexTip)];
    auto middleTip = landmarks[static_cast<int>(HandLandmark::MiddleTip)];
    auto ringTip = landmarks[static_cast<int>(HandLandmark::RingTip)];
    auto pinkyTip = landmarks[static_cast<int>(HandLandmark::PinkyTip)];

    auto middleMcp = landmarks[static_cast<int>(HandLandmark::MiddleMcp)];
    auto ringMcp = landmarks[static_cast<int>(HandLandmark::RingMcp)];
    auto pinkyMcp = landmarks[static_cast<int>(HandLandmark::PinkyMcp)];

    // 1. 检查拇指尖和食指尖距离（应该很近）
    float thumbIndexDistance = distance3D(thumbTip, indexTip);
    bool isCircleFormed = thumbIndexDistance < m_thumbIndexThreshold;

    // 2. 检查其他三指是否伸直
    bool middleExtended = middleTip[1] < middleMcp[1];  // y 坐标比较
    bool ringExtended = ringTip[1] < ringMcp[1];
    bool pinkyExtended = pinkyTip[1] < pinkyMcp[1];

    // 至少 2 根手指伸直
    int extendedCount = (middleExtended ? 1 : 0) + (ringExtended ? 1 : 0) + (pinkyExtended ? 1 : 0);
    bool fingersExtended = extendedCount >= m_minExtendedFingers;

    // 综合判断
    bool isOk = isCircleFormed && fingersExtended;

    if (isOk) {
        std::cout << "[GestureDetector] OK gesture detected! thumbIndexDist="
                  << thumbIndexDistance << ", extendedFingers=" << extendedCount << "\n";
    }

    return isOk;
}

float GestureDetector::distance3D(const std::array<float, 3>& p1, const std::array<float, 3>& p2) {
    float dx = p1[0] - p2[0];
    float dy = p1[1] - p2[1];
    float dz = p1[2] - p2[2];
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

} // namespace popcorn
