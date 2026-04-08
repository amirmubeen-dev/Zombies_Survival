#pragma once
#include "Entity.h"
#include <glm/glm.hpp>
#include <vector>

enum ZombieType {
    ZOMBIE_WALKER = 0,
    ZOMBIE_RUNNER = 1,
    ZOMBIE_TANK   = 2
};

struct Zombie : Entity {
    ZombieType type;
    float speed;
    float damage;
    float attackCooldown = 0.0f;

    // Visuals & Animations
    bool  flashing   = false;
    float flashTimer = 0.0f;
    float walkAnim   = 0.0f;
    float angle      = 0.0f;
    
    // Death sequence
    bool  isDying      = false;
    float deathTimer   = 0.0f; // 0.8s fall + 2s fade
    float jitter = ((rand() % 100) / 100.0f - 0.5f) * 0.3f;

    Zombie() {}

    void updateAnimation(float dt, bool moving) {
    if (flashing) {
        flashTimer -= dt;
        if (flashTimer <= 0) flashing = false;
    }

    if (isDying) {
        deathTimer += dt;
        return;
    }

    if (moving) {
        // 🔥 natural walk (not robotic)
        walkAnim += speed * dt * 2.0f;

        // 🔥 slight body sway (human-like imbalance)
        float sway = sinf(walkAnim * 0.5f) * 0.2f;

        // 🔥 angle variation (not perfectly straight)
        angle += sway * dt;
    } else {
        walkAnim = 0.0f;
    }

    if (attackCooldown > 0) attackCooldown -= dt;
}

    void takeDamage(float dmg) {
    if (isDying) return;

    health -= dmg;
    flashing = true;
    flashTimer = 0.15f;

    if (health <= 0) {
        health = 0;
        alive = false;

        // 🔥 start death animation
        isDying = true;
        deathTimer = 0.0f;
    }
}

    float getScale() const {
        // Ensure zombie model height is consistent with player height for realistic play
        return 1.0f;
    }

    glm::vec3 getBaseColor() const {
        if (type == ZOMBIE_TANK)   return {0.2f, 0.1f, 0.4f}; // Dark Purple
        if (type == ZOMBIE_RUNNER) return {0.6f, 0.4f, 0.2f}; // Orange tint
        return {0.45f, 0.55f, 0.40f};                         // Sickly Green-Grey
    }

    static Zombie createWalker(glm::vec3 p) {
        Zombie z; z.type = ZOMBIE_WALKER; z.pos = p; z.vel = {0,0,0};
        z.health = 100; z.maxHealth = 100; z.speed = 2.0f; z.damage = 15;
        return z;
    }
    static Zombie createRunner(glm::vec3 p) {
        Zombie z; z.type = ZOMBIE_RUNNER; z.pos = p; z.vel = {0,0,0};
        z.health = 60; z.maxHealth = 60; z.speed = 5.5f; z.damage = 10;
        return z;
    }
    static Zombie createTank(glm::vec3 p) {
        Zombie z; z.type = ZOMBIE_TANK; z.pos = p; z.vel = {0,0,0};
        z.health = 400; z.maxHealth = 400; z.speed = 1.0f; z.damage = 35;
        return z;
    }
};
