#pragma once
#include <glm/glm.hpp>
#include <string>

struct Entity {
    glm::vec3 pos    = {0, 0, 0};
    glm::vec3 vel    = {0, 0, 0};
    float     health = 100.0f;
    float     maxHealth = 100.0f;
    bool      alive  = true;

    bool isDead() const { return !alive || health <= 0.0f; }

    void takeDamage(float dmg) {
        health -= dmg;
        if (health <= 0.0f) { health = 0.0f; alive = false; }
    }

    void heal(float amt) {
        health = std::min(maxHealth, health + amt);
    }

    virtual ~Entity() = default;
};
