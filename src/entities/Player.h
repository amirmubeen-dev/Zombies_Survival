#pragma once
#include "Entity.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

const float PLAYER_SPEED   = 6.0f;
const float GRAVITY        = -20.0f;
const float MAP_RADIUS     = 300.0f;

struct Player : Entity {
    // Actions & weapons
    float shootCooldown = 0.0f;
    float gunRecoil     = 0.0f;
    int   currentWeapon = 0;

    // Movement states
    bool  onGround    = true;
    bool  isCrouching = false;
    bool  isSprinting = false;

    // DBNO (Revival System)
    bool  isDown      = false;
    float bleedOutTimer = 45.0f;
    bool  isReviving  = false;
    float reviveTimer = 0.0f;

    // Healing & Armor
    float lastDamageTime = 0.0f;
    int   medkits     = 0;
    int   bandages    = 3;
    bool  isHealing   = false;
    float healTimer   = 0.0f;
    int   healTarget  = 0; // 1=bandage, 2=medkit
    float armor       = 0.0f;
    float maxArmor    = 100.0f;

    // Stats
    int   kills = 0;
    int   score = 0;
    float totalPlayTime = 0.0f;

    Player() {
        health    = 100.0f;
        maxHealth = 100.0f;
        pos       = {0, 1.7f, 0};
        vel       = {0, 0, 0};
    }

    void reset() {
        health    = 100.0f;
        alive     = true;
        isDown    = false;
        pos       = {0, 1.7f, 0};
        vel       = {0, 0, 0};
        shootCooldown = 0.0f;
        gunRecoil     = 0.0f;
        currentWeapon = 0;
        
        kills = 0;
        score = 0;
        medkits = 0;
        bandages = 3;
        armor = 0.0f;
        isHealing = false;
        isReviving = false;
    }

    void takeDamage(float dmg) {
        if (!alive) return;
        lastDamageTime = 0.0f;
        
        // Armor mitigates 40%
        if (armor > 0) {
            float reduced = dmg * 0.4f;
            armor = std::max(0.0f, armor - reduced);
            health -= (dmg - reduced);
        } else {
            health -= dmg;
        }

        if (health <= 0 && !isDown) {
            isDown = true;
            bleedOutTimer = 45.0f;
            health = 0;
        } else if (health <= 0 && isDown) {
            alive = false;
        }
    }

    void update(float dt) {
        if (!alive) return;
        totalPlayTime += dt;
        
        // Auto-heal logic
        if (!isDown) {
            lastDamageTime += dt;
            if (lastDamageTime >= 10.0f && health < 75.0f) {
                health = std::min(75.0f, health + 1.0f * dt); // 1 HP/sec
            }
        } else {
            // DBNO bleed-out
            if (!isReviving) {
                bleedOutTimer -= dt;
                if (bleedOutTimer <= 0) alive = false;
            }
        }

        // Healing Item use logic
        if (isHealing) {
            healTimer -= dt;
            if (healTimer <= 0) {
                isHealing = false;
                if (healTarget == 1 && health < 75.0f) health = std::min(75.0f, health + 15.0f);
                else if (healTarget == 2) health = maxHealth;
            }
        }

        shootCooldown = std::max(0.0f, shootCooldown - dt);
        if (gunRecoil > 0.0f) gunRecoil = std::max(0.0f, gunRecoil - dt * 4.0f);
    }
    
    // Starts an item use context
    void startHealing(int type) {
        if (isHealing || isDown) return;
        if (type == 1 && bandages > 0 && health < 75.0f) {
            bandages--;
            isHealing = true;
            healTimer = 2.0f;
            healTarget = 1;
        } else if (type == 2 && medkits > 0 && health < maxHealth) {
            medkits--;
            isHealing = true;
            healTimer = 4.0f;
            healTarget = 2;
        }
    }
};
