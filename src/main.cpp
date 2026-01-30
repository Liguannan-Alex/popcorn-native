/**
 * Popcorn Battle - Native C++ Version
 *
 * 跨平台体感游戏：Mac 开发，Windows 运行
 * 技术栈：SDL2 + OpenGL + OpenCV + MediaPipe
 */

#include <iostream>
#include <memory>
#include "core/Application.h"

int main(int argc, char* argv[]) {
    std::cout << "========================================\n";
    std::cout << "  Popcorn Battle - Native C++ Version\n";
    std::cout << "========================================\n";

    try {
        // 创建应用实例
        auto app = std::make_unique<popcorn::Application>();

        // 初始化
        if (!app->initialize(1920, 1080, "爆米花大作战")) {
            std::cerr << "Failed to initialize application\n";
            return -1;
        }

        // 运行主循环
        app->run();

        // 清理
        app->shutdown();

    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "Application exited normally.\n";
    return 0;
}
