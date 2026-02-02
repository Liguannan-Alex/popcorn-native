#include "Application.h"
#include "Window.h"
#include "Renderer.h"
#include "camera/CameraCapture.h"
#include "detection/PoseDetector.h"
#include "detection/GestureDetector.h"
#include "game/GameEngine.h"

#include <iostream>
#include <chrono>
#include <thread>

namespace popcorn {

Application::Application() = default;

Application::~Application() {
    shutdown();
}

bool Application::initialize(int width, int height, const std::string& title) {
    std::cout << "[Application] Initializing...\n";

    // 1. 创建窗口
    std::cout << "[Application] Creating window...\n";
    m_window = std::make_unique<Window>();
    if (!m_window->create(width, height, title)) {
        std::cerr << "[Application] Failed to create window\n";
        return false;
    }

    // 2. 初始化渲染器
    std::cout << "[Application] Initializing renderer...\n";
    m_renderer = std::make_unique<Renderer>();
    if (!m_renderer->initialize(width, height)) {
        std::cerr << "[Application] Failed to initialize renderer\n";
        return false;
    }

    // 3. 初始化摄像头
    std::cout << "[Application] Initializing camera...\n";
    m_camera = std::make_unique<CameraCapture>();
    if (!m_camera->initialize(0, 1280, 720)) {
        std::cerr << "[Application] Failed to initialize camera\n";
        return false;
    }

    // 4. 初始化姿态检测器 (MoveNet via ONNX Runtime)
    std::cout << "[Application] Initializing pose detector...\n";
    m_poseDetector = std::make_unique<PoseDetector>();
    if (!m_poseDetector->initialize("assets/models/movenet_lightning.onnx")) {
        std::cerr << "[Application] Failed to initialize pose detector\n";
        std::cout << "[Application] Warning: Pose detector not available, continuing without it\n";
    } else {
        std::cout << "[Application] Pose detector initialized successfully!\n";
    }

    // 5. 初始化手势检测器 (用于 OK 手势检测)
    std::cout << "[Application] Initializing gesture detector...\n";
    m_gestureDetector = std::make_unique<GestureDetector>();
    if (!m_gestureDetector->initialize("assets/models/hand_landmarker.task")) {
        std::cerr << "[Application] Failed to initialize gesture detector\n";
        std::cout << "[Application] Warning: Using simulation mode for gesture detection\n";
    } else {
        std::cout << "[Application] Gesture detector initialized!\n";
    }

    // 6. 初始化游戏引擎
    std::cout << "[Application] Initializing game engine...\n";
    m_gameEngine = std::make_unique<GameEngine>();
    if (!m_gameEngine->initialize(width, height)) {
        std::cerr << "[Application] Failed to initialize game engine\n";
        return false;
    }

    m_running = true;
    m_lastFPSTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    std::cout << "[Application] Initialization complete!\n";
    return true;
}

void Application::run() {
    std::cout << "[Application] Starting main loop...\n";

    auto lastTime = std::chrono::steady_clock::now();
    constexpr float TARGET_FPS = 60.0f;
    constexpr auto FRAME_DURATION = std::chrono::duration<float>(1.0f / TARGET_FPS);

    while (m_running) {
        auto frameStart = std::chrono::steady_clock::now();

        // 计算 deltaTime
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        // 1. 处理事件
        processEvents();

        // 2. 更新逻辑
        update(deltaTime);

        // 3. 渲染
        render();

        // 4. 计算 FPS
        calculateFPS();

        // 5. 帧率限制（可选，VSync 通常已处理）
        auto frameEnd = std::chrono::steady_clock::now();
        auto frameDuration = frameEnd - frameStart;
        if (frameDuration < FRAME_DURATION) {
            std::this_thread::sleep_for(FRAME_DURATION - frameDuration);
        }
    }

    std::cout << "[Application] Main loop ended.\n";
}

void Application::processEvents() {
    if (m_window) {
        m_window->pollEvents();
        if (m_window->shouldClose()) {
            m_running = false;
        }
    }
}

void Application::update(float deltaTime) {
    // 1. 获取摄像头帧
    cv::Mat frame;
    if (m_camera && m_camera->getFrame(frame)) {
        std::vector<DetectedPerson> persons;
        GestureResult gesture;

        // 2. 姿态检测
        if (m_poseDetector && m_poseDetector->isInitialized()) {
            auto startTime = std::chrono::steady_clock::now();

            persons = m_poseDetector->detect(frame);

            auto endTime = std::chrono::steady_clock::now();
            m_detectionTime = std::chrono::duration<float, std::milli>(endTime - startTime).count();
        }

        // 3. 手势检测 (用于 OK 手势启动游戏)
        if (m_gestureDetector && m_gestureDetector->isInitialized()) {
            gesture = m_gestureDetector->detect(frame);
        }

        // 4. 更新游戏逻辑
        if (m_gameEngine) {
            m_gameEngine->update(deltaTime, persons, gesture);
        }

        // 5. 更新渲染器的视频纹理
        if (m_renderer) {
            m_renderer->updateVideoTexture(frame);
        }
    }
}

void Application::render() {
    if (!m_renderer || !m_window) return;

    m_renderer->beginFrame();

    // 渲染视频背景
    m_renderer->renderVideoBackground();

    // 渲染游戏元素
    if (m_gameEngine) {
        // 渲染掉落物
        for (const auto& item : m_gameEngine->getFallingItems()) {
            m_renderer->renderFallingItem(item);
        }

        // 渲染手部位置
        for (const auto& person : m_gameEngine->getDetectedPersons()) {
            m_renderer->renderHand(person.leftHand);
            m_renderer->renderHand(person.rightHand);
        }

        // 渲染 UI（分数、时间等）
        m_renderer->renderUI(m_gameEngine->getScore(), m_gameEngine->getRemainingTime(), m_fps, m_detectionTime);
    }

    m_renderer->endFrame();

    // 交换缓冲区
    m_window->swapBuffers();
}

void Application::calculateFPS() {
    m_frameCount++;

    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    if (currentTime - m_lastFPSTime >= 1000) {
        m_fps = static_cast<float>(m_frameCount);
        m_frameCount = 0;
        m_lastFPSTime = currentTime;

        // 每秒输出一次性能信息
        std::cout << "[Performance] FPS: " << m_fps
                  << " | Detection: " << m_detectionTime << "ms\n";
    }
}

void Application::shutdown() {
    std::cout << "[Application] Shutting down...\n";

    m_running = false;

    m_gameEngine.reset();
    m_gestureDetector.reset();
    m_poseDetector.reset();
    m_camera.reset();
    m_renderer.reset();
    m_window.reset();

    std::cout << "[Application] Shutdown complete.\n";
}

} // namespace popcorn
