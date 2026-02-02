#include "Renderer.h"

// Windows MSVC 需要此宏才能使用 M_PI
#define _USE_MATH_DEFINES
#include <cmath>

#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>

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

    std::cout << "[Renderer] Initialized " << width << "x" << height << "\n";
    return true;
}

void Renderer::shutdown() {
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
}

bool Renderer::initShaders() {
    // 顶点着色器
    const char* vertexShaderSource = R"(
        #version 410 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        out vec2 TexCoord;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    // 片段着色器
    const char* fragmentShaderSource = R"(
        #version 410 core
        in vec2 TexCoord;
        out vec4 FragColor;
        uniform sampler2D uTexture;
        void main() {
            FragColor = texture(uTexture, TexCoord);
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
        float angle = 2.0f * M_PI * i / CIRCLE_SEGMENTS;
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

void Renderer::drawCircle(float cx, float cy, float radius, float r, float g, float b, float a) {
    glUseProgram(m_primitiveShader);

    // 屏幕坐标转 NDC
    float ndcX = screenToNDCX(cx);
    float ndcY = screenToNDCY(cy);
    float scaleX = radius / m_width * 2.0f;
    float scaleY = radius / m_height * 2.0f;

    glUniform2f(glGetUniformLocation(m_primitiveShader, "uPosition"), ndcX, ndcY);
    glUniform2f(glGetUniformLocation(m_primitiveShader, "uScale"), scaleX, scaleY);
    glUniform4f(glGetUniformLocation(m_primitiveShader, "uColor"), r, g, b, a);

    glBindVertexArray(m_circleVao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGMENTS + 2);
    glBindVertexArray(0);
}

void Renderer::beginFrame() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endFrame() {
    // 帧结束处理（如果需要）
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
    glBindTexture(GL_TEXTURE_2D, m_videoTexture);
    glBindVertexArray(m_vao);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::renderFallingItem(const FallingItem& item) {
    if (!item.active) return;

    float r, g, b;
    item.getColor(r, g, b);

    // 渲染主圆形
    drawCircle(item.x, item.y, item.size / 2.0f, r, g, b, 1.0f);

    // 渲染高亮（偏移小圆）
    float highlightOffset = item.size * 0.15f;
    drawCircle(item.x - highlightOffset, item.y - highlightOffset,
               item.size * 0.15f, 1.0f, 1.0f, 1.0f, 0.5f);
}

void Renderer::renderHand(const HandPosition& hand) {
    if (!hand.valid) return;

    // 外圈（半透明白色）
    drawCircle(hand.x, hand.y, 60.0f, 1.0f, 1.0f, 1.0f, 0.3f);

    // 内圈（实心绿色）
    drawCircle(hand.x, hand.y, 40.0f, 0.2f, 0.9f, 0.4f, 0.8f);

    // 中心点
    drawCircle(hand.x, hand.y, 10.0f, 1.0f, 1.0f, 1.0f, 1.0f);
}

void Renderer::renderUI(int score, float remainingTime, float fps, float detectionTime) {
    // 注意：纯 OpenGL 渲染文字较复杂，需要字体纹理或 ImGui
    // 当前仅在控制台输出，后续可集成 ImGui 或 freetype

    // 绘制顶部半透明条
    // TODO: 需要矩形绘制方法

    // 临时方案：用圆形作为状态指示
    // 左上角：分数指示（绿色圆形大小表示分数）
    float scoreRadius = 20.0f + (score / 10.0f);
    if (scoreRadius > 100.0f) scoreRadius = 100.0f;
    drawCircle(80.0f, 60.0f, scoreRadius, 0.2f, 0.8f, 0.3f, 0.7f);

    // 右上角：时间指示（红色圆形大小表示剩余时间）
    float timeRadius = remainingTime;
    if (timeRadius > 60.0f) timeRadius = 60.0f;
    drawCircle(m_width - 80.0f, 60.0f, timeRadius, 0.9f, 0.3f, 0.2f, 0.7f);
}

} // namespace popcorn
