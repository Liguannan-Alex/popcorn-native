#pragma once

#include <vector>
#include "FallingItem.h"
#include "../detection/PoseDetector.h"

namespace popcorn {

/**
 * 碰撞结果
 */
struct CollisionResult {
    int itemId{-1};
    int personId{-1};
    bool isLeftHand{false};
    int scoreChange{0};
};

/**
 * 碰撞检测系统
 */
class CollisionSystem {
public:
    CollisionSystem();
    ~CollisionSystem() = default;

    /**
     * 设置碰撞检测范围
     * @param handRadius 手部碰撞半径
     */
    void setHandRadius(float handRadius) { m_handRadius = handRadius; }

    /**
     * 检测碰撞
     * @param items 掉落物列表
     * @param persons 检测到的人物列表
     * @return 碰撞结果列表
     */
    std::vector<CollisionResult> detectCollisions(
        std::vector<FallingItem>& items,
        const std::vector<DetectedPerson>& persons
    );

private:
    /**
     * 检测单个手和物品的碰撞
     */
    bool checkHandItemCollision(
        const HandPosition& hand,
        const FallingItem& item
    ) const;

    /**
     * 计算两点距离的平方
     */
    float distanceSquared(float x1, float y1, float x2, float y2) const;

private:
    float m_handRadius{50.0f};  // 手部碰撞半径
};

} // namespace popcorn
