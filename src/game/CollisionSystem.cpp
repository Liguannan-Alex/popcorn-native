#include "CollisionSystem.h"
#include <cmath>

namespace popcorn {

CollisionSystem::CollisionSystem() = default;

std::vector<CollisionResult> CollisionSystem::detectCollisions(
    std::vector<FallingItem>& items,
    const std::vector<DetectedPerson>& persons
) {
    std::vector<CollisionResult> results;

    for (auto& item : items) {
        if (!item.active) continue;

        for (const auto& person : persons) {
            // 检测左手碰撞
            if (person.leftHand.valid &&
                checkHandItemCollision(person.leftHand, item)) {

                CollisionResult result;
                result.itemId = item.id;
                result.personId = person.id;
                result.isLeftHand = true;
                result.scoreChange = item.getScore();
                results.push_back(result);

                item.active = false;
                break;  // 一个物品只能被碰撞一次
            }

            // 检测右手碰撞
            if (person.rightHand.valid &&
                checkHandItemCollision(person.rightHand, item)) {

                CollisionResult result;
                result.itemId = item.id;
                result.personId = person.id;
                result.isLeftHand = false;
                result.scoreChange = item.getScore();
                results.push_back(result);

                item.active = false;
                break;
            }
        }
    }

    return results;
}

bool CollisionSystem::checkHandItemCollision(
    const HandPosition& hand,
    const FallingItem& item
) const {
    // 碰撞半径 = 手部半径 + 物品半径
    float collisionRadius = m_handRadius + item.size / 2.0f;
    float collisionRadiusSq = collisionRadius * collisionRadius;

    float distSq = distanceSquared(hand.x, hand.y, item.x, item.y);

    return distSq <= collisionRadiusSq;
}

float CollisionSystem::distanceSquared(float x1, float y1, float x2, float y2) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return dx * dx + dy * dy;
}

} // namespace popcorn
