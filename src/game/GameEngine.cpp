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

void GameEngine::update(float deltaTime, const std::vector<DetectedPerson>& persons,
                        const GestureResult& gesture) {
    // 保存检测到的人物
    m_detectedPersons = persons;

    switch (m_state) {
        case GameState::Calibrating:
            // 等待检测到玩家并做出 OK 手势
            if (!persons.empty()) {
                // 检测到玩家
                if (gesture.anyOkGesture()) {
                    // 检测到 OK 手势，开始游戏！
                    std::cout << "[GameEngine] OK gesture detected! Starting game...\n";
                    startGame();
                } else {
                    // 显示提示：请做 OK 手势开始游戏
                    static int hintCounter = 0;
                    if (++hintCounter % 60 == 0) {  // 每秒提示一次
                        std::cout << "[GameEngine] Player detected. Show OK gesture (👌) to start!\n";
                    }
                }
            }
            break;

        case GameState::Countdown:
            // 倒计时逻辑（由外部控制）
            break;

        case GameState::Playing:
            // 更新游戏时间
            m_gameTime += deltaTime;
            m_remainingTime -= deltaTime;
            if (m_remainingTime <= 0) {
                m_remainingTime = 0;
                m_state = GameState::GameOver;
                std::cout << "[GameEngine] Game Over! P1: " << m_p1Score << " P2: " << m_p2Score << "\n";
                break;
            }

            // 更新游戏阶段
            updatePhase();

            // 更新连击计时器
            m_p1ComboTimer -= deltaTime;
            m_p2ComboTimer -= deltaTime;
            if (m_p1ComboTimer <= 0) m_p1Combo = 0;
            if (m_p2ComboTimer <= 0) m_p2Combo = 0;

            // 生成掉落物（根据阶段调整间隔）
            m_spawnTimer += deltaTime;
            {
                auto it = PHASE_CONFIGS.find(m_phase);
                float rate = it != PHASE_CONFIGS.end() ? it->second.spawnRate : 4.0f;
                float interval = 1.0f / rate;
                if (m_spawnTimer >= interval) {
                    spawnItem();
                    m_spawnTimer = 0;
                }
            }

            // 更新掉落物位置
            updateItems(deltaTime);

            // 碰撞检测
            if (m_collisionSystem && !persons.empty()) {
                auto collisions = m_collisionSystem->detectCollisions(m_fallingItems, persons);
                for (const auto& collision : collisions) {
                    // TODO: 根据碰撞的手(玩家)分配分数
                    // 暂时假设玩家1
                    int playerId = 0;
                    if (playerId == 0) {
                        m_p1Score += collision.scoreChange;
                        if (collision.scoreChange > 0) {
                            m_p1Combo++;
                            m_p1ComboTimer = GameSettings::COMBO_TIMEOUT;
                        }
                    } else {
                        m_p2Score += collision.scoreChange;
                        if (collision.scoreChange > 0) {
                            m_p2Combo++;
                            m_p2ComboTimer = GameSettings::COMBO_TIMEOUT;
                        }
                    }
                    std::cout << "[GameEngine] Collision! Score: " << collision.scoreChange
                              << ", P1: " << m_p1Score << ", P2: " << m_p2Score << "\n";
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
    m_p1Score = 0;
    m_p2Score = 0;
    m_p1Combo = 0;
    m_p2Combo = 0;
    m_p1ComboTimer = 0;
    m_p2ComboTimer = 0;
    m_gameTime = 0;
    m_remainingTime = GameSettings::GAME_DURATION;
    m_phase = GamePhase::Warmup;
    m_spawnTimer = 0;
    m_fallingItems.clear();
    m_nextItemId = 0;
    m_state = GameState::Calibrating;
    std::cout << "[GameEngine] Game reset\n";
}

void GameEngine::updatePhase() {
    if (m_gameTime < GameSettings::PHASE_WARMUP_END) {
        m_phase = GamePhase::Warmup;
    } else if (m_gameTime < GameSettings::PHASE_RUSH_END) {
        m_phase = GamePhase::Rush;
    } else {
        m_phase = GamePhase::Finale;
    }
}

void GameEngine::spawnItem() {
    // 随机数生成器
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<float> xDist(0.1f, 0.9f);
    static std::uniform_real_distribution<float> speedVariation(0.8f, 1.2f);
    static std::uniform_real_distribution<float> rotSpeedDist(-180.0f, 180.0f);
    static std::uniform_int_distribution<int> typeDist(0, 99);

    FallingItem item;
    item.id = m_nextItemId++;
    item.x = xDist(gen) * m_width;
    item.y = -50.0f;  // 从屏幕顶部上方开始
    item.rotationSpeed = rotSpeedDist(gen);

    // 随机类型（使用配置表权重）
    int typeRoll = typeDist(gen);
    int cumulative = 0;
    for (const auto& [itemType, weight] : ITEM_SPAWN_WEIGHTS) {
        cumulative += weight;
        if (typeRoll < cumulative) {
            item.initFromConfig(itemType);
            break;
        }
    }
    // 如果没有匹配到（不应该发生），默认为爆米花
    if (!item.size) {
        item.initFromConfig(ItemType::Popcorn);
    }

    // 根据阶段设置速度
    auto phaseIt = PHASE_CONFIGS.find(m_phase);
    float baseSpeed = phaseIt != PHASE_CONFIGS.end() ? phaseIt->second.fallSpeed : 400.0f;

    // 根据物品重量调整速度
    auto itemIt = ITEM_CONFIGS.find(item.type);
    float speedMult = itemIt != ITEM_CONFIGS.end() ? itemIt->second.speedMultiplier : 1.0f;

    item.speed = baseSpeed * speedMult * speedVariation(gen);

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
