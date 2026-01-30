#pragma once

#include <string>

// 前向声明 SDL 类型
struct SDL_Window;
typedef void* SDL_GLContext;

namespace popcorn {

/**
 * 窗口管理类
 * 封装 SDL2 窗口和 OpenGL 上下文
 */
class Window {
public:
    Window();
    ~Window();

    /**
     * 创建窗口
     * @param width 宽度
     * @param height 高度
     * @param title 标题
     * @return 成功返回 true
     */
    bool create(int width, int height, const std::string& title);

    /**
     * 销毁窗口
     */
    void destroy();

    /**
     * 处理事件
     */
    void pollEvents();

    /**
     * 交换缓冲区
     */
    void swapBuffers();

    /**
     * 是否应该关闭
     */
    bool shouldClose() const { return m_shouldClose; }

    /**
     * 获取窗口尺寸
     */
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

    /**
     * 获取 SDL 窗口指针
     */
    SDL_Window* getSDLWindow() { return m_window; }

private:
    SDL_Window* m_window{nullptr};
    SDL_GLContext m_glContext{nullptr};

    int m_width{0};
    int m_height{0};
    bool m_shouldClose{false};
};

} // namespace popcorn
