#pragma once

#include <cstdint>
#include <string>
#include <map>

namespace popcorn {

// ============= 颜色定义 =============
namespace Colors {
    constexpr uint32_t P1 = 0x007AFF;       // 蓝色
    constexpr uint32_t P2 = 0xFF3B30;       // 红色
    constexpr uint32_t Shared = 0xFFD700;   // 金色
    constexpr uint32_t White = 0xFFFFFF;
    constexpr uint32_t Black = 0x000000;
    constexpr uint32_t Popcorn = 0xFFFFCC;
    constexpr uint32_t Ticket = 0xFF6B35;
    constexpr uint32_t Cola = 0xFF0000;
    constexpr uint32_t Filmroll = 0xFFD700;
    constexpr uint32_t Bomb = 0xFF0000;
}

// ============= 掉落物类型 =============
enum class ItemType {
    Popcorn,    // 爆米花 🍿 - 10分
    Ticket,     // 电影票 🎫 - 25分
    Cola,       // 可乐杯 🥤 - 50分
    Filmroll,   // 胶片卷 🎞️ - 100分
    Bomb        // 炸弹 💣 - -30分
};

// ============= 掉落物配置 =============
struct ItemConfig {
    ItemType type;
    std::string name;
    std::string emoji;
    int score;
    uint32_t color;
    float size;
    float speedMultiplier;  // 速度乘数 (light=0.8, medium=1.0, heavy=1.2)
};

// 掉落物配置表
inline const std::map<ItemType, ItemConfig> ITEM_CONFIGS = {
    {ItemType::Popcorn,  {ItemType::Popcorn,  "爆米花", "🍿",   10, Colors::Popcorn,  65.0f, 0.8f}},
    {ItemType::Ticket,   {ItemType::Ticket,   "电影票", "🎫",   25, Colors::Ticket,   70.0f, 1.0f}},
    {ItemType::Cola,     {ItemType::Cola,     "可乐杯", "🥤",   50, Colors::Cola,     75.0f, 1.2f}},
    {ItemType::Filmroll, {ItemType::Filmroll, "胶片卷", "🎞️",  100, Colors::Filmroll, 85.0f, 1.2f}},
    {ItemType::Bomb,     {ItemType::Bomb,     "炸弹",   "💣",  -30, Colors::Bomb,     70.0f, 1.0f}},
};

// 掉落物出现权重（总和100）
inline const std::map<ItemType, int> ITEM_SPAWN_WEIGHTS = {
    {ItemType::Popcorn,  40},   // 40%
    {ItemType::Ticket,   22},   // 22%
    {ItemType::Cola,     15},   // 15%
    {ItemType::Filmroll, 15},   // 15%
    {ItemType::Bomb,      8},   // 8%
};

// ============= 游戏阶段 =============
enum class GamePhase {
    Warmup,     // 热身期 (0-15秒)
    Rush,       // 加速期 (15-30秒)
    Finale      // 终局期 (30-45秒)
};

struct PhaseConfig {
    float duration;         // 持续时间（秒）
    float fallSpeed;        // 下落速度（像素/秒）
    float spawnRate;        // 生成速率（个/秒）
    float specialRate;      // 高分物品概率
    float obstacleRate;     // 炸弹概率
    std::string title;
    std::string subtitle;
};

inline const std::map<GamePhase, PhaseConfig> PHASE_CONFIGS = {
    {GamePhase::Warmup, {15.0f, 400.0f, 4.0f, 0.10f, 0.05f, "热身期", "观众入场"}},
    {GamePhase::Rush,   {15.0f, 620.0f, 6.0f, 0.25f, 0.10f, "加速期", "人潮涌动!"}},
    {GamePhase::Finale, {15.0f, 880.0f, 8.0f, 0.40f, 0.08f, "终局期", "最后冲刺!"}},
};

// ============= 游戏配置 =============
struct GameSettings {
    // 游戏时长
    static constexpr float GAME_DURATION = 45.0f;
    static constexpr int TARGET_FPS = 60;

    // 屏幕区域划分（从左到右）
    static constexpr float ZONE_P1 = 0.4f;      // P1区域：40%
    static constexpr float ZONE_SHARED = 0.2f;  // 共享区：20%
    static constexpr float ZONE_P2 = 0.4f;      // P2区域：40%

    // 捕获半径（像素）
    static constexpr float CAPTURE_RADIUS = 100.0f;
    static constexpr float PERFECT_CAPTURE_RADIUS = 30.0f;

    // 屏幕尺寸
    static constexpr int SCREEN_WIDTH = 1920;
    static constexpr int SCREEN_HEIGHT = 1080;

    // HUD 区域
    static constexpr int HUD_HEIGHT = 80;

    // 连击配置
    static constexpr float COMBO_TIMEOUT = 2.0f;  // 连击超时时间（秒）

    // 阶段时间边界
    static constexpr float PHASE_WARMUP_END = 15.0f;
    static constexpr float PHASE_RUSH_END = 30.0f;
};

// ============= 计分配置 =============
struct ScoreConfig {
    // 连击倍率
    static constexpr float COMBO_2X = 1.2f;
    static constexpr float COMBO_5X = 1.5f;
    static constexpr float COMBO_10X = 2.0f;
    static constexpr float COMBO_20X = 3.0f;

    // 特殊加分
    static constexpr int PERFECT_CAPTURE_BONUS = 5;
    static constexpr int EXTREME_CAPTURE_BONUS = 10;
    static constexpr int MULTI_CAPTURE_BONUS = 20;

    static float getComboMultiplier(int combo) {
        if (combo >= 20) return COMBO_20X;
        if (combo >= 10) return COMBO_10X;
        if (combo >= 5) return COMBO_5X;
        if (combo >= 2) return COMBO_2X;
        return 1.0f;
    }
};

} // namespace popcorn
