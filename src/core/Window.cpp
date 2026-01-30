#include "Window.h"

#include <SDL2/SDL.h>
#include <iostream>

// OpenGL 头文件（跨平台）
#ifdef __APPLE__
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
#elif defined(_WIN32)
    #include <GL/glew.h>
#else
    #include <GL/gl.h>
#endif

namespace popcorn {

Window::Window() = default;

Window::~Window() {
    destroy();
}

bool Window::create(int width, int height, const std::string& title) {
    // 初始化 SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cerr << "[Window] SDL_Init failed: " << SDL_GetError() << "\n";
        return false;
    }

    // 设置 OpenGL 属性
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);  // Mac 最高支持 4.1
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

    // 创建窗口
    m_window = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!m_window) {
        std::cerr << "[Window] SDL_CreateWindow failed: " << SDL_GetError() << "\n";
        return false;
    }

    // 创建 OpenGL 上下文
    m_glContext = SDL_GL_CreateContext(m_window);
    if (!m_glContext) {
        std::cerr << "[Window] SDL_GL_CreateContext failed: " << SDL_GetError() << "\n";
        return false;
    }

    // 启用 VSync
    if (SDL_GL_SetSwapInterval(1) < 0) {
        std::cerr << "[Window] Warning: Unable to set VSync: " << SDL_GetError() << "\n";
    }

#ifdef _WIN32
    // Windows: 初始化 GLEW
    glewExperimental = GL_TRUE;
    GLenum glewErr = glewInit();
    if (glewErr != GLEW_OK) {
        std::cerr << "[Window] GLEW init failed: " << glewGetErrorString(glewErr) << "\n";
        return false;
    }
#endif

    m_width = width;
    m_height = height;
    m_shouldClose = false;

    std::cout << "[Window] Created " << width << "x" << height << " window\n";
    std::cout << "[Window] OpenGL version: " << glGetString(GL_VERSION) << "\n";

    return true;
}

void Window::destroy() {
    if (m_glContext) {
        SDL_GL_DeleteContext(m_glContext);
        m_glContext = nullptr;
    }

    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }

    SDL_Quit();
}

void Window::pollEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                m_shouldClose = true;
                break;

            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    m_shouldClose = true;
                }
                // F11 切换全屏
                if (event.key.keysym.sym == SDLK_F11) {
                    Uint32 flags = SDL_GetWindowFlags(m_window);
                    if (flags & SDL_WINDOW_FULLSCREEN_DESKTOP) {
                        SDL_SetWindowFullscreen(m_window, 0);
                    } else {
                        SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                    }
                }
                break;

            case SDL_WINDOWEVENT:
                if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
                    m_shouldClose = true;
                }
                if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                    m_width = event.window.data1;
                    m_height = event.window.data2;
                    std::cout << "[Window] Resized to " << m_width << "x" << m_height << "\n";
                }
                break;
        }
    }
}

void Window::swapBuffers() {
    if (m_window) {
        SDL_GL_SwapWindow(m_window);
    }
}

} // namespace popcorn
