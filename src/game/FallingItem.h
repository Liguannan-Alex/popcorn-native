#pragma once

#include <string>

namespace popcorn {

/**
 * 掉落物类型
 */
enum class ItemType {
    Popcorn,    // 爆米花（加分）
    Bomb,       // 炸弹（扣分）
    Special     // 特殊道具
};

/**
 * 掉落物
 */
struct FallingItem {
    int id{0};
    ItemType type{ItemType::Popcorn};

    float x{0.0f};          // 位置 X
    float y{0.0f};          // 位置 Y
    float size{40.0f};      // 尺寸
    float speed{200.0f};    // 下落速度（像素/秒）

    float rotation{0.0f};   // 旋转角度
    float rotationSpeed{0.0f}; // 旋转速度

    bool active{true};      // 是否激活

    // 根据类型获取颜色
    void getColor(float& r, float& g, float& b) const {
        switch (type) {
            case ItemType::Popcorn:
                r = 1.0f; g = 0.9f; b = 0.3f; // 黄色
                break;
            case ItemType::Bomb:
                r = 0.2f; g = 0.2f; b = 0.2f; // 黑色
                break;
            case ItemType::Special:
                r = 0.3f; g = 0.8f; b = 1.0f; // 蓝色
                break;
        }
    }

    // 获取分数
    int getScore() const {
        switch (type) {
            case ItemType::Popcorn: return 10;
            case ItemType::Bomb: return -20;
            case ItemType::Special: return 50;
            default: return 0;
        }
    }
};

} // namespace popcorn
