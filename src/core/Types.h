#pragma once
// Shared minimal types used across multiple systems
// Include this instead of individual headers to avoid duplicate definitions

#include <glm/glm.hpp>

struct Particle3D {
    glm::vec3 pos, vel;
    glm::vec3 color;
    float     life = 1.0f, maxLife = 1.0f, size = 0.1f;
    bool      gravity = true;
};
