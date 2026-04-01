#pragma once
#include "Weapon.h"
#include "Pistol.h"
#include "Shotgun.h"
#include "Rifle.h"
#include "../renderer/ParticleSystem.h"
#include <memory>
#include <vector>
#include <string>
#include <cmath>

class WeaponManager {
public:
    std::vector<std::unique_ptr<Weapon>> weapons;
    int currentWeapon = 0;
    
    // Physical bullets currently flying
    std::vector<Bullet> activeBullets;

    void init() {
        weapons.clear();
        weapons.push_back(std::make_unique<Pistol>());
        weapons.push_back(std::make_unique<Shotgun>());
        weapons.push_back(std::make_unique<Rifle>());
        currentWeapon = 0;
        activeBullets.clear();
    }

    Weapon& getActive() { return *weapons[currentWeapon]; }
    const Weapon& getActive() const { return *weapons[currentWeapon]; }

    void switchWeapon(int index) {
        // Drop reload on switch
        if (index >= 0 && index < (int)weapons.size()) {
            getActive().reloading = false;
            currentWeapon = index;
        }
    }

    void scrollWeapon(int delta) {
        getActive().reloading = false;
        currentWeapon = (currentWeapon + delta + (int)weapons.size()) % (int)weapons.size();
    }

    // Call this from Game.h passing the ParticleSystem directly
    void updateBullets(float dt, std::vector<Zombie>& zombies, ParticleSystem& particles, float mapRadius) {
        for (auto& w : weapons) w->update(dt);

        for (auto& b : activeBullets) {
            if (!b.alive) continue;

            // Physics step
            glm::vec3 nextPos = b.pos + b.vel * dt;
            
            // Rifle bullet drop
            if (b.weaponType == 2) {
                b.vel.y -= 2.0f * dt;
            }
            
            b.pos = nextPos;
            b.updateTrail();
            b.lifetime -= dt;
            
            if (b.lifetime <= 0) {
                b.alive = false;
                continue;
            }
            
            // Ground hit
            if (b.pos.y <= 0) {
                b.alive = false;
                particles.spawnDust(b.pos, 10);
                continue;
            }
            // Map boundary hit
            if (glm::length(glm::vec2(b.pos.x, b.pos.z)) > mapRadius) {
                b.alive = false;
                continue;
            }

            // Zombie Hit Check
            // Rough sphere check against torso
            for (auto& z : zombies) {
                if (!z.alive || z.isDying) continue;
                
                // Torso is around (pos.x, pos.y + 0.85 * scale, pos.z)
                float scale = z.getScale();
                glm::vec3 zCenter = z.pos + glm::vec3(0, 0.85f * scale, 0);
                float radius = 0.65f * scale; // slightly larger hit radius for reliable shooting

                if (glm::distance(b.pos, zCenter) < radius) {
                    z.takeDamage(b.damage);
                    particles.spawnBlood(b.pos, 30);
                    b.alive = false;
                    
                    if (z.health <= 0) z.alive = false;
                    break;
                }
            }
        }

        // Cleanup dead bullets
        activeBullets.erase(std::remove_if(activeBullets.begin(), activeBullets.end(),
            [](const Bullet& b) { return !b.alive; }), activeBullets.end());
    }

    void shoot(const Camera& cam, glm::vec3 spawnPos, float& shakeAmount) {
        getActive().shoot(cam, spawnPos, activeBullets, shakeAmount);
    }

    void reload() { getActive().startReload(); }

    void drawViewModel(GLuint shader, const Mesh& boxMesh, float recoil, float aspect) {
        getActive().drawViewModel(shader, boxMesh, recoil, aspect);
    }

    int getAmmo()       const { return getActive().ammo; }
    int getMaxAmmo()    const { return getActive().maxAmmo; }
    int getReserve()    const { return getActive().reserveAmmo; }
    std::string getName() const { return getActive().name; }
    bool isReloading()  const { return getActive().reloading; }
    float getReloadT()  const { return getActive().reloadTimer / getActive().reloadTime; }
};
