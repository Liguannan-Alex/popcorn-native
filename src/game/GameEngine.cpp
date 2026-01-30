#include "GameEngine.h"
#include <iostream>
#include <random>
#include <algorithm>

namespace popcorn {

GameEngine::GameEngine() = default;

GameEngine::~GameEngine() = default;

bool GameEngine::initialize(int width, int height) {
    m_width = width;
    m_height = height;

    m_collisionSystem = std::make_unique<CollisionSystem>();
    m_collisionSystem->setHandRadius(50.0f);

    std::cout << "[GameEngine] Initialized with size " << width << "x" << height << "\n";
    return true;
}

void GameEngine::update(float deltaTime, const std::vector<DetectedPerson>& persons) {
    // 保存检测到的人物
    m_detectedPersons = persons;

    switch (m_state) {
        case GameState::Calibrating:
            // 等待检测到玩家
            if (!persons.empty()) {
                // 检测到玩家，可以开始游戏
                std::cout << "[GameEngine] Player detected, ready to start\n";
            }
            break;

        case GameState::Countdown:
            // 倒计时逻辑（由外部控制）
            break;

        case GameState::Playing:
            // 更新游戏时间
            m_remainingTime -= deltaTime;
            if (m_remainingTime <= 0) {
                m_remainingTime = 0;
                m_state = GameState::GameOver;
                std::cout << "[GameEngine] Game Over! Final score: " << m_score << "\n";
                break;
            }

            // 生成掉落物
            m_spawnTimer += deltaTime;
            if (m_spawnTimer >= m_spawnInterval) {
                spawnItem();
                m_spawnTimer = 0;
            }

            // 更新掉落物位置
            updateItems(deltaTime);

            // 碰撞检测
            if (m_collisionSystem && !persons.empty()) {
                auto collisions = m_collisionSystem->detectCollisions(m_fallingItems, persons);
                for (const auto& collision : collisions) {
                    m_score += collision.scoreChange;
                    // 可以在这里添加音效、特效触发
                    std::cout << "[GameEngine] Collision! Score change: " << collision.scoreChange
                              << ", Total: " << m_score << "\n";
                }
            }

            // 移除失效物品
            removeOffscreenItems();
            break;

        case GameState::Paused:
            // 暂停状态，不更新
            break;

        case GameState::GameOver:
            // 游戏结束
            break;
    }
}

void GameEngine::startGame() {
    if (m_state == GameState::Calibrating || m_state == GameState::GameOver) {
        reset();
        m_state = GameState::Playing;
        std::cout << "[GameEngine] Game started!\n";
    }
}

void GameEngine::togglePause() {
    if (m_state == GameState::Playing) {
        m_state = GameState::Paused;
        std::cout << "[GameEngine] Game paused\n";
    } else if (m_state == GameState::Paused) {
        m_state = GameState::Playing;
        std::cout << "[GameEngine] Game resumed\n";
    }
}

void GameEngine::reset() {
    m_score = 0;
    m_remainingTime = 60.0f;
    m_spawnTimer = 0;
    m_fallingItems.clear();
    m_nextItemId = 0;
    m_state = GameState::Calibrating;
    std::cout << "[GameEngine] Game reset\n";
}

void GameEngine::spawnItem() {
    // 随机数生成器
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> xDist(0.1f, 0.9f);
    static std::uniform_real_distribution<float> speedDist(150.0f, 300.0f);
    static std::uniform_real_distribution<float> rotSpeedDist(-180.0f, 180.0f);
    static std::uniform_int_distribution<int> typeDist(0, 100);

    FallingItem item;
    item.id = m_nextItemId++;
    item.x = xDist(gen) * m_width;
    item.y = -50.0f;  // 从屏幕顶部上方开始
    item.speed = speedDist(gen);
    item.rotationSpeed = rotSpeedDist(gen);

    // 随机类型：70% 爆米花，20% 炸弹，10% 特殊
    int typeRoll = typeDist(gen);
    if (typeRoll < 70) {
        item.type = ItemType::Popcorn;
        item.size = 40.0f;
    } else if (typeRoll < 90) {
        item.type = ItemType::Bomb;
        item.size = 45.0f;
    } else {
        item.type = ItemType::Special;
        item.size = 35.0f;
    }

    m_fallingItems.push_back(item);
}

void GameEngine::updateItems(float deltaTime) {
    for (auto& item : m_fallingItems) {
        if (!item.active) continue;

        // 更新位置
        item.y += item.speed * deltaTime;

        // 更新旋转
        item.rotation += item.rotationSpeed * deltaTime;

        // 保持旋转在 0-360 范围
        while (item.rotation >= 360.0f) item.rotation -= 360.0f;
        while (item.rotation < 0.0f) item.rotation += 360.0f;
    }
}

void GameEngine::removeOffscreenItems() {
    // 移除超出屏幕底部的物品
    m_fallingItems.erase(
        std::remove_if(m_fallingItems.begin(), m_fallingItems.end(),
            [this](const FallingItem& item) {
                return !item.active || item.y > m_height + 100;
            }),
        m_fallingItems.end()
    );
}

} // namespace popcorn
