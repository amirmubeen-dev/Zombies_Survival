#pragma once
#include <glm/glm.hpp>

struct Particle3D {
    glm::vec3 pos, vel;
    glm::vec3 color;
    float     life = 1.0f, maxLife = 1.0f, size = 0.1f;
    bool      gravity = true;
};
