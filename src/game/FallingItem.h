#pragma once

#include <string>
#include "GameConfig.h"

namespace popcorn {

/**
 * 掉落物
 */
struct FallingItem {
    int id{0};
    ItemType type{ItemType::Popcorn};

    float x{0.0f};          // 位置 X
    float y{0.0f};          // 位置 Y
    float size{65.0f};      // 尺寸
    float speed{400.0f};    // 下落速度（像素/秒）

    float rotation{0.0f};       // 旋转角度（度）
    float rotationSpeed{0.0f};  // 旋转速度（度/秒）

    bool active{true};      // 是否激活
    bool captured{false};   // 是否已被捕获
    float captureAlpha{1.0f}; // 捕获动画透明度

    uint32_t color{Colors::Popcorn};  // 颜色
    std::string emoji{"🍿"};           // Emoji

    // 从配置初始化
    void initFromConfig(ItemType itemType) {
        type = itemType;
        auto it = ITEM_CONFIGS.find(itemType);
        if (it != ITEM_CONFIGS.end()) {
            const auto& config = it->second;
            size = config.size;
            color = config.color;
            emoji = config.emoji;
        }
    }

    // 获取分数
    int getScore() const {
        auto it = ITEM_CONFIGS.find(type);
        if (it != ITEM_CONFIGS.end()) {
            return it->second.score;
        }
        return 0;
    }

    // 是否为炸弹
    bool isBomb() const {
        return type == ItemType::Bomb;
    }

    // 是否为高价值物品（50分以上）
    bool isHighValue() const {
        return getScore() >= 50;
    }

    // 获取颜色分量（0-1范围）
    void getColorRGB(float& r, float& g, float& b) const {
        r = ((color >> 16) & 0xFF) / 255.0f;
        g = ((color >> 8) & 0xFF) / 255.0f;
        b = (color & 0xFF) / 255.0f;
    }

    // 获取颜色分量（0-255范围）
    void getColorRGB255(uint8_t& r, uint8_t& g, uint8_t& b) const {
        r = (color >> 16) & 0xFF;
        g = (color >> 8) & 0xFF;
        b = color & 0xFF;
    }
};

} // namespace popcorn
