#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include "game/FallingItem.h"
#include "game/GameConfig.h"
#include "detection/PoseDetector.h"

// OpenCV 前向声明
namespace cv {
    class Mat;
}

namespace popcorn {

// 前向声明
class ParticleSystem;

/**
 * 分数弹出动画
 */
struct ScorePopup {
    float x, y;
    int score;
    float startTime;
    float duration{1.0f};
    uint32_t color;
};

/**
 * OpenGL 渲染器（增强版）
 */
class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize(int width, int height);
    void shutdown();

    void beginFrame();
    void endFrame();

    // 更新视频纹理
    void updateVideoTexture(const cv::Mat& frame);

    // 渲染视频背景
    void renderVideoBackground();

    // 渲染区域背景（P1、共享、P2）
    void renderZones();

    // 渲染掉落物
    void renderFallingItem(const FallingItem& item);

    // 渲染手部（带捕获范围）
    void renderHand(const HandPosition& hand, int playerId = 0);

    // 渲染手部捕获范围圈
    void renderCaptureZone(float x, float y, int playerId, float captureRadius, float perfectRadius);

    // 渲染 UI
    void renderUI(int p1Score, int p2Score, float remainingTime, float fps, float detectionTime,
                  GamePhase phase, int p1Combo, int p2Combo);

    // 渲染游戏状态提示
    void renderGameStateHint(const std::string& hint);

    // 分数弹出
    void showScorePopup(float x, float y, int score, bool isPerfect = false);

    // 更新动画
    void updateAnimations(float deltaTime);

    // 获取粒子系统
    ParticleSystem* getParticleSystem() { return m_particleSystem.get(); }

    // 屏幕震动
    void triggerScreenShake(float intensity = 15.0f, float duration = 0.3f);

    // 屏幕闪光
    void triggerFlash(float intensity = 0.6f);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    bool initShaders();
    bool initVideoTexture();
    bool initPrimitiveShader();
    bool initCircleGeometry();
    bool initRectGeometry();

    // 绘制圆形
    void drawCircle(float cx, float cy, float radius, float r, float g, float b, float a = 1.0f);

    // 绘制圆环
    void drawRing(float cx, float cy, float innerRadius, float outerRadius, float r, float g, float b, float a = 1.0f);

    // 绘制矩形
    void drawRect(float x, float y, float width, float height, float r, float g, float b, float a = 1.0f);

    // 绘制渐变矩形（用于区域背景）
    void drawGradientRect(float x, float y, float width, float height, uint32_t color, float alpha);

    // 更新分数弹出动画
    void updateScorePopups(float deltaTime);

    // 渲染分数弹出
    void renderScorePopups();

    // 更新震屏效果
    void updateScreenShake(float deltaTime);

    // 更新闪光效果
    void updateFlash(float deltaTime);

    // 屏幕坐标转 NDC
    float screenToNDCX(float x) const { return (x / m_width) * 2.0f - 1.0f; }
    float screenToNDCY(float y) const { return 1.0f - (y / m_height) * 2.0f; }

    // 颜色转换
    static void colorToRGB(uint32_t color, float& r, float& g, float& b) {
        r = ((color >> 16) & 0xFF) / 255.0f;
        g = ((color >> 8) & 0xFF) / 255.0f;
        b = (color & 0xFF) / 255.0f;
    }

private:
    int m_width{0};
    int m_height{0};

    // OpenGL 资源 - 视频
    uint32_t m_videoTexture{0};
    uint32_t m_shaderProgram{0};
    uint32_t m_vao{0};
    uint32_t m_vbo{0};

    // OpenGL 资源 - 图形绘制
    uint32_t m_primitiveShader{0};
    uint32_t m_circleVao{0};
    uint32_t m_circleVbo{0};
    uint32_t m_rectVao{0};
    uint32_t m_rectVbo{0};

    // 粒子系统
    std::unique_ptr<ParticleSystem> m_particleSystem;

    // 分数弹出列表
    std::vector<ScorePopup> m_scorePopups;
    float m_currentTime{0.0f};

    // 震屏效果
    float m_shakeIntensity{0.0f};
    float m_shakeDuration{0.0f};
    float m_shakeTime{0.0f};
    float m_shakeOffsetX{0.0f};
    float m_shakeOffsetY{0.0f};

    // 闪光效果
    float m_flashIntensity{0.0f};
};

} // namespace popcorn
