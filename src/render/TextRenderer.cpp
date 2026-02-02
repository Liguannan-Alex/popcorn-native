#include "TextRenderer.h"
#include <iostream>

#ifdef HAS_SDL_TTF
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#endif

namespace popcorn {

TextRenderer::TextRenderer() = default;

TextRenderer::~TextRenderer() {
    shutdown();
}

bool TextRenderer::initialize(SDL_Renderer* renderer) {
#ifdef HAS_SDL_TTF
    if (TTF_Init() < 0) {
        std::cerr << "[TextRenderer] TTF_Init failed: " << TTF_GetError() << "\n";
        return false;
    }

    m_renderer = renderer;
    m_initialized = true;
    std::cout << "[TextRenderer] Initialized successfully\n";
    return true;
#else
    std::cout << "[TextRenderer] SDL_ttf not available, text rendering disabled\n";
    m_renderer = renderer;
    m_initialized = true;
    return true;
#endif
}

void TextRenderer::shutdown() {
#ifdef HAS_SDL_TTF
    for (auto& [name, font] : m_fonts) {
        if (font) {
            TTF_CloseFont(font);
        }
    }
    m_fonts.clear();

    if (m_initialized) {
        TTF_Quit();
    }
#endif
    m_initialized = false;
    std::cout << "[TextRenderer] Shutdown complete\n";
}

bool TextRenderer::loadFont(const std::string& name, const std::string& path, int size) {
#ifdef HAS_SDL_TTF
    if (!m_initialized) {
        std::cerr << "[TextRenderer] Not initialized\n";
        return false;
    }

    TTF_Font* font = TTF_OpenFont(path.c_str(), size);
    if (!font) {
        std::cerr << "[TextRenderer] Failed to load font " << path << ": " << TTF_GetError() << "\n";
        return false;
    }

    // 如果已存在同名字体，先释放
    auto it = m_fonts.find(name);
    if (it != m_fonts.end() && it->second) {
        TTF_CloseFont(it->second);
    }

    m_fonts[name] = font;
    std::cout << "[TextRenderer] Loaded font: " << name << " (" << path << ", size " << size << ")\n";
    return true;
#else
    return false;
#endif
}

void TextRenderer::renderText(const std::string& text, int x, int y,
                              const std::string& fontName,
                              uint8_t r, uint8_t g, uint8_t b,
                              TextAlign align) {
#ifdef HAS_SDL_TTF
    if (!m_initialized || !m_renderer) return;

    auto it = m_fonts.find(fontName);
    if (it == m_fonts.end() || !it->second) {
        // 没有找到字体，尝试使用默认字体
        it = m_fonts.find("default");
        if (it == m_fonts.end() || !it->second) {
            return;
        }
    }

    TTF_Font* font = it->second;
    SDL_Color color = {r, g, b, 255};

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);
    if (!surface) {
        std::cerr << "[TextRenderer] Failed to render text: " << TTF_GetError() << "\n";
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(m_renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return;
    }

    // 计算位置
    int renderX = x;
    switch (align) {
        case TextAlign::Center:
            renderX = x - surface->w / 2;
            break;
        case TextAlign::Right:
            renderX = x - surface->w;
            break;
        default:
            break;
    }

    SDL_Rect dstRect = {renderX, y, surface->w, surface->h};
    SDL_RenderCopy(m_renderer, texture, nullptr, &dstRect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
#endif
}

void TextRenderer::renderTextWithOutline(const std::string& text, int x, int y,
                                         const std::string& fontName,
                                         uint8_t r, uint8_t g, uint8_t b,
                                         uint8_t outlineR, uint8_t outlineG, uint8_t outlineB,
                                         int outlineWidth,
                                         TextAlign align) {
#ifdef HAS_SDL_TTF
    // 先渲染描边（在四个方向偏移）
    for (int dx = -outlineWidth; dx <= outlineWidth; dx++) {
        for (int dy = -outlineWidth; dy <= outlineWidth; dy++) {
            if (dx == 0 && dy == 0) continue;
            renderText(text, x + dx, y + dy, fontName, outlineR, outlineG, outlineB, align);
        }
    }
    // 再渲染主文本
    renderText(text, x, y, fontName, r, g, b, align);
#endif
}

void TextRenderer::getTextSize(const std::string& text, const std::string& fontName, int& width, int& height) {
    width = 0;
    height = 0;

#ifdef HAS_SDL_TTF
    if (!m_initialized) return;

    auto it = m_fonts.find(fontName);
    if (it == m_fonts.end() || !it->second) {
        it = m_fonts.find("default");
        if (it == m_fonts.end() || !it->second) {
            return;
        }
    }

    TTF_SizeUTF8(it->second, text.c_str(), &width, &height);
#endif
}

bool TextRenderer::hasFont(const std::string& name) const {
    return m_fonts.find(name) != m_fonts.end();
}

} // namespace popcorn
