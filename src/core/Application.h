#pragma once

#include <memory>
#include <string>
#include <atomic>

namespace popcorn {

// 前向声明
class Window;
class Renderer;
class CameraCapture;
class PoseDetector;
class GestureDetector;
class GameEngine;

/**
 * 应用程序主类
 * 负责初始化、主循环、资源管理
 */
class Application {
public:
    Application();
    ~Application();

    // 禁止拷贝
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    /**
     * 初始化应用
     * @param width 窗口宽度
     * @param height 窗口高度
     * @param title 窗口标题
     * @return 成功返回 true
     */
    bool initialize(int width, int height, const std::string& title);

    /**
     * 运行主循环
     */
    void run();

    /**
     * 关闭应用
     */
    void shutdown();

    /**
     * 请求退出
     */
    void requestQuit() { m_running = false; }

    /**
     * 获取帧率
     */
    float getFPS() const { return m_fps; }

    /**
     * 获取检测时间（毫秒）
     */
    float getDetectionTime() const { return m_detectionTime; }

private:
    // 处理输入事件
    void processEvents();

    // 更新逻辑
    void update(float deltaTime);

    // 渲染
    void render();

    // 计算 FPS
    void calculateFPS();

private:
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    std::unique_ptr<CameraCapture> m_camera;
    std::unique_ptr<PoseDetector> m_poseDetector;
    std::unique_ptr<GestureDetector> m_gestureDetector;
    std::unique_ptr<GameEngine> m_gameEngine;

    std::atomic<bool> m_running{false};
    float m_fps{0.0f};
    float m_detectionTime{0.0f};

    // 帧率计算
    uint64_t m_frameCount{0};
    uint64_t m_lastFPSTime{0};
};

} // namespace popcorn
