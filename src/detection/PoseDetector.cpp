#include "PoseDetector.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <fstream>

// ONNX Runtime (条件编译)
#ifdef HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>

#ifdef _WIN32
// DirectML for GPU acceleration on Windows
#include <dml_provider_factory.h>
#endif
#endif

namespace popcorn {

/**
 * PoseDetector 内部实现
 * 使用 ONNX Runtime 加载 MoveNet 模型
 */
struct PoseDetector::Impl {
#ifdef HAS_ONNXRUNTIME
    std::unique_ptr<Ort::Env> env;
    std::unique_ptr<Ort::Session> session;
    std::unique_ptr<Ort::SessionOptions> sessionOptions;
    std::unique_ptr<Ort::MemoryInfo> memoryInfo;
#endif
    bool hasModel{false};
};

PoseDetector::PoseDetector() : m_impl(std::make_unique<Impl>()) {}

PoseDetector::~PoseDetector() {
    shutdown();
}

bool PoseDetector::initialize(const std::string& modelPath) {
    std::cout << "[PoseDetector] Initializing with model: " << modelPath << "\n";

    // 检查模型文件是否存在
    std::ifstream file(modelPath);
    if (!file.good()) {
        std::cerr << "[PoseDetector] Model file not found: " << modelPath << "\n";
        return false;
    }
    file.close();

#ifdef HAS_ONNXRUNTIME
    try {
        // 创建 ONNX Runtime 环境
        m_impl->env = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "PoseDetector");

        // 会话选项
        m_impl->sessionOptions = std::make_unique<Ort::SessionOptions>();
        m_impl->sessionOptions->SetIntraOpNumThreads(2);
        m_impl->sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

#ifdef _WIN32
        // Windows: 尝试使用 DirectML (GPU via DirectX)
        try {
            OrtSessionOptionsAppendExecutionProvider_DML(*m_impl->sessionOptions, 0);
            std::cout << "[PoseDetector] DirectML GPU acceleration enabled\n";
        } catch (...) {
            std::cout << "[PoseDetector] DirectML not available, using CPU\n";
        }
#endif

        // 创建会话
#ifdef _WIN32
        // Windows 使用宽字符路径
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
        auto inputName = m_impl->session->GetInputNameAllocated(0, allocator);
        auto inputShape = m_impl->session->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape();

        std::cout << "[PoseDetector] Input name: " << inputName.get() << "\n";
        std::cout << "[PoseDetector] Input shape: ";
        for (auto dim : inputShape) {
            std::cout << dim << " ";
        }
        std::cout << "\n";

        // 根据模型设置输入尺寸
        if (inputShape.size() >= 4) {
            m_inputHeight = static_cast<int>(inputShape[1]);
            m_inputWidth = static_cast<int>(inputShape[2]);
        }

        m_impl->hasModel = true;
        m_initialized = true;
        std::cout << "[PoseDetector] Initialized successfully! Input size: "
                  << m_inputWidth << "x" << m_inputHeight << "\n";
        return true;

    } catch (const Ort::Exception& e) {
        std::cerr << "[PoseDetector] ONNX Runtime error: " << e.what() << "\n";
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[PoseDetector] Error: " << e.what() << "\n";
        return false;
    }
#else
    std::cerr << "[PoseDetector] ONNX Runtime not available (HAS_ONNXRUNTIME not defined)\n";
    return false;
#endif
}

void PoseDetector::shutdown() {
#ifdef HAS_ONNXRUNTIME
    if (m_impl) {
        m_impl->session.reset();
        m_impl->sessionOptions.reset();
        m_impl->memoryInfo.reset();
        m_impl->env.reset();
        m_impl->hasModel = false;
    }
#endif
    m_initialized = false;
    std::cout << "[PoseDetector] Shutdown complete\n";
}

cv::Mat PoseDetector::preprocessImage(const cv::Mat& frame) {
    cv::Mat resized, rgb;

    // BGR -> RGB
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

    // 调整大小
    cv::resize(rgb, resized, cv::Size(m_inputWidth, m_inputHeight));

    // 保持 uint8 格式，值范围 0-255（模型期望 int32，但值相同）
    return resized;
}

DetectedPerson PoseDetector::parseOutput(const float* output, int frameWidth, int frameHeight) {
    DetectedPerson person;
    person.id = 0;

    // MoveNet 输出格式: [1, 1, 17, 3]
    // 每个关键点: [y, x, confidence]
    auto getKeypoint = [&](int index) -> HandPosition {
        HandPosition pos;
        float y = output[index * 3 + 0];
        float x = output[index * 3 + 1];
        float conf = output[index * 3 + 2];

        pos.x = x * frameWidth;
        pos.y = y * frameHeight;
        pos.visibility = conf;
        pos.valid = conf > m_confidenceThreshold;

        return pos;
    };

    // 提取关键点
    person.head = getKeypoint(static_cast<int>(MoveNetKeypoint::Nose));
    person.leftShoulder = getKeypoint(static_cast<int>(MoveNetKeypoint::LeftShoulder));
    person.rightShoulder = getKeypoint(static_cast<int>(MoveNetKeypoint::RightShoulder));
    person.leftElbow = getKeypoint(static_cast<int>(MoveNetKeypoint::LeftElbow));
    person.rightElbow = getKeypoint(static_cast<int>(MoveNetKeypoint::RightElbow));
    person.leftHand = getKeypoint(static_cast<int>(MoveNetKeypoint::LeftWrist));
    person.rightHand = getKeypoint(static_cast<int>(MoveNetKeypoint::RightWrist));

    // 计算肩部中心
    auto leftShoulder = getKeypoint(static_cast<int>(MoveNetKeypoint::LeftShoulder));
    auto rightShoulder = getKeypoint(static_cast<int>(MoveNetKeypoint::RightShoulder));
    if (leftShoulder.valid && rightShoulder.valid) {
        person.shoulder.x = (leftShoulder.x + rightShoulder.x) / 2.0f;
        person.shoulder.y = (leftShoulder.y + rightShoulder.y) / 2.0f;
        person.shoulder.visibility = (leftShoulder.visibility + rightShoulder.visibility) / 2.0f;
        person.shoulder.valid = true;
    }

    // 计算臀部中心
    auto leftHip = getKeypoint(static_cast<int>(MoveNetKeypoint::LeftHip));
    auto rightHip = getKeypoint(static_cast<int>(MoveNetKeypoint::RightHip));
    if (leftHip.valid && rightHip.valid) {
        person.hip.x = (leftHip.x + rightHip.x) / 2.0f;
        person.hip.y = (leftHip.y + rightHip.y) / 2.0f;
        person.hip.visibility = (leftHip.visibility + rightHip.visibility) / 2.0f;
        person.hip.valid = true;
    }

    return person;
}

std::vector<DetectedPerson> PoseDetector::detect(const cv::Mat& frame) {
    if (!m_initialized || frame.empty()) {
        return {};
    }

#ifndef HAS_ONNXRUNTIME
    return {};
#else
    if (!m_impl->hasModel) {
        return {};
    }

    auto startTime = std::chrono::steady_clock::now();

    std::vector<DetectedPerson> persons;

    try {
        // 预处理
        cv::Mat input = preprocessImage(frame);

        // 准备输入张量 (int32 格式，值范围 0-255)
        std::vector<int64_t> inputShape = {1, m_inputHeight, m_inputWidth, 3};
        size_t inputSize = m_inputHeight * m_inputWidth * 3;
        std::vector<int32_t> inputData(inputSize);

        // 将 uint8 转换为 int32
        for (size_t i = 0; i < inputSize; ++i) {
            inputData[i] = static_cast<int32_t>(input.data[i]);
        }

        // 创建输入张量 (int32)
        Ort::Value inputTensor = Ort::Value::CreateTensor<int32_t>(
            *m_impl->memoryInfo,
            inputData.data(),
            inputSize,
            inputShape.data(),
            inputShape.size()
        );

        // 获取输入/输出名称
        Ort::AllocatorWithDefaultOptions allocator;
        auto inputName = m_impl->session->GetInputNameAllocated(0, allocator);
        auto outputName = m_impl->session->GetOutputNameAllocated(0, allocator);

        const char* inputNames[] = {inputName.get()};
        const char* outputNames[] = {outputName.get()};

        // 推理
        auto outputTensors = m_impl->session->Run(
            Ort::RunOptions{nullptr},
            inputNames,
            &inputTensor,
            1,
            outputNames,
            1
        );

        // 获取输出形状并打印调试信息
        auto outputInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
        auto outputShape = outputInfo.GetShape();

        static int debugCount = 0;
        if (debugCount < 5) {
            std::cout << "[PoseDetector] Output shape: ";
            for (auto dim : outputShape) {
                std::cout << dim << " ";
            }
            std::cout << "\n";
        }

        // 解析输出
        float* outputData = outputTensors[0].GetTensorMutableData<float>();

        // 打印前几个关键点的原始值（调试用）
        if (debugCount < 5) {
            std::cout << "[PoseDetector] Raw output (first 10 values): ";
            for (int i = 0; i < 10 && i < static_cast<int>(outputInfo.GetElementCount()); ++i) {
                std::cout << outputData[i] << " ";
            }
            std::cout << "\n";

            // 打印左手腕 (index 9) 和右手腕 (index 10) 的值
            std::cout << "[PoseDetector] LeftWrist (idx 9): y=" << outputData[9*3+0]
                      << " x=" << outputData[9*3+1] << " conf=" << outputData[9*3+2] << "\n";
            std::cout << "[PoseDetector] RightWrist (idx 10): y=" << outputData[10*3+0]
                      << " x=" << outputData[10*3+1] << " conf=" << outputData[10*3+2] << "\n";
            debugCount++;
        }

        DetectedPerson person = parseOutput(outputData, frame.cols, frame.rows);

        // 只有检测到有效关键点才添加
        if (person.leftHand.valid || person.rightHand.valid || person.shoulder.valid) {
            persons.push_back(person);
        }

    } catch (const Ort::Exception& e) {
        std::cerr << "[PoseDetector] Inference error: " << e.what() << "\n";
    } catch (const std::exception& e) {
        std::cerr << "[PoseDetector] Error: " << e.what() << "\n";
    }

    auto endTime = std::chrono::steady_clock::now();
    m_lastDetectionTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    return persons;
#endif
}

} // namespace popcorn
