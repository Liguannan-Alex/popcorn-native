#include "ParticleSystem.h"
#include <SDL2/SDL.h>
#include <cmath>
#include <algorithm>

namespace popcorn {

ParticleSystem::ParticleSystem() : m_rng(std::random_device{}()) {}

ParticleSystem::~ParticleSystem() {
    clear();
}

void ParticleSystem::initialize(int maxParticles) {
    m_particles.resize(maxParticles);
    for (auto& p : m_particles) {
        p.active = false;
    }
    m_activeCount = 0;
}

void ParticleSystem::update(float deltaTime) {
    m_activeCount = 0;

    for (auto& p : m_particles) {
        if (!p.active) continue;

        // 更新生命
        p.life -= deltaTime / p.maxLife;
        if (p.life <= 0) {
            p.active = false;
            continue;
        }

        // 更新速度（重力）
        p.vy += p.gravity * deltaTime;

        // 更新位置
        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;

        // 更新透明度
        p.a = static_cast<uint8_t>(255 * p.life);

        // 缩小
        p.size *= (1.0f - deltaTime * 0.5f);
        if (p.size < 1.0f) p.size = 1.0f;

        m_activeCount++;
    }
}

void ParticleSystem::render(SDL_Renderer* renderer) {
    if (!renderer) return;

    // 设置混合模式为加法混合（更亮的效果）
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    for (const auto& p : m_particles) {
        if (!p.active) continue;

        SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, p.a);

        // 绘制粒子（简单的圆形用矩形近似，或用多个点）
        int halfSize = static_cast<int>(p.size / 2);
        if (halfSize < 1) halfSize = 1;

        // 绘制填充圆（用多个矩形近似）
        for (int dy = -halfSize; dy <= halfSize; dy++) {
            int width = static_cast<int>(std::sqrt(halfSize * halfSize - dy * dy));
            SDL_Rect rect = {
                static_cast<int>(p.x) - width,
                static_cast<int>(p.y) + dy,
                width * 2,
                1
            };
            SDL_RenderFillRect(renderer, &rect);
        }
    }
}

Particle* ParticleSystem::acquireParticle() {
    for (auto& p : m_particles) {
        if (!p.active) {
            p.active = true;
            return &p;
        }
    }
    return nullptr;  // 没有空闲粒子
}

void ParticleSystem::emit(float x, float y, int count,
                          float minSpeed, float maxSpeed,
                          float minSize, float maxSize,
                          float minLife, float maxLife,
                          uint8_t r, uint8_t g, uint8_t b,
                          float gravity) {
    for (int i = 0; i < count; i++) {
        Particle* p = acquireParticle();
        if (!p) break;

        float angle = m_angleDist(m_rng);
        float speed = minSpeed + m_unitDist(m_rng) * (maxSpeed - minSpeed);

        p->x = x;
        p->y = y;
        p->vx = std::cos(angle) * speed;
        p->vy = std::sin(angle) * speed;
        p->size = minSize + m_unitDist(m_rng) * (maxSize - minSize);
        p->maxLife = minLife + m_unitDist(m_rng) * (maxLife - minLife);
        p->life = 1.0f;
        p->r = r;
        p->g = g;
        p->b = b;
        p->a = 255;
        p->gravity = gravity;
    }
}

void ParticleSystem::createCaptureExplosion(float x, float y, bool isPerfect) {
    if (isPerfect) {
        // 完美捕获：金色闪光 + 更多粒子
        emit(x, y, 30, 100.0f, 300.0f, 4.0f, 12.0f, 0.3f, 0.8f,
             255, 215, 0, 100.0f);  // 金色
        emit(x, y, 15, 50.0f, 150.0f, 2.0f, 6.0f, 0.2f, 0.5f,
             255, 255, 255, 50.0f);  // 白色闪光
    } else {
        // 普通捕获：黄色爆炸
        emit(x, y, 20, 80.0f, 200.0f, 3.0f, 8.0f, 0.2f, 0.6f,
             255, 255, 150, 150.0f);  // 浅黄色
    }
}

void ParticleSystem::createBombExplosion(float x, float y) {
    // 炸弹爆炸：红色 + 橙色 + 黑色烟雾
    emit(x, y, 40, 150.0f, 400.0f, 5.0f, 15.0f, 0.3f, 1.0f,
         255, 100, 50, 200.0f);  // 橙红色火焰
    emit(x, y, 30, 100.0f, 250.0f, 8.0f, 20.0f, 0.5f, 1.2f,
         80, 80, 80, 50.0f);  // 灰色烟雾
    emit(x, y, 15, 50.0f, 150.0f, 2.0f, 5.0f, 0.2f, 0.4f,
         255, 255, 100, 300.0f);  // 黄色火星
}

void ParticleSystem::createComboEffect(float x, float y, int comboCount) {
    // 连击特效：根据连击数增加粒子
    int particleCount = std::min(10 + comboCount * 3, 50);
    float speed = 50.0f + comboCount * 10.0f;

    // 彩虹色粒子
    for (int i = 0; i < particleCount; i++) {
        Particle* p = acquireParticle();
        if (!p) break;

        float hue = static_cast<float>(i) / particleCount;
        // HSV to RGB (simplified)
        int r, g, b;
        float h = hue * 6.0f;
        int hi = static_cast<int>(h) % 6;
        float f = h - static_cast<int>(h);
        switch (hi) {
            case 0: r = 255; g = static_cast<int>(255 * f); b = 0; break;
            case 1: r = static_cast<int>(255 * (1 - f)); g = 255; b = 0; break;
            case 2: r = 0; g = 255; b = static_cast<int>(255 * f); break;
            case 3: r = 0; g = static_cast<int>(255 * (1 - f)); b = 255; break;
            case 4: r = static_cast<int>(255 * f); g = 0; b = 255; break;
            default: r = 255; g = 0; b = static_cast<int>(255 * (1 - f)); break;
        }

        float angle = m_angleDist(m_rng);
        float actualSpeed = speed * (0.5f + m_unitDist(m_rng) * 0.5f);

        p->x = x;
        p->y = y;
        p->vx = std::cos(angle) * actualSpeed;
        p->vy = std::sin(angle) * actualSpeed - 100.0f;  // 向上偏移
        p->size = 4.0f + m_unitDist(m_rng) * 4.0f;
        p->maxLife = 0.5f + m_unitDist(m_rng) * 0.5f;
        p->life = 1.0f;
        p->r = static_cast<uint8_t>(r);
        p->g = static_cast<uint8_t>(g);
        p->b = static_cast<uint8_t>(b);
        p->a = 255;
        p->gravity = 100.0f;
    }
}

void ParticleSystem::createScorePopup(float x, float y, int score, uint32_t color) {
    // 分数弹出时的小粒子效果
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    int count = std::abs(score) / 10 + 3;
    emit(x, y, count, 30.0f, 80.0f, 2.0f, 5.0f, 0.2f, 0.5f, r, g, b, -50.0f);  // 负重力向上
}

void ParticleSystem::clear() {
    for (auto& p : m_particles) {
        p.active = false;
    }
    m_activeCount = 0;
}

} // namespace popcorn
