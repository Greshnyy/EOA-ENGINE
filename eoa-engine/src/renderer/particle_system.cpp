#include "renderer/particle_system.h"
#include <random>
#include <algorithm>
#include <chrono>

namespace eoa {

namespace {
float Randf() {
    static std::mt19937 rng((unsigned)std::chrono::steady_clock::now().time_since_epoch().count());
    static std::uniform_real_distribution<float> d(0.0f, 1.0f);
    return d(rng);
}
glm::vec3 RandomInSphere(float radius) {
    float theta = Randf() * 6.2831853f;
    float phi = acosf(2.0f * Randf() - 1.0f);
    float r = radius * powf(Randf(), 0.333f);
    return glm::vec3(r*sinf(phi)*cosf(theta), r*sinf(phi)*sinf(theta), r*cosf(phi));
}
}

void ParticleSystem::AddEmitter(const ParticleEmitter& e) {
    emitters_.push_back(e);
}

void ParticleSystem::RemoveEmitter(const std::string& name) {
    emitters_.erase(std::remove_if(emitters_.begin(), emitters_.end(),
        [&](auto& e){ return e.name == name; }), emitters_.end());
}

void ParticleSystem::Update(float deltaTime, const glm::vec3& systemPos) {
    deltaTime = glm::min(deltaTime, 0.1f);

    // Update existing particles
    for (auto& p : particles_) {
        if (!p.alive) continue;
        p.velocity += emitters_[0].gravity * deltaTime;
        p.position += p.velocity * deltaTime;
        p.life -= deltaTime / p.maxLife;
        float t = glm::clamp(p.life, 0.0f, 1.0f);
        p.size = glm::mix(p.endSize, p.startSize, t);
        p.color = glm::mix(p.endColor, p.startColor, t);
        if (p.life <= 0.0f) p.alive = false;
    }

    // Spawn new particles
    for (auto& emitter : emitters_) {
        if (!emitter.enabled) continue;
        spawnAccum_ += emitter.spawnRate * deltaTime;
        int toSpawn = (int)spawnAccum_;
        spawnAccum_ -= toSpawn;
        for (int i = 0; i < toSpawn; i++) {
            Particle p;
            p.position = systemPos + RandomInSphere(emitter.shapeParams.x);
            glm::vec3 dir = glm::normalize(glm::vec3(Randf()-0.5f, Randf(), Randf()-0.5f));
            p.velocity = emitter.velocity + dir * emitter.velocityRandom;
            p.startSize = emitter.startSize;
            p.endSize = emitter.endSize;
            p.size = emitter.startSize;
            p.maxLife = emitter.lifetime;
            p.life = 1.0f;
            p.color = emitter.startColor;
            p.endColor = emitter.endColor;
            p.alive = true;
            particles_.push_back(p);
        }
    }

    // Remove dead (compact swap-pop)
    int totalCap = 0;
    for (auto& e : emitters_) totalCap += e.maxParticles;
    if ((int)particles_.size() > totalCap) {
        auto it = std::remove_if(particles_.begin(), particles_.end(),
            [](auto& p){ return !p.alive; });
        particles_.erase(it, particles_.end());
        if ((int)particles_.size() > totalCap)
            particles_.resize(totalCap);
    }
}
} // namespace eoa
