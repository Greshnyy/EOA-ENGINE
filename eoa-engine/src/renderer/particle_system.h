#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace eoa {

struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float size = 0.1f;
    float life = 1.0f;     // 1=born, 0=dead
    float maxLife = 2.0f;
    glm::vec4 startColor = glm::vec4(1,1,1,1);
    glm::vec4 color = glm::vec4(1,1,1,1);
    glm::vec4 endColor = glm::vec4(1,1,1,0);
    float startSize = 0.1f;
    float endSize = 0.0f;
    bool alive = true;
};

enum class EmitterShape { Sphere, Box, Cone };

struct ParticleEmitter {
    std::string name;
    EmitterShape shape = EmitterShape::Sphere;
    glm::vec3 shapeParams = glm::vec3(1,1,1); // radius or half-extents
    float spawnRate = 100.0f;       // particles per second
    float lifetime = 2.0f;
    float startSize = 0.1f;
    float endSize = 0.0f;
    glm::vec4 startColor = glm::vec4(1,1,1,1);
    glm::vec4 endColor = glm::vec4(1,1,1,0);
    glm::vec3 velocity = glm::vec3(0,1,0);
    float velocityRandom = 0.5f;
    glm::vec3 gravity = glm::vec3(0,-0.5f,0);
    int maxParticles = 10000;
    bool enabled = true;
};

class ParticleSystem {
public:
    ParticleSystem() = default;
    void AddEmitter(const ParticleEmitter& emitter);
    void RemoveEmitter(const std::string& name);
    void Update(float deltaTime, const glm::vec3& systemPos = glm::vec3(0));

    std::vector<ParticleEmitter>& GetEmitters() { return emitters_; }
    std::vector<Particle>& GetParticles() { return particles_; }

private:
    std::vector<ParticleEmitter> emitters_;
    std::vector<Particle> particles_;
    float spawnAccum_ = 0;
};
} // namespace eoa
