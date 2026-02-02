#pragma once

#include <vector>
#include <memory>
#include "FallingItem.h"
#include "CollisionSystem.h"
#include "detection/PoseDetector.h"
#include "detection/GestureDetector.h"

namespace popcorn {

/**
 * 游戏状态
 */
enum class GameState {
    Calibrating,    // 校准中（等待玩家）
    Countdown,      // 倒计时
    Playing,        // 游戏中
    Paused,         // 暂停
    GameOver        // 游戏结束
};

/**
 * 游戏引擎
 */
class GameEngine {
public:
    GameEngine();
    ~GameEngine();

    /**
     * 初始化
     * @param width 游戏区域宽度
     * @param height 游戏区域高度
     */
    bool initialize(int width, int height);

    /**
     * 更新游戏逻辑
     * @param deltaTime 时间增量（秒）
     * @param persons 检测到的人物
     * @param gesture 手势检测结果
     */
    void update(float deltaTime, const std::vector<DetectedPerson>& persons,
                const GestureResult& gesture = GestureResult{});

    /**
     * 开始游戏
     */
    void startGame();

    /**
     * 暂停/恢复
     */
    void togglePause();

    /**
     * 重置游戏
     */
    void reset();

    // Getters
    GameState getState() const { return m_state; }
    int getP1Score() const { return m_p1Score; }
    int getP2Score() const { return m_p2Score; }
    int getScore() const { return m_p1Score + m_p2Score; }  // 兼容旧代码
    float getRemainingTime() const { return m_remainingTime; }
    const std::vector<FallingItem>& getFallingItems() const { return m_fallingItems; }
    const std::vector<DetectedPerson>& getDetectedPersons() const { return m_detectedPersons; }
    GamePhase getPhase() const { return m_phase; }
    int getP1Combo() const { return m_p1Combo; }
    int getP2Combo() const { return m_p2Combo; }

private:
    // 生成掉落物
    void spawnItem();

    // 更新掉落物
    void updateItems(float deltaTime);

    // 移除超出屏幕的物品
    void removeOffscreenItems();

private:
    int m_width{0};
    int m_height{0};

    GameState m_state{GameState::Calibrating};
    GamePhase m_phase{GamePhase::Warmup};

    // 分数（双人）
    int m_p1Score{0};
    int m_p2Score{0};

    // 连击（双人）
    int m_p1Combo{0};
    int m_p2Combo{0};
    float m_p1ComboTimer{0.0f};
    float m_p2ComboTimer{0.0f};

    float m_gameTime{0.0f};         // 游戏进行时间
    float m_remainingTime{GameSettings::GAME_DURATION};   // 剩余时间
    float m_spawnTimer{0.0f};
    float m_spawnInterval{0.25f};   // 生成间隔（秒）

    std::vector<FallingItem> m_fallingItems;
    std::vector<DetectedPerson> m_detectedPersons;

    std::unique_ptr<CollisionSystem> m_collisionSystem;

    int m_nextItemId{0};

    // 更新游戏阶段
    void updatePhase();
};

} // namespace popcorn
