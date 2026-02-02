#include "Renderer.h"
#include "render/ParticleSystem.h"

// Windows MSVC 需要此宏才能使用 M_PI
#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <random>

// 如果 M_PI 仍未定义（某些编译器）
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OpenGL 头文件（跨平台）
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#elif defined(_WIN32)
    #include <GL/glew.h>  // Windows 需要 GLEW
#else
    #include <GL/gl.h>
#endif

#include <opencv2/opencv.hpp>

namespace popcorn {

// 圆形顶点数
constexpr int CIRCLE_SEGMENTS = 32;

// 随机数生成器
static std::mt19937 s_rng(std::random_device{}());

Renderer::Renderer() = default;

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize(int width, int height) {
    m_width = width;
    m_height = height;

    // 设置视口
    glViewport(0, 0, width, height);

    // 启用混合（透明度）
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 初始化着色器
    if (!initShaders()) {
        std::cerr << "[Renderer] Failed to init shaders\n";
        return false;
    }

    // 初始化视频纹理
    if (!initVideoTexture()) {
        std::cerr << "[Renderer] Failed to init video texture\n";
        return false;
    }

    // 初始化图形着色器
    if (!initPrimitiveShader()) {
        std::cerr << "[Renderer] Failed to init primitive shader\n";
        return false;
    }

    // 初始化圆形几何
    if (!initCircleGeometry()) {
        std::cerr << "[Renderer] Failed to init circle geometry\n";
        return false;
    }

    // 初始化矩形几何
    if (!initRectGeometry()) {
        std::cerr << "[Renderer] Failed to init rect geometry\n";
        return false;
    }

    // 初始化粒子系统
    m_particleSystem = std::make_unique<ParticleSystem>();
    m_particleSystem->initialize(500);

    std::cout << "[Renderer] Initialized " << width << "x" << height << "\n";
    return true;
}

void Renderer::shutdown() {
    m_particleSystem.reset();

    if (m_videoTexture) {
        glDeleteTextures(1, &m_videoTexture);
        m_videoTexture = 0;
    }
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_primitiveShader) {
        glDeleteProgram(m_primitiveShader);
        m_primitiveShader = 0;
    }
    if (m_circleVao) {
        glDeleteVertexArrays(1, &m_circleVao);
        m_circleVao = 0;
    }
    if (m_circleVbo) {
        glDeleteBuffers(1, &m_circleVbo);
        m_circleVbo = 0;
    }
    if (m_rectVao) {
        glDeleteVertexArrays(1, &m_rectVao);
        m_rectVao = 0;
    }
    if (m_rectVbo) {
        glDeleteBuffers(1, &m_rectVbo);
        m_rectVbo = 0;
    }
}

bool Renderer::initShaders() {
    // 顶点着色器
    const char* vertexShaderSource = R"(
        #version 410 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        uniform vec2 uOffset;
        void main() {
            gl_Position = vec4(aPos + uOffset, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    // 片段着色器
    const char* fragmentShaderSource = R"(
        #version 410 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        uniform float uFlash;
        void main() {
            vec4 color = texture(uTexture, TexCoord);
            // 添加闪光效果
            color.rgb = mix(color.rgb, vec3(1.0), uFlash);
            FragColor = color;
        }
    )";

    // 编译顶点着色器
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "[Renderer] Vertex shader error: " << infoLog << "\n";
        return false;
    }

    // 编译片段着色器
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "[Renderer] Fragment shader error: " << infoLog << "\n";
        return false;
    }

    // 链接着色器程序
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_shaderProgram, 512, nullptr, infoLog);
        std::cerr << "[Renderer] Shader program link error: " << infoLog << "\n";
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // 创建全屏四边形顶点数据
    // 位置 (x, y) + 纹理坐标 (u, v)
    float vertices[] = {
        // 位置          // 纹理坐标（翻转 Y 轴，镜像 X 轴）
        -1.0f,  1.0f,   1.0f, 0.0f,  // 左上
        -1.0f, -1.0f,   1.0f, 1.0f,  // 左下
         1.0f, -1.0f,   0.0f, 1.0f,  // 右下

        -1.0f,  1.0f,   1.0f, 0.0f,  // 左上
         1.0f, -1.0f,   0.0f, 1.0f,  // 右下
         1.0f,  1.0f,   0.0f, 0.0f,  // 右上
    };

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

bool Renderer::initVideoTexture() {
    glGenTextures(1, &m_videoTexture);
    glBindTexture(GL_TEXTURE_2D, m_videoTexture);

    // 设置纹理参数
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

bool Renderer::initPrimitiveShader() {
    // 顶点着色器 - 支持 transform
    const char* vertexShaderSource = R"(
        #version 410 core
        layout (location = 0) in vec2 aPos;
        uniform vec2 uPosition;
        uniform vec2 uScale;
        void main() {
            vec2 pos = aPos * uScale + uPosition;
            gl_Position = vec4(pos, 0.0, 1.0);
        }
    )";

    // 片段着色器 - 纯色
    const char* fragmentShaderSource = R"(
        #version 410 core
        out vec4 FragColor;
        uniform vec4 uColor;
        void main() {
            FragColor = uColor;
        }
    )";

    // 编译顶点着色器
    uint32_t vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    int success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "[Renderer] Primitive vertex shader error: " << infoLog << "\n";
        return false;
    }

    // 编译片段着色器
    uint32_t fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "[Renderer] Primitive fragment shader error: " << infoLog << "\n";
        return false;
    }

    // 链接着色器程序
    m_primitiveShader = glCreateProgram();
    glAttachShader(m_primitiveShader, vertexShader);
    glAttachShader(m_primitiveShader, fragmentShader);
    glLinkProgram(m_primitiveShader);

    glGetProgramiv(m_primitiveShader, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_primitiveShader, 512, nullptr, infoLog);
        std::cerr << "[Renderer] Primitive shader link error: " << infoLog << "\n";
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

bool Renderer::initCircleGeometry() {
    // 生成圆形顶点（扇形）
    std::vector<float> vertices;
    vertices.reserve((CIRCLE_SEGMENTS + 2) * 2);

    // 中心点
    vertices.push_back(0.0f);
    vertices.push_back(0.0f);

    // 圆周点
    for (int i = 0; i <= CIRCLE_SEGMENTS; ++i) {
        float angle = 2.0f * static_cast<float>(M_PI) * i / CIRCLE_SEGMENTS;
        vertices.push_back(std::cos(angle));
        vertices.push_back(std::sin(angle));
    }

    glGenVertexArrays(1, &m_circleVao);
    glGenBuffers(1, &m_circleVbo);

    glBindVertexArray(m_circleVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_circleVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    return true;
}

bool Renderer::initRectGeometry() {
    // 矩形顶点（两个三角形）
    float vertices[] = {
        0.0f, 0.0f,  // 左下
        1.0f, 0.0f,  // 右下
        1.0f, 1.0f,  // 右上

        0.0f, 0.0f,  // 左下
        1.0f, 1.0f,  // 右上
        0.0f, 1.0f,  // 左上
    };

    glGenVertexArrays(1, &m_rectVao);
    glGenBuffers(1, &m_rectVbo);

    glBindVertexArray(m_rectVao);
    glBindBuffer(GL_ARRAY_BUFFER, m_rectVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    return true;
}

void Renderer::drawCircle(float cx, float cy, float radius, float r, float g, float b, float a) {
    glUseProgram(m_primitiveShader);

    // 屏幕坐标转 NDC（应用震屏偏移）
    float ndcX = screenToNDCX(cx + m_shakeOffsetX);
    float ndcY = screenToNDCY(cy + m_shakeOffsetY);
    float scaleX = radius / m_width * 2.0f;
    float scaleY = radius / m_height * 2.0f;

    glUniform2f(glGetUniformLocation(m_primitiveShader, "uPosition"), ndcX, ndcY);
    glUniform2f(glGetUniformLocation(m_primitiveShader, "uScale"), scaleX, scaleY);
    glUniform4f(glGetUniformLocation(m_primitiveShader, "uColor"), r, g, b, a);

    glBindVertexArray(m_circleVao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS + 2);
    glBindVertexArray(0);
}

void Renderer::drawRing(float cx, float cy, float innerRadius, float outerRadius, float r, float g, float b, float a) {
    // 绘制外圈
    drawCircle(cx, cy, outerRadius, r, g, b, a);
    // 用背景色（黑色透明）绘制内圈来"挖洞"效果不理想
    // 这里简化为绘制多个同心圆
    int rings = 3;
    for (int i = 0; i < rings; i++) {
        float t = static_cast<float>(i) / rings;
        float radius = innerRadius + (outerRadius - innerRadius) * t;
        drawCircle(cx, cy, radius, r, g, b, a * 0.3f);
    }
}

void Renderer::drawRect(float x, float y, float width, float height, float r, float g, float b, float a) {
    glUseProgram(m_primitiveShader);

    // 转换为 NDC（应用震屏偏移）
    float ndcX = screenToNDCX(x + m_shakeOffsetX);
    float ndcY = screenToNDCY(y + height + m_shakeOffsetY);  // Y 轴翻转
    float scaleX = width / m_width * 2.0f;
    float scaleY = height / m_height * 2.0f;

    glUniform2f(glGetUniformLocation(m_primitiveShader, "uPosition"), ndcX, ndcY);
    glUniform2f(glGetUniformLocation(m_primitiveShader, "uScale"), scaleX, scaleY);
    glUniform4f(glGetUniformLocation(m_primitiveShader, "uColor"), r, g, b, a);

    glBindVertexArray(m_rectVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::drawGradientRect(float x, float y, float width, float height, uint32_t color, float alpha) {
    float r, g, b;
    colorToRGB(color, r, g, b);
    drawRect(x, y, width, height, r, g, b, alpha);
}

void Renderer::beginFrame() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    // 帧结束处理
}

void Renderer::updateVideoTexture(const cv::Mat& frame) {
    if (frame.empty()) return;

    glBindTexture(GL_TEXTURE_2D, m_videoTexture);

    // OpenCV 默认是 BGR，转换为 RGB
    cv::Mat rgbFrame;
    cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
                 rgbFrame.cols, rgbFrame.rows, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, rgbFrame.data);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::renderVideoBackground() {
    glUseProgram(m_shaderProgram);

    // 设置震屏偏移
    float offsetX = m_shakeOffsetX / m_width * 2.0f;
    float offsetY = -m_shakeOffsetY / m_height * 2.0f;
    glUniform2f(glGetUniformLocation(m_shaderProgram, "uOffset"), offsetX, offsetY);

    // 设置闪光强度
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uFlash"), m_flashIntensity);

    glBindTexture(GL_TEXTURE_2D, m_videoTexture);
    glBindVertexArray(m_vao);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::renderZones() {
    // 计算区域边界（注意：摄像头画面是镜像的）
    // 从用户视角：左边是P1，右边是P2
    // 但由于镜像，实际渲染坐标需要反转

    float p1Width = m_width * GameSettings::ZONE_P1;
    float sharedWidth = m_width * GameSettings::ZONE_SHARED;
    float p2Width = m_width * GameSettings::ZONE_P2;

    // 由于镜像，从渲染坐标来看：
    // 左边（x=0）对应用户右边 = P2
    // 右边（x=width）对应用户左边 = P1

    // P2 区域（渲染左侧 = 用户右侧）
    drawGradientRect(0, 0, p2Width, m_height, Colors::P2, 0.1f);

    // 共享区
    drawGradientRect(p2Width, 0, sharedWidth, m_height, Colors::Shared, 0.15f);

    // P1 区域（渲染右侧 = 用户左侧）
    drawGradientRect(p2Width + sharedWidth, 0, p1Width, m_height, Colors::P1, 0.1f);

    // 绘制分隔线
    float lineWidth = 4.0f;

    // P2/共享 分隔线
    float r, g, b;
    colorToRGB(Colors::P2, r, g, b);
    drawRect(p2Width - lineWidth/2, 0, lineWidth, m_height, r, g, b, 0.5f);
    drawRect(p2Width - 1, 0, 2, m_height, 1.0f, 1.0f, 1.0f, 0.8f);

    // 共享/P1 分隔线
    colorToRGB(Colors::P1, r, g, b);
    drawRect(p2Width + sharedWidth - lineWidth/2, 0, lineWidth, m_height, r, g, b, 0.5f);
    drawRect(p2Width + sharedWidth - 1, 0, 2, m_height, 1.0f, 1.0f, 1.0f, 0.8f);
}

void Renderer::renderFallingItem(const FallingItem& item) {
    if (!item.active) return;

    float r, g, b;
    item.getColorRGB(r, g, b);
    float radius = item.size / 2.0f;

    float alpha = item.captured ? item.captureAlpha : 1.0f;
    float scale = item.captured ? (1.0f + (1.0f - item.captureAlpha) * 0.5f) : 1.0f;
    float actualRadius = radius * scale;

    // 高价值物品外发光
    if (item.isHighValue() && !item.isBomb()) {
        drawCircle(item.x, item.y, actualRadius + 8, 1.0f, 0.84f, 0.0f, 0.3f * alpha);
    }

    // 阴影
    drawCircle(item.x + 2, item.y + 2, actualRadius, 0.0f, 0.0f, 0.0f, 0.3f * alpha);

    // 白色背景圈
    drawCircle(item.x, item.y, actualRadius, 1.0f, 1.0f, 1.0f, alpha);

    // 颜色内圈
    float innerRadius = actualRadius * 0.85f;
    float colorAlpha = item.isBomb() ? 0.9f : 0.7f;
    drawCircle(item.x, item.y, innerRadius, r, g, b, colorAlpha * alpha);

    // 边框
    if (item.isHighValue() && !item.isBomb()) {
        // 金色边框
        drawRing(item.x, item.y, actualRadius - 2, actualRadius, 1.0f, 0.84f, 0.0f, alpha);
    } else if (item.isBomb()) {
        // 红色警告边框
        drawRing(item.x, item.y, actualRadius - 2, actualRadius, 1.0f, 0.0f, 0.0f, alpha);
    }

    // 高亮点
    float highlightOffset = actualRadius * 0.3f;
    drawCircle(item.x - highlightOffset, item.y - highlightOffset,
               actualRadius * 0.15f, 1.0f, 1.0f, 1.0f, 0.6f * alpha);
}

void Renderer::renderHand(const HandPosition& hand, int playerId) {
    if (!hand.valid) return;

    renderCaptureZone(hand.x, hand.y, playerId,
                      GameSettings::CAPTURE_RADIUS,
                      GameSettings::PERFECT_CAPTURE_RADIUS);
}

void Renderer::renderCaptureZone(float x, float y, int playerId, float captureRadius, float perfectRadius) {
    float r, g, b;
    uint32_t color = (playerId == 0) ? Colors::P1 : Colors::P2;
    colorToRGB(color, r, g, b);

    // 外圈（捕获范围）- 半透明填充
    drawCircle(x, y, captureRadius, r, g, b, 0.05f);

    // 外圈边框
    drawRing(x, y, captureRadius - 3, captureRadius, r, g, b, 0.3f);

    // 完美捕获圈（金色）
    drawCircle(x, y, perfectRadius, 1.0f, 0.84f, 0.0f, 0.1f);
    drawRing(x, y, perfectRadius - 2, perfectRadius, 1.0f, 0.84f, 0.0f, 0.5f);

    // 中心点
    drawCircle(x, y, 8, r, g, b, 0.8f);
    drawCircle(x, y, 4, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Renderer::renderUI(int p1Score, int p2Score, float remainingTime, float fps, float detectionTime,
                        GamePhase phase, int p1Combo, int p2Combo) {
    // 顶部 HUD 背景
    drawRect(0, 0, m_width, GameSettings::HUD_HEIGHT, 0.0f, 0.0f, 0.0f, 0.6f);

    // P1 分数指示（左侧 - 蓝色）
    float p1ScoreRadius = 20.0f + std::min(p1Score / 5.0f, 40.0f);
    float r, g, b;
    colorToRGB(Colors::P1, r, g, b);
    drawCircle(100.0f, 40.0f, p1ScoreRadius, r, g, b, 0.8f);

    // P1 连击指示
    if (p1Combo > 1) {
        for (int i = 0; i < std::min(p1Combo, 10); i++) {
            drawCircle(60.0f + i * 8.0f, 65.0f, 3.0f, 1.0f, 0.84f, 0.0f, 0.8f);
        }
    }

    // P2 分数指示（右侧 - 红色）
    float p2ScoreRadius = 20.0f + std::min(p2Score / 5.0f, 40.0f);
    colorToRGB(Colors::P2, r, g, b);
    drawCircle(m_width - 100.0f, 40.0f, p2ScoreRadius, r, g, b, 0.8f);

    // P2 连击指示
    if (p2Combo > 1) {
        for (int i = 0; i < std::min(p2Combo, 10); i++) {
            drawCircle(m_width - 60.0f - i * 8.0f, 65.0f, 3.0f, 1.0f, 0.84f, 0.0f, 0.8f);
        }
    }

    // 时间指示（中央）
    float timeProgress = remainingTime / GameSettings::GAME_DURATION;
    float timeBarWidth = 300.0f;
    float timeBarHeight = 20.0f;
    float timeBarX = (m_width - timeBarWidth) / 2.0f;
    float timeBarY = 30.0f;

    // 时间条背景
    drawRect(timeBarX, timeBarY, timeBarWidth, timeBarHeight, 0.3f, 0.3f, 0.3f, 0.5f);

    // 时间条进度（根据剩余时间变色）
    float timeR = (timeProgress < 0.3f) ? 1.0f : 0.2f;
    float timeG = (timeProgress > 0.3f) ? 0.8f : 0.2f;
    drawRect(timeBarX, timeBarY, timeBarWidth * timeProgress, timeBarHeight, timeR, timeG, 0.2f, 0.8f);

    // 阶段指示
    float phaseX = m_width / 2.0f;
    float phaseY = 60.0f;
    float phaseRadius = 8.0f;

    // 三个阶段点
    for (int i = 0; i < 3; i++) {
        float px = phaseX - 30.0f + i * 30.0f;
        bool active = (i == static_cast<int>(phase));
        if (active) {
            colorToRGB(Colors::Shared, r, g, b);
            drawCircle(px, phaseY, phaseRadius, r, g, b, 1.0f);
        } else {
            drawCircle(px, phaseY, phaseRadius * 0.6f, 0.5f, 0.5f, 0.5f, 0.5f);
        }
    }

    // FPS 和检测时间（右上角小字，用小圆点表示）
    float fpsIndicator = std::min(fps / 60.0f, 1.0f);
    drawCircle(m_width - 30.0f, 20.0f, 5.0f + fpsIndicator * 5.0f, 0.0f, 1.0f, 0.0f, 0.6f);
}

void Renderer::renderGameStateHint(const std::string& hint) {
    // 由于没有文字渲染，用图形提示
    // 中央大圆圈脉冲效果
    float pulse = 0.5f + 0.5f * std::sin(m_currentTime * 3.0f);
    float radius = 100.0f + pulse * 20.0f;

    drawCircle(m_width / 2.0f, m_height / 2.0f, radius, 1.0f, 1.0f, 1.0f, 0.3f * pulse);
    drawCircle(m_width / 2.0f, m_height / 2.0f, radius * 0.7f, 0.2f, 0.8f, 0.3f, 0.5f * pulse);
}

void Renderer::showScorePopup(float x, float y, int score, bool isPerfect) {
    ScorePopup popup;
    popup.x = x;
    popup.y = y;
    popup.score = score;
    popup.startTime = m_currentTime;
    popup.duration = 1.0f;

    if (score < 0) {
        popup.color = Colors::Bomb;
    } else if (isPerfect) {
        popup.color = Colors::Shared;  // 金色
    } else {
        popup.color = Colors::White;
    }

    m_scorePopups.push_back(popup);

    // 触发粒子效果
    if (m_particleSystem) {
        if (score < 0) {
            m_particleSystem->createBombExplosion(x, y);
        } else {
            m_particleSystem->createCaptureExplosion(x, y, isPerfect);
        }
        m_particleSystem->createScorePopup(x, y, score, popup.color);
    }
}

void Renderer::updateAnimations(float deltaTime) {
    m_currentTime += deltaTime;
    updateScorePopups(deltaTime);
    updateScreenShake(deltaTime);
    updateFlash(deltaTime);

    if (m_particleSystem) {
        m_particleSystem->update(deltaTime);
    }
}

void Renderer::updateScorePopups(float deltaTime) {
    m_scorePopups.erase(
        std::remove_if(m_scorePopups.begin(), m_scorePopups.end(),
            [this](const ScorePopup& p) {
                return (m_currentTime - p.startTime) > p.duration;
            }),
        m_scorePopups.end()
    );
}

void Renderer::renderScorePopups() {
    for (const auto& popup : m_scorePopups) {
        float progress = (m_currentTime - popup.startTime) / popup.duration;
        float alpha = 1.0f - progress;
        float yOffset = progress * 80.0f;

        float r, g, b;
        colorToRGB(popup.color, r, g, b);

        // 分数大小根据数值变化
        float radius = 15.0f + std::abs(popup.score) / 10.0f;
        radius *= (1.0f + (1.0f - progress) * 0.3f);  // 初始放大效果

        drawCircle(popup.x, popup.y - yOffset, radius, r, g, b, alpha * 0.8f);
    }
}

void Renderer::triggerScreenShake(float intensity, float duration) {
    m_shakeIntensity = intensity;
    m_shakeDuration = duration;
    m_shakeTime = 0.0f;
}

void Renderer::updateScreenShake(float deltaTime) {
    if (m_shakeDuration <= 0) {
        m_shakeOffsetX = 0;
        m_shakeOffsetY = 0;
        return;
    }

    m_shakeTime += deltaTime;
    if (m_shakeTime >= m_shakeDuration) {
        m_shakeDuration = 0;
        m_shakeOffsetX = 0;
        m_shakeOffsetY = 0;
        return;
    }

    float progress = m_shakeTime / m_shakeDuration;
    float currentIntensity = m_shakeIntensity * (1.0f - progress);

    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    m_shakeOffsetX = dist(s_rng) * currentIntensity;
    m_shakeOffsetY = dist(s_rng) * currentIntensity;
}

void Renderer::triggerFlash(float intensity) {
    m_flashIntensity = intensity;
}

void Renderer::updateFlash(float deltaTime) {
    if (m_flashIntensity > 0) {
        m_flashIntensity = std::max(0.0f, m_flashIntensity - deltaTime * 3.0f);
    }
}

} // namespace popcorn
