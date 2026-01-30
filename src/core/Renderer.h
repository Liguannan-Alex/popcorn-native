#pragma once

#include <cstdint>
#include "game/FallingItem.h"
#include "detection/PoseDetector.h"

// OpenCV 前向声明
namespace cv {
    class Mat;
}

namespace popcorn {

/**
 * OpenGL 渲染器
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

    // 渲染掉落物
    void renderFallingItem(const FallingItem& item);

    // 渲染手部
    void renderHand(const HandPosition& hand);

    // 渲染 UI
    void renderUI(int score, float remainingTime, float fps, float detectionTime);

private:
    bool initShaders();
    bool initVideoTexture();
    bool initPrimitiveShader();
    bool initCircleGeometry();

    // 绘制圆形
    void drawCircle(float cx, float cy, float radius, float r, float g, float b, float a = 1.0f);

    // 屏幕坐标转 NDC
    float screenToNDCX(float x) const { return (x / m_width) * 2.0f - 1.0f; }
    float screenToNDCY(float y) const { return 1.0f - (y / m_height) * 2.0f; }

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
};

} // namespace popcorn
