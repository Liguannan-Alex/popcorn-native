#pragma once

#include <vector>
#include <random>

struct SDL_Renderer;

namespace popcorn {

/**
 * 单个粒子
 */
struct Particle {
    float x, y;           // 位置
    float vx, vy;         // 速度
    float size;           // 大小
    float life;           // 剩余生命（0-1）
    float maxLife;        // 最大生命
    uint8_t r, g, b, a;   // 颜色
    float gravity;        // 重力影响
    bool active{false};
};

/**
 * 粒子系统
 * 用于爆炸、捕获等视觉特效
 */
class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    /**
     * 初始化
     * @param maxParticles 最大粒子数量
     */
    void initialize(int maxParticles = 500);

    /**
     * 更新粒子
     * @param deltaTime 时间增量（秒）
     */
    void update(float deltaTime);

    /**
     * 渲染粒子
     * @param renderer SDL 渲染器
     */
    void render(SDL_Renderer* renderer);

    /**
     * 创建捕获爆炸特效
     * @param x, y 位置
     * @param isPerfect 是否完美捕获
     */
    void createCaptureExplosion(float x, float y, bool isPerfect = false);

    /**
     * 创建炸弹爆炸特效
     * @param x, y 位置
     */
    void createBombExplosion(float x, float y);

    /**
     * 创建连击特效
     * @param x, y 位置
     * @param comboCount 连击数
     */
    void createComboEffect(float x, float y, int comboCount);

    /**
     * 创建分数弹出特效
     * @param x, y 位置
     * @param score 分数
     * @param color 颜色
     */
    void createScorePopup(float x, float y, int score, uint32_t color);

    /**
     * 清除所有粒子
     */
    void clear();

    /**
     * 获取活跃粒子数量
     */
    int getActiveCount() const { return m_activeCount; }

private:
    /**
     * 获取一个空闲粒子
     */
    Particle* acquireParticle();

    /**
     * 发射一组粒子
     */
    void emit(float x, float y, int count,
              float minSpeed, float maxSpeed,
              float minSize, float maxSize,
              float minLife, float maxLife,
              uint8_t r, uint8_t g, uint8_t b,
              float gravity = 200.0f);

private:
    std::vector<Particle> m_particles;
    int m_activeCount{0};

    // 随机数生成器
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_angleDist{0.0f, 6.28318f};  // 0 to 2*PI
    std::uniform_real_distribution<float> m_unitDist{0.0f, 1.0f};
};

} // namespace popcorn
