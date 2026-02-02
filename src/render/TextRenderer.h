#pragma once

#include <string>
#include <unordered_map>
#include <memory>

// 前向声明
struct SDL_Renderer;
struct _TTF_Font;
typedef struct _TTF_Font TTF_Font;
struct SDL_Texture;

namespace popcorn {

/**
 * 文本对齐方式
 */
enum class TextAlign {
    Left,
    Center,
    Right
};

/**
 * 文本渲染器
 * 使用 SDL_ttf 渲染文本
 */
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    /**
     * 初始化
     * @param renderer SDL 渲染器
     * @return 成功返回 true
     */
    bool initialize(SDL_Renderer* renderer);

    /**
     * 关闭
     */
    void shutdown();

    /**
     * 加载字体
     * @param name 字体名称（用于后续引用）
     * @param path 字体文件路径
     * @param size 字体大小
     * @return 成功返回 true
     */
    bool loadFont(const std::string& name, const std::string& path, int size);

    /**
     * 渲染文本
     * @param text 文本内容
     * @param x X 坐标
     * @param y Y 坐标
     * @param fontName 字体名称
     * @param r, g, b 颜色（0-255）
     * @param align 对齐方式
     */
    void renderText(const std::string& text, int x, int y,
                    const std::string& fontName = "default",
                    uint8_t r = 255, uint8_t g = 255, uint8_t b = 255,
                    TextAlign align = TextAlign::Left);

    /**
     * 渲染带描边的文本
     */
    void renderTextWithOutline(const std::string& text, int x, int y,
                               const std::string& fontName,
                               uint8_t r, uint8_t g, uint8_t b,
                               uint8_t outlineR, uint8_t outlineG, uint8_t outlineB,
                               int outlineWidth = 2,
                               TextAlign align = TextAlign::Left);

    /**
     * 获取文本尺寸
     */
    void getTextSize(const std::string& text, const std::string& fontName, int& width, int& height);

    /**
     * 检查字体是否已加载
     */
    bool hasFont(const std::string& name) const;

private:
    SDL_Renderer* m_renderer{nullptr};
    std::unordered_map<std::string, TTF_Font*> m_fonts;
    bool m_initialized{false};
};

} // namespace popcorn
