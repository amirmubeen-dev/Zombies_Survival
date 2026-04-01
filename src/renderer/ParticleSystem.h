#pragma once
#include "../core/Types.h"
#include <glm/glm.hpp>
#include <vector>
#include <cmath>
#include <algorithm>

class ParticleSystem {
public:
    std::vector<Particle3D> particles;
    static const int MAX_PARTICLES = 2000;

    void spawn(glm::vec3 pos, glm::vec3 dir, glm::vec3 color,
               int count, float speed, float size, float life,
               bool useGravity = true)
    {
        for (int i = 0; i < count && (int)particles.size() < MAX_PARTICLES; i++) {
            Particle3D p;
            p.pos     = pos;
            float rx  = ((float)rand()/RAND_MAX - 0.5f) * 2.0f;
            float ry  = ((float)rand()/RAND_MAX - 0.5f) * 2.0f;
            float rz  = ((float)rand()/RAND_MAX - 0.5f) * 2.0f;
            p.vel     = dir * speed + glm::vec3(rx, ry, rz) * speed * 0.5f;
            p.color   = color;
            p.life    = p.maxLife = life;
            p.size    = size;
            p.gravity = useGravity;
            particles.push_back(p);
        }
    }

    void spawnBlood(glm::vec3 hitPos, int count = 20) {
        spawn(hitPos, {0, 0.5f, 0}, {0.7f, 0.0f, 0.0f}, count, 4.0f, 0.07f, 0.5f);
    }

    void spawnSparks(glm::vec3 muzzlePos, glm::vec3 forward, int count = 12) {
        spawn(muzzlePos, forward, {1.0f, 0.85f, 0.3f}, count, 7.0f, 0.04f, 0.2f, false);
    }

    void spawnSmoke(glm::vec3 pos, int count = 5) {
        spawn(pos, {0, 1, 0}, {0.4f, 0.4f, 0.4f}, count, 0.5f, 0.3f, 1.5f, false);
    }

    void spawnDust(glm::vec3 pos, int count = 10) {
        spawn(pos, {0, 1, 0}, {0.6f, 0.5f, 0.4f}, count, 3.0f, 0.08f, 0.4f, true);
    }

    void update(float dt) {
        static const float GRAVITY = -9.8f;
        for (auto& p : particles) {
            p.pos  += p.vel * dt;
            if (p.gravity) p.vel.y += GRAVITY * 0.15f * dt;
            p.vel  *= 0.95f;
            p.life -= dt;
        }
        particles.erase(
            std::remove_if(particles.begin(), particles.end(),
                [](const Particle3D& p){ return p.life <= 0; }),
            particles.end());
    }

    void clear() { particles.clear(); }
};
